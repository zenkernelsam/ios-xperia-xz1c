/* SPDX-License-Identifier: GPL-2.0-only
 * Real runtime/transport/GLINK/FIFO code over the shared authored peer.
 * Separate CPU and DSP byte views make cache-release order observable.
 * Clock hooks advance inside IO and cache callbacks to test late replies. */
#include "../../xnu/support/LilacPcmPlaybackRuntime.h"
#include "LilacPcmSmemPeerFixture.h"
#include <array>

using Runtime = LilacPcmPlaybackRuntime;

struct Harness {
    Peer peer;
    alignas(64) std::array<uint8_t,8320> backing;
    std::array<uint8_t,8192> device;
    Runtime::Readiness ready{9,true,true,true,true};
    uint64_t now=0;
    bool clock_ok=true;
    Result release_result=Result::OK,acquire_result=Result::OK;
    unsigned releases=0,acquires=0;
    unsigned read_delay=0,notify_delay=0,release_delay=0,acquire_delay=0;
    bool lose_epoch_on_release=false,lose_codec_on_acquire=false;
    struct Event {char kind;uint32_t offset,span;};
    std::vector<Event> events;
    Runtime runtime;
    Harness(Runtime::MemoryPolicy policy=Runtime::MemoryPolicy::CacheMaintenance)
        : runtime(configuration(),peer.callbacks(),{backing.data()+64,UINT64_C(0x123456000),9,8192,64,policy},
            {this,readiness,clock,release,acquire},{20,{10,20,20,20}})
    {
        backing.fill(0xa5);device.fill(0xcc);
        peer.hook_context=this;peer.on_read=read_hook;peer.on_notify=notify_hook;peer.inspect_pcm=inspect;
    }
    static Result readiness(void *context,Runtime::Readiness *out)
    {
        auto &h=*static_cast<Harness *>(context);*out=h.ready;return Result::OK;
    }
    static bool clock(void *context,uint64_t *out)
    {
        auto &h=*static_cast<Harness *>(context);*out=h.now;return h.clock_ok;
    }
    static Result release(void *context,uint64_t epoch,uint32_t offset,uint32_t span,Runtime::MemoryPolicy policy)
    {
        auto &h=*static_cast<Harness *>(context);assert(epoch==9 && !(offset&63) && !(span&63) && offset+span<=8192);
        assert(policy==Runtime::MemoryPolicy::CacheMaintenance || policy==Runtime::MemoryPolicy::WriteCombined);
        ++h.releases;h.events.push_back({'D',offset,span});h.now+=h.release_delay;
        if(h.lose_epoch_on_release)h.ready.epoch=10;
        if(h.release_result!=Result::OK)return h.release_result;
        std::memcpy(h.device.data()+offset,h.backing.data()+64+offset,span);return Result::OK;
    }
    static Result acquire(void *context,uint64_t epoch,uint32_t offset,uint32_t span,Runtime::MemoryPolicy policy)
    {
        auto &h=*static_cast<Harness *>(context);assert(epoch==9 && offset+span<=8192);
        assert(policy!=Runtime::MemoryPolicy::Unknown);
        ++h.acquires;h.events.push_back({'C',offset,span});h.now+=h.acquire_delay;
        if(h.lose_codec_on_acquire)h.ready.codec_ready=false;
        return h.acquire_result;
    }
    static void inspect(void *context,const uint8_t *packet,size_t bytes)
    {
        auto &h=*static_cast<Harness *>(context);assert(bytes==52);
        const uint64_t address=uint64_t(ADSP::read32(packet+20))|(uint64_t(ADSP::read32(packet+24))<<32);
        const unsigned offset=unsigned(address-UINT64_C(0x123456000)),length=ADSP::read32(packet+32);
        assert(offset+length<=8192 && !h.events.empty());
        bool released=false;
        for(const auto &event:h.events)if(event.kind=='D' && event.offset==offset)released=true;
        assert(released && !std::memcmp(h.device.data()+offset,h.backing.data()+64+offset,length));
        h.events.push_back({'P',offset,length});
    }
    static void read_hook(void *context,uint32_t address)
    {
        auto &h=*static_cast<Harness *>(context);
        if(h.read_delay && address>=Peer::Rx && address<Peer::Rx+Peer::Ring) {
            h.now+=h.read_delay;h.read_delay=0;
        }
    }
    static void notify_hook(void *context)
    {
        auto &h=*static_cast<Harness *>(context);
        if(h.notify_delay){h.now+=h.notify_delay;h.notify_delay=0;}
    }
    Result tick() {++now;return runtime.advance();}
    void start()
    {
        assert(runtime.start()==Result::OK);
        for(unsigned i=0;i<20 && runtime.state()!=Runtime::State::Running;++i) {
            const Result result=tick();assert(result==Result::OK || result==Result::NotReady);
        }
        assert(runtime.state()==Runtime::State::Running && peer.opcodes.size()==8);
    }
    std::array<uint8_t,256> samples(unsigned seed)
    {
        std::array<uint8_t,256> values{};
        for(unsigned i=0;i<values.size();++i)values[i]=uint8_t(i*17+seed*29);
        return values;
    }
    uint32_t queue(unsigned offset,unsigned seed=1)
    {
        const auto input=samples(seed);uint32_t token=0;const unsigned published=peer.opcodes.size();
        assert(runtime.queue_pcm(offset,1024,input.data(),input.size(),&token)==LILAC_ASM_QUEUE_RESERVED);
        assert(token && peer.opcodes.size()==published);
        assert(!std::memcmp(backing.data()+64+offset,input.data(),input.size()));
        for(unsigned i=offset+input.size();i<offset+1024;++i)assert(backing[64+i]==0xa5);
        return token;
    }
    void guards()
    {
        for(unsigned i=0;i<64;++i)assert(backing[i]==0xa5 && backing[8256+i]==0xa5);
    }
    void complete_one(unsigned index)
    {
        assert(index<peer.writes.size());const auto request=peer.writes[index];
        peer.response(request.data(),ADSP::WriteDone,16,true);
        std::memcpy(peer.responses.back().data()+20,request.data()+20,12);
        peer.writes.erase(peer.writes.begin()+index);peer.flush_responses();
    }
    void finish()
    {
        assert(runtime.begin_drain());peer.complete_writes();
        for(unsigned i=0;i<25 && runtime.state()!=Runtime::State::Stopped;++i) {
            const Result result=tick();assert(result==Result::OK || result==Result::NotReady);
        }
        assert(runtime.state()==Runtime::State::Stopped && runtime.region_release_confirmed());
        assert(!runtime.retained_spans() && !runtime.pending_buffers());guards();
    }
};

