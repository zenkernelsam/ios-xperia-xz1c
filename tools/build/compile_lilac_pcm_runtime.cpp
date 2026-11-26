/* SPDX-License-Identifier: GPL-2.0-only
 * Concrete retained runtime ABI check. Callbacks and PCM mapping are supplied
 * externally; no object allocation, driver activation or hardware IO here. */
#include "../../xnu/support/LilacPcmPlaybackRuntime.h"

using Runtime = LilacPcmPlaybackRuntime;

size_t lilac_pcm_runtime_storage_bytes()
{
    return sizeof(Runtime);
}

ADSP::Result lilac_pcm_runtime_start(Runtime &runtime)
{
    return runtime.start();
}

ADSP::Result lilac_pcm_runtime_advance(Runtime &runtime)
{
    return runtime.advance();
}

lilac_asm_pcm_queue_result lilac_pcm_runtime_queue(Runtime &runtime,uint32_t offset,
    uint32_t span,const void *data,uint32_t bytes,uint32_t *token)
{
    return runtime.queue_pcm(offset,span,data,bytes,token);
}

bool lilac_pcm_runtime_drain(Runtime &runtime)
{
    return runtime.begin_drain();
}

bool lilac_pcm_runtime_region_release_confirmed(const Runtime &runtime)
{
    return runtime.region_release_confirmed();
}

void lilac_pcm_runtime_uncertain(Runtime &runtime)
{
    runtime.uncertain();
}
