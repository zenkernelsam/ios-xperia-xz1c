/* SPDX-License-Identifier: GPL-2.0-only
 * PCM session -> original ADSP::Link -> real callback FIFO, with an authored
 * DSP peer in host memory. Both 16KiB rings begin near their wrap boundary.
 * No real SMEM mapping, mailbox interrupt, codec or device operation. */
#include "LilacPcmSmemPeerFixture.h"

int main()
{
    for(bool rendered_first:{false,true}) {
        Peer peer;peer.rendered_first=rendered_first;
        Fifo fifo(peer.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        Transport transport(configuration(),{10,200,200,200});
        ADSP::Link<Fifo,Transport> link(fifo,transport);
        assert(link.begin(9)==Result::OK);
        for(unsigned i=0;i<20 && !link.ready();++i)assert(link.poll()==Result::OK);
        assert(link.ready() && transport.admit(link,true,true,0));
        uint64_t now=0;
        while(transport.session_state()!=Transport::Session::State::Running) {
            assert(++now<100);assert(transport.advance(link,true,true,now)==Result::OK);
        }
        for(unsigned i=0;i<4;++i) {
            uint32_t token=0;assert(transport.queue_write(i*1024,1024,256,now,&token)==LILAC_ASM_QUEUE_RESERVED);
            assert(transport.advance(link,true,true,++now)==Result::OK);
            assert(transport.pending_buffers()==i+1);
        }
        assert(peer.writes.size()==4 && transport.begin_drain(now));
        peer.complete_writes();
        while(transport.state()!=Transport::State::Stopped) {
            assert(++now<200);const Result result=transport.advance(link,true,true,now);
            assert(result==Result::OK || result==Result::NotReady);
        }
        assert(link.closed() && transport.region_release_confirmed() && !transport.pending_buffers());
        assert(transport.published_packets()==17 && peer.opcodes.size()==17);
        assert(peer.tx_wraps && peer.rx_wraps && fifo.state()==Fifo::State::Ready);
        assert(fifo.progress().tx_head_publications>17 && fifo.progress().rx_tail_publications>17);
        assert(peer.responses.empty() && fifo.progress().notifications==peer.notifications);
    }
    {
        Peer peer;Fifo fifo(peer.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        Transport transport(configuration(),{10,200,200,200});
        ADSP::Link<Fifo,Transport> link(fifo,transport);
        assert(link.begin(9)==Result::OK);
        for(unsigned i=0;i<20 && !link.ready();++i)assert(link.poll()==Result::OK);
        assert(link.ready() && transport.admit(link,true,true,0));
        peer.fail_notify=true;
        assert(transport.advance(link,true,true,1)==Result::IO);
        assert(fifo.progress().tx_publication_uncertain && fifo.state()==Fifo::State::Uncertain);
        assert(transport.state()==Transport::State::Uncertain && !transport.region_release_confirmed());
        const auto head=peer.word(Peer::Descriptor+4);
        assert(transport.advance(link,true,true,2)==Result::Quarantined && peer.word(Peer::Descriptor+4)==head);
    }
    puts("Lilac PCM SMEM chain PASS: real callback FIFO -> locked GLINK -> PCM session, wrapped 16KiB rings, four delayed out-of-order completions, either rendered-EOS order, typed responses, close ACK and publication uncertainty; host peer only");
    return 0;
}
