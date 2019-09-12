/**************************************************************************/
/**
*
* @file     bp2016_timer.c
*
* @brief    BP2016 timer functions
*
***************************************************************************/
/*==========================================================================
//
//      bp2016_timer.c
//
//      HAL timer code using the HG Timer Counter
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

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>

#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // necessary?
#include <cyg/hal/timer/timer_imp.h>
#include <cyg/hal/a7/cortex_a7.h>

#define HAL_TIMER_ENABLE(__period) hal_ostimer_enable(CYGNUM_HAL_RTC_TIMER_ID, __period, IRQ_MODE)
#define HAL_TIMER_DISABLE() hal_ostimer_disable(CYGNUM_HAL_RTC_TIMER_ID)
#define HAL_TIMER_VALUE() hal_ostimer_get_cur_value(CYGNUM_HAL_RTC_TIMER_ID)
#define HAL_TIMER_COUNTER_H_VALUE() hal_ostimer_get_counter_hvalue(CYGNUM_HAL_RTC_TIMER_ID)
#define HAL_TIMER_COUNTER_L_VALUE() hal_ostimer_get_counter_lvalue(CYGNUM_HAL_RTC_TIMER_ID)

/* Internal tick units */
static cyg_uint32 _period;

/****************************************************************************/
/**
*
* HAL clock initialize: Initialize OS timer 
*
* @param    period - value for load to private timer.
*
* @return   none
*
*****************************************************************************/

void hal_clock_initialize(cyg_uint32 period)
{
    HAL_TIMER_ENABLE(period);
    _period = period;
}

/****************************************************************************/
/**
*
* HAL clock reset handler: Reset OS timer
*
* @param    vector - interrupt number of private timer.
* @param    period - value for load to private timer.
*
* @return   none
*
*****************************************************************************/
void hal_clock_reset(cyg_uint32 vector, cyg_uint32 period)
{
     HAL_TIMER_DISABLE();
     HAL_TIMER_ENABLE(period);
     _period = period;
}

/****************************************************************************/
/**
*
* HAL clock read
*
* @param    pvalue - pointer to cyg_uint32 variable for filling current timer value.
*
* @return   none
*
*****************************************************************************/
void hal_clock_read(cyg_uint32 *pvalue)
{
    cyg_uint32 i;

    i = HAL_TIMER_VALUE();
    *pvalue = _period - i;
}

cyg_uint64 hal_ostimer_get_counter_value(void)
{
    cyg_uint32 l, h;
    CYG_INTERRUPT_STATE old_ints;
    
    HAL_DISABLE_INTERRUPTS(old_ints);
    l = HAL_TIMER_COUNTER_L_VALUE();
    h = HAL_TIMER_COUNTER_H_VALUE();
    HAL_RESTORE_INTERRUPTS(old_ints);
    //interrupt restore	
    return ((cyg_uint64)h << 32) | l;
}
/****************************************************************************/
/**
*
* HAL us delay
*
* @param    usecs - number of usecs for delay.
*
* @return   none
*
*****************************************************************************/
// This will only work up to 1000us and then it will wrap twice and fail to be accurate.
// The 1000 comes from the counter value used by EPIT2. Because that value also sets
// the 1ms RTC, it can't be changed.
void hal_delay_us(cyg_int32 usecs)
{
    cyg_uint64 s, e;
    s = arch_counter_get_cntpct();
    e = s + ((cyg_uint64)usecs * archtimer_perus_to_tick);
    while(e > s) {
    	s = arch_counter_get_cntpct();
    }	  
}

// bp2016_timer.c
