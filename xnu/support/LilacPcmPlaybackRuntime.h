/* SPDX-License-Identifier: GPL-2.0-only
 * One retained PCM endpoint: concrete SMEM FIFO, original ADSP::Link and
 * session transport. The owner provides verified DSP/codec/SMEM readiness,
 * monotonic time and PCM cache/ownership transitions. No hardware startup,
 * mapping, allocation, reset or physical-page release occurs here. All
 * entry points and callbacks are serialized and non-reentrant by the owner;
 * retain this large endpoint outside the small kernel stack. */
#ifndef LILAC_PCM_PLAYBACK_RUNTIME_H
#define LILAC_PCM_PLAYBACK_RUNTIME_H

#include "LilacAprSmemFifo.h"
#include "LilacPcmPlaybackTransport.h"
#include "LilacAPRLink.h"

class LilacPcmPlaybackRuntime {
public:
    using Result = ADSP::Result;
    using Transport = LilacPcmPlaybackTransport;
    using Session = Transport::Session;
    enum class State : uint8_t { NotReady, Negotiating, Starting, Running, Draining, Closing, Stopped, Uncertain };
    enum class Failure : uint8_t { None, Clock, Readiness, Negotiation, Transport, Memory };
    enum class MemoryPolicy : uint8_t { Unknown, WriteCombined, CacheMaintenance };
    struct Readiness {
        uint64_t epoch = 0;
        bool exclusive_endpoint = false, smem_uncached = false, dsp_ready = false, codec_ready = false;
    };
    struct Region {
        uint8_t *cpu;
        uint64_t device_address, epoch;
        uint32_t bytes, cache_line_bytes;
        MemoryPolicy policy;
    };
    struct Callbacks {
        void *context;
        Result (*readiness)(void *,Readiness *);
        bool (*now_ms)(void *,uint64_t *);
        // These transitions must finish all required cache maintenance and
        // memory barriers. WriteCombined still requires store publication.
        Result (*release_pcm_to_device)(void *,uint64_t,uint32_t,uint32_t,MemoryPolicy);
        Result (*acquire_pcm_from_device)(void *,uint64_t,uint32_t,uint32_t,MemoryPolicy);
    };
    struct Timeouts { uint32_t negotiation_ms; Transport::Timeouts protocol; };

    LilacPcmPlaybackRuntime(const Session::Configuration &configuration,
        const sweet_adsp_apr_ops &fifo_callbacks,Region mapping,Callbacks runtime_callbacks,Timeouts limits)
        : config(configuration), pcm(mapping), callbacks(runtime_callbacks), timeouts(limits),
          fifo(fifo_callbacks), transport(configuration,limits.protocol,{this,transport_clock}),
          sink{*this}, link(fifo,sink), fifo_epoch(fifo_callbacks.generation) {}
    LilacPcmPlaybackRuntime(const LilacPcmPlaybackRuntime &) = delete;
    LilacPcmPlaybackRuntime &operator=(const LilacPcmPlaybackRuntime &) = delete;
    static void *operator new(size_t,void *memory) { return memory; }
    static void operator delete(void *,void *) {}

    State state() const { return phase; }
    bool configuration_valid() const { return valid(); }
    Failure failure() const { return failed_at; }
    Result failure_result() const { return failed_result; }
    Transport::Failure transport_failure() const { return transport.failure(); }
    unsigned pending_buffers() const { return transport.pending_buffers(); }
    unsigned retained_spans() const
    {
        unsigned count=0;for(const auto &span:spans)count+=span.owned;return count;
    }
    uint64_t published_packets() const { return transport.published_packets(); }

    Result start()
    {
        if (phase==State::Uncertain) return Result::Quarantined;
        if (phase!=State::NotReady || !valid()) return Result::Invalid;
        Readiness ready;
        Result result=callbacks.readiness(callbacks.context,&ready);
        if (result!=Result::OK) return result;
        if (ready.epoch!=config.epoch) return Result::Stale;
        if (!ready.exclusive_endpoint || !ready.smem_uncached || !ready.dsp_ready || !ready.codec_ready)
            return Result::NotReady;
        uint64_t now=0;if (!time(now)) return failed_result;
        result=fifo.admit(config.epoch,true,true,true);
        if (result!=Result::OK) return fail(Failure::Negotiation,result);
        result=link.begin(config.epoch);
        if (result!=Result::OK) return fail(Failure::Negotiation,result);
        negotiation_started_ms=now;phase=State::Negotiating;
        return Result::OK;
    }

