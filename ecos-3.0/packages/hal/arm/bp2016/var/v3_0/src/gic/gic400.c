#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/var_io.h>
#include <cyg/hal/gic400/arm-gic.h>

#undef rprintf 
#define rprintf(fmt, ...)  
#define writel_relaxed(v,c)	((*(volatile u32 *) (c)) = (v))
#define readl_relaxed(c)	(*(volatile u32 *) (c))

static U32 gic_dist_base, gic_cpu_base;

static U32 gic_get_cpumask(U32 base)
{
	U32 mask, i,cpu_num;

	cpu_num = (readl_relaxed(base + GIC_DIST_CTR) & 0xe0) >> 5;
	if (!cpu_num)
		return 1;   /*uniprocessor, nor SMP*/

	for (i = mask = 0; i < 32; i += 4) {
		mask = readl_relaxed(base + GIC_DIST_TARGET + i);
		mask |= mask >> 16;
		mask |= mask >> 8;
		if (mask)
			break;
	}

	if (!mask) {
		rprintf("GIC CPU mask not found \r\n"); /*hardware configure error*/
	}
	return mask;
}

static void gic_dist_init(U32 base)
{
	unsigned int i;
	U32 cpumask, irqs_num;

	irqs_num = readl_relaxed(base + GIC_DIST_CTR) & 0x1f;
	irqs_num = (irqs_num + 1) * 32; /*32 * (n + 1)*/
	if (irqs_num > 1020)	        /*max is 1020 interrupts*/
		irqs_num = 1020;


	writel_relaxed(0, base + GIC_DIST_CTRL);

	/*
	 * Set all global interrupts to be level triggered, active low.
	 */
	for (i = 32; i < irqs_num; i += 16)
		writel_relaxed(0, base + GIC_DIST_CONFIG + i * 4 / 16);

	/*
	 * Set all global interrupts to this CPU only.
	 */
	cpumask = gic_get_cpumask(base);
	cpumask |= cpumask << 8;
	cpumask |= cpumask << 16;
	rprintf("cpumask = 0x%x\r\n", cpumask);
	for (i = 32; i < irqs_num; i += 4)
		writel_relaxed(cpumask, base + GIC_DIST_TARGET + i * 4 / 4);

	/*
	 * Set priority on all global interrupts.
	 */
	for (i = 32; i < irqs_num; i += 4)
		writel_relaxed(GICD_INT_DEF_PRI_X4, base + GIC_DIST_PRI + i * 4 / 4);

	/*
	 * Disable all interrupts.  Leave the PPI and SGIs alone
	 * as these enables are banked registers.
	 */
	for (i = 32; i < irqs_num; i += 32)
		writel_relaxed(0xffffffff, base + GIC_DIST_ENABLE_CLEAR + i * 4 / 32);

	writel_relaxed(1, base + GIC_DIST_CTRL);
}

static void gic_cpu_init(U32 base, U32 dist_base)
{
	int i;

		/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	 */
	writel_relaxed(0xffff0000, dist_base + GIC_DIST_ENABLE_CLEAR);
	writel_relaxed(0x0000ffff, dist_base + GIC_DIST_ENABLE_SET);

	/*
	 * Set priority on PPI and SGI interrupts
	 */
	for (i = 0; i < 32; i += 4)
		writel_relaxed(GICD_INT_DEF_PRI_X4, dist_base + GIC_DIST_PRI + i * 4 / 4);

	writel_relaxed(0xf0, base + GIC_CPU_PRIMASK);
	writel_relaxed(1, base + GIC_CPU_CTRL);
}

static void gic_mask_irq_(int irq, U32 base)
{
	u32 mask = 1 << (irq % 32);

	writel_relaxed(mask, base + GIC_DIST_ENABLE_CLEAR + (irq / 32) * 4);
}

static void gic_unmask_irq_(int irq, U32 base)
{
	u32 mask = 1 << (irq % 32);

	writel_relaxed(mask, base + GIC_DIST_ENABLE_SET + (irq / 32) * 4);
	rprintf("gic_unmask_irq, irq %d: 0x%x->0x%x\r\n", irq ,mask, base + GIC_DIST_ENABLE_SET + (irq / 32) * 4);
}

