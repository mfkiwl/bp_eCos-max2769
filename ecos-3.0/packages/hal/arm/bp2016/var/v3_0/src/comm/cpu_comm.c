#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h> // base types
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_if.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/plf/iomap.h>
#include <cyg/hal/plf/irq_defs.h>
#include <cyg/hal/regs/scm.h>

/*  CPU0--------send-------->CPU1
 *   regs[0], int0--------CPU1 int SYS_IRQ_ID_SGI0
 *  CPU0<--------ack---------CPU1
 *   CPU0 int SYS_IRQ_ID_SGI0---------regs[0],int0
 *
 *
 *  CPU1--------send-------->CPU0
 *   regs[16], int16--------CPU1 int SYS_IRQ_ID_SGI16
 *  CPU1<--------ack---------CPU0
 *   CPU1 int SYS_IRQ_ID_SGI16---------regs[16],int16
 *
 * for CPU0, trigger 0~15 int of cpu1 to send msgs, and int0~15
 * of cpu0 are acks from CPU1 when processed msgs.
 * Using regs[0~15] store key info.
 *
 * for CPU1, recv int0~15 means new msgs from CPU0, 
 * read regs[0~15] for key info and deal with it
 * when done, trigger ack to CPU0(trigger int0~15 of CPU0)
 *
 */


#ifdef CYGPKG_IO_COMMON_COMM_ARM_BP2016
#include <pkgconf/io_common_comm_arm_bp2016.h>

#define COMM_CH_NUM    (16)
// base interface
#ifdef CYGNUM_DEVS_ARM_BP2016_SELECT_CPU0

static void cpu_comm_trigger(U32 ch)
{
    hwp_apSCM->cpu0to1_int = (0x1 << ch);
}

/* send data function */
void cpu_comm_ch_para_set(U32 ch, U32 addr, U32 len)
{
    CPU_COMM_REGS *cpu_comm_reg = &hwp_apSCM->cpu0_int_regs[ch];
    cpu_comm_reg->addr = addr;
    cpu_comm_reg->size = len;
}

void cpu_comm_send_trigger(U32 ch)
{
    cpu_comm_trigger(ch);
}

cyg_uint32 cpu_comm_ch_ret_get(U32 ch)
{
    CPU_COMM_REGS *cpu_comm_reg = &hwp_apSCM->cpu0_int_regs[ch];
    return cpu_comm_reg->size;
}

/* recv data function */
void cpu_comm_ch_para_get(U32 ch, volatile U32 *addr, volatile U32 *len)
{
    CPU_COMM_REGS *cpu_comm_reg = &hwp_apSCM->cpu1_int_regs[ch];
    *addr = cpu_comm_reg->addr;
    *len = cpu_comm_reg->size;
}

void cpu_comm_ch_ack_ret_set(U32 ch, U32 ret)
{
    CPU_COMM_REGS *cpu_comm_reg = NULL;
    cpu_comm_reg = &hwp_apSCM->cpu1_int_regs[ch];
    cpu_comm_reg->size = ret;
}

void cpu_comm_ack_trigger(U32 ch)
{
    cpu_comm_trigger(ch + COMM_CH_NUM);
}

/* irq */
void cpu_comm_clr_irq(U32 irq_num)
{
    hwp_apSCM->cpu1to0_int_clr = (0x1 << (irq_num - SYS_IRQ_ID_SGI0));
}

void cpu_comm_int_clr_all(void)
{
    hwp_apSCM->cpu1to0_int_clr = 0xFFFFFFFF;
}

#else // CPU1 codes

static void cpu_comm_trigger(U32 ch)
{
    hwp_apSCM->cpu1to0_int = (0x1 << ch);
}

/* send data function */
void cpu_comm_ch_para_set(U32 ch, U32 addr, U32 len)
{
    CPU_COMM_REGS *cpu_comm_reg = &hwp_apSCM->cpu1_int_regs[ch];
    cpu_comm_reg->addr = addr;
    cpu_comm_reg->size = len;
}

void cpu_comm_send_trigger(U32 ch)
{
    cpu_comm_trigger(ch + COMM_CH_NUM);
}

cyg_uint32 cpu_comm_ch_ret_get(U32 ch)
{
    CPU_COMM_REGS *cpu_comm_reg = &hwp_apSCM->cpu1_int_regs[ch];
    return cpu_comm_reg->size;
}

/* recv data function */
void cpu_comm_ch_para_get(U32 ch, volatile U32 *addr, volatile U32 *len)
{
    CPU_COMM_REGS *cpu_comm_reg = &hwp_apSCM->cpu0_int_regs[ch];
    *addr = cpu_comm_reg->addr;
    *len = cpu_comm_reg->size;
}

void cpu_comm_ch_ack_ret_set(U32 ch, U32 ret)
{
    CPU_COMM_REGS *cpu_comm_reg = &hwp_apSCM->cpu0_int_regs[ch];
    cpu_comm_reg->size = ret;
}

void cpu_comm_ack_trigger(U32 ch)
{
    cpu_comm_trigger(ch);
}

/* irq */
void cpu_comm_clr_irq(U32 irq_num)
{
    hwp_apSCM->cpu0to1_int_clr = (0x1 << (irq_num - SYS_IRQ_ID_SGI0));
}

void cpu_comm_int_clr_all(void)
{
    hwp_apSCM->cpu0to1_int_clr = 0xFFFFFFFF;
}

#endif

#endif  // CYGPKG_IO_COMMON_COMM_ARM_BP2016
