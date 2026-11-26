/* SPDX-License-Identifier: GPL-2.0-only
 * Actual FIFO callback adapter with authored shared-memory register words.
 * Exhaustive 8-byte ring positions, fault injection and publication order;
 * no real memory mapping, mailbox interrupt, DSP or device access. */
#include "../../xnu/support/LilacAprSmemFifo.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

using Fifo = LilacAprSmemFifo;
using Result = ADSP::Result;
namespace G = Baseband::GLINK;

struct Memory {
    enum : uint32_t { Descriptor=0x100, Tx=0x1000, Rx=0x5000, Ring=16384, Bytes=0x9000 };
    struct Event { char type; uint32_t address, value; };
    std::vector<uint8_t> bytes=std::vector<uint8_t>(Bytes,0xa5);
    std::vector<Event> events;
    uint32_t fail_read=UINT32_MAX,fail_write=UINT32_MAX;
    bool fail_notification=false,write_then_fail=false;
    Memory()
    {
        for(unsigned i=0;i<4;++i)ADSP::write32(bytes.data()+Descriptor+i*4,0);
    }
    uint32_t word(unsigned offset) const {return ADSP::read32(bytes.data()+offset);}
    void word(unsigned offset,uint32_t value) {ADSP::write32(bytes.data()+offset,value);}
    static int read(void *opaque,uint32_t offset,uint32_t *value)
    {
        auto &m=*static_cast<Memory *>(opaque);
        assert(value && !(offset&3u) && offset+4<=Bytes);
        m.events.push_back({'r',offset,0});
        if(offset==m.fail_read)return 0;
        *value=m.word(offset);return 1;
    }
    static int write(void *opaque,uint32_t offset,uint32_t value)
    {
        auto &m=*static_cast<Memory *>(opaque);
        assert(!(offset&3u) && offset+4<=Bytes);
        assert(offset==Descriptor+4 || offset==Descriptor+8 || (offset>=Tx && offset<Tx+Ring));
        m.events.push_back({'w',offset,value});
        if(offset!=m.fail_write || m.write_then_fail)m.word(offset,value);
        return offset!=m.fail_write;
    }
    static void fence(void *opaque)
    {
        static_cast<Memory *>(opaque)->events.push_back({'f',0,0});
    }
    static int notify(void *opaque)
    {
        auto &m=*static_cast<Memory *>(opaque);m.events.push_back({'n',0,0});return !m.fail_notification;
    }
    sweet_adsp_apr_ops callbacks()
    {
        sweet_adsp_apr_ops ops{};
        ops.context=this;ops.generation=9;ops.partition=0;ops.partition_bytes=Bytes;
        ops.descriptor=Descriptor;ops.tx=Tx;ops.tx_bytes=Ring;ops.rx=Rx;ops.rx_bytes=Ring;
        ops.read32=read;ops.write32=write;ops.fence=fence;ops.notify=notify;
        return ops;
    }
    void receive(unsigned tail,const uint8_t *packet,unsigned bytes)
    {
        assert(!(tail&7u) && !(bytes&7u));
        for(unsigned i=0;i<bytes;++i)this->bytes[Rx+(tail+i)%Ring]=packet[i];
        word(Descriptor+8,tail);word(Descriptor+12,(tail+bytes)%Ring);
    }
    void assert_no_write() const
    {
        for(const auto &event:events)assert(event.type!='w' && event.type!='n');
    }
    void check_send(unsigned head,const uint8_t *packet,unsigned count)
    {
        unsigned writes=0;
        for(const auto &event:events)if(event.type=='w') {
            if(writes<count/4)assert(event.address==Tx+(head+writes*4)%Ring &&
                event.value==ADSP::read32(packet+writes*4));
            else assert(writes==count/4 && event.address==Descriptor+4 &&
                event.value==(head+count)%Ring);
            ++writes;
        }
        assert(writes==count/4+1 && events.size()>=count/4+5);
        assert(events[events.size()-4].type=='f');
        assert(events[events.size()-3].type=='w' && events[events.size()-3].address==Descriptor+4);
        assert(events[events.size()-2].type=='f' && events.back().type=='n');
    }
};

static void datagram(uint8_t *packet,unsigned payload)
{
    std::memset(packet,0,16+payload);
    G::header(packet,G::TXData,7,1);ADSP::write32(packet+8,payload);
    for(unsigned i=0;i<payload;++i)packet[16+i]=uint8_t(i*17+3);
}

