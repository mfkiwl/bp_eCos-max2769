#ifndef CYGONCE_HAL_VAR_INTR_H
#define CYGONCE_HAL_VAR_INTR_H
//==========================================================================
//
//      var_intr.h
//
//      HAL Interrupt and clock assignments for BP2016 variants
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2011 Free Software Foundation, Inc.                        
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
// Author(s):     Mike Jones
// Date:          2013-08-08
// Purpose:       Define Interrupt support
// Description:   The interrupt specifics for BP2016 variants are
//                defined here.
//
// Usage:         #include <cyg/hal/var_intr.h>
//                However applications should include using <cyg/hal/hal_intr.h>
//                instead to allow for platform overrides.
//
//####DESCRIPTIONEND####
//
//==========================================================================
#include <pkgconf/hal.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_platform_ints.h>
//==========================================================================

//----------------------------------------------------------------------------
// Interrupt routines

#if defined(__cplusplus)
extern "C" {
#endif

void hal_interrupt_mask(int vector);
void hal_interrupt_unmask(int vector);
void hal_interrupt_acknowledge(int vector);
void hal_interrupt_configure(int vector, int level, int up);
void hal_interrupt_set_level(int vector, int level);
void hal_disable_interrupt(cyg_uint32 vector);
void hal_enable_interrupt(cyg_uint32 vector);
void hal_interrupt_distributor_enable(cyg_bool enable);
void hal_interrupt_cpu_enable(cyg_bool enable);
void hal_interrupt_init(void);


#if defined(__cplusplus)
}
#endif


#endif // CYGONCE_HAL_VAR_INTR_H
// EOF var_intr.h
