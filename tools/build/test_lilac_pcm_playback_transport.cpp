/* SPDX-License-Identifier: GPL-2.0-only
 * Actual locked ADSP::Link and PCM session over an authored FIFO peer.
 * No real FIFO, DSP, codec, physical pages or device operations. */
#include "../../xnu/support/LilacPcmPlaybackTransport.h"
#include "LilacAPRLink.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

using Transport = LilacPcmPlaybackTransport;
using Result = ADSP::Result;
using Session = LilacPcmPlaybackSession;
namespace G = Baseband::GLINK;

static Session::Configuration config()
{
    return {9,0x101,0x10be4,0x10be4,UINT64_C(0x123456000),8192,
        LilacAfeSlimbusPlayback::ParameterSchema::V2,{0,{144,145}}};
}

struct Fifo {
    std::vector<uint8_t> rx,last_request,last_busy;
    unsigned apr_attempts=0,apr_sent=0,versions=0,closes=0,busy_for=0;
    uint32_t last_remote_intent=0;
    Result next_data_result=Result::OK;
    bool ambiguous_publication=false;
    Result send(const void *data,size_t bytes)
    {
        const auto *p=static_cast<const uint8_t *>(data);
        const unsigned command=ADSP::read16(p);
        if(command==G::Version)++versions;
        if(command==G::Close)++closes;
        if(command==G::TXData) {
            assert(bytes>=16 && ADSP::read32(p+8)==bytes-16 && !ADSP::read32(p+12));
            ++apr_attempts;
            if(busy_for) {
                --busy_for;
                const std::vector<uint8_t> attempted(p+16,p+bytes);
                if(!last_busy.empty())assert(last_busy==attempted);
                last_busy=attempted;
                return Result::Busy;
            }
            if(next_data_result!=Result::OK) {
                if(ambiguous_publication) {
                    last_request.assign(p+16,p+bytes);
                    last_remote_intent=ADSP::read32(p+4);++apr_sent;
                }
                const Result result=next_data_result;next_data_result=Result::OK;
                return result;
            }
            if(!last_busy.empty())assert(last_busy==std::vector<uint8_t>(p+16,p+bytes));
            last_busy.clear();
            last_request.assign(p+16,p+bytes);
            last_remote_intent=ADSP::read32(p+4);++apr_sent;
        }
        return Result::OK;
    }
    Result available(size_t &bytes) {bytes=rx.size();return Result::OK;}
    Result peek(void *data,size_t bytes)
    {
        assert(bytes<=rx.size());std::memcpy(data,rx.data(),bytes);return Result::OK;
    }
    Result consume(size_t bytes)
    {
        assert(bytes<=rx.size());rx.erase(rx.begin(),rx.begin()+bytes);return Result::OK;
    }
    void frame(G::Command command,uint16_t channel,uint32_t argument,
        const void *payload=nullptr,size_t bytes=0,uint32_t remaining=0)
    {
        const bool datagram=command==G::TXData || command==G::TXDataContinue;
        const size_t header=datagram?16:8,wire=(header+bytes+7)&~size_t(7);
        const size_t begin=rx.size();rx.resize(begin+wire);
        auto *p=rx.data()+begin;G::header(p,command,channel,argument);
        if(datagram){ADSP::write32(p+8,bytes);ADSP::write32(p+12,remaining);}
        if(bytes)std::memcpy(p+header,payload,bytes);
    }
    void credit() {frame(G::RXDoneReuse,7,last_remote_intent);}
};

template<class Link>
static void handshake(Link &link,Fifo &fifo)
{
    assert(link.begin(9)==Result::OK && link.poll()==Result::OK);
    fifo.frame(G::VersionAck,1,0);
    static const char name[]="apr_audio_svc";
    fifo.frame(G::Open,7,sizeof(name),name,sizeof(name));
    fifo.frame(G::OpenAck,1,0);
    uint8_t credits[64]{};
    for(unsigned i=0;i<8;++i){ADSP::write32(credits+i*8,4096);ADSP::write32(credits+i*8+4,i);}
    fifo.frame(G::Intent,7,8,credits,sizeof(credits));
    assert(link.poll()==Result::OK && link.ready() && fifo.versions==1);
}

