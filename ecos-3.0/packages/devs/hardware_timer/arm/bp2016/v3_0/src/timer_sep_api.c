#include <cyg/kernel/kapi.h>
#include <cyg/io/devtab.h>
#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>

#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/plf/chip_config.h>
#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // necessary?
#include <cyg/hal/hal_platform_ints.h> 
#include <cyg/hal/regs/timer.h>
#include <cyg/hal/timer/timer_imp.h>
#include <cyg/hal/api/timer_api.h>
#include <cyg/hal/gic400/arm-gic.h>


typedef struct timer_info {
    cyg_uint32     id;
    bool           init;
    cyg_addrword_t base;
    int            irq_num;
    int            irq_pri;
    cyg_uint32     period;
    cyg_mutex_t    mt;
    //spin_lock_t    spin;
    callback_func  isr_call_back;
    callback_func  dsr_call_back;
    void           *arg;
    cyg_interrupt  interrupt;
    cyg_handle_t   interrupt_handle;
} timer_info;

static timer_info  timer2_info = {
    2,
    false,
    REG_AP_TIMER2_BASE,
    SYS_IRQ_ID_TIMER2, 
    CYGNUM_HAL_KERNEL_COUNTERS_CLOCK_ISR_DEFAULT_PRIORITY
};

static timer_info  timer3_info = {
    3,
    false,
    REG_AP_TIMER3_BASE,
    SYS_IRQ_ID_TIMER3, 
    CYGNUM_HAL_KERNEL_COUNTERS_CLOCK_ISR_DEFAULT_PRIORITY
};

static bool      hardware_timer_init(struct cyg_devtab_entry *tab);
static Cyg_ErrNo hardware_timer_lookup(struct cyg_devtab_entry **tab,
                            struct cyg_devtab_entry *st,
                            const char *name);

CHAR_DEVIO_TABLE(hardware_timer_handler,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL);


CHAR_DEVTAB_ENTRY(hardware_timer_device,
                  "dev/hardware_timer",
                  NULL,          // Base device name
                  &hardware_timer_handler,
                  hardware_timer_init,
                  hardware_timer_lookup,
                  (void*) &timer2_info);   // Private data pointer

static cyg_uint32 
timer_ISR(cyg_vector_t vector, cyg_addrword_t data)
{
    HWP_TIMER_AP_T *hwp;
    timer_info *info = (timer_info *)data;

    cyg_interrupt_mask(info->irq_num);
    cyg_interrupt_acknowledge(info->irq_num);

    hwp = (HWP_TIMER_AP_T *)info->base;
    hwp->irq_eoi= TIMER_AP_IRQ_CAUSE;

    if(info->isr_call_back){
        info->isr_call_back(info->arg);
    }
    //cyg_interrupt_unmask(info->irq_num);
    //return CYG_ISR_HANDLED;
    return CYG_ISR_HANDLED | CYG_ISR_CALL_DSR;
}

// high level interrupt handler (DSR)
static void       
timer_DSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    timer_info *info = (timer_info *)data;

    if(info->dsr_call_back){
        info->dsr_call_back(info->arg);
    }
    cyg_interrupt_unmask(info->irq_num);
}

static void timer_m_init(void)
{
    cyg_mutex_init(&timer2_info.mt);
    cyg_mutex_init(&timer3_info.mt);

    // spinlock init
}

static bool hardware_timer_init(struct cyg_devtab_entry *tab)
{
    diag_printf("timer module init...\n");
    timer_m_init();
    return ENOERR;
}

static Cyg_ErrNo hardware_timer_lookup(struct cyg_devtab_entry **tab,
           struct cyg_devtab_entry *st,
           const char *name)
{
    return ENOERR;
}

void* TimerInit(cyg_uint32 id, cyg_uint32 pri, cyg_uint32 period_us, 
                callback_func isr_callback, callback_func dsr_callback, void * callback_arg)
{
    bool reinit = false, int_init = false, timer_enable = false;
    cyg_uint32 period;
    timer_info * timer = NULL;

    if(TIMER_ID_2 == id){ 
        timer = &timer2_info;
    }else if(TIMER_ID_3 == id){ 
        timer = &timer3_info;
    }else{ 
        return NULL;
    }

    if((0 == period_us) || (period_us >= (0xFFFFFFFF/TICKS_PER_US))){
        return NULL;
    }
    period = period_us * TICKS_PER_US;

    if(pri > CYGNUM_HAL_KERNEL_COUNTERS_CLOCK_ISR_DEFAULT_PRIORITY){
        pri = CYGNUM_HAL_KERNEL_COUNTERS_CLOCK_ISR_DEFAULT_PRIORITY;
    }

    cyg_mutex_lock(&timer->mt);
    // if false == spin_lock()  unlock, return NULL;
    timer_enable = hal_ostimer_enable_get(timer->id);
    if(false == timer_enable){
        int_init = true;
        reinit = true;
    }else if((true == timer_enable) && (false == timer->init)){
        // spin_unlock();
        cyg_mutex_unlock(&timer->mt);
        return NULL;
    }

    cyg_interrupt_mask(timer->irq_num);

    if(pri != timer->irq_pri){
        timer->irq_pri = pri;
        HAL_INTERRUPT_SET_LEVEL(timer->irq_num, pri);
    }
    if(period != timer->period){
        timer->period = period;
        reinit = true;
    }
    if(isr_callback != timer->isr_call_back){
        timer->isr_call_back = isr_callback;
    }
    if(dsr_callback != timer->dsr_call_back){
        timer->dsr_call_back = dsr_callback;
    }

    timer->arg = callback_arg;
    if(true == reinit){
        hal_ostimer_enable(timer->id, period, 1);
    }

    if(true == int_init){
        cyg_interrupt_create(timer->irq_num,
                timer->irq_pri,
                (cyg_addrword_t)timer,
                timer_ISR,
                timer_DSR,
                &timer->interrupt_handle,
                &timer->interrupt);
        cyg_interrupt_attach(timer->interrupt_handle);
    }

    cyg_interrupt_unmask(timer->irq_num);

    // spin_unlock();
    cyg_mutex_unlock(&timer->mt);
    return timer;
}

void TimerStop(void *handle)
{
    timer_info * timer = NULL;
    if(NULL == handle)
        return;
    timer = (timer_info *)handle;
    cyg_interrupt_mask(timer->irq_num);
    cyg_mutex_lock(&timer->mt);
    timer->init = false;
    timer->period = 0;
    timer->irq_pri = CYGNUM_HAL_KERNEL_COUNTERS_CLOCK_ISR_DEFAULT_PRIORITY;
    timer->isr_call_back = NULL;
    timer->dsr_call_back = NULL;
    // spin_lock();
    hal_ostimer_disable(timer->id);
    // spin_unlock();

    cyg_interrupt_detach(timer->interrupt_handle);
    cyg_interrupt_delete(timer->interrupt_handle);
    cyg_mutex_unlock(&timer->mt);
}