int main()
{
    for(auto policy:{Runtime::MemoryPolicy::CacheMaintenance,Runtime::MemoryPolicy::WriteCombined})
      for(bool rendered_first:{false,true}) {
        Harness h(policy);h.peer.rendered_first=rendered_first;h.start();
        for(unsigned i=0;i<4;++i){h.queue(i*1024,i+1);assert(h.tick()==Result::OK);}
        assert(h.runtime.retained_spans()==4 && h.runtime.pending_buffers()==4);
        const auto before=h.backing;uint32_t token=0;const auto input=h.samples(99);
        assert(h.runtime.queue_pcm(64,1024,input.data(),256,&token)==LILAC_ASM_QUEUE_BUSY);
        assert(h.runtime.queue_pcm(4096,1024,input.data(),256,&token)==LILAC_ASM_QUEUE_BUSY);
        assert(h.backing==before && h.releases==4 && h.acquires==0);
        h.finish();assert(h.acquires==4 && h.runtime.published_packets()==17);
        assert(h.peer.tx_wraps && h.peer.rx_wraps);
    }
    {Harness h;h.start();const auto before=h.backing;const auto input=h.samples(2);uint32_t token=0;
     for(auto offset:{1u,32u,8192u})assert(h.runtime.queue_pcm(offset,1024,input.data(),256,&token)==LILAC_ASM_QUEUE_INVALID);
     assert(h.runtime.queue_pcm(0,1024,input.data(),255,&token)==LILAC_ASM_QUEUE_INVALID);
     assert(h.backing==before && h.releases==0);}
    {Harness h;h.start();h.queue(0);assert(!h.runtime.begin_drain());
     assert(h.tick()==Result::OK);h.peer.complete_writes();assert(h.tick()==Result::OK);
     assert(h.acquires==1 && !h.runtime.retained_spans());
     h.backing.fill(0xa5);h.queue(0,5);assert(h.tick()==Result::OK);h.finish();}
    for(bool backward:{false,true}) {
        Harness h;h.start();auto *cpu=h.backing.data()+64;
        for(unsigned i=0;i<320;++i)cpu[i]=uint8_t(i*13+7);
        const unsigned source=backward?0:64,target=backward?64:0;
        std::array<uint8_t,256> original{};
        std::memcpy(original.data(),cpu+source,original.size());uint32_t token=0;
        assert(h.runtime.queue_pcm(target,1024,cpu+source,256,&token)==LILAC_ASM_QUEUE_RESERVED);
        assert(!std::memcmp(cpu+target,original.data(),original.size()));
        assert(h.tick()==Result::OK);h.finish();
    }
    {Harness h;h.start();h.queue(0);assert(h.tick()==Result::OK);
     auto stale=h.peer.writes[0];
     ADSP::write32(stale.data()+12,ADSP::read32(stale.data()+12)+99);
     h.peer.response(stale.data(),ADSP::WriteDone,16,true);
     std::memcpy(h.peer.responses.back().data()+20,stale.data()+20,12);h.peer.flush_responses();
     assert(h.tick()==Result::OK && h.acquires==0 && h.runtime.retained_spans()==1);
     h.finish();}
    {Harness h;h.start();h.release_result=Result::IO;const auto input=h.samples(1);uint32_t token=0;
     assert(h.runtime.queue_pcm(0,1024,input.data(),256,&token)==LILAC_ASM_QUEUE_QUARANTINED);
     assert(h.runtime.failure()==Runtime::Failure::Memory && h.runtime.retained_spans()==1);
     assert(h.peer.opcodes.size()==8 && !h.runtime.region_release_confirmed());}
    {Harness h;h.start();h.queue(0);assert(h.tick()==Result::OK);
     h.acquire_result=Result::IO;h.peer.complete_writes();assert(h.tick()==Result::IO);
     assert(h.runtime.retained_spans()==1 && !h.runtime.pending_buffers());
     assert(!h.runtime.region_release_confirmed());}
    {Harness h;h.start();h.lose_epoch_on_release=true;const auto input=h.samples(1);uint32_t token=0;
     assert(h.runtime.queue_pcm(0,1024,input.data(),256,&token)==LILAC_ASM_QUEUE_QUARANTINED);
     assert(h.runtime.failure()==Runtime::Failure::Readiness && h.runtime.retained_spans()==1);
     assert(h.peer.opcodes.size()==8);}
    {Harness h;h.start();h.queue(0);assert(h.tick()==Result::OK);
     h.lose_codec_on_acquire=true;h.peer.complete_writes();assert(h.tick()==Result::NotReady);
     assert(h.runtime.retained_spans()==1 && !h.runtime.region_release_confirmed());}
    {Harness h;assert(h.runtime.start()==Result::OK);assert(h.tick()==Result::OK);
     assert(h.runtime.state()==Runtime::State::Starting);assert(h.tick()==Result::OK);
     assert(h.peer.opcodes.size()==1);h.read_delay=50;
     assert(h.tick()==Result::Timeout);
     assert(h.runtime.transport_failure()==Transport::Failure::ReplyTimeout);
     assert(h.peer.opcodes.size()==1 && !h.runtime.region_release_confirmed());}
    {Harness h;h.start();h.queue(0);h.notify_delay=50;
     assert(h.tick()==Result::Timeout && h.runtime.transport_failure()==Transport::Failure::WriteTimeout);
     assert(h.runtime.retained_spans()==1 && h.runtime.pending_buffers()==1 && h.peer.writes.size()==1);}
    {Harness h;h.start();h.queue(0);assert(h.tick()==Result::OK);
     h.queue(1024);assert(h.tick()==Result::OK);h.acquire_delay=50;h.complete_one(0);
     assert(h.tick()==Result::Timeout && h.runtime.transport_failure()==Transport::Failure::WriteTimeout);
     assert(h.runtime.retained_spans()==2 && h.runtime.pending_buffers()==1 && h.acquires==1);}
    {Harness h;h.start();h.queue(0);assert(h.tick()==Result::OK);
     h.release_delay=50;const auto input=h.samples(3);uint32_t token=0;
     assert(h.runtime.queue_pcm(1024,1024,input.data(),256,&token)==LILAC_ASM_QUEUE_QUARANTINED);
     assert(h.runtime.retained_spans()==2 && h.runtime.pending_buffers()==1 && h.peer.writes.size()==1);}
    {Harness h;h.start();assert(h.runtime.begin_drain());
     for(unsigned i=0;i<15 && h.runtime.state()!=Runtime::State::Closing;++i) {
         const auto result=h.tick();assert(result==Result::OK || result==Result::NotReady);
     }
     assert(h.runtime.state()==Runtime::State::Closing);h.read_delay=50;
     assert(h.tick()==Result::Timeout && h.runtime.transport_failure()==Transport::Failure::CloseTimeout);
     assert(!h.runtime.region_release_confirmed());}
    {Harness h;h.peer.responding=false;assert(h.runtime.start()==Result::OK);
     assert(h.tick()==Result::NotReady);h.now=20;const unsigned read=h.peer.bus_reads;
     assert(h.runtime.advance()==Result::Timeout && h.peer.bus_reads==read);
     assert(h.runtime.failure()==Runtime::Failure::Negotiation);}
    {Harness h;h.clock_ok=false;assert(h.runtime.start()==Result::IO);
     assert(h.runtime.failure()==Runtime::Failure::Clock && h.peer.bus_reads==0);}
    {Harness h;h.start();h.now=0;assert(h.runtime.advance()==Result::Invalid);
     assert(h.runtime.failure()==Runtime::Failure::Clock);}
    {Harness h(Runtime::MemoryPolicy::Unknown);assert(h.runtime.start()==Result::Invalid);
     assert(h.peer.bus_reads==0 && h.peer.bus_writes==0);}
    {Harness h;h.ready.codec_ready=false;assert(h.runtime.start()==Result::NotReady);
     assert(h.peer.bus_reads==0);h.ready.codec_ready=true;h.start();}
    std::printf("Lilac PCM retained runtime bytes=%zu outside_kernel_stack_required=1\n",sizeof(Runtime));
    puts("Lilac PCM runtime PASS: concrete FIFO/GLINK/session owner, matched readiness, byte-exact PCM -> cache release -> publication, four retained spans and acquire before reuse, callbacks/late IO deadlines/epoch/clock failures; host memory only");
    return 0;
}
