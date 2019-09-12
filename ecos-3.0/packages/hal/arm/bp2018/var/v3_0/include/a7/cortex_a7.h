/*
 * Copyright (c) 2012, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#if !defined(__CORTEX_A7_H__)
#define __CORTEX_A7_H__

#include <cyg/infra/cyg_type.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/plf/irq_defs.h>

//! @addtogroup cortexa9
//! @{

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! @name Instruction macros
//@{
#define _ARM_NOP()  asm volatile ("nop\n\t")
#define _ARM_WFI()  asm volatile ("wfi\n\t")
#define _ARM_WFE()  asm volatile ("wfe\n\t")
#define _ARM_SEV()  asm volatile ("sev\n\t")
#define _ARM_DSB()  asm volatile ("dsb\n\t")
#define _ARM_DMB()  asm volatile ("dmb\n\t")
#define _ARM_ISB()  asm volatile ("isb\n\t")

#define isb()                           \
        ({asm volatile(                 \
        "isb" : : : "memory");          \
        })

#define	dsb() asm volatile("dsb" : : : "memory");

#define _ARM_MRC(coproc, opcode1, Rt, CRn, CRm, opcode2)	\
    asm volatile ("mrc p" #coproc ", " #opcode1 ", %[output], c" #CRn ", c" #CRm ", " #opcode2 "\n" : [output] "=r" (Rt))
    
#define _ARM_MCR(coproc, opcode1, Rt, CRn, CRm, opcode2)	\
    asm volatile ("mcr p" #coproc ", " #opcode1 ", %[input], c" #CRn ", c" #CRm ", " #opcode2 "\n" :: [input] "r" (Rt))
//@}

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
extern "C" {
#endif

/*cache interface*/
void v7_dma_inv_range(u32, u32);
void v7_dma_clean_range(u32, u32);
void v7_dma_flush_range(u32, u32);

void invalidate_dcache_all(void);
void invalidate_icache_all(void);
void flush_dcache_all(void);

void hal_bbram_cache_enable(void); 
void hal_bbram_cache_disable(void); 
unsigned int  hal_get_pagetable_base(void);
/*mmu*/
void mmu_setup(void);

/*arch timer */
int arch_timer_starting_cpu(void);  
inline u64 arch_counter_get_cntpct(void); 
inline u32 arch_timer_get_cntfrq(void); 

void hal_interrupt_stats_display(void);
cyg_int32 hal_get_interrupt_stats(SYS_IRQ_ID_T irqnum, 
                                cyg_uint32 *isr_latency, 
                                cyg_uint32 *isr,  
                                cyg_uint32 *dsr_latency, 
                                cyg_uint32 *dsr,
                                cyg_uint32 *max_us,
                                cyg_uint32 *avg_us
                                );

unsigned int arch_local_irq_save(void);
void arch_local_irq_restore(unsigned int flags); 
extern void hal_delay_us(cyg_int32 usecs);

#if defined(__cplusplus)
}
#endif

//! @}

#endif // __CORTEX_A7_H__
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
