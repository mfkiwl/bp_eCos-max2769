//==========================================================================
//
//        uart_api_test.c
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
#if 0
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
unsigned char stack1[4096];
cyg_thread thread_data1;
cyg_handle_t thread_handle1;
#define TEST_SER_PORT    1
// This routine will be called if the read "times out"
static void
do_abort(void *handle)
{
    cyg_io_handle_t io_handle = (cyg_io_handle_t)handle;
    cyg_int32 len = 1;  // Need something here
    diag_printf("time out---abort#####\n");
    UartAbort(TEST_SER_PORT);
    //cyg_io_get_config(io_handle, CYG_IO_GET_CONFIG_SERIAL_ABORT, 0, &len);
}

#include "timeout.inl"
#endif

static void
dump_buf_with_offset(unsigned char *p, int s, unsigned char *base)
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
serial_test1( void )
{
    Cyg_ErrNo res;
    cyg_io_handle_t handle;
    char msg[] = "test1--This is a test\n";
    int msglen = sizeof(msg)-1;
    char in_msg[30];
    int in_msglen = sizeof(in_msg)-1;
    cyg_uart_info_t serial_info;
    cyg_tty_info_t tty_info;
    char short_msg[] = "test1--This is a short message\n";
    char long_msg[] = "test1--This is a longer message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz message 0123456789abcdefghijklmnopqrstuvwxyz\n";
    char filler[] = "          ";
    char prompt[] = "\ntest1--Please enter some data: ";
    char read_end[] = "\r\ntest1--read data: ";
    int i, len;
#ifdef CYGFUN_KERNEL_API_C
    cyg_uint32 stamp;
#endif

    res = UartOpen(TEST_SER_PORT, &handle);
    if (res != ENOERR) {
        diag_printf("test1--Can't lookup - DEVIO error: %d\n", res);
        return;
    }
    len = sizeof(serial_info);
    res = UartGetConfig(TEST_SER_PORT, &serial_info);
    if (res != ENOERR) {
        diag_printf("test1--Can't get serial config - DEVIO error: %d\n", res);
hang();
        return;
    }
    //len = sizeof(tty_info);
    //res = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_TTY_INFO, &tty_info, &len);
    //if (res != ENOERR) {
    //    diag_printf("Can't get tty config - DEVIO error: %d\n", res);
//hang();
//        return;
//    }
//    diag_printf("Config - baud: %d, stop: %d, parity: %d, out flags: %x, in flags: %x\n", 
//                serial_info.baud, serial_info.stop, serial_info.parity,
//                tty_info.tty_out_flags, tty_info.tty_in_flags);
//    len = sizeof(serial_info);
//    res = UartSetConfig(1, &serial_info);
//    if (res != ENOERR) {
//        diag_printf("Can't set serial config - DEVIO error: %d\n", res);
//hang();
//        return;
//    }
//    len = sizeof(tty_info);
//    res = cyg_io_set_config(handle, CYG_IO_SET_CONFIG_TTY_INFO, &tty_info, &len);
//    if (res != ENOERR) {
//        diag_printf("Can't set tty config - DEVIO error: %d\n", res);
//hang();
//        return;
//    }
    msglen = strlen(msg);
    res = UartWrite(TEST_SER_PORT, msg, &msglen);
    if (res != ENOERR) {
        diag_printf("test1--Can't write data - DEVIO error: %d\n", res);
hang();
        return;
    }
    for (i = 0;  i < 10;  i++) {
        len = strlen(short_msg);
        res = UartWrite(TEST_SER_PORT, short_msg, &len);
        if (res != ENOERR) {
            diag_printf("test1--Can't write [short] data - DEVIO error: %d\n", res);
            hang();
            return;
        }
    }
    for (i = 0;  i < 20;  i++) {
        len = (i % 10) + 1;
        UartWrite(TEST_SER_PORT, filler, &len);
        len = strlen(long_msg);
        res = UartWrite(TEST_SER_PORT, long_msg, &len);
        if (res != ENOERR) {
            diag_printf("test1--Can't write [long] data - DEVIO error: %d\n", res);
            hang();
            return;
        }
    }
    len = strlen(prompt);
    diag_printf("test1--Please enter some data\n");
    UartWrite(TEST_SER_PORT, prompt, &len);
#ifdef CYGFUN_KERNEL_API_C
    stamp = timeout(1500, do_abort, handle);
#endif
    res = UartRead(TEST_SER_PORT, in_msg, &in_msglen);
    if (res != ENOERR) {
        diag_printf("test1--Can't read enough data - DEVIO timeout: %d\n", res);
        diag_printf("test1--Read %d bytes\n", in_msglen);
        dump_buf(in_msg, in_msglen);
        len = strlen(read_end);
        UartWrite(TEST_SER_PORT, read_end, &len);
        UartWrite(TEST_SER_PORT, in_msg, &in_msglen);
        CYG_TEST_PASS_FINISH("test1--Serial I/O test End");
        //hang();
        return;
    }
#ifdef CYGFUN_KERNEL_API_C
    untimeout(stamp);
#endif
    diag_printf("test1--Read %d bytes\n", in_msglen);
    dump_buf(in_msg, in_msglen);
    len = strlen(read_end);
    UartWrite(TEST_SER_PORT, read_end, &len);
    UartWrite(TEST_SER_PORT, in_msg, &in_msglen);
    CYG_TEST_PASS_FINISH("test1--Serial I/O test OK");
}


