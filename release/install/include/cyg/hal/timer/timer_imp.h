#ifndef _TIMER_IMP_H_
#define	_TIMER_IMP_H_

#define IRQ_MODE	(1)

extern bool hal_ostimer_enable_get(cyg_uint32 instance);
extern cyg_int32 hal_ostimer_enable(cyg_uint32 instance, cyg_uint32 load_val, cyg_uint32 irq_mode);
extern cyg_int32 hal_ostimer_disable(cyg_uint32 instance);
extern inline cyg_uint32 hal_ostimer_get_cur_value(cyg_uint32 instance);
extern inline cyg_uint32 hal_ostimer_get_counter_hvalue(cyg_uint32 instance);
extern inline cyg_uint32 hal_ostimer_get_counter_lvalue(cyg_uint32 instance);
#endif
