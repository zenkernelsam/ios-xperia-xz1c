/* SPDX-License-Identifier: GPL-2.0-only */
#include "LilacPcmRuntimeAPI.h"
#include "LilacPcmPlaybackRuntime.h"

namespace {
using Runtime=LilacPcmPlaybackRuntime;
using Result=Runtime::Result;
constexpr uint64_t Magic=UINT64_C(0x315854434d43504c);
struct Context {
    uint64_t magic=0,epoch=0;
    uintptr_t self=0;
    uint32_t version=0,object_bytes=0;
    bool entered=false;
    lilac_pcm_runtime_config config{};
    lilac_pcm_runtime_callbacks callbacks{};
    alignas(Runtime) uint8_t object[sizeof(Runtime)]{};
    static void *operator new(size_t,void *memory) {return memory;}
    static void operator delete(void *,void *) {}
    Runtime &runtime() {return *reinterpret_cast<Runtime *>(object);}
};
static_assert(sizeof(Context)<=65536,"retained PCM context exceeds its bounded storage contract");
#define CHECK_RESULT(cpp,c) static_assert(uint32_t(Result::cpp)==c,"PCM C result ABI differs")
CHECK_RESULT(OK,LILAC_PCM_OK);CHECK_RESULT(Invalid,LILAC_PCM_INVALID);
CHECK_RESULT(Unsupported,LILAC_PCM_UNSUPPORTED);CHECK_RESULT(NotReady,LILAC_PCM_NOT_READY);
CHECK_RESULT(Busy,LILAC_PCM_BUSY);CHECK_RESULT(Timeout,LILAC_PCM_TIMEOUT);
CHECK_RESULT(Authentication,LILAC_PCM_AUTHENTICATION);CHECK_RESULT(NoMemory,LILAC_PCM_NO_MEMORY);
CHECK_RESULT(IO,LILAC_PCM_IO);CHECK_RESULT(Stale,LILAC_PCM_STALE);
CHECK_RESULT(Quarantined,LILAC_PCM_QUARANTINED);
#undef CHECK_RESULT
#define CHECK_STATE(cpp,c) static_assert(uint32_t(Runtime::State::cpp)==c,"PCM C state ABI differs")
CHECK_STATE(NotReady,LILAC_PCM_STATE_NOT_READY);CHECK_STATE(Negotiating,LILAC_PCM_STATE_NEGOTIATING);
CHECK_STATE(Starting,LILAC_PCM_STATE_STARTING);CHECK_STATE(Running,LILAC_PCM_STATE_RUNNING);
CHECK_STATE(Draining,LILAC_PCM_STATE_DRAINING);CHECK_STATE(Closing,LILAC_PCM_STATE_CLOSING);
CHECK_STATE(Stopped,LILAC_PCM_STATE_STOPPED);CHECK_STATE(Uncertain,LILAC_PCM_STATE_UNCERTAIN);
#undef CHECK_STATE
static_assert(uint32_t(Runtime::MemoryPolicy::WriteCombined)==LILAC_PCM_WRITE_COMBINED &&
              uint32_t(Runtime::MemoryPolicy::CacheMaintenance)==LILAC_PCM_CACHE_MAINTENANCE,
              "PCM C memory policy ABI differs");

bool range(uintptr_t base,size_t bytes) {return base && bytes && bytes<=UINTPTR_MAX-base;}
bool overlaps(uintptr_t a,size_t na,uintptr_t b,size_t nb)
{
    return range(a,na) && range(b,nb) && a<b+nb && b<a+na;
}
Context *context(void *memory,size_t bytes)
{
    const uintptr_t address=reinterpret_cast<uintptr_t>(memory);
    if(bytes<sizeof(Context) || !range(address,sizeof(Context)) || address%alignof(Context))return nullptr;
    // Unconstructed or already destroyed storage has no Context lifetime.
    // Inspect its byte representation before referring to a live object.
    const auto *data=static_cast<const uint8_t *>(memory);
    uint64_t magic=0,epoch=0;uintptr_t self=0;uint32_t version=0,object_bytes=0;
    auto copy=[data](void *out,size_t offset,size_t count) {
        auto *target=static_cast<uint8_t *>(out);
        for(size_t i=0;i<count;++i)target[i]=data[offset+i];
    };
    copy(&magic,offsetof(Context,magic),sizeof(magic));
    copy(&epoch,offsetof(Context,epoch),sizeof(epoch));
    copy(&self,offsetof(Context,self),sizeof(self));
    copy(&version,offsetof(Context,version),sizeof(version));
    copy(&object_bytes,offsetof(Context,object_bytes),sizeof(object_bytes));
    return magic==Magic && self==address && version==LILAC_PCM_RUNTIME_API_VERSION &&
        object_bytes==sizeof(Runtime) && epoch?static_cast<Context *>(memory):nullptr;
}
bool output(Context &ctx,const void *pointer,size_t bytes,size_t alignment)
{
    const uintptr_t address=reinterpret_cast<uintptr_t>(pointer);
    return range(address,bytes) && !(address%alignment) &&
        !overlaps(address,bytes,reinterpret_cast<uintptr_t>(&ctx),sizeof(ctx)) &&
        !overlaps(address,bytes,reinterpret_cast<uintptr_t>(ctx.config.pcm_cpu),ctx.config.pcm_bytes);
}
Result callback_result(uint32_t value)
{
    return value<=LILAC_PCM_QUARANTINED?static_cast<Result>(value):Result::Invalid;
}
Result readiness(void *opaque,Runtime::Readiness *out)
{
    auto &ctx=*static_cast<Context *>(opaque);lilac_pcm_runtime_readiness ready{};
    const Result result=callback_result(ctx.callbacks.readiness(ctx.callbacks.context,&ready));
    if(result!=Result::OK)return result;
    if(ready.exclusive_endpoint>1 || ready.smem_uncached>1 || ready.dsp_ready>1 || ready.codec_ready>1)
        return Result::Invalid;
    *out={ready.epoch,ready.exclusive_endpoint==1,ready.smem_uncached==1,ready.dsp_ready==1,ready.codec_ready==1};
    return Result::OK;
}
bool clock(void *opaque,uint64_t *now)
{
    auto &ctx=*static_cast<Context *>(opaque);
    return ctx.callbacks.now_ms(ctx.callbacks.context,now)==1;
}
Result release(void *opaque,uint64_t epoch,uint32_t offset,uint32_t span,Runtime::MemoryPolicy policy)
{
    auto &ctx=*static_cast<Context *>(opaque);
    return callback_result(ctx.callbacks.release_pcm_to_device(ctx.callbacks.context,epoch,offset,span,uint32_t(policy)));
}
Result acquire(void *opaque,uint64_t epoch,uint32_t offset,uint32_t span,Runtime::MemoryPolicy policy)
{
    auto &ctx=*static_cast<Context *>(opaque);
    return callback_result(ctx.callbacks.acquire_pcm_from_device(ctx.callbacks.context,epoch,offset,span,uint32_t(policy)));
}
void clear(void *memory)
{
    auto *data=static_cast<uint8_t *>(memory);
    for(size_t i=0;i<sizeof(Context);++i)data[i]=0;
}
struct Entry {
    Context &ctx;
    explicit Entry(Context &value):ctx(value){ctx.entered=true;}
    ~Entry(){ctx.entered=false;}
};
}

