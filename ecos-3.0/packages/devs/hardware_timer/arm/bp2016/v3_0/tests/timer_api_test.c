//==========================================================================
//
//        timer_api_test.c
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
// Date:          1999-02-05
// Description:   Minimal testing of "serial" I/O
//####DESCRIPTIONEND####

#include <pkgconf/kernel.h>
#include <pkgconf/io.h>
#include <cyg/io/io.h>
#include <cyg/io/ttyio.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/testcase.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/api/timer_api.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <stdio.h>
unsigned char my_local_stack[4096*2];
cyg_thread thread_data;
cyg_handle_t thread_handle;
cyg_uint32 irq_count = 0;

void timer_call_back(void * arg)
{
    static enter_cnt = 0;
    enter_cnt++;

    if(enter_cnt == 2){
        diag_printf("enter, arg=0x%x !!!\n", (cyg_uint32)arg);
        enter_cnt = 0;
    }
}


static void api_test( void )
{
    cyg_uint32 loop = 0;
    cyg_uint32 arg = 0x100;
    void * timer = NULL;
    diag_printf("timer api test enter!\n");

    timer = TimerInit(TIMER_ID_2, 100, 1000000, timer_call_back, (void *)arg);
    if(NULL == timer){
        diag_printf("timer Init err!\n");
    }

    while(1){
        cyg_thread_delay(100);
        loop++;
        printf("loop %d\n", loop);

        if(loop % 10 == 0){
            TimerStop(timer);
            printf("Stop and restart timer\n");
            if(loop == 120) loop = 0;
            timer = TimerInit(TIMER_ID_2, 100, 1000000 - loop * 10000, timer_call_back, (void *)arg);
            if(NULL == timer){
                diag_printf("timer Init err!\n");
            }
        }
    }
}

void cyg_start(void)
{
    memset(my_local_stack, 0xff, 4096*2);

    CYG_TEST_INIT();
    cyg_thread_create(10,                   // Priority - just a number
                      (cyg_thread_entry_t*)api_test,      // entry
                      (cyg_addrword_t) 0,   // entry data
                      "timer_thread",      // Name
                      &my_local_stack[0],            // Stack
                      4096*2,           // Size
                      &thread_handle,       // Handle
                      &thread_data          // Thread data structure
        );
    cyg_thread_resume(thread_handle);

    cyg_scheduler_start();
}
// EOF api_test.c
