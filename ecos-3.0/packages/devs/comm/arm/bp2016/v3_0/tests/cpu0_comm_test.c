//==========================================================================
//
//        comm_test.c
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
#include <cyg/hal/api/comm_api.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <stdio.h>

#include <cyg/hal/plf/iomap.h>

unsigned char my_local_stack[4096*2];
cyg_thread thread_data;
cyg_handle_t thread_handle;
cyg_uint32 irq_count = 0;

extern void v7_dma_clean_range(U32, U32);                                                                                                
extern void v7_dma_inv_range(U32, U32);


void hang(void)
{
    while (true) ;
}

cyg_uint32 recv_addr, recv_len;
char  recv_data;

static cyg_uint32 recv_callback(cyg_uint32 addr, cyg_uint32 len)
{
    recv_addr = addr;
    recv_len = len;
    recv_data = *(char *)addr;
    printf("callback: addr:0x%x, len:0x%x, data:0x%x\n", addr, len, recv_data);
    return 0x2;
}


static void api_test( void )
{
    cyg_uint32 i = 0;
    cyg_uint32 loop = 0;
    cyg_uint32 ch = 0xFF;
    cyg_uint32 src_addr_t = CONFIG_CPU_SHARE_MEM_START;
    void * comm_channel[16];

    printf("comm test enter!\n");
    printf("stack:0x%x!\n", my_local_stack);

    for(i = 0; i < 16; i++){
        if(NULL == (comm_channel[i] = cyg_cpu_comm_alloc(i))){
            printf("no valid channel!\n");
        }else{
            cyg_cpu_comm_recv_callback_register(comm_channel[i], recv_callback);
            printf("get valid comm channel %d, addr:0x%x!\n", *((cyg_uint32 *)comm_channel[i]), (cyg_uint32)comm_channel[i]);
        }
    }

    i = 0;
    while(1){
        cyg_thread_delay(100);
        printf("channel %d!\n", i);
        // set src data
        memset((char *)src_addr_t, loop, 0x100);
        if(true == cyg_cpu_comm_send_msg(comm_channel[i], (char *)src_addr_t, 0x100, true, 100)){
            printf("send ok, recv back data!\n");

            if(true == cyg_cpu_comm_recv(comm_channel[i], 100)){
                printf("recv back data OK!\n");
            }else{
                printf("recv back data ERR!\n");
            }

        }else{
            printf("send func return error!\n");
        }
        i++;
        loop++;
        loop = loop%0x100;
        if(i == 16) i = 0;
    }
}

void cyg_start(void)
{
    memset(my_local_stack, 0xff, 4096);

    CYG_TEST_INIT();
    cyg_thread_create(10,                   // Priority - just a number
                      (cyg_thread_entry_t*)api_test,      // entry
                      (cyg_addrword_t) 0,   // entry data
                      "dma_thread",      // Name
                      &my_local_stack[0],            // Stack
                      4096*2,           // Size
                      &thread_handle,       // Handle
                      &thread_data          // Thread data structure
        );
    cyg_thread_resume(thread_handle);

    cyg_scheduler_start();
}
// EOF api_test.c