extern "C" size_t lilac_pcm_runtime_context_bytes(void) {return sizeof(Context);}
extern "C" size_t lilac_pcm_runtime_context_alignment(void) {return alignof(Context);}

extern "C" uint32_t lilac_pcm_runtime_construct(void *memory,size_t bytes,
    const lilac_pcm_runtime_config *configuration,const lilac_pcm_runtime_callbacks *callbacks)
{
    const uintptr_t address=reinterpret_cast<uintptr_t>(memory);
    if(!configuration || !callbacks || bytes<sizeof(Context) || !range(address,sizeof(Context)) ||
        address%alignof(Context))return LILAC_PCM_INVALID;
    const auto config=*configuration;const auto ops=*callbacks;
    if(config.version!=LILAC_PCM_RUNTIME_API_VERSION || config.bytes!=sizeof(config) ||
        ops.version!=LILAC_PCM_RUNTIME_API_VERSION || ops.bytes!=sizeof(ops) ||
        (config.parameter_schema!=2 && config.parameter_schema!=3) ||
        (config.memory_policy!=LILAC_PCM_WRITE_COMBINED && config.memory_policy!=LILAC_PCM_CACHE_MAINTENANCE) ||
        !ops.readiness || !ops.now_ms || !ops.release_pcm_to_device || !ops.acquire_pcm_from_device ||
        !range(reinterpret_cast<uintptr_t>(config.pcm_cpu),config.pcm_bytes) ||
        overlaps(address,sizeof(Context),reinterpret_cast<uintptr_t>(config.pcm_cpu),config.pcm_bytes) ||
        overlaps(address,sizeof(Context),reinterpret_cast<uintptr_t>(ops.context),1) ||
        overlaps(address,sizeof(Context),reinterpret_cast<uintptr_t>(ops.fifo.context),1))
        return LILAC_PCM_INVALID;
    const auto *original=static_cast<const uint8_t *>(memory);
    for(size_t i=0;i<sizeof(Context);++i)if(original[i])return LILAC_PCM_BUSY;
    auto *ctx=new(memory) Context{};
    ctx->config=config;ctx->callbacks=ops;
    const Runtime::Session::Configuration session{
        config.epoch,config.stream_port,config.stream_topology,config.output_topology,
        config.pcm_device_address,config.pcm_bytes,
        config.parameter_schema==2?LilacAfeSlimbusPlayback::ParameterSchema::V2:LilacAfeSlimbusPlayback::ParameterSchema::V3,
        {config.codec_device,{config.codec_channels[0],config.codec_channels[1]}}
    };
    const Runtime::Region pcm{static_cast<uint8_t *>(config.pcm_cpu),config.pcm_device_address,
        config.epoch,config.pcm_bytes,config.cache_line_bytes,static_cast<Runtime::MemoryPolicy>(config.memory_policy)};
    const Runtime::Callbacks runtime_callbacks{ctx,readiness,clock,release,acquire};
    const Runtime::Timeouts timeouts{config.negotiation_ms,
        {config.publish_ms,config.reply_ms,config.write_ms,config.close_ms}};
    auto *runtime=new(ctx->object) Runtime(session,ops.fifo,pcm,runtime_callbacks,timeouts);
    if(!runtime->configuration_valid()) {
        runtime->~Runtime();ctx->~Context();clear(memory);return LILAC_PCM_INVALID;
    }
    ctx->magic=Magic;ctx->self=address;ctx->version=LILAC_PCM_RUNTIME_API_VERSION;
    ctx->object_bytes=sizeof(Runtime);ctx->epoch=config.epoch;
    return LILAC_PCM_OK;
}

