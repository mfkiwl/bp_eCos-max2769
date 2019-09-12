/**************************************************************************/
/**
*
* @file     bp2018_intr.c
*
* @brief    BP2018 intr functions
*
***************************************************************************/
/*==========================================================================
//
//      bp2018_intr.c
//
//      HAL intr code using the HG Timer Counter
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):      Mike Jones
// Contributor(s): ITR-GmbH
// Date:           2013-08-08
// Purpose:        HAL board support
// Description:    Implementations of HAL board interfaces
//
//####DESCRIPTIONEND####
//
//========================================================================
*/

#include <pkgconf/hal.h>
#include <cyg/io/devtab.h>

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>

#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // necessary?
#include <cyg/hal/gic400/gic_imp.h>
#include <cyg/hal/gic400/arm-gic.h>
#include <cyg/hal/a7/cortex_a7.h>

#ifdef  CYGINT_HAL_INTERRUPT_TIME_STAT 
struct hal_interrupt_stat_per_irq {
    cyg_uint64   s;
    cyg_uint64   e;

    cyg_uint32   isr_latency;
    cyg_uint32   isr_cycle;
    cyg_uint32   dsr_latency;
    cyg_uint32   dsr_cycle;

    cyg_uint32   total_us;
    cyg_uint32   max_us;
    cyg_uint32   count;
};

static struct hal_interrupt_stat_per_irq  hal_interrupt_stats[NB_SYS_IRQ];
cyg_uint64 hal_irq_stat_s;
#endif 
extern void gic_init(void);
extern inline void gic_mask_irq(int irq);
extern inline void gic_unmask_irq(int irq);
extern inline int gic_ack_irq(void);
extern inline void gic_eoi_irq(int irq);
extern void gic_set_type(int irq, int type);
extern void gic_set_pri(int irq, int pri);

void hal_interrupt_init(void)
{
   gic_init();	
}
/****************************************************************************/
/**
*
* IRQ handler.
* This routine is called to respond to a hardware interrupt (IRQ).  It
* should interrogate the hardware and return the IRQ vector number.
*
* @return   none
*
*****************************************************************************/
int hal_IRQ_handler(unsigned char *frame)
{
    cyg_int32 vector;
    cyg_int32 intr;

    intr = gic_ack_irq();
    vector = intr;

    if (intr >= 1020) {
        return CYGNUM_HAL_INTERRUPT_NONE;
    }    
    else
    {
        // An invalid interrupt source is treated as a spurious interrupt    
        if (intr < CYGNUM_HAL_ISR_MIN || intr > CYGNUM_HAL_ISR_MAX)
            return CYGNUM_HAL_INTERRUPT_NONE;
    }
    if(vector >= 32)
	    vector = vector - 32;
    
    return vector;
}

/****************************************************************************/
/**
*
* Interrupt control: mask interrupt.
*
* @param    vector
*
* @return   none
*
*****************************************************************************/
void hal_interrupt_mask(int vector)
{
    gic_mask_irq(vector);
}

/****************************************************************************/
/**
*
* Interrupt control: unmask interrupt.
*
* @param    vector
*
* @return   none
*
*****************************************************************************/
void hal_interrupt_unmask(int vector)
{
    gic_unmask_irq(vector);
}

/****************************************************************************/
/**
*
* Interrupt acknowlage.
*
* @param    vector
*
* @return   none
*
*****************************************************************************/
void hal_interrupt_acknowledge(int vector)
{
    if (vector!= CYGNUM_HAL_INTERRUPT_NONE)
	gic_eoi_irq(vector);
}

/****************************************************************************/
/**
*
* Interrupt control: Set interrupt configuration.
*
* @param    vector - interrupt number [0..94].
* @param    level  - priority is the new priority for the IRQ source. 0x00 is highest, 0xFF lowest.
* @param    up     - trigger type for the IRQ source.
*
* @return   none
*
*****************************************************************************/
void hal_interrupt_configure(int vector, int level, int up)
{
    cyg_uint32 type;
    if(true == level){
        type = IRQ_TYPE_LEVEL_HIGH;
    } else {
        type = IRQ_TYPE_EDGE_RISING;
    }
	
    gic_set_type(vector, type);
}

/****************************************************************************/
/**
*
* Interrupt control: Set reduced interrupt configuration.
*
* @param    vector -
* @param    level  - priority is the new priority for the IRQ source. 0x00 is highest, 0xFF lowest.
*
* @return   none
*
*****************************************************************************/
void hal_interrupt_set_level(int vector, int level)
{
    gic_set_pri(vector, level);
}

#ifdef  CYGINT_HAL_INTERRUPT_TIME_STAT 
void hal_interrupt_irq_early_enter(void)
{
    hal_irq_stat_s = arch_counter_get_cntpct();
}

void hal_interrupt_irq_enter(int vector)
{
    cyg_uint64      c;
    cyg_uint32      isr_latency;
    struct hal_interrupt_stat_per_irq  * intr_s;
    if((vector < 0) || (vector >= NB_SYS_IRQ))
        return ;

    c = arch_counter_get_cntpct();
    isr_latency = (cyg_uint32)(c - hal_irq_stat_s);

    intr_s = &hal_interrupt_stats[vector];
    if(isr_latency > intr_s->isr_latency)
        intr_s->isr_latency = isr_latency;
    intr_s->count ++;
    intr_s->e = c;
    intr_s->s = hal_irq_stat_s;
}