struct Fixture {
    Transport transport{config(),{5,20,10,15}};
    Fifo fifo;
    ADSP::Link<Fifo,Transport> link{fifo,transport};
    uint64_t now=0;
    Fixture()
    {
        handshake(link,fifo);
        assert(transport.admit(link,true,true,now));
    }
    Result pump() {return transport.advance(link,true,true,now);}
    void packet(const std::vector<uint8_t> &request,uint32_t opcode,size_t payload,
        bool event=false,uint32_t status=0,bool typed=false,bool fragmented=false)
    {
        uint8_t reply[36]{};
        const size_t bytes=20+payload;
        ADSP::write16(reply,event?0x50:0x150);ADSP::write16(reply+2,bytes);
        reply[4]=reply[8]=request[4];reply[5]=4;reply[9]=5;
        ADSP::write16(reply+6,ADSP::read16(request.data()+10));
        ADSP::write16(reply+10,ADSP::read16(request.data()+6));
        ADSP::write32(reply+12,ADSP::read32(request.data()+12));ADSP::write32(reply+16,opcode);
        if(typed && opcode==ADSP::MapResponse)ADSP::write32(reply+20,17);
        else if(typed && opcode==LilacAdmPlaybackDevice::OpenResponseV5)ADSP::write16(reply+24,3);
        else if(opcode==ADSP::BasicResponse) {
            ADSP::write32(reply+20,ADSP::read32(request.data()+16));ADSP::write32(reply+24,status);
        }
        if(fragmented) {
            const unsigned sent=fifo.apr_sent;
            fifo.frame(G::TXData,7,1,reply,12,bytes-12);
            ++now;assert(pump()==Result::OK && fifo.apr_sent==sent);
            assert(transport.session_state()==Session::State::AwaitingReply && !link.quarantined());
            fifo.frame(G::TXDataContinue,7,1,reply+12,bytes-12);
        } else fifo.frame(G::TXData,7,1,reply,bytes);
    }
    void accepted()
    {
        packet(fifo.last_request,ADSP::Accepted,0);
        const unsigned sent=fifo.apr_sent;++now;
        assert(pump()==Result::OK && fifo.apr_sent==sent && !link.quarantined());
        assert(transport.session_state()==Session::State::AwaitingReply);
    }
    void complete(bool busy_next=false)
    {
        const auto request=fifo.last_request;
        const unsigned opcode=ADSP::read32(request.data()+16);
        fifo.credit();
        if(opcode==ADSP::Map || opcode==LilacAdmPlaybackDevice::OpenV5) {
            packet(request,ADSP::BasicResponse,8);
            const unsigned sent=fifo.apr_sent;++now;
            assert(pump()==Result::OK && !link.quarantined() && fifo.apr_sent==sent);
            assert(transport.session_state()==Session::State::AwaitingReply);
            packet(request,opcode==ADSP::Map?ADSP::MapResponse:LilacAdmPlaybackDevice::OpenResponseV5,
                opcode==ADSP::Map?4:8,false,0,true);
        } else packet(request,ADSP::BasicResponse,8);
        if(busy_next)fifo.busy_for=1;
        ++now;const auto result=pump();
        assert((result==Result::OK || result==Result::Busy || result==Result::NotReady) && !link.quarantined());
    }
    void start()
    {
        fifo.busy_for=1;assert(pump()==Result::Busy && transport.pending_publication());
        const uint32_t startup[]={ADSP::OpenWrite,ADSP::FormatUpdate,ADSP::Map,
            LilacAfeSlimbusPlayback::SetParamV2,LilacAfeSlimbusPlayback::DeviceStart,
            LilacAdmPlaybackDevice::OpenV5,ADSP::MatrixMap,ADSP::Run};
        for(unsigned i=0;i<8;++i) {
            assert(transport.pending_publication());++now;
            assert(pump()==Result::OK && !transport.pending_publication());
            assert(ADSP::read32(fifo.last_request.data()+16)==startup[i]);
            assert(transport.published_packets()==i+1);
            accepted();complete(i<7);
        }
        assert(transport.session_state()==Session::State::Running && fifo.apr_sent==8);
    }
    std::vector<uint8_t> write(unsigned offset)
    {
        uint32_t token=0;fifo.busy_for=1;
        assert(transport.queue_write(offset,1024,256,now,&token)==LILAC_ASM_QUEUE_RESERVED && token);
        assert(pump()==Result::Busy);
        uint32_t next=0;
        assert(transport.queue_write(offset+1024,1024,256,now,&next)==LILAC_ASM_QUEUE_BUSY);
        ++now;assert(pump()==Result::OK);
        assert(ADSP::read32(fifo.last_request.data()+12)==token);
        return fifo.last_request;
    }
    void done(const std::vector<uint8_t> &request)
    {
        uint8_t reply[36]{};
        ADSP::write16(reply,0x50);ADSP::write16(reply+2,sizeof(reply));
        reply[4]=reply[8]=ADSP::ASM;reply[5]=4;reply[9]=5;
        ADSP::write16(reply+6,0x101);ADSP::write16(reply+10,0x101);
        ADSP::write32(reply+12,ADSP::read32(request.data()+12));ADSP::write32(reply+16,ADSP::WriteDone);
        std::memcpy(reply+20,request.data()+20,12);
        fifo.frame(G::TXData,7,1,reply,sizeof(reply));
    }
    void eos(bool event_first)
    {
        assert(ADSP::read32(fifo.last_request.data()+16)==LilacAsmPcmStream::EndOfStream);
        const auto request=fifo.last_request;fifo.credit();
        if(event_first)packet(request,LilacAsmPcmStream::RenderedEndOfStream,0,true);
        else packet(request,ADSP::BasicResponse,8);
        ++now;assert(pump()==Result::OK && !link.quarantined());
        assert(transport.session_state()==Session::State::AwaitingReply);
        if(event_first)packet(request,ADSP::BasicResponse,8);
        else packet(request,LilacAsmPcmStream::RenderedEndOfStream,0,true);
        fifo.busy_for=1;++now;assert(pump()==Result::Busy);
    }
    void stop(bool close_peer=true)
    {
        const uint32_t closing[]={ADSP::Close,ADSP::Unmap,
            LilacAdmPlaybackDevice::CloseV5,LilacAfeSlimbusPlayback::DeviceStop};
        for(unsigned i=0;i<4;++i) {
            ++now;assert(pump()==Result::OK);
            assert(ADSP::read32(fifo.last_request.data()+16)==closing[i]);
            complete(i<3);
        }
        assert(transport.state()==Transport::State::Closing && !transport.region_release_confirmed());
        assert(fifo.closes==0);++now;assert(pump()==Result::NotReady && fifo.closes==1);
        if(close_peer) {
            fifo.frame(G::CloseAck,1,0);fifo.frame(G::Close,7,0);
            ++now;assert(pump()==Result::OK);
            assert(link.closed() && transport.state()==Transport::State::Stopped);
            assert(transport.region_release_confirmed());
        }
    }
};

