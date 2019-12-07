#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>

#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // necessary?
#include <cyg/hal/regs/timer.h>
#include <cyg/hal/timer/timer_imp.h>

static HWP_TIMER_AP_T * hal_ostimer_get(cyg_uint32 instance)
{
    switch(instance) {
        case 0:
            return hwp_apTimer0;
        case 1:
            return hwp_apTimer1;
        case 2:
            return hwp_apTimer2;
        case 3:
            return hwp_apTimer3;
        default:
            return NULL;
    }
    return 0;
}

bool hal_ostimer_enable_get(cyg_uint32 instance)
{
    HWP_TIMER_AP_T * timer = hal_ostimer_get(instance);
    if(NULL == timer)
        return false;
    if(timer->ctrl & 0x1){
        return true;
    }
    return false;
}

cyg_int32 hal_ostimer_enable(cyg_uint32 instance, cyg_uint32 load_val, cyg_uint32 irq_mode)
{
    HWP_TIMER_AP_T * timer = hal_ostimer_get(instance);
    if(NULL == timer)
        return -1;

    timer->ctrl = 0;

    if (load_val != 0)
        timer->loadval= load_val;

    timer->irq_eoi= TIMER_AP_IRQ_CAUSE;

    if (irq_mode == IRQ_MODE)
        timer->irq_enable = TIMER_AP_IRQ_ENABLE;
    else
        timer->irq_enable = 0;

    timer->ctrl= TIMER_AP_ENABLE | TIMER_AP_REPEAT | TIMER_AP_LOAD;
    return 0;
}

cyg_int32 hal_ostimer_disable(cyg_uint32 instance)
{
    HWP_TIMER_AP_T * timer = hal_ostimer_get(instance);
    if(timer == NULL)
        return -1;

    timer->irq_enable = 0;
    timer->irq_eoi= TIMER_AP_IRQ_CAUSE; 	
    timer->ctrl = 0;
    return 0;
}

inline cyg_uint32 hal_ostimer_get_cur_value(cyg_uint32 instance)
{
    HWP_TIMER_AP_T * timer = hal_ostimer_get(instance);
    if(NULL == timer)
        return -1;
    return (cyg_uint32)timer->curval; 
}

inline cyg_uint32 hal_ostimer_get_counter_hvalue(cyg_uint32 instance)
{
    HWP_TIMER_AP_T * timer = hal_ostimer_get(instance);
    if(NULL == timer)
        return -1;
    return (cyg_uint32) timer->lockval_h;
}

inline cyg_uint32 hal_ostimer_get_counter_lvalue(cyg_uint32 instance)
{
    HWP_TIMER_AP_T * timer = hal_ostimer_get(instance);
    if(NULL == timer)
        return -1;
    return (cyg_uint32) timer->lockval_l;
}

cyg_uint64 hal_ostimer_get_current(cyg_uint32 instance)
{
    cyg_uint64  ret;
    HWP_TIMER_AP_T * timer = hal_ostimer_get(instance);
    if(NULL == timer)
        return 0;

    ret = ((((cyg_uint64)(timer->lockval_h)) << 32) | (cyg_uint64)(timer->lockval_l));
    return ret;
}
