/*==========================================================================
//
//      fpga_misc.c
//
//      HAL misc board support for Freescale Sabre board
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2006 Free Software Foundation, Inc.
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
// Author(s):    Mike Jones
// Date:         2013-08-08
// Description:  Platform Hal for Freescale Sabre
//
//####DESCRIPTIONEND####
//
//========================================================================*/

#include <pkgconf/system.h>
#include <pkgconf/hal.h>
#include <string.h>

#include <sys/reent.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_diag.h>

#include <cyg/hal/a7/cortex_a7.h> // From SDK
#include <cyg/hal/a7/arm_cp_registers.h> // From SDK

#include <cyg/hal/hal_mmu.h>


#define errno (*__errno())

//-------------------------------------------------------------------------
//reset platfrom
void hal_reset(void)
{
    while (1);
}

// -------------------------------------------------------------------------
// Hardware init

void hal_plf_hardware_init(void) {
#ifndef CYGPKG_IO_WATCHDOG

#endif
}

//--------------------------------------------------------------------------
// EOF fpga_misc.c