void
serial_test( void )
{
    Cyg_ErrNo res;
    cyg_io_handle_t handle;
    char msg[] = "This is a test\n";
    int msglen = sizeof(msg)-1;
    char in_msg[30];
    int in_msglen = sizeof(in_msg)-1;
    cyg_uart_info_t serial_info;
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
    len = sizeof(serial_info);
    res = UartGetConfig(TEST_SER_PORT, &serial_info);
    if (res != ENOERR) {
        diag_printf("Can't get serial config - DEVIO error: %d\n", res);
hang();
        return;
    }
    //len = sizeof(tty_info);
    //res = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_TTY_INFO, &tty_info, &len);
    //if (res != ENOERR) {
    //    diag_printf("Can't get tty config - DEVIO error: %d\n", res);
//hang();
//        return;
//    }
//    diag_printf("Config - baud: %d, stop: %d, parity: %d, out flags: %x, in flags: %x\n", 
//                serial_info.baud, serial_info.stop, serial_info.parity,
//                tty_info.tty_out_flags, tty_info.tty_in_flags);
//    len = sizeof(serial_info);
//    res = UartSetConfig(1, &serial_info);
//    if (res != ENOERR) {
//        diag_printf("Can't set serial config - DEVIO error: %d\n", res);
//        hang();
//        return;
//    }
//    len = sizeof(tty_info);
//    res = cyg_io_set_config(handle, CYG_IO_SET_CONFIG_TTY_INFO, &tty_info, &len);
//    if (res != ENOERR) {
//        diag_printf("Can't set tty config - DEVIO error: %d\n", res);
//hang();
//        return;
//    }
    msglen = strlen(msg);
    res = UartWrite(TEST_SER_PORT, msg, &msglen);
    if (res != ENOERR) {
        diag_printf("Can't write data - DEVIO error: %d\n", res);
hang();
        return;
    }
    for (i = 0;  i < 10;  i++) {
        len = strlen(short_msg);
        res = UartWrite(TEST_SER_PORT, short_msg, &len);
        if (res != ENOERR) {
            diag_printf("Can't write [short] data - DEVIO error: %d\n", res);
            hang();
            return;
        }
    }
    for (i = 0;  i < 100;  i++) {
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
        CYG_TEST_PASS_FINISH("test1--Serial I/O test End");
        //hang();
        return;
    }
#ifdef CYGFUN_KERNEL_API_C
    untimeout(stamp);
#endif
    diag_printf("Read %d bytes\n", in_msglen);
    dump_buf(in_msg, in_msglen);
    len = strlen(read_end);
    UartWrite(TEST_SER_PORT, read_end, &len);
    UartWrite(TEST_SER_PORT, in_msg, &in_msglen);
    CYG_TEST_PASS_FINISH("Serial I/O test OK");
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
    cyg_thread_create(10,                   // Priority - just a number
                      (cyg_thread_entry_t*)serial_test1,      // entry
                      (cyg_addrword_t) 0,   // entry data
                      "serial_thread1",      // Name
                      &stack1[0],            // Stack
                      4096,           // Size
                      &thread_handle1,       // Handle
                      &thread_data1          // Thread data structure
        );
   cyg_thread_resume(thread_handle1);
   cyg_scheduler_start();
#else
    serial_test();
#endif
}
// EOF serial.c
#else



#include "common.h"
#include <pkgconf/kernel.h>
#include <pkgconf/io.h>
#include <cyg/io/io.h>
#include <cyg/io/ttyio.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/testcase.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/api/uart_api.h>

#ifdef CYGPKG_IO_SERIAL_ARM_BP2016

#define TEST_SER_PORT    1
#define TEST_SER_LEN     8192
char long_msg[TEST_SER_LEN];

#define STACK_SIZE   8192
unsigned char stack[STACK_SIZE];
cyg_thread thread_data;
cyg_handle_t thread_handle;


cyg_thread thread_data1;
cyg_handle_t thread_handle1;
unsigned char stack1[8192];

cyg_uint32  port_send, port_recv,  baud = 115200;

