#ifndef _GIC_IMP_H_
#define	_GIC_IMP_H_

extern void hal_interrupt_init(void);
extern inline void gic_mask_irq(int irq);
extern inline void gic_unmask_irq(int irq);
extern inline void gic_eoi_irq(int irq);
extern inline int gic_ack_irq(void);
extern void gic_set_type(int irq, int type);
extern void gic_set_pri(int irq, int pri);
#endif
