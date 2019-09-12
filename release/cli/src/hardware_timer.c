//==========================================================================
//
//        hardware_timer.c
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

#include "common.h"
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

#ifdef CYGPKG_IO_COMMON_TIMER_ARM_BP2016
unsigned char my_local_stack[4096*2];
cyg_thread thread_data;
cyg_handle_t thread_handle;

static void timer_call_back_isr(void * arg)
{
    static cyg_uint32 enter_cnt = 0;
    enter_cnt++;

    if(enter_cnt == 2){
        diag_printf("isr enter, arg=0x%x !!!\n", (cyg_uint32)arg);
        enter_cnt = 0;
    }
}

static void timer_call_back_dsr(void * arg)
{
    static cyg_uint32 enter_cnt = 0;
    enter_cnt++;

    if(enter_cnt == 2){
        diag_printf("dsr enter, arg=0x%x !!!\n", (cyg_uint32)arg);
        enter_cnt = 0;
    }
}

static void timer_call_back_us_isr(void * arg)
{
    static cyg_uint32 enter_cnt_us = 0;
    enter_cnt_us++;

    if(enter_cnt_us %1000 == 0){
        diag_printf("timer_call_back_us_isr enter, arg=0x%x !!!\n", (cyg_uint32)arg);
        enter_cnt_us = 1;
    }
}

static void timer_call_back_us_dsr(void * arg)
{
    static cyg_uint32 enter_cnt_us = 0;
    enter_cnt_us++;

    if(enter_cnt_us %1000 == 0){
        diag_printf("timer_call_back_us_dsr enter, arg=0x%x !!!\n", (cyg_uint32)arg);
        enter_cnt_us = 1;
    }
}

int do_hardware_timer_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    cyg_uint32 time_out_us = 50000;
    cyg_uint32 loop = 0, id;
    cyg_uint32 arg;
    void * timer = NULL;
    callback_func timer_func_isr = timer_call_back_isr;
    callback_func timer_func_dsr = timer_call_back_dsr;
    if (argc >= 2){
        id = simple_strtoul(argv[1], NULL, 10);
        if(id > 1)
            id = 0;
        if (argc >= 3){
            time_out_us = simple_strtoul(argv[2], NULL, 10);
            if(time_out_us < 400){
                time_out_us = 400;
                timer_func_isr = timer_call_back_us_isr;
                timer_func_dsr = timer_call_back_us_dsr;
            }
        }
    }
    arg = time_out_us;
    diag_printf("hardware timer[%d] tout %d us!\n", id, time_out_us);

    timer = TimerInit(id, 100, time_out_us, timer_func_isr, timer_func_dsr, (void *)arg);
    if(NULL == timer){
        diag_printf("1 timer Init err!\n");
        return -1;
    }
    diag_printf("thread delay 2 S waitting for int!\n");
    cyg_thread_delay(200);

    while(1){
        loop++;
        printf("loop %d\n", loop);

        if(loop % 10 == 0){
            if(loop == 120) loop = 0;
            arg = time_out_us - loop * 1000;
            TimerStop(timer);
            printf("Stop and restart timer, new period is %d us\n", arg);
            timer = TimerInit(id, 100, time_out_us - loop * 1000, timer_func_isr, timer_func_dsr, (void *)arg);
            if(NULL == timer){
                diag_printf("2 timer Init err!\n");
                return -1;
            }
        }
        cyg_thread_delay(100);
    }
}
#endif
// EOF 
