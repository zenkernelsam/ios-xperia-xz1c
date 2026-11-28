/* Actual producer framing and no-query paths; no hardware commands. */
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../../xnu/support/LilacSpmiLaneSnapshot.h"
static bool live=true;
static unsigned queries,fail_mode;
static std::vector<std::string> records;
static bool vermeer_loop_live_ready(void){return live;}
static void sweet_screen_record(const char *format,...)
{
    char line[512];va_list args;va_start(args,format);int n=vsnprintf(line,sizeof(line),format,args);va_end(args);
    assert(n>0 && n<512);records.emplace_back(line);puts(line);
}
extern "C" int sweet_lilac_wled_lane_snapshot(unsigned ppid,struct LilacSpmiLaneSnapshot *out)
{
    assert(live && (ppid==0x3d8 || ppid==0x3d9));++queries;
    if(fail_mode==1)return -1;
    *out={};out->revision=1;out->ppid=ppid;out->matches=1;out->selected=ppid==0x3d8?11:12;
    out->lanes[0]={out->selected,(ppid<<8)|0x42,0xa0,1,0,0};
    if(fail_mode==2)out->revision=99;
    if(fail_mode==3)live=false;
    if(fail_mode==4)out->ppid=ppid==0x3d8?0x3d9:0x3d8;
    return 0;
}
#include "../../xnu/support/LilacWledLaneScreenProbe.h"
static void reset(const char *name)
{printf("HOST WLED LANES %s BEGIN\n",name);live=true;queries=fail_mode=0;records.clear();lilac_wled_lanes_emitted=false;}
static void end(const char *name){printf("HOST WLED LANES %s END\n",name);}
int main(void)
{
    const char *token="0123456789abcdef";
    reset("success");lilac_wled_lane_screen_probe(token,1);
    assert(queries==2 && records.size()==6 && records.back().find("result=0")!=std::string::npos);
    lilac_wled_lane_screen_probe(token,1);assert(queries==2 && records.size()==6);end("success");
    reset("not-live");live=false;lilac_wled_lane_screen_probe(token,1);assert(!queries && records.empty());end("not-live");
    reset("wrong-frame");lilac_wled_lane_screen_probe(token,2);assert(!queries && records.empty());end("wrong-frame");
    reset("bad-token");for(const char *bad:{"", "0123456789abcde", "0123456789abcdef0", "0123456789abcdeg"}){
        lilac_wled_lane_screen_probe(bad,1);assert(!queries && records.empty());}
    lilac_wled_lane_screen_probe(nullptr,1);assert(!queries && records.empty());end("bad-token");
    for(unsigned mode=1;mode<=4;++mode){
        const char *name=mode==1?"getter-failed":mode==2?"invalid-state":mode==3?"live-dropped":"wrong-ppid";
        reset(name);fail_mode=mode;lilac_wled_lane_screen_probe(token,1);
        if(mode==3)assert(queries==1 && records.size()==1);
        else assert(queries==2 && records.size()==4 && records.back().find("result=1")!=std::string::npos);
        end(name);
    }
    puts("WLED lane screen producer host PASS: 8 cases; first-frame/current-LIVE only, no repeated queries");
}
