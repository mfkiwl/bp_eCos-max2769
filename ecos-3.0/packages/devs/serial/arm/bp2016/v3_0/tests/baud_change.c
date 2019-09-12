//==========================================================================
//
//        baud_change.c
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
unsigned char stack[4096];
cyg_thread thread_data;
cyg_handle_t thread_handle;

// This routine will be called if the read "times out"
static void
do_abort(void *handle)
{
    cyg_io_handle_t io_handle = (cyg_io_handle_t)handle;
    cyg_int32 len = 1;  // Need something here
        diag_printf("do abort\n");
    cyg_io_get_config(io_handle, CYG_IO_GET_CONFIG_SERIAL_ABORT, 0, &len);
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

#define TEST_SER_PORT      1
static baud_arr[10] = {9600, 14400, 19200, 38400, 57600, 115200, 230400, 380400, 460800, 921600};

void
serial_test( void )
{
    int loop = 0;
    Cyg_ErrNo res;
    cyg_io_handle_t handle;
    char msg[] = "This is a test\n";
    int msglen = sizeof(msg)-1;
    char in_msg[30];
    int in_msglen = sizeof(in_msg)-1;
    cyg_serial_info_t serial_info;
    cyg_tty_info_t tty_info;
    char short_msg[] = "This is a short message\n";
    char long_msg[] = "This is a longer message 0123456789abcdefghijklmnopqrstuvwxyz\n";
    char filler[] = "          ";
    char prompt[] = "\nPlease enter some data: ";
    char read_end[] = "\r\ntest1--read data: ";
    int i, len;
#ifdef CYGFUN_KERNEL_API_C
    cyg_uint32 stamp;
#endif

    res = UartOpen(TEST_SER_PORT, &handle);
    if (res != ENOERR) {
        diag_printf("Can't lookup - DEVIO error: %d\n", res);
        return;
    }
    for(loop = 0; loop < 10; loop++){
#if 0
        len = sizeof(serial_info);
        printf("serial_info len:%d\n", len); 
        len = sizeof(serial_info.baud);
        printf("serial_info.baud len:%d\n", len); 
        len = sizeof(serial_info.stop);
        printf("serial_info.stop len:%d\n", len); 
        len = sizeof(serial_info.parity);
        printf("serial_info.parity len:%d\n", len); 
        len = sizeof(serial_info.word_length);
        printf("serial_info.word_length len:%d\n", len);
#endif
        res = UartGetConfig(TEST_SER_PORT, &serial_info);
        if (res != ENOERR) {
            diag_printf("Can't get serial config - DEVIO error: %d\n", res);
            hang();
            return;
        }
        printf("len:%d,curr Config - baud: %d, stop: %d, parity: %d\n", sizeof(serial_info), 
                serial_info.baud, serial_info.stop, serial_info.parity);
        serial_info.baud = baud_arr[loop];
        printf("ready to set baud: %d \n", baud_arr[loop]);
        //cyg_thread_delay(100);
        res = UartSetConfig(1, &serial_info);
        if (res != ENOERR) {
            diag_printf("Can't set serial config - DEVIO error: %d\n", res);
            hang();
            return;
        }
        printf("set baud: %d success, change PC tool now....\n", baud_arr[loop]);
        cyg_thread_delay(1000);
        printf("\n\n\n\n****************************baud: %d from now on....\n", baud_arr[loop]);
        cyg_thread_delay(100);
        msglen = strlen(msg);
        res = UartWrite(TEST_SER_PORT, msg, &msglen);
        if (res != ENOERR) {
            diag_printf("Can't write data - DEVIO error: %d\n", res);
            hang();
            return;
        }
        for (i = 0;  i < 2;  i++) {
            len = (i % 10) + 1;
            UartWrite(TEST_SER_PORT, filler, &len);
            len = strlen(long_msg);
            res = UartWrite(TEST_SER_PORT, long_msg, &len);
            if (res != ENOERR) {
                diag_printf("Can't write [long] data - DEVIO error: %d\n", res);
                hang();
                return;
            }
        }

#if 1
        len = strlen(prompt);
        diag_printf("Please enter some data\n");
        UartWrite(TEST_SER_PORT, prompt, &len);

#ifdef CYGFUN_KERNEL_API_C
        stamp = timeout(1000, do_abort, handle);
#endif
        res = UartRead(TEST_SER_PORT, in_msg, &in_msglen);
        if (res != ENOERR) {
            diag_printf("Can't read enough data - DEVIO timeout: %d\n", res);
            diag_printf("Read %d bytes\n", in_msglen);
            dump_buf(in_msg, in_msglen);
            len = strlen(read_end);
            UartWrite(TEST_SER_PORT, read_end, &len);
            UartWrite(TEST_SER_PORT, in_msg, &in_msglen);
            printf("not enouth data...test--Serial change baud test loop(%d) End\n", loop);
            //hang();
            //return;
        }else{
#ifdef CYGFUN_KERNEL_API_C
            untimeout(stamp);
#endif
            diag_printf("Read %d bytes\n", in_msglen);
            dump_buf(in_msg, in_msglen);
            len = strlen(read_end);
            UartWrite(TEST_SER_PORT, read_end, &len);
            UartWrite(TEST_SER_PORT, in_msg, &in_msglen);
            printf("test--Serial change baud test loop(%d) End\n", loop);
        }
#endif

    }
    CYG_TEST_PASS_FINISH("Serial baud change test OK");
}

void
cyg_start(void)
{
    CYG_TEST_INIT();
#ifdef CYGFUN_KERNEL_API_C
    cyg_thread_create(10,                   // Priority - just a number
                      (cyg_thread_entry_t*)serial_test,      // entry
                      (cyg_addrword_t) 0,   // entry data
                      "change_baud_thread",      // Name
                      &stack[0],            // Stack
                      4096,           // Size
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
