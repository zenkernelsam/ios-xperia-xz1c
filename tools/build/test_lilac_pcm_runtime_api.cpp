/* SPDX-License-Identifier: GPL-2.0-only
 * Real C11 caller and production C++ runtime over the authored SMEM peer.
 * Host byte views only; no actual DSP, cache instructions or codec IO. */
#include "LilacPcmRuntimeCTest.h"
#include "LilacPcmSmemPeerFixture.h"
#include <array>

struct ApiHarness {
    Peer peer;
    alignas(64) std::array<uint8_t,65536> storage{};
    alignas(64) std::array<uint8_t,8320> backing;
    std::array<uint8_t,8192> device;
    lilac_pcm_runtime_config config{};
    lilac_pcm_runtime_callbacks callbacks{};
    lilac_pcm_runtime_readiness ready{9,1,1,1,1};
    uint64_t now=0;
    uint32_t ready_result=LILAC_PCM_OK,release_result=LILAC_PCM_OK,acquire_result=LILAC_PCM_OK;
    unsigned readiness_calls=0,clock_calls=0,releases=0,acquires=0,publications=0;
    int clock_result=1;
    bool reenter=false;
    size_t bytes=c_pcm_bytes();
    ApiHarness()
    {
        assert(bytes<=storage.size() && c_pcm_alignment()<=64);
        backing.fill(0xa5);device.fill(0xcc);
        const auto session=configuration();
        config.version=LILAC_PCM_RUNTIME_API_VERSION;config.bytes=sizeof(config);
        config.epoch=9;config.pcm_device_address=UINT64_C(0x123456000);
        config.pcm_cpu=backing.data()+64;config.pcm_bytes=8192;config.cache_line_bytes=64;
        config.memory_policy=LILAC_PCM_CACHE_MAINTENANCE;
        config.stream_topology=session.stream_topology;config.output_topology=session.output_topology;
        config.stream_port=session.stream_port;config.parameter_schema=2;config.codec_device=session.codec_channels.device;
        config.codec_channels[0]=session.codec_channels.channels[0];config.codec_channels[1]=session.codec_channels.channels[1];
        config.negotiation_ms=20;config.publish_ms=10;config.reply_ms=20;config.write_ms=20;config.close_ms=20;
        callbacks.version=LILAC_PCM_RUNTIME_API_VERSION;callbacks.bytes=sizeof(callbacks);
        callbacks.fifo=peer.callbacks();callbacks.context=this;
        callbacks.readiness=readiness;callbacks.now_ms=clock;
        callbacks.release_pcm_to_device=release;callbacks.acquire_pcm_from_device=acquire;
        peer.hook_context=this;peer.inspect_pcm=inspect;
    }
    void check_reentry()
    {
        if(!reenter)return;
        lilac_pcm_runtime_snapshot snapshot{};uint32_t token=0;
        assert(c_pcm_start(storage.data(),bytes)==LILAC_PCM_BUSY);
        assert(c_pcm_advance(storage.data(),bytes)==LILAC_PCM_BUSY);
        assert(c_pcm_drain(storage.data(),bytes)==LILAC_PCM_BUSY);
        assert(c_pcm_uncertain(storage.data(),bytes)==LILAC_PCM_BUSY);
        assert(c_pcm_destroy(storage.data(),bytes)==LILAC_PCM_BUSY);
        assert(c_pcm_status(storage.data(),bytes,&snapshot)==LILAC_PCM_BUSY);
        assert(c_pcm_queue(storage.data(),bytes,0,1024,backing.data()+64,256,&token)==LILAC_ASM_QUEUE_BUSY);
    }
    static uint32_t readiness(void *context,lilac_pcm_runtime_readiness *out)
    {
        auto &h=*static_cast<ApiHarness *>(context);++h.readiness_calls;h.check_reentry();
        *out=h.ready;return h.ready_result;
    }
    static int clock(void *context,uint64_t *out)
    {
        auto &h=*static_cast<ApiHarness *>(context);++h.clock_calls;h.check_reentry();
        *out=h.now;return h.clock_result;
    }
    static uint32_t release(void *context,uint64_t epoch,uint32_t offset,uint32_t span,uint32_t policy)
    {
        auto &h=*static_cast<ApiHarness *>(context);++h.releases;h.check_reentry();
        assert(epoch==9 && offset+span<=8192 && !(offset&63) && !(span&63));
        assert(policy==LILAC_PCM_CACHE_MAINTENANCE || policy==LILAC_PCM_WRITE_COMBINED);
        if(h.release_result==LILAC_PCM_OK)
            std::memcpy(h.device.data()+offset,h.backing.data()+64+offset,span);
        return h.release_result;
    }
    static uint32_t acquire(void *context,uint64_t epoch,uint32_t offset,uint32_t span,uint32_t policy)
    {
        auto &h=*static_cast<ApiHarness *>(context);++h.acquires;h.check_reentry();
        assert(epoch==9 && offset+span<=8192 && policy!=0);return h.acquire_result;
    }
    static void inspect(void *context,const uint8_t *packet,size_t bytes)
    {
        auto &h=*static_cast<ApiHarness *>(context);
        assert(bytes==52 && h.releases>h.publications);
        const uint64_t address=uint64_t(ADSP::read32(packet+20))|(uint64_t(ADSP::read32(packet+24))<<32);
        const size_t offset=address-UINT64_C(0x123456000);
        assert(offset+256<=8192 && !std::memcmp(h.device.data()+offset,h.backing.data()+64+offset,256));
        ++h.publications;
    }
    bool zeroed() const
    {
        for(size_t i=0;i<bytes;++i)if(storage[i])return false;
        return true;
    }
    void no_io() const
    {
        assert(peer.bus_reads==0 && peer.bus_writes==0 && readiness_calls==0 && clock_calls==0 &&
               releases==0 && acquires==0);
    }
    void construct()
    {
        assert(c_pcm_construct(storage.data(),bytes,&config,&callbacks)==LILAC_PCM_OK);
        no_io();assert(!zeroed());
        for(size_t i=bytes;i<storage.size();++i)assert(storage[i]==0);
    }
    lilac_pcm_runtime_snapshot status()
    {
        lilac_pcm_runtime_snapshot value{};
        assert(c_pcm_status(storage.data(),bytes,&value)==LILAC_PCM_OK);
        assert(value.version==1 && value.bytes==sizeof(value) && value.epoch==9 && value.reserved==0);
        return value;
    }
    uint32_t tick() {++now;return c_pcm_advance(storage.data(),bytes);}
    void start()
    {
        assert(c_pcm_start(storage.data(),bytes)==LILAC_PCM_OK);
        for(unsigned i=0;i<20 && status().state!=LILAC_PCM_STATE_RUNNING;++i) {
            const auto result=tick();assert(result==LILAC_PCM_OK || result==LILAC_PCM_NOT_READY);
        }
        assert(status().state==LILAC_PCM_STATE_RUNNING && peer.opcodes.size()==8);
    }
    uint32_t queue(unsigned offset)
    {
        std::array<uint8_t,256> data{};
        for(unsigned i=0;i<data.size();++i)data[i]=uint8_t(i*17+offset/64);
        uint32_t token=0;
        assert(c_pcm_queue(storage.data(),bytes,offset,1024,data.data(),data.size(),&token)==LILAC_ASM_QUEUE_RESERVED);
        assert(token && !std::memcmp(backing.data()+64+offset,data.data(),data.size()));
        return token;
    }
    void finish()
    {
        assert(c_pcm_drain(storage.data(),bytes)==LILAC_PCM_OK);peer.complete_writes();
        for(unsigned i=0;i<25 && status().state!=LILAC_PCM_STATE_STOPPED;++i) {
            const auto result=tick();assert(result==LILAC_PCM_OK || result==LILAC_PCM_NOT_READY);
        }
        const auto value=status();
        assert(value.state==LILAC_PCM_STATE_STOPPED && value.region_release_confirmed==1 &&
               value.retained_spans==0 && value.pending_buffers==0 && value.published_packets==13+publications);
        for(unsigned i=0;i<64;++i)assert(backing[i]==0xa5 && backing[8256+i]==0xa5);
        const unsigned reads=peer.bus_reads,writes=peer.bus_writes,clocks=clock_calls;
        assert(c_pcm_destroy(storage.data(),bytes)==LILAC_PCM_OK && zeroed());
        assert(peer.bus_reads==reads && peer.bus_writes==writes && clock_calls==clocks);
    }
};

