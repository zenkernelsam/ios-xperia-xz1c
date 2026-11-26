/* SPDX-License-Identifier: GPL-2.0-only
 * C ABI for a caller-retained PCM runtime. The owner supplies aligned zeroed
 * storage outside the kernel stack and retains PCM mapping/backend contexts.
 * Storage must not be copied or relocated during its constructed lifetime.
 * Calls are externally serialized; callbacks must not reenter this API.
 * This interface allocates nothing and never starts/resets DSP hardware. */
#ifndef LILAC_PCM_RUNTIME_API_H
#define LILAC_PCM_RUNTIME_API_H

#include <stdint.h>
#include <stddef.h>
#include "ADSPAPRProbe.h"
#include "LilacAsmPcmQueue.h"

enum {
    LILAC_PCM_RUNTIME_API_VERSION=1,
    LILAC_PCM_OK=0, LILAC_PCM_INVALID=1, LILAC_PCM_UNSUPPORTED=2,
    LILAC_PCM_NOT_READY=3, LILAC_PCM_BUSY=4, LILAC_PCM_TIMEOUT=5,
    LILAC_PCM_AUTHENTICATION=6, LILAC_PCM_NO_MEMORY=7, LILAC_PCM_IO=8,
    LILAC_PCM_STALE=9, LILAC_PCM_QUARANTINED=10,
    LILAC_PCM_WRITE_COMBINED=1, LILAC_PCM_CACHE_MAINTENANCE=2,
    LILAC_PCM_STATE_NOT_READY=0, LILAC_PCM_STATE_NEGOTIATING=1,
    LILAC_PCM_STATE_STARTING=2, LILAC_PCM_STATE_RUNNING=3,
    LILAC_PCM_STATE_DRAINING=4, LILAC_PCM_STATE_CLOSING=5,
    LILAC_PCM_STATE_STOPPED=6, LILAC_PCM_STATE_UNCERTAIN=7
};

struct lilac_pcm_runtime_config {
    uint32_t version,bytes;
    uint64_t epoch,pcm_device_address;
    void *pcm_cpu;
    uint32_t pcm_bytes,cache_line_bytes,memory_policy;
    uint32_t stream_topology,output_topology;
    uint16_t stream_port,parameter_schema,codec_device;
    uint8_t codec_channels[2];
    uint32_t negotiation_ms,publish_ms,reply_ms,write_ms,close_ms;
};
struct lilac_pcm_runtime_readiness {
    uint64_t epoch;
    uint32_t exclusive_endpoint,smem_uncached,dsp_ready,codec_ready;
};
struct lilac_pcm_runtime_callbacks {
    uint32_t version,bytes;
    struct sweet_adsp_apr_ops fifo;
    void *context;
    uint32_t (*readiness)(void *,struct lilac_pcm_runtime_readiness *);
    int (*now_ms)(void *,uint64_t *);
    uint32_t (*release_pcm_to_device)(void *,uint64_t,uint32_t,uint32_t,uint32_t);
    uint32_t (*acquire_pcm_from_device)(void *,uint64_t,uint32_t,uint32_t,uint32_t);
};
struct lilac_pcm_runtime_snapshot {
    uint32_t version,bytes,state,failure,failure_result,transport_failure;
    uint32_t pending_buffers,retained_spans,region_release_confirmed,reserved;
    uint64_t epoch,published_packets;
};

#ifdef __cplusplus
extern "C" {
#endif
size_t lilac_pcm_runtime_context_bytes(void);
size_t lilac_pcm_runtime_context_alignment(void);
uint32_t lilac_pcm_runtime_construct(void *,size_t,const struct lilac_pcm_runtime_config *,
                                    const struct lilac_pcm_runtime_callbacks *);
uint32_t lilac_pcm_runtime_start(void *,size_t);
uint32_t lilac_pcm_runtime_advance(void *,size_t);
enum lilac_asm_pcm_queue_result lilac_pcm_runtime_queue(void *,size_t,uint32_t,uint32_t,
                                                       const void *,uint32_t,uint32_t *);
uint32_t lilac_pcm_runtime_drain(void *,size_t);
uint32_t lilac_pcm_runtime_status(void *,size_t,struct lilac_pcm_runtime_snapshot *);
uint32_t lilac_pcm_runtime_uncertain(void *,size_t);
/* Destruction is allowed only before start or after confirmed session and
 * bidirectional GLINK shutdown. Uncertain/in-flight objects remain retained.
 * The owner, rather than this API, releases the actual physical PCM pages. */
uint32_t lilac_pcm_runtime_destroy(void *,size_t);
#ifdef __cplusplus
}
#endif
#endif
