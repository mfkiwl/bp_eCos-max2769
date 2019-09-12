//==========================================================================
//
//        gpio_api_test.c
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
//#include <cyg/hal/regs/gpio.h>
//#include <cyg/hal/gpio/gpio_if.h>
#include <cyg/hal/api/gpio_api.h>
unsigned char my_local_stack[4096];
cyg_thread thread_data;
cyg_handle_t thread_handle;
cyg_uint32 irq_count = 0;

void hang(void)
{
    while (true) ;
}

static int
strlen(char *c)
{
    int l = 0;
    while (*c++) l++;
    return l;
}

void dump_buf_fmt_32bit(cyg_uint32 * buf, cyg_uint32 addr, int len)                                                                                  
{
    int i = 0;
    for(i = 0; i < len; i++) {
        if(i % 4 == 0) 
            printf("\r\n%08x: ", addr + i * 4);
        printf("%08x ", *buf ++); 
    }
    printf("\r\n");
}

cyg_uint32 gpio_isr_only(cyg_uint32 vector, cyg_uint32 data)
{
    cyg_uint32 st;
    printf("isr-vector:%d\n", vector);
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);

    st = gpio_interupt_get_intstatus();
    printf("isr-st:0x%x, vector:%d, cnt:%d\n", st, vector, irq_count++);
    if (st & (1 << (vector - SYS_IRQ_ID_GPIO0))) {
        gpio_interrupt_clr(1 << (vector - SYS_IRQ_ID_GPIO0));
        printf("isr-clr gpio%d interrupt\n", (vector - SYS_IRQ_ID_GPIO0));
    }

    cyg_interrupt_unmask(vector);

    return 1;
}



cyg_uint32 gpio_isr(cyg_uint32 vector, cyg_uint32 data)
{
    //printf("isr-vector:%d\n", vector);
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);
    return CYG_ISR_HANDLED | CYG_ISR_CALL_DSR;  // Cause DSR to be run
}

void gpio_dsr(cyg_uint32 vector, cyg_uint32 cnt, cyg_uint32 data)
{
    cyg_uint32 st;

    st = gpio_interupt_get_intstatus();
    printf("dsr-st:0x%x, vector:%d, cnt:%d\n", st, vector, irq_count++);
    if (st & (1 << (vector - SYS_IRQ_ID_GPIO0))) {
        gpio_interrupt_clr(1 << (vector - SYS_IRQ_ID_GPIO0));
        printf("clr gpio%d interrupt\n", (vector - SYS_IRQ_ID_GPIO0));
    }

    cyg_interrupt_unmask(vector);
    return ;
}

static void api_test( void )
{
    cyg_uint32 loop = 0;
    diag_printf("gpio api test enter!\n");
    gpio_api_init();
    diag_printf("gpio api init done!\n");
    diag_printf("stack:0x%x!\n", my_local_stack);

    if(false == gpio_api_set_irq(1, gpio_isr, gpio_dsr, 0, GPIO_INT_HIGH_LEVEL)){
        diag_printf("gpio1 Set irq mode error!\n");
    }

    gpio_api_set_output(0);

    diag_printf("gpio setting reg display!\n");
    dump_buf_fmt_32bit((cyg_uint32 *)0xc0130000, 0xc0130000, 64);

    while(1){
#if 0
        cyg_thread_delay(50);
#else
        gpio_api_set_high(0);
        cyg_thread_delay(50);
        gpio_api_set_low(0);
        cyg_thread_delay(50);
        loop++;
        if(loop % 2) diag_printf("gpio0 output loop %d\n", loop);
#endif
    }
}

void cyg_start(void)
{
    memset(my_local_stack, 0xff, 4096);

    CYG_TEST_INIT();
    cyg_thread_create(10,                   // Priority - just a number
                      (cyg_thread_entry_t*)api_test,      // entry
                      (cyg_addrword_t) 0,   // entry data
                      "gpio_thread",      // Name
                      &my_local_stack[0],            // Stack
                      4096,           // Size
                      &thread_handle,       // Handle
                      &thread_data          // Thread data structure
        );
    cyg_thread_resume(thread_handle);

    cyg_scheduler_start();
}
// EOF api_test.c