void serial_send( void )
{
    Cyg_ErrNo res;
    char msg[] = "This is a test\n";
    unsigned int msglen = sizeof(msg)-1;
    cyg_uart_info_t serial_info;
    char short_msg[] = "This is a short message\n";
    char my_long_msg[] = "This is a longer message 0123456789abcdefghijklmnopqrstuvwxyz\n";
#ifdef CYGFUN_KERNEL_API_C
    // cyg_uint32 stamp;
#endif

    printf("serial send thread start, send port is %d\n", port_send);

    res = UartOpen(port_send);
	if (res != ENOERR) {
		printf("test1--Can't lookup port %d- DEVIO error: %d\n", port_send, res);
		return ;
	}

    res = UartGetConfig(port_send, &serial_info);
    if (res != ENOERR) {
        printf("test1--Can't get serial config - DEVIO error: %d\n", res);
        return ;
    }

//	diag_printf("Config - baud: %d, stop: %d, parity: %d\n",
//				serial_info.baud, serial_info.stop, serial_info.parity);
	serial_info.baud = baud;
	res = UartSetConfig(port_send, &serial_info);
	if (res != ENOERR) {
		printf("Can't set serial config - DEVIO error: %d\n", res);
		return ;
	}

	while(1){
		msglen = strlen(short_msg);
	    res = UartWrite(port_send, short_msg, &msglen);
	    if (res != ENOERR) {
	        diag_printf("Can't write short data - DEVIO error: %d\n", res);
	        return;
	    }
	    cyg_thread_delay(1);
	    //printf("Delay 2s to send next message............\n");
		//cyg_thread_delay(200);

		msglen = strlen(my_long_msg);
		res = UartWrite(port_send, my_long_msg, &msglen);
		if (res != ENOERR) {
			diag_printf("Can't write long data - DEVIO error: %d\n", res);
			return;
		}
		cyg_thread_delay(1);
		printf("ready to send next message............\n");
		//cyg_thread_delay(200);
	}
}

void serial_recv( void )
{
    Cyg_ErrNo res;
    char msg[] = "This is a test\n";
    unsigned int msglen = sizeof(msg)-1;
//    int in_msglen = sizeof(in_msg)-1;
    cyg_uart_info_t serial_info;

    unsigned int recv_total = 0;
#ifdef CYGFUN_KERNEL_API_C
    // cyg_uint32 stamp;
#endif

    printf("serial recv thread start, recv port is %d\n", port_recv);

    res = UartOpen(port_recv);
	if (res != ENOERR) {
		printf("test1--Can't lookup port %d- DEVIO error: %d\n", port_recv, res);
		return ;
	}

    res = UartGetConfig(port_recv, &serial_info);
    if (res != ENOERR) {
        printf("test1--Can't get serial config - DEVIO error: %d\n", res);
        return ;
    }

//	diag_printf("Config - baud: %d, stop: %d, parity: %d\n",
//				serial_info.baud, serial_info.stop, serial_info.parity);
	serial_info.baud = baud;
	res = UartSetConfig(port_recv, &serial_info);
	if (res != ENOERR) {
		printf("Can't set recv serial config - DEVIO error: %d\n", res);
		return ;
	}

	while(1){
		msglen = 1;
	    res = UartRead(port_recv, long_msg, &msglen);
	    if (res != ENOERR) {
	        diag_printf("Can't read data - read total: %d\n", recv_total);
	        return;
	    }
	    recv_total++;
	    //if(recv_total % 16 == 0)printf("total len %d\n", recv_total);
	    printf("%c", long_msg[0]);
	}
}

