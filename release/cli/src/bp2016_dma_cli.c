//==========================================================================
//
//        dma_api_test.c
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
#include <cyg/hal/api/dma_api.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef CYGPKG_IO_COMMON_DMA_ARM_BP2016
unsigned char my_local_stack[4096*2];
cyg_thread thread_data;
cyg_handle_t thread_handle;
cyg_uint32 irq_count = 0;

cyg_uint32 src_addr = 0x210000;
cyg_uint32 dst_addr = 0x220000;
cyg_uint32 t_len = 0x200;

#if 0
static void
do_timeout(cyg_handle_t alarm, cyg_addrword_t data)
{
    static cyg_uint32 alarm_cnt = 0;
    alarm_cnt++;
    diag_printf("\n$$$enter alarm %d times$$$\n", alarm_cnt);
}
#endif

int do_dma_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    cyg_uint32 loop = 0, loops;
    cyg_uint32 src_addr_t = src_addr, dst_addr_t = dst_addr;
    cyg_uint32 src_addr_uart = (src_addr + 0x20000), dst_addr_uart = 0xc0010000;
    void * dma_channel = NULL;
    cyg_uint32 src_width = DMAC_CTL_TR_WIDTH_E_8, dst_width = DMAC_CTL_TR_WIDTH_E_8;
#if 0
    cyg_handle_t h;
    static cyg_handle_t timeout_alarm_handle;
    static cyg_alarm timeout_alarm;
#endif
    diag_printf("dma api test enter!\n");

    if(argc >= 2){
        loops = (cyg_uint32) atoi(argv[1]);
        if(loops > 200){
            loops = 200;
        }
    }else{
        loops = 20;
    }
#if 0
    if (1) {
        cyg_clock_to_counter(cyg_real_time_clock(), &h);
        cyg_alarm_create(h, do_timeout, 0, &timeout_alarm_handle, &timeout_alarm);
        cyg_alarm_initialize(timeout_alarm_handle, cyg_current_time() + 10, 250);
    }
#endif

    while(loops--){
        cyg_thread_delay(100);
        src_width = rand()%4;
        dst_width = rand()%4;
        if(NULL != (dma_channel = DMA_alloc(DMAC_SLAVE_ID_E_MEM))){
            printf("get valid channel %d!\n", *((cyg_uint32 *)dma_channel + 1));

            memset((char *)src_addr_t, loop+1, t_len);
            v7_dma_clean_range(src_addr_t, src_addr_t + t_len);
            v7_dma_inv_range(dst_addr_t, dst_addr_t + t_len);
            // config
            //if(true == DMA_set_config(dma_channel, 0, src_addr_t, dst_addr_t, t_len, 
            //            src_width, dst_width, 
            //            DMAC_CTL_MSIZE_E_4, DMAC_CTL_MSIZE_E_4, true)){
            if(true == DMA_m2m_set_config(dma_channel, src_addr_t, dst_addr_t, t_len, true)){
                printf("loop:%d, src:0x%x, dst:0x%x", loop, src_addr_t, dst_addr_t);
                if(0 == DMA_trans(dma_channel)){
                    if(memcmp((char *)src_addr_t, (char *)dst_addr_t, t_len) != 0){
                        printf("-------------------data error!\n");
                    }else{
                        printf("-------------------trans ok!\n");
                    }
                } else {
                    printf("---DMA_trans func return err!\n");
                }
            }else{
                printf("m2m loop:%d, set config error!,src:0x%x, dst:0x%x, len:0x%x, s_w:0x%x, d_w:0x%x\n", loop, src_addr_t, dst_addr_t, t_len, src_width, dst_width);
            }
            DMA_free(dma_channel);
        }else{
            printf("m2m no valid channel!\n");
        }
        loop++;
        src_addr_t = (src_addr + loop%0x100+src_width);
        dst_addr_t = (dst_addr + loop%0x100+dst_width);
        //if(loop % 2) printf("loop %d\n", loop);

// uart 1 dma print
        cyg_thread_delay(100);
        if(NULL != (dma_channel = DMA_alloc(DMAC_SLAVE_ID_E_UART1))){
            printf("uart send get valid channel %d!\n", *((cyg_uint32 *)dma_channel + 1));

            memset((char *)src_addr_uart, 0x30 + (loop+1)%0x40, t_len);
            *(char *)(src_addr_uart + 62) = 0xA;
            *(char *)(src_addr_uart + 63) = 0xD;
            v7_dma_clean_range(src_addr_uart, src_addr_uart + t_len);
            // config
            //if(true == DMA_set_config(dma_channel, 1, src_addr_uart, dst_addr_uart, 64, DMAC_CTL_TR_WIDTH_E_8, DMAC_CTL_TR_WIDTH_E_8, DMAC_CTL_MSIZE_E_4, DMAC_CTL_MSIZE_E_4, true)){
            if(true == DMA_m2p_set_config(dma_channel, src_addr_uart, dst_addr_uart, 64, DMAC_CTL_TR_WIDTH_E_8, DMAC_CTL_MSIZE_E_4, true)){
                DMA_trans(dma_channel);
            }else{
                printf("m2p loop:%d, set config error!,src:0x%x, dst:0x%x, len:0x%x, s_w:0x%x, d_w:0x%x\n", loop, src_addr_t, dst_addr_t, t_len, src_width, dst_width);
            }

            DMA_free(dma_channel);
        }else{
            printf("m2p no valid channel!\n");
        }
    }
    return 0;
}
#endif
// EOF api_test.c