int main()
{
    uint8_t packet[64];datagram(packet,48);
    for(unsigned position=0;position<Memory::Ring;position+=8) {
        Memory memory;Fifo fifo(memory.callbacks());
        assert(fifo.admit(9,true,true,true)==Result::OK);memory.events.clear();
        memory.word(Memory::Descriptor,position);memory.word(Memory::Descriptor+4,position);
        assert(fifo.send(packet,sizeof(packet))==Result::OK);
        memory.check_send(position,packet,sizeof(packet));
        assert(fifo.progress().tx_head_publications==1 && fifo.progress().notifications==1);
        memory.receive(position,packet,sizeof(packet));memory.events.clear();
        size_t available=0;assert(fifo.available(available)==Result::OK && available==sizeof(packet));
        uint8_t copied[80];std::memset(copied,0xb6,sizeof(copied));
        assert(fifo.peek(copied+8,64)==Result::OK && !std::memcmp(copied+8,packet,64));
        for(unsigned i=0;i<8;++i)assert(copied[i]==0xb6 && copied[72+i]==0xb6);
        memory.assert_no_write();memory.events.clear();
        assert(fifo.consume(64)==Result::OK);
        assert(memory.word(Memory::Descriptor+8)==(position+64)%Memory::Ring);
        assert(memory.events.size()==6 && memory.events[2].type=='f' &&
            memory.events[3].type=='w' && memory.events[3].address==Memory::Descriptor+8 &&
            memory.events[4].type=='f' && memory.events[5].type=='n');
        assert(fifo.progress().rx_tail_publications==1 && fifo.progress().notifications==2);
        assert(fifo.consume(64)==Result::Invalid);
    }
    {
        Memory memory;Fifo fifo(memory.callbacks());
        assert(fifo.admit(8,true,true,true)==Result::Stale);
        assert(fifo.admit(9,false,true,true)==Result::NotReady);
        assert(fifo.admit(9,true,false,true)==Result::NotReady);
        assert(fifo.admit(9,true,true,false)==Result::NotReady);
        assert(fifo.send(packet,sizeof(packet))==Result::NotReady);memory.assert_no_write();
        assert(fifo.admit(9,true,true,true)==Result::OK);memory.events.clear();
        memory.word(Memory::Descriptor,72); // 64 bytes would consume the reserve.
        assert(fifo.send(packet,sizeof(packet))==Result::Busy);memory.assert_no_write();
        assert(memory.word(Memory::Descriptor+4)==0 && fifo.state()==Fifo::State::Ready);
        memory.word(Memory::Descriptor,80);memory.events.clear();
        assert(fifo.send(packet,sizeof(packet))==Result::OK);
        memory.check_send(0,packet,sizeof(packet));
    }
    for(unsigned failure=0;failure<16;++failure) {
        Memory memory;Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        memory.fail_write=Memory::Tx+failure*4;memory.events.clear();
        assert(fifo.send(packet,sizeof(packet))==Result::IO);
        assert(fifo.state()==Fifo::State::Uncertain && fifo.progress().failure_phase==Fifo::Phase::WriteData);
        assert(!fifo.progress().tx_head_publications && memory.word(Memory::Descriptor+4)==0);
        const auto events=memory.events.size();assert(fifo.send(packet,sizeof(packet))==Result::Quarantined);
        assert(memory.events.size()==events);
    }
    for(bool committed:{false,true}) {
        Memory memory;Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        memory.fail_write=Memory::Descriptor+4;memory.write_then_fail=committed;
        assert(fifo.send(packet,sizeof(packet))==Result::IO);
        assert(fifo.progress().tx_publication_uncertain && !fifo.progress().tx_head_publications);
        assert(memory.word(Memory::Descriptor+4)==(committed?64:0));
        assert(fifo.send(packet,sizeof(packet))==Result::Quarantined);
    }
    {
        Memory memory;Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        memory.fail_notification=true;assert(fifo.send(packet,sizeof(packet))==Result::IO);
        assert(fifo.progress().tx_head_publications==1 && !fifo.progress().notifications &&
            fifo.progress().tx_publication_uncertain && memory.word(Memory::Descriptor+4)==64);
    }
    for(unsigned failure=0;failure<16;++failure) {
        Memory memory;Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        memory.receive(Memory::Ring-8,packet,sizeof(packet));
        memory.fail_read=Memory::Rx+(Memory::Ring-8+failure*4)%Memory::Ring;
        uint8_t copied[64];std::memset(copied,0xb6,sizeof(copied));memory.events.clear();
        assert(fifo.peek(copied,sizeof(copied))==Result::IO);
        for(auto value:copied)assert(value==0xb6);
        memory.assert_no_write();assert(fifo.consume(64)==Result::Quarantined);
    }
    for(unsigned failure=0;failure<4;++failure) {
        Memory memory;memory.word(Memory::Descriptor+failure*4,4);
        Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::Invalid);
        memory.assert_no_write();assert(fifo.state()==Fifo::State::Uncertain);
    }
    {
        Memory memory;Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        memory.receive(0,packet,64);uint8_t copied[64];assert(fifo.peek(copied,64)==Result::OK);
        assert(fifo.consume(8)==Result::Invalid);
        memory.word(Memory::Descriptor+8,8);
        assert(fifo.consume(64)==Result::Invalid && fifo.state()==Fifo::State::Uncertain);
    }
    for(bool notification:{false,true}) {
        Memory memory;Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        memory.receive(0,packet,64);uint8_t copied[64];assert(fifo.peek(copied,64)==Result::OK);
        if(notification)memory.fail_notification=true;
        else memory.fail_write=Memory::Descriptor+8;
        assert(fifo.consume(64)==Result::IO && fifo.progress().rx_publication_uncertain);
        assert(fifo.consume(64)==Result::Quarantined);
    }
    {
        Memory memory;Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        uint8_t open[22]{};G::header(open,G::Open,1,14);
        std::memcpy(open+8,"apr_audio_svc",14);
        assert(fifo.send(open,sizeof(open))==Result::OK);
        assert(memory.bytes[Memory::Tx+22]==0 && memory.bytes[Memory::Tx+23]==0);
        memory.events.clear();open[21]=1;
        assert(fifo.send(open,sizeof(open))==Result::Invalid);memory.assert_no_write();
    }
    {
        Memory memory;auto ops=memory.callbacks();ops.rx=ops.tx;
        Fifo fifo(ops);assert(fifo.admit(9,true,true,true)==Result::Invalid);memory.assert_no_write();
    }
    for(unsigned index=0;index<4;++index) {
        Memory memory;memory.fail_read=Memory::Descriptor+index*4;
        Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::IO);
        assert(fifo.state()==Fifo::State::Uncertain);memory.assert_no_write();
    }
    {
        Memory memory;Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        memory.receive(0,packet,64);uint8_t copied[80];std::memset(copied,0xb6,sizeof(copied));
        assert(fifo.peek(copied,7)==Result::Invalid && fifo.consume(64)==Result::Invalid);
        assert(fifo.peek(copied,72)==Result::Invalid);
        assert(fifo.peek(copied,8)==Result::OK && fifo.consume(8)==Result::Invalid);
        // A complete frame is required before publishing RX consumption.
        assert(fifo.peek(copied,64)==Result::OK && fifo.consume(64)==Result::OK);
        size_t available=9;assert(fifo.available(available)==Result::OK && available==0);
        assert(fifo.peek(copied,8)==Result::Invalid);
    }
    {
        Memory memory;Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        uint8_t corrupt[64];std::memcpy(corrupt,packet,64);ADSP::write16(corrupt,UINT16_MAX);
        memory.receive(0,corrupt,64);uint8_t copied[64];
        assert(fifo.peek(copied,64)==Result::OK && !std::memcmp(copied,corrupt,64));
        assert(fifo.consume(64)==Result::Invalid && memory.word(Memory::Descriptor+8)==0);
        memory.events.clear();
        assert(fifo.send(corrupt,64)==Result::Invalid);memory.assert_no_write();
    }
    {
        Memory memory;Fifo fifo(memory.callbacks());assert(fifo.admit(9,true,true,true)==Result::OK);
        std::vector<uint8_t> maximum(ADSP::MaxPacket+16);datagram(maximum.data(),ADSP::MaxPacket);
        const unsigned tail=Memory::Ring-8;memory.receive(tail,maximum.data(),maximum.size());
        std::vector<uint8_t> copied(maximum.size());assert(fifo.peek(copied.data(),copied.size())==Result::OK);
        assert(copied==maximum && fifo.consume(copied.size())==Result::OK);
        memory.word(Memory::Descriptor,tail);memory.word(Memory::Descriptor+4,tail);
        assert(fifo.send(maximum.data(),maximum.size())==Result::OK);
        memory.word(Memory::Descriptor,8);memory.word(Memory::Descriptor+4,8);
        std::vector<uint8_t> too_large(maximum.size()+8);
        assert(fifo.send(too_large.data(),too_large.size())==Result::Invalid);
    }
    puts("Lilac APR SMEM FIFO PASS: 2048 TX/RX ring positions, exact 16-byte reserve, staged RX guards, data/barrier/head/notify order, AP-only cursors, failure at each word and ambiguous publication; callback memory only");
    return 0;
}
