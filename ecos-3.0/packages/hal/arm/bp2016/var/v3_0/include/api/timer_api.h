#ifndef _TIMER_API_H_
#define _TIMER_API_H_

#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/regs/timer.h>

typedef void (*callback_func)(void * arg);

typedef enum {
    TIMER_ID_2,
    TIMER_ID_3,
}TIMER_ID_E;

// function declare
void *TimerInit(cyg_uint32 id, cyg_uint32 pri, cyg_uint32 period_us, callback_func isr_callback, callback_func dsr_callback, void *arg);
void TimerStop(void *handle);
cyg_uint64 TimerGetCurrent(cyg_uint32 id);

#endif
