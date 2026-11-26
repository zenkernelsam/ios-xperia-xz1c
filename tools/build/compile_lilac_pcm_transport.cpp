/* SPDX-License-Identifier: GPL-2.0-only
 * Compile the PCM adapter against real locked ADSP::Link for iOS ARM64.
 * External FIFO declarations are deliberately unimplemented; this object
 * does not link a device driver or perform any hardware IO. */
#include "../../xnu/support/LilacPcmPlaybackTransport.h"
#include "LilacAPRLink.h"

struct LilacPcmCompileIO {
    ADSP::Result send(const void *,size_t);
    ADSP::Result available(size_t &);
    ADSP::Result peek(void *,size_t);
    ADSP::Result consume(size_t);
};
using Transport = LilacPcmPlaybackTransport;
using Link = ADSP::Link<LilacPcmCompileIO,Transport>;

Transport::State lilac_pcm_transport_initial_state(const Transport::Session::Configuration &configuration,
    Transport::Timeouts timeouts)
{
    Transport transport(configuration,timeouts);
    return transport.state();
}

bool lilac_pcm_transport_admit(Transport &transport,const Link &link,uint64_t now,
    bool dsp_ready,bool codec_ready)
{
    return transport.admit(link,dsp_ready,codec_ready,now);
}

ADSP::Result lilac_pcm_transport_advance(Transport &transport,Link &link,uint64_t now,
    bool dsp_ready,bool codec_ready)
{
    return transport.advance(link,dsp_ready,codec_ready,now);
}

lilac_asm_pcm_queue_result lilac_pcm_transport_queue_write(Transport &transport,
    uint32_t offset,uint32_t span,uint32_t bytes,uint64_t now,uint32_t *token)
{
    return transport.queue_write(offset,span,bytes,now,token);
}

ADSP::Result lilac_pcm_transport_receive(Transport &transport,const void *packet,size_t bytes)
{
    return transport.receive(packet,bytes);
}

bool lilac_pcm_transport_begin_drain(Transport &transport,uint64_t now)
{
    return transport.begin_drain(now);
}

bool lilac_pcm_transport_region_release_confirmed(const Transport &transport)
{
    return transport.region_release_confirmed();
}

void lilac_pcm_transport_uncertain(Transport &transport)
{
    transport.uncertain();
}
