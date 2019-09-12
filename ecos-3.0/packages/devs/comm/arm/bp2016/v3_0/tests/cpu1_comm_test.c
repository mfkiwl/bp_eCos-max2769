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

unsigned char my_local_stack[16][4096*2];
cyg_thread thread_data[16];
cyg_handle_t thread_handle[16];
cyg_uint32 irq_count = 0;

extern void v7_dma_clean_range(U32, U32);
extern void v7_dma_inv_range(U32, U32);

char  comm_recv_buf[16][0x200];

cyg_uint32 recv_addr, recv_len;
char  recv_data;

static cyg_uint32 recv_callback(cyg_uint32 addr, cyg_uint32 len)
{
    recv_addr = addr;
    recv_len = len;
    recv_data = *(char *)addr;
    printf("callback: addr:0x%x, len:0x%x, data:0x%x\n", recv_addr, recv_len, recv_data);
    return 0x2;
}

static void api_test(cyg_uint32 id)
{
    cyg_uint32 loop = 0;
    cyg_uint32 ch = 0xFF;
    void * comm_channel = NULL;

    diag_printf("comm test enter!\n");
    printf("stack:0x%x!\n", (cyg_uint32)&my_local_stack[id][0]);

    if(NULL != (comm_channel = cyg_cpu_comm_alloc(id))){
        printf("id-%d----handle addr:0x%x!\n", id, (cyg_uint32)comm_channel);

        cyg_cpu_comm_recv_callback_register(comm_channel, recv_callback);
        printf("get valid recv comm channel %d!\n", *((cyg_uint32 *)comm_channel));
        while(1){
            printf("id-%d----loop %d\n", id, loop);
            // set src data
            //memset((char *)&comm_recv_buf[id][0], 0, 0x200);
            if(true == cyg_cpu_comm_recv(comm_channel, 100000)){
                printf("id-%d----recv data, addr:0x%x, len:0x%x, data:0x%x\n", id, recv_addr, recv_len, recv_data);
                // set src data
                memset((char *)recv_addr, (recv_data + 1)%0x100, recv_len);
                if(true == cyg_cpu_comm_send_msg(comm_channel, (char *)recv_addr, recv_len, true, 100)){
                    printf("send data back ok!\n");
                }else{
                    printf("send func return error!\n");
                }
            }else{
                printf("id-%d----recv func return error!\n", id);
            }
            loop++;
        }
    }else{
        printf("id-%d----no valid channel!\n", id);
    }
}

void cyg_start(void)
{
    cyg_uint32 i;
    memset(&my_local_stack[i][0], 0xff, 4096);

    CYG_TEST_INIT();
    for(i = 0; i < 16; i++){
        cyg_thread_create(10,                   // Priority - just a number
                (cyg_thread_entry_t*)api_test,      // entry
                (cyg_addrword_t) i,   // entry data
                "dma_thread",      // Name
                &my_local_stack[i][0],            // Stack
                4096*2,           // Size
                &thread_handle[i],       // Handle
                &thread_data[i]          // Thread data structure
                );
        cyg_thread_resume(thread_handle[i]);
    }
    cyg_scheduler_start();
}
// EOF api_test.c
