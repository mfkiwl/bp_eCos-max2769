//==========================================================================
//
//        pll_api_test.c
//
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
// Author(s):     gthomas
// Contributors:  gthomas
// Date:          2018-10-30
// Description:   Minimal testing of "pll" I/O
//####DESCRIPTIONEND####

#include <pkgconf/kernel.h>
#include <pkgconf/io.h>
#include <cyg/io/io.h>
#include <cyg/io/ttyio.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/testcase.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/api/pll_api.h>

unsigned char my_local_stack[9096];
cyg_thread thread_data;
cyg_handle_t thread_handle;

void hang(void)
{
    while (true) ;
}

static void api_test( void )
{
    bool ret;
    int pll0_kHz = 0, pll1_kHz = 0, pll2_kHz = 0, pll3_kHz = 0;
    int i = 0;

    printf("pll api test enter!\n");
    pll_api_init();
    printf("pll api init done!\n");
    printf("stack:0x%x!\n", my_local_stack);

    pll0_kHz = pll_api_get_rate_kHz(0);
    pll1_kHz = pll_api_get_rate_kHz(1);
    pll2_kHz = pll_api_get_rate_kHz(2);
    pll3_kHz = pll_api_get_rate_kHz(3);
    printf("init value: pll0: %dkHz, pll1: %dkHz, pll2: %dkHz, \
            pll3: %dkHz\n", pll0_kHz, pll1_kHz, pll2_kHz, pll3_kHz);
#if 0
    ret = pll_api_set_rate_kHz(0, 1500000);
    if(ret != true){
        printf("Set pll0 failed!\n");
    }
#endif
    ret = pll_api_set_rate_kHz(1, 1000000);
    if(ret != true){
        printf("Set pll1 failed!\n");
    }
    ret = pll_api_set_rate_kHz(2, 1200000);
    if(ret != true){
        printf("Set pll2 failed!\n");
    }
    ret = pll_api_set_rate_kHz(3, 800000);
    if(ret != true){
        printf("Set pll3 failed!\n");
    }

    pll0_kHz = pll_api_get_rate_kHz(0);
    pll1_kHz = pll_api_get_rate_kHz(1);
    pll2_kHz = pll_api_get_rate_kHz(2);
    pll3_kHz = pll_api_get_rate_kHz(3);
    printf("after set: pll0: %dkHz, pll1: %dkHz, pll2: %dkHz, \
            pll3: %dkHz\n", pll0_kHz, pll1_kHz, pll2_kHz, pll3_kHz);
 
    hang();
}

void cyg_start(void)
{
    memset(my_local_stack, 0xff, sizeof(my_local_stack));

    CYG_TEST_INIT();
    cyg_thread_create(10,                   // Priority - just a number
                      (cyg_thread_entry_t*)api_test,      // entry
                      (cyg_addrword_t) 0,   // entry data
                      "pll_thread",      // Name
                      &my_local_stack[0],            // Stack
                      sizeof(my_local_stack),           // Size
                      &thread_handle,       // Handle
                      &thread_data          // Thread data structure
    );
    cyg_thread_resume(thread_handle);

    cyg_scheduler_start();
}
// EOF api_test.c
