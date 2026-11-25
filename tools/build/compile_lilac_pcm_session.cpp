/* SPDX-License-Identifier: GPL-2.0-only
 * Compile every protocol entry for ARM64 without starting a DSP or linking
 * this fixture into the device kernel. The transport owns the real object. */
#include "../../xnu/support/LilacPcmPlaybackSession.h"

using Session = LilacPcmPlaybackSession;

Session::State lilac_pcm_initial_state(const Session::Configuration &configuration)
{
    Session session(configuration);
    return session.state();
}

bool lilac_pcm_admit(Session &session, uint64_t epoch, bool dsp_ready, bool codec_ready)
{
    return session.admit(epoch, dsp_ready, codec_ready);
}

size_t lilac_pcm_build_next(Session &session, void *packet, size_t capacity, Session::Request &request)
{
    return session.build_next(packet, capacity, request);
}

ADSP::Result lilac_pcm_receive(Session &session, uint64_t epoch, const void *packet, size_t bytes)
{
    return session.receive(epoch, packet, bytes);
}

lilac_asm_pcm_queue_result lilac_pcm_write(Session &session, uint32_t offset, uint32_t span,
    uint32_t bytes, uint8_t *packet, size_t capacity, uint32_t *token)
{
    return session.write(offset, span, bytes, packet, capacity, token);
}

bool lilac_pcm_begin_drain(Session &session)
{
    return session.begin_drain();
}

void lilac_pcm_uncertain(Session &session)
{
    session.uncertain();
}

bool lilac_pcm_region_release_confirmed(const Session &session)
{
    return session.region_release_confirmed();
}

unsigned lilac_pcm_pending_buffers(const Session &session)
{
    return session.pending_buffers();
}