int do_uart_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    Cyg_ErrNo res;
    cyg_uart_info_t serial_info;
    int i;
    unsigned int msglen;

    cyg_uint32 port_id = 1;
    cyg_uint32 w_r = 0, op_len = 0;
    cyg_uint32 timeout = 0;
    cyg_uart_buf_info_t buf_info;

    if(argc < 5){
        printf("para limited! uart [port] [baud] [op] [len]\n");
        return -1;
    }

    port_id = (cyg_uint32) atoi(argv[1]);
    if(port_id > 7){
        printf("error port: %d\n", port_id);
        return -1;
    }
    printf("input port: %d\n", port_id);

    baud = (cyg_uint32) atoi(argv[2]);

    if (strcmp(argv[3], "read") == 0) {
        w_r = 1;
    }else if(strcmp(argv[3], "write") == 0) {
        w_r = 0;
    } else if (strcmp(argv[3], "loop67") == 0) {
        w_r = 67;
        port_send = 6;
        port_recv = 7;
    } else if (strcmp(argv[3], "loop76") == 0) {
        w_r = 76;
        port_send = 7;
        port_recv = 6;
    }else if (strcmp(argv[3], "rtimeout") == 0) {
        w_r = 10;
        if(argc > 5){
            timeout = (cyg_uint32) atoi(argv[5]);
            if(0 == timeout || timeout > 10000)
                timeout = 200;
        }
        else{
            timeout = 200;
        }
        printf("read timeout test, timeout:%d ticks(10ms)\n", timeout);
    }

    op_len = (cyg_uint32) atoi(argv[4]);

    if ((w_r == 1) || (w_r == 2)) {
        res = UartOpen(port_id);
        if (res != ENOERR) {
            printf("test1--Can't lookup port %d- DEVIO error: %d\n", port_id,
                    res);
            return -1;
        }
        res = UartGetConfig(port_id, &serial_info);
        if (res != ENOERR) {
            printf("test1--Can't get serial config - DEVIO error: %d\n", res);
            return -1;
        }

        serial_info.baud = baud;
        res = UartSetConfig(port_id, &serial_info);
        if (res != ENOERR) {
            diag_printf("Can't set serial config - DEVIO error: %d\n", res);
            return -1;
        }
    }

    for(i = 0; i < TEST_SER_LEN; i++){
        long_msg[i] = i%0x30 + 0x30;
    }

    //    msglen = strlen(long_msg);
    // write
    if (0 == w_r) {
        msglen = op_len;
        res = UartWrite(port_id, long_msg, &msglen);
        if (res != ENOERR) {
            printf("test--Can't write data - DEVIO error: %d\n", res);
            return -1;
        }
        cyg_thread_delay(1);
        res = UartGetBufInfo(port_id, &buf_info);
        if (res != ENOERR) {
            printf("test--Can't write data - DEVIO error: %d\n", res);
            return -1;
        }
        printf("\r\nUart write [%d]bytes to buffer done, current buf info is-- rx_buf_size:0x%x, rx_data_count:0x%x[%d], tx_buf_size:0x%x, tx_data_to_be_send:0x%x[%d]. \n", msglen, buf_info.rx_bufsize, buf_info.rx_data_count, buf_info.rx_data_count, buf_info.tx_bufsize, buf_info.tx_data_count, buf_info.tx_data_count);
    }

    if(1 == w_r){
        msglen = op_len;
        res = UartRead(port_id, long_msg, &msglen);
        if (res != ENOERR) {
            printf("test--Can't Read data - DEVIO error: %d\n", res);
            return -1;
        }
        printf("Read %d data success!\n", msglen);
        for(i = 0; i < msglen; i++){
            if((i%16) == 0)printf("\r\n");
            printf("0x%x-[%c] ", long_msg[i], long_msg[i]);
        }
        printf("\r\nUart read data to buffer done, return. \n");
    }

    if((67 == w_r) || (76 == w_r)){
        printf("\r\n create recv thread... recv port %d \n", port_recv);
        //		(unsigned char *)stack1 = (unsigned char *)0x220000;
        cyg_thread_create(12,                   // Priority - just a number
                (cyg_thread_entry_t*)serial_recv,      // entry
                (cyg_addrword_t) 0,   // entry data
                "serial_recv",      // Name
                (unsigned char *)0x220000, //&stack1[0],            // Stack
                8192,           // Size
                &thread_handle1,       // Handle
                &thread_data1          // Thread data structure
                );
        cyg_thread_resume(thread_handle1);
        //cyg_thread_delay(100);

        //	    (unsigned char *)stack = (unsigned char *)0x230000;
        printf("\r\n create send thread... send port %d \n", port_send);
        cyg_thread_create(14,                   // Priority - just a number
                (cyg_thread_entry_t*)serial_send,      // entry
                (cyg_addrword_t) 0,   // entry data
                "serial_send",      // Name
                (unsigned char *)0x230000, //&stack[0],            // Stack
                STACK_SIZE,           // Size
                &thread_handle,       // Handle
                &thread_data          // Thread data structure
                );
        cyg_thread_resume(thread_handle);

        printf("\r\n Create thread done, return. \n");
    }

    if(10 == w_r){
        msglen = op_len;
        res = UartReadTimeOut(port_id, long_msg, &msglen, timeout);
        if (res == ENOERR) {
            printf("Read %d data success!\n", msglen);
            for(i = 0; i < msglen; i++){
                if((i%16) == 0)printf("\r\n");
                printf("0x%x-[%c] ", long_msg[i], long_msg[i]);
            }
            printf("\r\nUart read data to buffer done, return. \n");
        }else{
            if(res == -EINTR){
                printf("Read %d data timeout, read %d already!\n", op_len, msglen);
                for(i = 0; i < msglen; i++){
                    if((i%16) == 0)printf("\r\n");
                    printf("0x%x-[%c] ", long_msg[i], long_msg[i]);
                }
                printf("\r\nUart read data to buffer done, return. \n");
            }else{
                printf("test--Can't Read data - DEVIO error: %d\n", res);
            }
            return -1;
        }
    }

    return 0;
}
#endif   //#ifdef CYGPKG_IO_SERIAL_ARM_BP2016
#endif