extern "C" uint32_t lilac_pcm_runtime_start(void *memory,size_t bytes)
{
    auto *ctx=context(memory,bytes);if(!ctx)return LILAC_PCM_INVALID;
    if(ctx->entered)return LILAC_PCM_BUSY;Entry entry(*ctx);
    return uint32_t(ctx->runtime().start());
}
extern "C" uint32_t lilac_pcm_runtime_advance(void *memory,size_t bytes)
{
    auto *ctx=context(memory,bytes);if(!ctx)return LILAC_PCM_INVALID;
    if(ctx->entered)return LILAC_PCM_BUSY;Entry entry(*ctx);
    return uint32_t(ctx->runtime().advance());
}
extern "C" lilac_asm_pcm_queue_result lilac_pcm_runtime_queue(void *memory,size_t storage_bytes,
    uint32_t offset,uint32_t span,const void *data,uint32_t bytes,uint32_t *token)
{
    auto *ctx=context(memory,storage_bytes);if(!ctx)return LILAC_ASM_QUEUE_INVALID;
    if(ctx->entered)return LILAC_ASM_QUEUE_BUSY;
    if(!output(*ctx,token,sizeof(*token),alignof(uint32_t)))return LILAC_ASM_QUEUE_INVALID;
    Entry entry(*ctx);return ctx->runtime().queue_pcm(offset,span,data,bytes,token);
}
extern "C" uint32_t lilac_pcm_runtime_drain(void *memory,size_t bytes)
{
    auto *ctx=context(memory,bytes);if(!ctx)return LILAC_PCM_INVALID;
    if(ctx->entered)return LILAC_PCM_BUSY;Entry entry(*ctx);
    const bool result=ctx->runtime().begin_drain();
    if(ctx->runtime().state()==Runtime::State::Uncertain)return uint32_t(ctx->runtime().failure_result());
    return result?LILAC_PCM_OK:LILAC_PCM_NOT_READY;
}
extern "C" uint32_t lilac_pcm_runtime_status(void *memory,size_t bytes,lilac_pcm_runtime_snapshot *out)
{
    auto *ctx=context(memory,bytes);if(!ctx)return LILAC_PCM_INVALID;
    if(ctx->entered)return LILAC_PCM_BUSY;
    if(!output(*ctx,out,sizeof(*out),alignof(lilac_pcm_runtime_snapshot)))return LILAC_PCM_INVALID;
    const auto &runtime=ctx->runtime();
    *out={LILAC_PCM_RUNTIME_API_VERSION,sizeof(*out),uint32_t(runtime.state()),uint32_t(runtime.failure()),
        uint32_t(runtime.failure_result()),uint32_t(runtime.transport_failure()),runtime.pending_buffers(),
        runtime.retained_spans(),runtime.region_release_confirmed()?1u:0u,0,ctx->epoch,runtime.published_packets()};
    return LILAC_PCM_OK;
}
extern "C" uint32_t lilac_pcm_runtime_uncertain(void *memory,size_t bytes)
{
    auto *ctx=context(memory,bytes);if(!ctx)return LILAC_PCM_INVALID;
    if(ctx->entered)return LILAC_PCM_BUSY;Entry entry(*ctx);ctx->runtime().uncertain();return LILAC_PCM_OK;
}
extern "C" uint32_t lilac_pcm_runtime_destroy(void *memory,size_t bytes)
{
    auto *ctx=context(memory,bytes);if(!ctx)return LILAC_PCM_INVALID;
    if(ctx->entered)return LILAC_PCM_BUSY;
    const auto state=ctx->runtime().state();
    if(state!=Runtime::State::NotReady &&
        (state!=Runtime::State::Stopped || !ctx->runtime().region_release_confirmed()))return LILAC_PCM_NOT_READY;
    ctx->runtime().~Runtime();ctx->~Context();clear(memory);return LILAC_PCM_OK;
}