int main()
{
    // A direct session sink returns NotReady for the Map basic ACK. The
    // original Link treats that as a transport failure; the new adapter
    // consumes the datagram while preserving the pending typed response.
    {
        struct DirectSink {
            Session session{config()};
            Result receive(const void *packet,size_t bytes) {return session.receive(9,packet,bytes);}
        } sink;
        Fifo fifo;ADSP::Link<Fifo,DirectSink> link{fifo,sink};handshake(link,fifo);
        assert(sink.session.admit(9,true,true));
        uint8_t packet[104]{},reply[28]{};
        Session::Request request;
        for(unsigned i=0;i<3;++i) {
            assert(sink.session.build_next(packet,sizeof(packet),request));
            assert(link.send(packet,request.bytes)==Result::OK);
            ADSP::write16(reply,0x150);ADSP::write16(reply+2,sizeof(reply));
            reply[4]=reply[8]=ADSP::ASM;reply[5]=4;reply[9]=5;
            ADSP::write16(reply+6,ADSP::read16(packet+10));
            ADSP::write16(reply+10,ADSP::read16(packet+6));
            ADSP::write32(reply+12,request.token);ADSP::write32(reply+16,ADSP::BasicResponse);
            ADSP::write32(reply+20,request.opcode);
            fifo.credit();fifo.frame(G::TXData,7,1,reply,sizeof(reply));
            if(i<2)assert(link.poll()==Result::OK && !link.quarantined());
            else {
                assert(request.action==Session::Action::MapRegion);
                assert(link.poll()==Result::NotReady && link.quarantined());
                assert(sink.session.state()==Session::State::AwaitingReply);
            }
        }
    }
    for(bool event_first:{false,true}) {
        Fixture f;f.start();std::vector<uint8_t> writes[4];
        for(unsigned i=0;i<4;++i){writes[i]=f.write(i*1024);f.fifo.credit();}
        assert(f.transport.pending_buffers()==4 && f.transport.begin_drain(f.now));
        for(unsigned i=0;i<4;++i) {
            f.done(writes[i]);assert(f.pump()==Result::OK);
            assert(f.transport.pending_buffers()==3-i);
        }
        f.eos(event_first);f.stop();
        assert(f.transport.published_packets()==17 && f.fifo.apr_sent==17 && f.fifo.versions==1);
    }
    {Fixture f;f.fifo.busy_for=100;assert(f.pump()==Result::Busy);f.now=5;
     assert(f.pump()==Result::Timeout && f.transport.failure()==Transport::Failure::PublishTimeout);
     assert(f.fifo.apr_sent==0 && f.fifo.apr_attempts==1 && !f.transport.region_release_confirmed());}
    {Fixture f;assert(f.pump()==Result::OK);f.now=10;f.accepted();f.now=20;
     assert(f.pump()==Result::Timeout && f.transport.failure()==Transport::Failure::ReplyTimeout);
     f.packet(f.fifo.last_request,ADSP::BasicResponse,8);assert(f.link.poll()==Result::Quarantined);
     assert(!f.transport.region_release_confirmed() && f.fifo.apr_sent==1);}
    {Fixture f;f.start();f.write(0);f.now+=10;
     assert(f.pump()==Result::Timeout && f.transport.failure()==Transport::Failure::WriteTimeout);
     assert(f.transport.pending_buffers()==1 && !f.transport.region_release_confirmed());}
    {Fixture f;f.start();assert(f.transport.begin_drain(f.now));assert(f.pump()==Result::OK);
     f.eos(false);f.stop(false);f.now+=15;
     assert(f.pump()==Result::Timeout && f.transport.failure()==Transport::Failure::CloseTimeout);
     assert(!f.transport.region_release_confirmed());}
    {Fixture f;f.fifo.next_data_result=Result::IO;f.fifo.ambiguous_publication=true;assert(f.pump()==Result::IO);
     assert(f.transport.failure()==Transport::Failure::Publish && !f.transport.region_release_confirmed());
     assert(f.transport.published_packets()==0 && f.fifo.apr_sent==1);
     f.packet(f.fifo.last_request,ADSP::BasicResponse,8);
     assert(f.pump()==Result::Quarantined && f.fifo.apr_attempts==1);}
    {Fixture f;f.start();f.write(0);
     assert(f.transport.advance(f.link,true,false,f.now)==Result::NotReady);
     assert(f.transport.failure()==Transport::Failure::Readiness && f.transport.pending_buffers()==1);}
    {Fixture f;f.now=2;assert(f.pump()==Result::OK);f.now=1;
     assert(f.pump()==Result::Invalid && f.transport.failure()==Transport::Failure::Clock);}
    {Fixture f;assert(f.pump()==Result::OK);f.packet(f.fifo.last_request,ADSP::BasicResponse,8,false,1);
     assert(f.pump()==Result::IO && f.link.quarantined() && !f.transport.region_release_confirmed());}
    {Fixture f;assert(f.pump()==Result::OK);f.fifo.credit();
     f.packet(f.fifo.last_request,ADSP::BasicResponse,8,false,0,false,true);
     ++f.now;assert(f.pump()==Result::OK && f.fifo.apr_sent==2 && !f.link.quarantined());}
    {Fixture f;f.fifo.busy_for=1;assert(f.pump()==Result::Busy);
     // A matching-looking reply to an unpublished command cannot advance.
     uint8_t reply[28]{};
     ADSP::write16(reply,0x150);ADSP::write16(reply+2,sizeof(reply));
     reply[4]=reply[8]=ADSP::ASM;reply[5]=4;reply[9]=5;
     ADSP::write16(reply+6,0x101);ADSP::write16(reply+10,0x101);
     ADSP::write32(reply+12,1);ADSP::write32(reply+16,ADSP::BasicResponse);
     ADSP::write32(reply+20,ADSP::OpenWrite);
     assert(f.transport.receive(reply,sizeof(reply))==Result::Stale);
     assert(f.transport.session_state()==Session::State::AwaitingReply && f.fifo.apr_sent==0);}
    {Fixture f;f.start();uint32_t token=0;f.fifo.busy_for=1;
     assert(f.transport.queue_write(0,1024,256,f.now,&token)==LILAC_ASM_QUEUE_RESERVED);
     assert(f.pump()==Result::Busy && f.transport.pending_buffers()==1);
     f.done(f.fifo.last_busy);f.fifo.busy_for=1;
     assert(f.pump()==Result::Busy && f.transport.pending_buffers()==1);
     assert(f.transport.pending_publication() && f.fifo.apr_sent==8);
     ++f.now;assert(f.pump()==Result::OK && f.fifo.apr_sent==9);
     assert(f.transport.pending_buffers()==1);}
    {Fixture f;f.start();f.fifo.frame(G::Close,7,0);
     assert(f.pump()==Result::Stale && f.link.quarantined());
     assert(f.transport.failure()==Transport::Failure::Poll && !f.transport.region_release_confirmed());}
    {Fixture f;
     struct LaterLink {
         ADSP::Link<Fifo,Transport> &link;
         uint64_t generation() const{return 10;}
         bool ready() const{return link.ready();}
         bool quarantined() const{return link.quarantined();}
         bool closed() const{return link.closed();}
         Result poll(){assert(false);return Result::IO;}
         Result send(const void *,size_t){assert(false);return Result::IO;}
         Result close(){assert(false);return Result::IO;}
     } later{f.link};
     assert(f.transport.advance(later,true,true,0)==Result::Stale);
     assert(f.transport.failure()==Transport::Failure::Epoch && f.fifo.apr_attempts==0);}
    puts("Lilac PCM transport PASS: actual locked GLINK/intent/receive ACKs, immutable Busy retry, fragmented/typed/accepted replies, four published buffers, rendered EOS, confirmed link close, publication/reply/write/close timeouts, ambiguous publication and unpublished reply rejection; no DSP/codec/device IO");
    return 0;
}
