/* SPDX-License-Identifier: BSD-2-Clause
 * Consume the actual same-process GLES suite signal before any native work. */
#pragma once
#include <stdint.h>
typedef uint32_t (*SweetProbeState)(void);
typedef void (*SweetProbePause)(void);
static inline int sweet_probe_wait_for_gles(SweetProbeState state,SweetProbePause pause,unsigned ticks)
{
    for(unsigned i=0;i<ticks;++i){uint32_t current=state();
        if(current==2)return 0;if(current==3 || current>3)return -1;pause();}
    return -2;
}
