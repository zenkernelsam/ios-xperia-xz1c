/* SPDX-License-Identifier: GPL-2.0-only
 * Shared authored SMEM/DSP peer for protocol and runtime regressions.
 * This fixture never maps or operates a real device. */
#pragma once
#include "../../xnu/support/LilacAprSmemFifo.h"
#include "../../xnu/support/LilacPcmPlaybackTransport.h"
#include "LilacAPRLink.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

using Fifo = LilacAprSmemFifo;
using Transport = LilacPcmPlaybackTransport;
using Result = ADSP::Result;
namespace G = Baseband::GLINK;

struct Peer {
    enum : uint32_t { Descriptor=0x100,Tx=0x1000,Rx=0x5000,Ring=16384,Bytes=0x9000 };
    std::vector<uint8_t> memory=std::vector<uint8_t>(Bytes,0xa5);
    std::vector<std::vector<uint8_t>> responses,writes;
    std::vector<uint32_t> opcodes;
    bool receive_intents[8]{};
    unsigned notifications=0,tx_wraps=0,rx_wraps=0,bus_reads=0,bus_writes=0;
    bool rendered_first=false,fail_notify=false,responding=true;
    void *hook_context=nullptr;
    void (*on_read)(void *,uint32_t)=nullptr;
    void (*on_notify)(void *)=nullptr;
    void (*inspect_pcm)(void *,const uint8_t *,size_t)=nullptr;
    Peer()
    {
        word(Descriptor,Ring-8);word(Descriptor+4,Ring-8);
        word(Descriptor+8,Ring-16);word(Descriptor+12,Ring-16);
    }
    uint32_t word(unsigned address) const {return ADSP::read32(memory.data()+address);}
    void word(unsigned address,uint32_t value) {ADSP::write32(memory.data()+address,value);}
    static int read(void *context,uint32_t address,uint32_t *value)
    {
        auto &peer=*static_cast<Peer *>(context);
        assert(value && !(address&3) && address+4<=Bytes);
        ++peer.bus_reads;if(peer.on_read)peer.on_read(peer.hook_context,address);
        *value=peer.word(address);return 1;
    }
    static int write(void *context,uint32_t address,uint32_t value)
    {
        auto &peer=*static_cast<Peer *>(context);
        assert(address==Descriptor+4 || address==Descriptor+8 || (address>=Tx && address<Tx+Ring));
        ++peer.bus_writes;
        peer.word(address,value);return 1;
    }
    static void fence(void *) {}
    static int notify(void *context)
    {
        auto &peer=*static_cast<Peer *>(context);
        ++peer.notifications;
        if(peer.on_notify)peer.on_notify(peer.hook_context);
        if(peer.fail_notify)return 0;
        peer.drain_tx();
        return 1;
    }
    sweet_adsp_apr_ops callbacks()
    {
        sweet_adsp_apr_ops ops{};
        ops.context=this;ops.generation=9;ops.partition=0;ops.partition_bytes=Bytes;
        ops.descriptor=Descriptor;ops.tx=Tx;ops.tx_bytes=Ring;ops.rx=Rx;ops.rx_bytes=Ring;
        ops.read32=read;ops.write32=write;ops.fence=fence;ops.notify=notify;return ops;
    }
    void frame(G::Command command,uint16_t channel,uint32_t argument,const void *payload=nullptr,size_t bytes=0)
    {
        const bool data=command==G::TXData;
        const size_t header=data?16:8,wire=(header+bytes+7)&~size_t(7);
        const unsigned tail=word(Descriptor+8),head=word(Descriptor+12);
        const unsigned free=tail<=head?Ring-head+tail:tail-head;
        assert(wire+8<=free);
        std::vector<uint8_t> buffer(wire);
        G::header(buffer.data(),command,channel,argument);
        if(data)ADSP::write32(buffer.data()+8,bytes);
        if(bytes)std::memcpy(buffer.data()+header,payload,bytes);
        for(unsigned i=0;i<wire;++i)memory[Rx+(head+i)%Ring]=buffer[i];
        if(head+wire>=Ring)++rx_wraps;
        word(Descriptor+12,(head+wire)%Ring);
    }
    void flush_responses()
    {
        for(unsigned id=0;id<8 && !responses.empty();++id)if(receive_intents[id]) {
            const auto response=responses.front();responses.erase(responses.begin());
            receive_intents[id]=false;frame(G::TXData,7,id+1,response.data(),response.size());
        }
    }
    void response(const uint8_t *request,uint32_t opcode,size_t payload,bool event=false)
    {
        std::vector<uint8_t> packet(20+payload);
        ADSP::write16(packet.data(),event?0x50:0x150);ADSP::write16(packet.data()+2,packet.size());
        packet[4]=packet[8]=request[4];packet[5]=4;packet[9]=5;
        ADSP::write16(packet.data()+6,ADSP::read16(request+10));
        ADSP::write16(packet.data()+10,ADSP::read16(request+6));
        ADSP::write32(packet.data()+12,ADSP::read32(request+12));ADSP::write32(packet.data()+16,opcode);
        if(opcode==ADSP::BasicResponse)ADSP::write32(packet.data()+20,ADSP::read32(request+16));
        if(opcode==ADSP::MapResponse)ADSP::write32(packet.data()+20,17);
        if(opcode==LilacAdmPlaybackDevice::OpenResponseV5)ADSP::write16(packet.data()+24,3);
        responses.push_back(packet);
    }
    void command(const uint8_t *packet,size_t bytes)
    {
        assert(bytes>=20 && ADSP::read16(packet)==0x250 && ADSP::read16(packet+2)==bytes);
        const uint32_t opcode=ADSP::read32(packet+16);opcodes.push_back(opcode);
        if(opcode==ADSP::Write) {
            assert(bytes==52 && ADSP::read32(packet+28)==17 && ADSP::read32(packet+32)==256);
            if(inspect_pcm)inspect_pcm(hook_context,packet,bytes);
            writes.emplace_back(packet,packet+bytes);
            return;
        }
        response(packet,ADSP::Accepted,0);
        if(opcode==LilacAsmPcmStream::EndOfStream && rendered_first)
            response(packet,LilacAsmPcmStream::RenderedEndOfStream,0,true);
        response(packet,ADSP::BasicResponse,8);
        if(opcode==LilacAsmPcmStream::EndOfStream && !rendered_first)
            response(packet,LilacAsmPcmStream::RenderedEndOfStream,0,true);
        if(opcode==ADSP::Map)response(packet,ADSP::MapResponse,4);
        if(opcode==LilacAdmPlaybackDevice::OpenV5)
            response(packet,LilacAdmPlaybackDevice::OpenResponseV5,8);
    }
    void drain_tx()
    {
        unsigned budget=0;
        while(word(Descriptor)!=word(Descriptor+4)) {
            assert(++budget<64);
            const unsigned tail=word(Descriptor),head=word(Descriptor+4);
            const unsigned available=head>=tail?head-tail:Ring-tail+head;
            std::vector<uint8_t> packet(available);
            for(unsigned i=0;i<available;++i)packet[i]=memory[Tx+(tail+i)%Ring];
            G::Frame parsed;assert(G::parse(packet.data(),packet.size(),parsed)==Result::OK);
            if(tail+parsed.wireBytes>=Ring)++tx_wraps;
            word(Descriptor,(tail+parsed.wireBytes)%Ring);
            switch(parsed.command) {
            case G::Version: {
                assert(parsed.channel==1 && parsed.argument==0);
                if(!responding)break;
                frame(G::VersionAck,1,0);
                static const char name[]="apr_audio_svc";
                frame(G::Open,7,sizeof(name),name,sizeof(name));
                break;
            }
            case G::Open: {
                if(!responding)break;
                frame(G::OpenAck,1,0);
                uint8_t pairs[64]{};
                for(unsigned i=0;i<8;++i){ADSP::write32(pairs+i*8,4096);ADSP::write32(pairs+i*8+4,i);}
                frame(G::Intent,7,8,pairs,sizeof(pairs));
                break;
            }
            case G::Intent:
                for(unsigned i=0;i<parsed.argument;++i) {
                    const unsigned id=ADSP::read32(parsed.payload+i*8+4);
                    assert(id>=1 && id<=8 && !receive_intents[id-1]);
                    receive_intents[id-1]=true;
                }
                break;
            case G::TXData:
                command(parsed.payload,parsed.payloadBytes);
                frame(G::RXDoneReuse,7,parsed.argument);
                break;
            case G::Close:
                frame(G::CloseAck,1,0);frame(G::Close,7,0);
                break;
            case G::OpenAck:case G::RXDone:case G::CloseAck:break;
            default:assert(false);
            }
            flush_responses();
        }
        flush_responses();
    }
    void complete_writes()
    {
        for(auto it=writes.rbegin();it!=writes.rend();++it) {
            response(it->data(),ADSP::WriteDone,16,true);
            auto &packet=responses.back();
            std::memcpy(packet.data()+20,it->data()+20,12);
        }
        writes.clear();flush_responses();
    }
};

static Transport::Session::Configuration configuration()
{
    return {9,0x101,0x10be4,0x10be4,UINT64_C(0x123456000),8192,
        LilacAfeSlimbusPlayback::ParameterSchema::V2,{0,{144,145}}};
}