int main()
{
    for(auto policy:{LILAC_PCM_WRITE_COMBINED,LILAC_PCM_CACHE_MAINTENANCE})
      for(auto schema:{2,3})for(bool rendered_first:{false,true}) {
        ApiHarness h;h.config.memory_policy=policy;h.config.parameter_schema=schema;
        h.peer.rendered_first=rendered_first;h.construct();h.reenter=true;
        // Configuration and callback tables are copied, backend contexts remain retained.
        h.config={};h.callbacks={};h.start();
        for(unsigned i=0;i<4;++i){h.queue(i*1024);assert(h.tick()==LILAC_PCM_OK);}
        assert(h.status().retained_spans==4 && h.status().pending_buffers==4);
        const auto before=h.backing;uint32_t token=0;
        assert(c_pcm_queue(h.storage.data(),h.bytes,4096,1024,h.backing.data()+64,256,&token)==LILAC_ASM_QUEUE_BUSY);
        assert(h.backing==before && token==0 && h.releases==4);
        assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_NOT_READY);
        h.finish();assert(h.acquires==4);
        lilac_pcm_runtime_snapshot snapshot{};
        assert(c_pcm_status(h.storage.data(),h.bytes,&snapshot)==LILAC_PCM_INVALID);
        assert(c_pcm_start(h.storage.data(),h.bytes)==LILAC_PCM_INVALID);
        assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_INVALID);
    }
    {ApiHarness h;h.construct();const auto before=h.storage;h.no_io();
     assert(c_pcm_construct(h.storage.data(),h.bytes,&h.config,&h.callbacks)==LILAC_PCM_BUSY);
     assert(h.storage==before);assert(h.status().state==LILAC_PCM_STATE_NOT_READY);h.no_io();
     assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_OK && h.zeroed());h.no_io();
     h.construct();assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_OK);}
    {ApiHarness h;h.construct();ApiHarness copy;copy.storage=h.storage;
     lilac_pcm_runtime_snapshot snapshot{};
     assert(c_pcm_status(copy.storage.data(),copy.bytes,&snapshot)==LILAC_PCM_INVALID);
     assert(c_pcm_start(copy.storage.data(),copy.bytes)==LILAC_PCM_INVALID);
     assert(c_pcm_advance(copy.storage.data(),copy.bytes)==LILAC_PCM_INVALID);
     assert(c_pcm_destroy(copy.storage.data(),copy.bytes)==LILAC_PCM_INVALID);
     h.no_io();copy.no_io();assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_OK);}
    {ApiHarness h;
     assert(c_pcm_construct(nullptr,h.bytes,&h.config,&h.callbacks)==LILAC_PCM_INVALID);
     assert(c_pcm_construct(h.storage.data()+1,h.bytes,&h.config,&h.callbacks)==LILAC_PCM_INVALID);
     assert(c_pcm_construct(h.storage.data(),h.bytes-1,&h.config,&h.callbacks)==LILAC_PCM_INVALID);
     assert(c_pcm_construct(h.storage.data(),h.bytes,nullptr,&h.callbacks)==LILAC_PCM_INVALID);
     h.storage[h.bytes-1]=1;
     assert(c_pcm_construct(h.storage.data(),h.bytes,&h.config,&h.callbacks)==LILAC_PCM_BUSY);h.no_io();}
    // Pure configuration validation rejects malformed rings/hooks before callback IO.
    for(unsigned mutation=0;mutation<22;++mutation) {
        ApiHarness h;
        switch(mutation) {
        case 0:h.config.version=2;break;case 1:--h.config.bytes;break;
        case 2:h.callbacks.version=2;break;case 3:--h.callbacks.bytes;break;
        case 4:h.config.parameter_schema=4;break;case 5:h.config.memory_policy=0;break;
        case 6:h.callbacks.readiness=nullptr;break;case 7:h.callbacks.now_ms=nullptr;break;
        case 8:h.callbacks.release_pcm_to_device=nullptr;break;case 9:h.callbacks.acquire_pcm_from_device=nullptr;break;
        case 10:h.config.pcm_cpu=h.storage.data();break;case 11:h.callbacks.context=h.storage.data()+16;break;
        case 12:h.callbacks.fifo.context=h.storage.data()+16;break;case 13:h.config.epoch=10;break;
        case 14:h.config.cache_line_bytes=63;break;case 15:--h.config.pcm_bytes;break;
        case 16:h.config.negotiation_ms=0;break;case 17:h.callbacks.fifo.read32=nullptr;break;
        case 18:h.callbacks.fifo.write32=nullptr;break;case 19:h.callbacks.fifo.fence=nullptr;break;
        case 20:h.callbacks.fifo.notify=nullptr;break;case 21:h.callbacks.fifo.rx=h.callbacks.fifo.tx;break;
        }
        assert(c_pcm_construct(h.storage.data(),h.bytes,&h.config,&h.callbacks)==LILAC_PCM_INVALID);
        assert(h.zeroed());h.no_io();
    }
    {ApiHarness h;h.construct();h.start();const auto before=h.backing;const auto object=h.storage;
     for(auto *token:{reinterpret_cast<uint32_t *>(h.storage.data()),reinterpret_cast<uint32_t *>(h.backing.data()+64),
                     reinterpret_cast<uint32_t *>(h.backing.data()+1)}) {
         assert(c_pcm_queue(h.storage.data(),h.bytes,0,1024,h.backing.data()+64,256,token)==LILAC_ASM_QUEUE_INVALID);
     }
     assert(c_pcm_queue(h.storage.data(),h.bytes,0,1024,h.backing.data()+64,256,nullptr)==LILAC_ASM_QUEUE_INVALID);
     assert(c_pcm_status(h.storage.data(),h.bytes,reinterpret_cast<lilac_pcm_runtime_snapshot *>(h.storage.data()))==LILAC_PCM_INVALID);
     assert(c_pcm_status(h.storage.data(),h.bytes,reinterpret_cast<lilac_pcm_runtime_snapshot *>(h.backing.data()+64))==LILAC_PCM_INVALID);
     assert(c_pcm_status(h.storage.data(),h.bytes,reinterpret_cast<lilac_pcm_runtime_snapshot *>(h.backing.data()+1))==LILAC_PCM_INVALID);
     assert(h.backing==before && h.storage==object && h.releases==0);h.finish();}
    for(auto field:{0,1,2,3}) {
        ApiHarness h;h.construct();
        switch(field) {case 0:h.ready.exclusive_endpoint=2;break;case 1:h.ready.smem_uncached=2;break;
                       case 2:h.ready.dsp_ready=2;break;case 3:h.ready.codec_ready=2;break;}
        assert(c_pcm_start(h.storage.data(),h.bytes)==LILAC_PCM_INVALID && h.peer.bus_reads==0);
        assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_OK);
    }
    {ApiHarness h;h.construct();h.ready.epoch=10;
     assert(c_pcm_start(h.storage.data(),h.bytes)==LILAC_PCM_STALE && h.peer.bus_reads==0);
     assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_OK);}
    {ApiHarness h;h.construct();h.ready.codec_ready=0;
     assert(c_pcm_start(h.storage.data(),h.bytes)==LILAC_PCM_NOT_READY && h.peer.bus_reads==0);
     h.ready.codec_ready=1;h.start();h.finish();}
    {ApiHarness h;h.construct();h.ready_result=999;
     assert(c_pcm_start(h.storage.data(),h.bytes)==LILAC_PCM_INVALID && h.peer.bus_reads==0);
     assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_OK);}
    for(int invalid_clock:{0,2,-1}) {
        ApiHarness h;h.construct();h.clock_result=invalid_clock;
        assert(c_pcm_start(h.storage.data(),h.bytes)==LILAC_PCM_IO && h.peer.bus_reads==0);
        assert(h.status().state==LILAC_PCM_STATE_UNCERTAIN);
        assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_NOT_READY);
    }
    for(auto result:{uint32_t(LILAC_PCM_IO),uint32_t(999)}) {
        ApiHarness h;h.construct();h.start();h.release_result=result;uint32_t token=0;
        assert(c_pcm_queue(h.storage.data(),h.bytes,0,1024,h.backing.data()+64,256,&token)==LILAC_ASM_QUEUE_QUARANTINED);
        const auto value=h.status();
        assert(value.state==LILAC_PCM_STATE_UNCERTAIN && value.retained_spans==1 && token==0);
        assert(value.failure_result==(result==999?LILAC_PCM_INVALID:LILAC_PCM_IO));
        assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_NOT_READY);
    }
    {ApiHarness h;h.construct();h.start();h.queue(0);assert(h.tick()==LILAC_PCM_OK);
     h.acquire_result=LILAC_PCM_IO;h.peer.complete_writes();
     assert(h.tick()==LILAC_PCM_IO && h.status().retained_spans==1);
     assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_NOT_READY);}
    {ApiHarness h;h.construct();h.start();h.queue(0);h.peer.fail_notify=true;
     assert(h.tick()==LILAC_PCM_IO && h.status().state==LILAC_PCM_STATE_UNCERTAIN);
     assert(h.status().retained_spans==1 && !h.status().region_release_confirmed);
     assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_NOT_READY);}
    {ApiHarness h;h.construct();assert(c_pcm_start(h.storage.data(),h.bytes)==LILAC_PCM_OK);
     assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_NOT_READY);
     assert(c_pcm_uncertain(h.storage.data(),h.bytes)==LILAC_PCM_OK);
     assert(c_pcm_advance(h.storage.data(),h.bytes)==LILAC_PCM_QUARANTINED);
     assert(c_pcm_destroy(h.storage.data(),h.bytes)==LILAC_PCM_NOT_READY);}
    std::printf("Lilac PCM C ABI context bytes=%zu alignment=%zu\n",c_pcm_bytes(),c_pcm_alignment());
    puts("Lilac PCM C ABI PASS: real C11 caller, copied tables, zeroed caller storage, pure validation, four byte-exact writes, EOS/close, reentry refusal and retained uncertain ownership; host only");
    return 0;
}
