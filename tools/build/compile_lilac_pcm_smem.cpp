/* SPDX-License-Identifier: GPL-2.0-only
 * Compile concrete PCM/GLINK/FIFO entry points against retained callbacks.
 * The owner still supplies physical mapping, readiness and hardware IO. */
#include "../../xnu/support/LilacAprSmemFifo.h"
#include "../../xnu/support/LilacPcmPlaybackTransport.h"
#include "LilacAPRLink.h"

using Fifo = LilacAprSmemFifo;
using Transport = LilacPcmPlaybackTransport;
using Link = ADSP::Link<Fifo,Transport>;

Fifo::State lilac_pcm_fifo_initial_state(const sweet_adsp_apr_ops &callbacks)
{
    Fifo fifo(callbacks);
    return fifo.state();
}

ADSP::Result lilac_pcm_fifo_admit(Fifo &fifo,uint64_t epoch,bool exclusive,bool uncached,bool dsp_ready)
{
    return fifo.admit(epoch,exclusive,uncached,dsp_ready);
}

ADSP::Result lilac_pcm_smem_link_begin(Link &link,uint64_t epoch)
{
    return link.begin(epoch);
}

ADSP::Result lilac_pcm_smem_link_poll_before_admission(Link &link)
{
    return link.poll();
}

ADSP::Result lilac_pcm_smem_advance(Transport &transport,Link &link,uint64_t now,
    bool dsp_ready,bool codec_ready)
{
    return transport.advance(link,dsp_ready,codec_ready,now);
}

bool lilac_pcm_smem_admit(Transport &transport,const Link &link,uint64_t now,
    bool dsp_ready,bool codec_ready)
{
    return transport.admit(link,dsp_ready,codec_ready,now);
}

lilac_asm_pcm_queue_result lilac_pcm_smem_write(Transport &transport,uint32_t offset,
    uint32_t span,uint32_t bytes,uint64_t now,uint32_t *token)
{
    return transport.queue_write(offset,span,bytes,now,token);
}

bool lilac_pcm_smem_drain(Transport &transport,uint64_t now)
{
    return transport.begin_drain(now);
}

bool lilac_pcm_smem_region_release_confirmed(const Transport &transport)
{
    return transport.region_release_confirmed();
}
