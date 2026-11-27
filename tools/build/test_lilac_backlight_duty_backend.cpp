#include "../../xnu/support/LilacBacklightDutyBackend.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <cmath>

struct Bus {
    uint8_t control[256]{},sink[256]{},staged[4]{};
    unsigned reads=0,writes=0,waits=0,ready_calls=0,fail_write=0;
    unsigned epoch_after_read=0,epoch_after_write=0;
    uint64_t epoch=123;
    int ready_value=1;
    bool change_epoch=false,reenter=false,epoch_after_wait=false;
    lilac_backlight_duty_backend *backend=nullptr;
};
static void initialize(Bus &b,unsigned duty=1989)
{b=Bus{};b.control[0x46]=0x80;b.sink[0x46]=0x30;b.sink[0x50]=b.sink[0x60]=0x80;b.sink[0x57]=b.sink[0x67]=uint8_t(duty);b.sink[0x58]=b.sink[0x68]=uint8_t(duty>>8);}
static int read(void *context,unsigned ppid,unsigned at,uint8_t *value)
{auto &b=*static_cast<Bus *>(context);assert(at<256 && (ppid==0x3d8 || ppid==0x3d9));++b.reads;*value=ppid==0x3d8?b.control[at]:b.sink[at];if(b.reads==b.epoch_after_read)++b.epoch;return 0;}
static int write(void *context,uint8_t at,uint8_t value)
{
    auto &b=*static_cast<Bus *>(context);const uint8_t offsets[]={0x57,0x58,0x67,0x68};unsigned index=b.writes++%6;
    if(index<4){assert(at==offsets[index]);b.staged[index]=value;}
    else if(index==4){assert(at==0x47 && value==15);b.sink[at]=value;}
    else{assert(at==0x47 && !value);b.sink[at]=0;for(unsigned i=0;i<4;++i)b.sink[offsets[i]]=b.staged[i];}
    if(b.writes==b.epoch_after_write)++b.epoch;
    return b.writes==b.fail_write?-1:0;
}
static int wait(void *context,uint16_t delay)
{auto &b=*static_cast<Bus *>(context);assert(delay==1000 && b.sink[0x47]==15);++b.waits;if(b.epoch_after_wait)++b.epoch;return 0;}
static int ready(void *context,uint64_t *epoch)
{
    auto &b=*static_cast<Bus *>(context);++b.ready_calls;
    if(b.reenter){assert(lilac_backlight_apply_nonzero(b.backend,{0,65535,32768},{50000,0,0,false})==LILAC_BACKLIGHT_BUSY);}
    *epoch=b.epoch+(b.change_epoch && b.ready_calls==2?1:0);return b.ready_value;
}
static lilac_backlight_duty_backend backend(Bus &b,unsigned ceiling=4095)
{return {{read,write,wait},&b,ready,123,uint16_t(ceiling),{0},false};}
static unsigned duty(const Bus &b){return b.sink[0x57]|(unsigned(b.sink[0x58]&15)<<8);}
int main()
{
    Bus b;initialize(b);auto io=backend(b);b.backend=&io;
    const lilac_backlight_range range={0,65535,32768};
    for(auto request:{lilac_backlight_request{0,0,0,false},{100,1,0,false},{100,0,1,true},{UINT64_MAX,0,0,false}}){
        unsigned reads=b.reads,writes=b.writes,readiness=b.ready_calls;
        auto result=lilac_backlight_apply_nonzero(&io,range,request);assert(result!=LILAC_BACKLIGHT_DUTY_VERIFIED);assert(b.reads==reads && b.writes==writes && b.ready_calls==readiness);
    }
    b.ready_value=-1;assert(lilac_backlight_apply_nonzero(&io,range,{32768,0,0,false})==LILAC_BACKLIGHT_NOT_READY && !b.reads);
    b.ready_value=1;b.epoch=124;assert(lilac_backlight_apply_nonzero(&io,range,{32768,0,0,false})==LILAC_BACKLIGHT_NOT_READY && !b.reads);b.epoch=123;
    b.reenter=true;assert(lilac_backlight_apply_nonzero(&io,range,{32768,0,0,false})==LILAC_BACKLIGHT_DUTY_VERIFIED);assert(duty(b)==2048 && b.waits==1);b.reenter=false;
    unsigned previous=0;
    for(unsigned value=1;value<=65535;++value){
        assert(lilac_backlight_apply_nonzero(&io,range,{value,0,0,false})==LILAC_BACKLIGHT_DUTY_VERIFIED);
        unsigned actual=duty(b),expected=unsigned(std::ceil(static_cast<long double>(value)*4095/65535));if(expected<16)expected=16;
        assert(actual==expected && actual>=16 && actual>=previous && actual<=4095);previous=actual;
    }
    assert(previous==4095 && b.control[0x46]==0x80 && b.sink[0x46]==0x30);
    for(unsigned at=1;at<=6;++at){initialize(b);io=backend(b);b.backend=&io;b.fail_write=at;
        assert(lilac_backlight_apply_nonzero(&io,range,{65535,0,0,false})==LILAC_BACKLIGHT_DUTY_UNCERTAIN);
        unsigned reads=b.reads,writes=b.writes;assert(lilac_backlight_apply_nonzero(&io,range,{100,0,0,false})==LILAC_BACKLIGHT_DUTY_UNCERTAIN);assert(reads==b.reads && writes==b.writes);
    }
    initialize(b);io=backend(b);b.backend=&io;b.change_epoch=true;
    assert(lilac_backlight_apply_nonzero(&io,range,{65535,0,0,false})==LILAC_BACKLIGHT_DUTY_UNCERTAIN && io.transaction.uncertain);
    for(unsigned at=1;at<=38;++at){
        initialize(b);io=backend(b);b.backend=&io;b.epoch_after_read=at;
        assert(lilac_backlight_apply_nonzero(&io,range,{65535,0,0,false})==LILAC_BACKLIGHT_DUTY_UNCERTAIN);
        assert(b.reads==at && b.writes==(at<=19?0u:6u) && io.transaction.uncertain);
        unsigned reads=b.reads,writes=b.writes;assert(lilac_backlight_apply_nonzero(&io,range,{100,0,0,false})==LILAC_BACKLIGHT_DUTY_UNCERTAIN);assert(b.reads==reads && b.writes==writes);
    }
    for(unsigned at=1;at<=6;++at){
        initialize(b);io=backend(b);b.backend=&io;b.epoch_after_write=at;
        assert(lilac_backlight_apply_nonzero(&io,range,{65535,0,0,false})==LILAC_BACKLIGHT_DUTY_UNCERTAIN);
        assert(b.writes==at && b.waits==(at==6?1u:0u) && io.transaction.uncertain);
    }
    initialize(b);io=backend(b);b.backend=&io;b.epoch_after_wait=true;
    assert(lilac_backlight_apply_nonzero(&io,range,{65535,0,0,false})==LILAC_BACKLIGHT_DUTY_UNCERTAIN && b.writes==5 && b.waits==1);
    initialize(b);io=backend(b,1989);b.backend=&io;assert(lilac_backlight_apply_nonzero(&io,range,{65535,0,0,false})==LILAC_BACKLIGHT_DUTY_VERIFIED && !b.writes);
    initialize(b,16);io=backend(b);b.backend=&io;assert(lilac_backlight_apply_nonzero(&io,{16,4095,16},{16,0,0,false})==LILAC_BACKLIGHT_DUTY_VERIFIED && !b.writes);
    initialize(b);io=backend(b);b.backend=&io;assert(lilac_backlight_apply_nonzero(&io,{0,LILAC_PROPERTY_MAX,1989},{LILAC_PROPERTY_MAX,0,0,false})==LILAC_BACKLIGHT_DUTY_VERIFIED && duty(b)==4095);
    std::puts("BACKLIGHT_DUTY PASS property_values=65535 nonlinear_optics_not_claimed=1 epoch_reentry_faults=1 epoch_read_positions=38 epoch_write_positions=6 epoch_wait_position=1 no_control_power_writes=1");
}
