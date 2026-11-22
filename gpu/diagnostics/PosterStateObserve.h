/* SPDX-License-Identifier: BSD-2-Clause
 * Record original return values, never invoke an environment getter or draw. */
#pragma once
#include <stdatomic.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <mach/mach_time.h>

typedef void (*SweetPosterStateEmit)(const char *, ...);
enum SweetPosterStateAPI {
    PST_PREVIEW, PST_SNAPSHOT, PST_LOW_LIGHT, PST_BACKLIGHT, PST_LINEAR,
    PST_UNLOCK, PST_MIN_SIZE, PST_DESIRED_SIZE, PST_RENDERER_ENV,
    PST_ENV_INIT, PST_ENV_UPDATE, PST_API_COUNT
};
enum { PST_SLOTS=256, PST_MAX_RECORDS=512 };
static const char *const poster_state_names[] = {
    "preview", "snapshot", "low_light", "backlight", "linear_backlight",
    "unlock", "min_size", "desired_size", "renderer_env", "env_init", "env_update"
};
struct SweetPosterStateSlot { uintptr_t receiver; uint64_t first,second; unsigned api,kind,outcome; };
static struct SweetPosterStateSlot poster_state_slots[PST_SLOTS];
static unsigned poster_state_used,poster_state_records;
static atomic_flag poster_state_guard=ATOMIC_FLAG_INIT;
static atomic_uint poster_state_limit_flags;
static SweetPosterStateEmit poster_state_emit;

static void poster_state_limit(unsigned bit,const char *reason)
{
    unsigned old=atomic_fetch_or_explicit(&poster_state_limit_flags,bit,memory_order_relaxed);
    if(!(old&bit) && poster_state_emit)
        poster_state_emit("SWEET PST LIMIT pid=%d reason=%s original_forwarded=1",getpid(),reason);
}

static void poster_state_observe(SweetPosterStateEmit emit)
{
    int saved=errno;poster_state_emit=emit;
    emit("SWEET PST CONFIG pid=%d revision=1 apis=11 slots=256 max=512 original_returns=1 getter_calls=0 GPU_work=0",getpid());
    errno=saved;
}

static void poster_state_value(unsigned api,uintptr_t receiver,unsigned kind,
                               uint64_t first,uint64_t second,bool returned)
{
    int saved=errno;
    if(!poster_state_emit || api>=PST_API_COUNT || !receiver){errno=saved;return;}
    if(atomic_flag_test_and_set_explicit(&poster_state_guard,memory_order_acquire)){
        poster_state_limit(1,"busy");errno=saved;return;
    }
    unsigned at=0;
    while(at<poster_state_used && (poster_state_slots[at].api!=api || poster_state_slots[at].receiver!=receiver))++at;
    unsigned outcome=returned?1:0;
    if(at<poster_state_used && poster_state_slots[at].kind==kind && poster_state_slots[at].outcome==outcome &&
       poster_state_slots[at].first==first && poster_state_slots[at].second==second){
        atomic_flag_clear_explicit(&poster_state_guard,memory_order_release);errno=saved;return;
    }
    if(at==PST_SLOTS || poster_state_records==PST_MAX_RECORDS){
        bool full=at==PST_SLOTS;atomic_flag_clear_explicit(&poster_state_guard,memory_order_release);
        poster_state_limit(full?2:4,full?"slots":"records");errno=saved;return;
    }
    if(at==poster_state_used)++poster_state_used;
    poster_state_slots[at]=(struct SweetPosterStateSlot){receiver,first,second,api,kind,outcome};
    unsigned sample=++poster_state_records;
    uint64_t ticks=mach_absolute_time();
    atomic_flag_clear_explicit(&poster_state_guard,memory_order_release);
    poster_state_emit("SWEET PST VALUE pid=%d sample=%u api=%s receiver=%016llx kind=%c outcome=%c ticks=%016llx",
        getpid(),sample,poster_state_names[api],(unsigned long long)receiver,kind,returned?'R':'T',(unsigned long long)ticks);
    poster_state_emit("SWEET PST BITS pid=%d sample=%u first=%016llx second=%016llx",
        getpid(),sample,(unsigned long long)first,(unsigned long long)second);
    errno=saved;
}