    Result advance()
    {
        if (phase==State::Uncertain) return Result::Quarantined;
        if (phase==State::NotReady) return Result::NotReady;
        if (phase==State::Stopped) return Result::OK;
        uint64_t now=0;Readiness ready;
        if (!time(now) || !readiness(ready)) return failed_result;
        if (phase==State::Negotiating) {
            if (now-negotiation_started_ms>=timeouts.negotiation_ms)
                return fail(Failure::Negotiation,Result::Timeout);
            Result result=link.poll();
            if (phase==State::Uncertain) return failed_result;
            if (result!=Result::OK) return fail(Failure::Negotiation,result);
            if (!time(now) || !readiness(ready)) return failed_result;
            if (now-negotiation_started_ms>=timeouts.negotiation_ms)
                return fail(Failure::Negotiation,Result::Timeout);
            if (!link.ready()) return Result::NotReady;
            if (!transport.admit(link,true,true,now))
                return fail(Failure::Transport,transport.failure_result());
            phase=State::Starting;
            return Result::OK;
        }
        Result result=transport.advance(link,ready.dsp_ready,ready.codec_ready,now);
        if (transport.state()==Transport::State::Uncertain)
            return fail(Failure::Transport,result);
        if (!time(now) || !readiness(ready)) return failed_result;
        // Transport retires only an exactly matched, successful, published
        // WRITE_DONE. Reacquire its PCM span before any new CPU write.
        for (auto &span:spans)if (span.owned && span.token && !transport.buffer_pending(span.token)) {
            const Result acquired=callbacks.acquire_pcm_from_device(callbacks.context,config.epoch,
                span.offset,span.bytes,pcm.policy);
            if (acquired!=Result::OK) return fail(Failure::Memory,acquired);
            if (!time(now) || !readiness(ready)) return failed_result;
            const Result timed=transport.check_time(now);
            if (timed!=Result::OK) return fail(Failure::Transport,timed);
            span={};
        }
        if (transport.state()==Transport::State::Stopped) {
            if (!transport.region_release_confirmed() || retained_spans() || !link.closed())
                return fail(Failure::Transport,Result::Invalid);
            phase=State::Stopped;
        } else if (transport.state()==Transport::State::Closing) {
            phase=State::Closing;
        } else if (transport.session_state()==Session::State::Running) {
            phase=State::Running;
        } else if (transport.session_state()==Session::State::Draining) {
            phase=State::Draining;
        }
        return result;
    }

    lilac_asm_pcm_queue_result queue_pcm(uint32_t offset,uint32_t span_bytes,const void *source,
        uint32_t bytes,uint32_t *token)
    {
        if (phase==State::Uncertain) return LILAC_ASM_QUEUE_QUARANTINED;
        if (phase!=State::Running || !source || !token || !bytes || (bytes&3u) ||
            bytes>span_bytes || !span_bytes || (offset&31u) || ((offset|span_bytes)&(pcm.cache_line_bytes-1)) ||
            offset>pcm.bytes || span_bytes>pcm.bytes-offset ||
            bytes>UINTPTR_MAX-reinterpret_cast<uintptr_t>(source)) return LILAC_ASM_QUEUE_INVALID;
        const uintptr_t output=reinterpret_cast<uintptr_t>(token),cpu=reinterpret_cast<uintptr_t>(pcm.cpu),
            object=reinterpret_cast<uintptr_t>(this);
        if (output%alignof(uint32_t) || sizeof(*token)>UINTPTR_MAX-output ||
            (output<cpu+pcm.bytes && cpu<output+sizeof(*token)) ||
            (output<object+sizeof(*this) && object<output+sizeof(*token))) return LILAC_ASM_QUEUE_INVALID;
        uint64_t now=0;Readiness ready;
        if (!time(now) || !readiness(ready)) return LILAC_ASM_QUEUE_QUARANTINED;
        if (transport.check_time(now)!=Result::OK) {
            fail(Failure::Transport,transport.failure_result());return LILAC_ASM_QUEUE_QUARANTINED;
        }
        if (transport.pending_publication()) return LILAC_ASM_QUEUE_BUSY;
        Span *owned=nullptr;
        for (auto &span:spans) {
            if (!span.owned) { if (!owned) owned=&span; }
            else if (offset<span.offset+span.bytes && span.offset<offset+span_bytes)
                return LILAC_ASM_QUEUE_BUSY;
        }
        if (!owned) return LILAC_ASM_QUEUE_BUSY;
        *owned={offset,span_bytes,0,true};
        auto *destination=pcm.cpu+offset;const auto *input=static_cast<const uint8_t *>(source);
        const uintptr_t from=reinterpret_cast<uintptr_t>(input),to=reinterpret_cast<uintptr_t>(destination);
        if (from<to && to<from+bytes) {
            for(uint32_t i=bytes;i;--i)destination[i-1]=input[i-1];
        } else {
            for(uint32_t i=0;i<bytes;++i)destination[i]=input[i];
        }
        const Result released=callbacks.release_pcm_to_device(callbacks.context,config.epoch,
            offset,span_bytes,pcm.policy);
        if (released!=Result::OK) {
            fail(Failure::Memory,released);return LILAC_ASM_QUEUE_QUARANTINED;
        }
        if (!time(now) || !readiness(ready)) return LILAC_ASM_QUEUE_QUARANTINED;
        uint32_t reserved=0;
        const auto result=transport.queue_write(offset,span_bytes,bytes,now,&reserved);
        if (result==LILAC_ASM_QUEUE_RESERVED) {
            owned->token=reserved;*token=reserved;
        } else if (result==LILAC_ASM_QUEUE_QUARANTINED) {
            fail(Failure::Transport,transport.failure_result());
        } else {
            *owned={};
        }
        return result;
    }