static void gic_eoi_irq_(int irq, U32 base)
{
	writel_relaxed(irq, base + GIC_CPU_EOI);
}

static int gic_set_type_(int irq, unsigned int type, U32 base)
{
	unsigned int gicirq = irq;
	u32 enablemask = 1 << (gicirq % 32);
	u32 enableoff = (gicirq / 32) * 4;
	u32 confmask = 0x2 << ((gicirq % 16) * 2);
	u32 confoff = (gicirq / 16) * 4;
	int enabled = 0;
	u32 val;

	/* Interrupt configuration for SGIs can't be changed */
	if (gicirq < 16)
		return -1;

	if (type != IRQ_TYPE_LEVEL_HIGH && type != IRQ_TYPE_EDGE_RISING)
		return -1;


	val = readl_relaxed(base + GIC_DIST_CONFIG + confoff);
	if (type == IRQ_TYPE_LEVEL_HIGH)
		val &= ~confmask;
	else if (type == IRQ_TYPE_EDGE_RISING)
		val |= confmask;

	/*
	 * As recommended by the spec, disable the interrupt before changing
	 * the configuration
	 */
	if (readl_relaxed(base + GIC_DIST_ENABLE_SET + enableoff) & enablemask) {
		writel_relaxed(enablemask, base + GIC_DIST_ENABLE_CLEAR + enableoff);
		enabled = 1;
	}

	writel_relaxed(val, base + GIC_DIST_CONFIG + confoff);

	if (enabled)
		writel_relaxed(enablemask, base + GIC_DIST_ENABLE_SET + enableoff);

	return 0;
}

static void gic_set_pri_(int irq, int pri, u32 base)
{
    u32 pri_off = (irq / 4) * 4;
    u32 pri_mask_off = (irq % 4) * 8;
    u32 val;

    if(pri > GICD_INT_DEF_PRI){
        return;
    }

    val = readl_relaxed(base + GIC_DIST_PRI + pri_off);
    val &= ~(0xFF << pri_mask_off);
    val |= ((pri & 0xFF) << pri_mask_off);
    writel_relaxed(val, base + GIC_DIST_PRI + pri_off);
}

static void gic_get_pri_(int irq, int *pri, u32 base)
{
    u32 pri_off = (irq / 4) * 4;
    u32 pri_mask_off = (irq % 4) * 8;
    u32 val;

    val = readl_relaxed(base + GIC_DIST_PRI + pri_off);
    *pri = ((val >> pri_mask_off) & 0xFF);
}

void gic_init(void)
{
	gic_dist_base = GIC400_BASE + 0x1000;
	gic_cpu_base = GIC400_BASE + 0x2000;

	gic_dist_init(gic_dist_base);
	gic_cpu_init(gic_cpu_base, gic_dist_base);

	rprintf("gic_init dist_base = 0x%x, cpu_base = 0x%x\r\n", gic_dist_base, gic_cpu_base);
}

/*
 *   only for sgi interrupt, so we need add 32
 */
inline void gic_mask_irq(int irq)
{
	irq += 32;
	gic_mask_irq_(irq, gic_dist_base);
}

inline void gic_unmask_irq(int irq)
{
	irq += 32;
	gic_unmask_irq_(irq, gic_dist_base);
}

inline void gic_eoi_irq(int irq)
{
	irq += 32;
	gic_eoi_irq_(irq, gic_cpu_base);
}

inline int gic_ack_irq(void)
{
	U32 intack = readl_relaxed(gic_cpu_base + GIC_CPU_INTACK);
	return (intack & 0x3ff);
}

void gic_set_type(int irq, int type)
{
	irq += 32;
	gic_set_type_(irq, type, gic_dist_base);
}

void gic_set_pri(int irq, int pri)
{
	irq += 32;
	gic_set_pri_(irq, pri, gic_dist_base);
}

bool gic_get_pri(int irq, int *pri)
{
	irq += 32;
	gic_get_pri_(irq, pri, gic_dist_base);
    return true;
}