void hal_interrupt_isr_exit(int vector)
{
    cyg_uint64      c;
    cyg_uint32      isr_cycle;
    struct hal_interrupt_stat_per_irq  * intr_s;
    if((vector < 0) || (vector >= NB_SYS_IRQ))
        return ;
        
    c = arch_counter_get_cntpct();
    intr_s = &hal_interrupt_stats[vector];
    isr_cycle = (cyg_uint32)(c - intr_s->e);
    if(isr_cycle > intr_s->isr_cycle)
        intr_s->isr_cycle = isr_cycle;
    intr_s->e = c;
}

void hal_interrupt_dsr_enter(cyg_int32 vector)
{
    cyg_uint64      c;
    cyg_uint32      dsr_latency;
    struct hal_interrupt_stat_per_irq  * intr_s;
    if((vector < 0) || (vector >= NB_SYS_IRQ))
        return ;
 
    intr_s = &hal_interrupt_stats[vector];
        
    c = arch_counter_get_cntpct();
    dsr_latency = (cyg_uint32)(c - intr_s->e);
    if(dsr_latency > intr_s->dsr_latency)
        intr_s->dsr_latency = dsr_latency;
    intr_s->e = c;
}

void hal_interrupt_irq_leave(int vector)
{
    cyg_uint64      c;
    cyg_uint32      dsr_cycle;
    cyg_uint32      us;
    struct hal_interrupt_stat_per_irq  * intr_s;
    if((vector < 0) || (vector >= NB_SYS_IRQ))
        return ;
 
    intr_s = &hal_interrupt_stats[vector];
 
    c = arch_counter_get_cntpct();
    dsr_cycle = (cyg_uint32)(c - intr_s->e);
    if(dsr_cycle > intr_s->dsr_cycle)
        intr_s->dsr_cycle = dsr_cycle;

    us = (cyg_uint32)(c- intr_s->s)/archtimer_perus_to_tick;
    if(us > intr_s->max_us)
        intr_s->max_us = us;
    intr_s->total_us +=us;

    hal_interrupt_stats[vector].e = c;
}

void hal_interrupt_stats_display(void)
{
    cyg_int32 i;
    struct hal_interrupt_stat_per_irq  * intr_s;

    diag_printf("     %10s %11s %10s %11s %10s %10s %10s %10s    (us)\n",                                                                                                                                               "count", "isr_latency", "isr", "dsr_latency", "dsr",
              "total", "max", "avg");

    for(i = 0; i < NB_SYS_IRQ; i++)
    {
        intr_s = &hal_interrupt_stats[i];
        if(intr_s->count == 0)
            continue;

        diag_printf("%4d:%10d %11d %10d %11d %10d %10d %10d %10d\n",
           i,
           intr_s->count,
           intr_s->isr_latency/archtimer_perus_to_tick,
           intr_s->isr_cycle/archtimer_perus_to_tick,
           intr_s->dsr_latency/archtimer_perus_to_tick,
           intr_s->dsr_cycle/archtimer_perus_to_tick,
           intr_s->total_us,
           intr_s->max_us,
           intr_s->total_us/ intr_s->count
        );
    }
}

cyg_int32 hal_get_interrupt_stats(SYS_IRQ_ID_T irqnum, 
                                cyg_uint32 *isr_latency, 
                                cyg_uint32 *isr,  
                                cyg_uint32 *dsr_latency, 
                                cyg_uint32 *dsr,
                                cyg_uint32 *max_us,
                                cyg_uint32 *avg_us
                                )
{
    struct hal_interrupt_stat_per_irq  * intr_s;

    if(irqnum >= NB_SYS_IRQ)
        return -EINVAL;

    intr_s = &hal_interrupt_stats[irqnum];

    if(isr_latency != NULL)
        *isr_latency = intr_s->isr_latency/archtimer_perus_to_tick;

    if(isr!= NULL)
        *isr = intr_s->isr_cycle/archtimer_perus_to_tick;

    if(dsr_latency != NULL)
        *dsr_latency = intr_s->dsr_latency/archtimer_perus_to_tick;

    if(dsr!= NULL)
        *dsr = intr_s->dsr_cycle/archtimer_perus_to_tick;

    if(max_us!= NULL)
        *max_us = intr_s->max_us;

    if(avg_us!= NULL) {
        if(intr_s->count > 0)
            *avg_us = intr_s->total_us/intr_s->count;
        else 
            *avg_us = 0;
    }

    return ENOERR;
}

#else
cyg_int32 hal_get_interrupt_stats(SYS_IRQ_ID_T irqnum, 
                                cyg_uint32 *isr_latency, 
                                cyg_uint32 *isr,  
                                cyg_uint32 *dsr_latency, 
                                cyg_uint32 *dsr,
                                cyg_uint32 *max_us,
                                cyg_uint32 *avg_us
                                )
{
    diag_printf("Need enable CYGINT_HAL_INTERRUPT_TIME_STAT\n");
    return ENOERR;
}
void hal_interrupt_stats_display(void)
{
    diag_printf("Need enable CYGINT_HAL_INTERRUPT_TIME_STAT\n");
}
#endif


// bp2018_intr.c
