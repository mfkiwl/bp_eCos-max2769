//==========================================================================
//
//        uart_polling_test.c
//
//        Initial device I/O tests
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
#ifdef CYGFUN_KERNEL_API_C
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_arch.h>
#define STACK_SIZE CYGNUM_HAL_STACK_SIZE_TYPICAL
unsigned char stack[STACK_SIZE];
cyg_thread thread_data;
cyg_handle_t thread_handle;

// This routine will be called if the read "times out"
static polling_abort = 0;
static void
do_abort(void *handle)
{
    diag_printf("polling getc timeout!\n");
    polling_abort = 1;
}

#include "timeout.inl"
#endif

static void
dump_buf_with_offset(unsigned char *p, 
                     int s, 
                     unsigned char *base)
{
    int i, c;
    if ((unsigned int)s > (unsigned int)p) {
        s = (unsigned int)s - (unsigned int)p;
    }
    while (s > 0) {
        if (base) {
            diag_printf("%08X: ", (int)p - (int)base);
        } else {
            diag_printf("%08X: ", p);
        }
        for (i = 0;  i < 16;  i++) {
            if (i < s) {
                diag_printf("%02X", p[i] & 0xFF);
            } else {
                diag_printf("  ");
            }
            if ((i % 2) == 1) diag_printf(" ");
            if ((i % 8) == 7) diag_printf(" ");
        }
        diag_printf(" |");
        for (i = 0;  i < 16;  i++) {
            if (i < s) {
                c = p[i] & 0xFF;
                if ((c < 0x20) || (c >= 0x7F)) c = '.';
            } else {
                c = ' ';
            }
            diag_printf("%c", c);
        }
        diag_printf("|\n");
        s -= 16;
        p += 16;
    }
}

static void
dump_buf(unsigned char *p, int s)
{
   dump_buf_with_offset(p, s, 0);
}

void
hang(void)
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

void
serial_test( void )
{
    Cyg_ErrNo res;
    cyg_io_handle_t handle;
    char msg[] = "This is a test\n";
    int msglen = sizeof(msg)-1;
    char in_msg[30];
    char read_c;
    int in_msglen = sizeof(in_msg)-1;
    int in_msglen_read = 0;
    cyg_serial_info_t serial_info;
    cyg_tty_info_t tty_info;
    char short_msg[] = "This is a short message\n";
    char long_msg[] = "This is a longer message 0123456789abcdefghijklmnopqrstuvwxyz\n";
    char filler[] = "          ";
    char prompt[] = "\nPlease enter some data: ";
    int i, len;
#ifdef CYGFUN_KERNEL_API_C
    cyg_uint32 stamp;
#endif

    res = UartPollingInit(1, 115200);
    if (res != ENOERR) {
        diag_printf("Can't Init error: %d\n", res);
        return;
    }
    len = sizeof(serial_info);
    
    for (i = 0;  i < 10;  i++) {
        UartPollingPutS(1, short_msg);
    }
    for (i = 0;  i < 10;  i++) {
        UartPollingPutS(1, long_msg);
    }
    UartPollingPutS(1, prompt);
#ifdef CYGFUN_KERNEL_API_C
    stamp = timeout(1000, do_abort, handle);
#endif
    while(1){
        if(ENOERR == UartPollingGetC(1, &read_c)){
            in_msg[in_msglen_read] = read_c;
            in_msglen_read++;
        }
        if(in_msglen_read == in_msglen) break;
        if(polling_abort == 1) break;
    }
    diag_printf("Read %d bytes\n", in_msglen_read);
    dump_buf(in_msg, in_msglen_read);
#ifdef CYGFUN_KERNEL_API_C
    untimeout(stamp);
#endif
    CYG_TEST_PASS_FINISH("Uart polling test OK");
}

void
cyg_start(void)
{
    CYG_TEST_INIT();
#ifdef CYGFUN_KERNEL_API_C
    cyg_thread_create(10,                   // Priority - just a number
                      (cyg_thread_entry_t*)serial_test,      // entry
                      (cyg_addrword_t) 0,   // entry data
                      "serial_thread",      // Name
                      &stack[0],            // Stack
                      STACK_SIZE,           // Size
                      &thread_handle,       // Handle
                      &thread_data          // Thread data structure
        );
    cyg_thread_resume(thread_handle);
    cyg_scheduler_start();
#else
    serial_test();
#endif
}
// EOF serial.c