    bool begin_drain()
    {
        if (phase!=State::Running) return false;
        uint64_t now=0;Readiness ready;
        if (!time(now) || !readiness(ready)) return false;
        const bool began=transport.begin_drain(now);
        if (transport.state()==Transport::State::Uncertain)
            fail(Failure::Transport,transport.failure_result());
        if (began) phase=State::Draining;
        return began;
    }
    void uncertain() { (void)fail(Failure::Readiness,Result::Quarantined); }
    bool region_release_confirmed() const
    {
        return phase==State::Stopped && !retained_spans() && transport.region_release_confirmed() &&
            link.closed() && fifo.state()==LilacAprSmemFifo::State::Ready;
    }

private:
    struct Sink {
        LilacPcmPlaybackRuntime &owner;
        Result receive(const void *packet,size_t bytes)
        {
            if (owner.phase==State::Negotiating) return Result::Stale;
            Readiness ready;
            if (!owner.readiness(ready)) return owner.failed_result;
            return owner.transport.receive(packet,bytes);
        }
    };
    struct Span { uint32_t offset=0,bytes=0,token=0; bool owned=false; };
    static bool transport_clock(void *context,uint64_t *now)
    {
        auto &runtime=*static_cast<LilacPcmPlaybackRuntime *>(context);
        return runtime.callbacks.now_ms && runtime.callbacks.now_ms(runtime.callbacks.context,now);
    }
    bool valid() const
    {
        const uintptr_t cpu=reinterpret_cast<uintptr_t>(pcm.cpu);
        const uintptr_t object=reinterpret_cast<uintptr_t>(this);
        return fifo.configuration_valid() && transport.configuration_valid() && timeouts.negotiation_ms &&
            config.epoch==fifo_epoch && pcm.epoch==config.epoch && pcm.device_address==config.region_address &&
            cpu && pcm.bytes==config.region_bytes && pcm.bytes<=UINTPTR_MAX-cpu &&
            !(cpu<object+sizeof(*this) && object<cpu+pcm.bytes) &&
            pcm.cache_line_bytes>=4 && pcm.cache_line_bytes<=4096 &&
            !(pcm.cache_line_bytes&(pcm.cache_line_bytes-1)) &&
            !((cpu|pcm.bytes)&(pcm.cache_line_bytes-1)) &&
            (pcm.policy==MemoryPolicy::WriteCombined || pcm.policy==MemoryPolicy::CacheMaintenance) &&
            callbacks.readiness && callbacks.now_ms && callbacks.release_pcm_to_device &&
            callbacks.acquire_pcm_from_device;
    }
    bool time(uint64_t &now)
    {
        if (!callbacks.now_ms(callbacks.context,&now)) {
            fail(Failure::Clock,Result::IO);return false;
        }
        if (now<last_ms) { fail(Failure::Clock,Result::Invalid);return false; }
        last_ms=now;return true;
    }
    bool readiness(Readiness &ready)
    {
        const Result result=callbacks.readiness(callbacks.context,&ready);
        if (result!=Result::OK) { fail(Failure::Readiness,result);return false; }
        if (ready.epoch!=config.epoch) { fail(Failure::Readiness,Result::Stale);return false; }
        if (!ready.exclusive_endpoint || !ready.smem_uncached || !ready.dsp_ready || !ready.codec_ready) {
            fail(Failure::Readiness,Result::NotReady);return false;
        }
        return true;
    }
    Result fail(Failure at,Result result)
    {
        if (phase!=State::Uncertain) {
            phase=State::Uncertain;failed_at=at;failed_result=result;
            transport.uncertain();fifo.uncertain();
        }
        return failed_result;
    }
    const Session::Configuration config;
    const Region pcm;
    const Callbacks callbacks;
    const Timeouts timeouts;
    LilacAprSmemFifo fifo;
    Transport transport;
    Sink sink;
    ADSP::Link<LilacAprSmemFifo,Sink> link;
    const uint64_t fifo_epoch;
    Span spans[LILAC_ASM_PCM_MAX_PENDING]{};
    uint64_t last_ms=0,negotiation_started_ms=0;
    State phase=State::NotReady;
    Failure failed_at=Failure::None;
    Result failed_result=Result::Quarantined;
};

#endif
