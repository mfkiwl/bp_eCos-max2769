#ifndef CYGONCE_HAL_PLATFORM_INTS_H
#define CYGONCE_HAL_PLATFORM_INTS_H
//==========================================================================
//
//      hal_platform_ints.h
//
//      HAL Interrupt and clock support
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
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
// Author(s):    jskov
// Contributors: jskov
// Date:         2001-11-02
// Purpose:      Define Interrupt support
// Description:  The interrupt details for the Agilent AAED2000 are defined here.
// Usage:
//               #include <cyg/hal/hal_platform_ints.h>
//               ...
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <cyg/hal/plf/chip_config.h>

#define CYGNUM_HAL_ISR_MIN            (0)
#define CYGNUM_HAL_ISR_MAX            (NB_SYS_IRQ - 1) 
#define CYGNUM_HAL_ISR_COUNT          (CYGNUM_HAL_ISR_MAX + 1) 


#include <cyg/hal/plf/irq_defs.h>

// The vector used by the Real time clock
#if CYGNUM_HAL_RTC_TIMER_ID==0
#define CYGNUM_HAL_INTERRUPT_RTC     SYS_IRQ_ID_TIMER0 
#elif CYGNUM_HAL_RTC_TIMER_ID==1
#define CYGNUM_HAL_INTERRUPT_RTC     SYS_IRQ_ID_TIMER1 
#elif CYGNUM_HAL_RTC_TIMER_ID==2
#define CYGNUM_HAL_INTERRUPT_RTC     SYS_IRQ_ID_TIMER2 
#elif CYGNUM_HAL_RTC_TIMER_ID==3
#define CYGNUM_HAL_INTERRUPT_RTC     SYS_IRQ_ID_TIMER3 
#endif
 
#define CYGNUM_HAL_INTERRUPT_UART0     SYS_IRQ_ID_UART0
#define CYGNUM_HAL_INTERRUPT_UART1     SYS_IRQ_ID_UART1 
#define CYGNUM_HAL_INTERRUPT_UART2     SYS_IRQ_ID_UART2 
#define CYGNUM_HAL_INTERRUPT_UART3     SYS_IRQ_ID_UART3 
#define CYGNUM_HAL_INTERRUPT_UART4     SYS_IRQ_ID_UART4 
#define CYGNUM_HAL_INTERRUPT_UART5     SYS_IRQ_ID_UART5 
#define CYGNUM_HAL_INTERRUPT_UART6     SYS_IRQ_ID_UART6 
#define CYGNUM_HAL_INTERRUPT_UART7     SYS_IRQ_ID_UART7 

//----------------------------------------------------------------------------
// Reset.

extern void cyg_hal_bp2016_soft_reset(void);
#define HAL_PLATFORM_RESET() cyg_hal_bp2016_soft_reset()
#define HAL_PLATFORM_RESET_ENTRY 0x00000000

#endif // CYGONCE_HAL_PLATFORM_INTS_H
