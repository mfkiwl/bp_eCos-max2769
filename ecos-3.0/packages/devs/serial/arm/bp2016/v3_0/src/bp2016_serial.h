#ifndef CYGONCE_ARM_BP2016_SERIAL_H
#define CYGONCE_ARM_BP2016_SERIAL_H

// ====================================================================
//
//      bp2016_serial.h
//
//      Device I/O - Description of HG BP2016 serial hardware
//
// ====================================================================
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
// ====================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    michael anburaj <michaelanburaj@hotmail.com>
// Contributors: michael anburaj <michaelanburaj@hotmail.com>
// Date:         2003-08-01
// Purpose:      Internal interfaces for serial I/O drivers
// Description:
//
//####DESCRIPTIONEND####
//
// ====================================================================
#if 0
#include <cyg/hal/plf/chip_config.h>
// Baud rate divisor registers values
// (PCLK/16./BAUD_RATE+0.5) -1)
//#define PCLK          (95000000)
#define PCLK           (CONFIG_APB_CLOCK)
#define UBRDIV_50      (PCLK/16/50)
#define UBRDIV_75      ((PCLK/16/75)+1)
#define UBRDIV_110     (PCLK/16/110)
#define UBRDIV_134_5   ((PCLK/16/134.5)+1)
#define UBRDIV_150     (PCLK/16/150)
#define UBRDIV_200     (PCLK/16/200)
#define UBRDIV_300     ((PCLK/16/300)+1)
#define UBRDIV_600     ((PCLK/16/600)+1)
#define UBRDIV_1200    ((PCLK/16/1200)+1)
#define UBRDIV_1800    ((PCLK/16/1800)+1)
#define UBRDIV_2400    ((PCLK/16/2400)+1)
#define UBRDIV_3600    (PCLK/16/3600)
#define UBRDIV_4800    ((PCLK/16/4800)+1)
#define UBRDIV_7200    ((PCLK/16/7200)+1)
#define UBRDIV_9600    (PCLK/16/9600)
#define UBRDIV_14400   (PCLK/16/14400)
#define UBRDIV_19200   (PCLK/16/19200)
#define UBRDIV_38400   ((PCLK/16/38400)+1)
#define UBRDIV_57600   (PCLK/16/57600)
#define UBRDIV_115200  ((PCLK/16/115200)+1)
#define UBRDIV_230400  ((PCLK/16/230400)+1)
#define UBRDIV_460800  ((PCLK/16/460800)+1)
#define UBRDIV_921600  (PCLK/16/921600)
#define UBRDIV_1875000 ((PCLK/16/1875000))
#define UBRDIV_2850000 ((PCLK/16/2850000))
// Baud rate values, based on PCLK
 static unsigned short select_baud[] = {
             0,    // Unused
     UBRDIV_50,    // 50
     UBRDIV_75,    // 75
     UBRDIV_110,   // 110
     UBRDIV_134_5, // 134.5
     UBRDIV_150,   // 150
     UBRDIV_200,   // 200
     UBRDIV_300,   // 300
     UBRDIV_600,   // 600
     UBRDIV_1200,  // 1200
     UBRDIV_1800,  // 1800
     UBRDIV_2400,  // 2400
     UBRDIV_3600,  // 3600
     UBRDIV_4800,  // 4800
     UBRDIV_7200,  // 7200
     UBRDIV_9600,  // 9600
     UBRDIV_14400, // 14400
     UBRDIV_19200, // 19200
     UBRDIV_38400, // 38400
     UBRDIV_57600, // 57600
     UBRDIV_115200,// 115200
     UBRDIV_230400,// 230400
     UBRDIV_460800,// 460800
     UBRDIV_921600,// 921600
     UBRDIV_1875000,// 1875000
     UBRDIV_2850000,// 2850000
 };
#endif

#define SER_REG(_x_) ((_x_)*4)

// Receive control Registers
#define REG_rhr SER_REG(0)    // Receive holding register
#define REG_isr SER_REG(2)    // Interrupt status register
#define REG_lsr SER_REG(5)    // Line status register
#define REG_msr SER_REG(6)    // Modem status register
#define REG_scr SER_REG(7)    // Scratch register

// Transmit control Registers
#define REG_thr SER_REG(0)    // Transmit holding register
#define REG_ier SER_REG(1)    // Interrupt enable register
#define REG_fcr SER_REG(2)    // FIFO control register
#define REG_lcr SER_REG(3)    // Line control register
#define REG_mcr SER_REG(4)    // Modem control register
#define REG_ldl SER_REG(0)    // LSB of baud rate
#define REG_mdl SER_REG(1)    // MSB of baud rate
#define REG_usr SER_REG(31)   // Uart status reg

// Interrupt Enable Register
#define IER_RCV 0x01
#define IER_XMT 0x02
#define IER_LS  0x04
#define IER_MS  0x08

// Line Control Register
#define LCR_WL5 0x00    // Word length
#define LCR_WL6 0x01
#define LCR_WL7 0x02
#define LCR_WL8 0x03
#define LCR_SB1 0x00    // Number of stop bits
#define LCR_SB1_5 0x04  // 1.5 -> only valid with 5 bit words
#define LCR_SB2 0x04
#define LCR_PN  0x00    // Parity mode - none
#define LCR_PE  0x18    // Parity mode - even
#define LCR_PO  0x08    // Parity mode - odd
#define LCR_PM  0x28    // Forced "mark" parity
#define LCR_PS  0x38    // Forced "space" parity
#define LCR_DL  0x80    // Enable baud rate latch

#define LSR_RSR 0x01
#define LSR_THE 0x20
#if 0
// Line Status Register
#define LSR_RSR 0x01
#define LSR_OE  0x02
#define LSR_PE  0x04
#define LSR_FE  0x08
#define LSR_BI  0x10
#define LSR_THE 0x20
#define LSR_TEMT 0x40
#define LSR_FIE 0x80
#endif

// Modem Control Register
#define MCR_DTR  0x01
#define MCR_RTS  0x02
#define MCR_INT  0x08   // Enable interrupts
#define MCR_LOOP 0x10   // Loopback mode
#define MCR_AFCE 0x20   // Auto flow control enable
#define MCR_SIRE 0x40   // SIR mode enable

// Interrupt status Register
#define ISR_MS        0x00
#define ISR_nIP       0x01
#define ISR_Tx        0x02
#define ISR_Rx        0x04
#define ISR_LS        0x06
#define ISR_RxTO      0x0C
#define ISR_64BFIFO   0x20
#define ISR_FIFOworks 0x40
#define ISR_FIFOen    0x80

// Modem Status Register
#define MSR_DCTS 0x01
#define MSR_DDSR 0x02
#define MSR_TERI 0x04
#define MSR_DDCD 0x08
#define MSR_CTS  0x10
#define MSR_DSR  0x20
#define MSR_RI   0x40
#define MSR_CD   0x80

// FIFO Control Register
#define FCR_FE   0x01    // FIFO enable
#define FCR_CRF  0x02    // Clear receive FIFO
#define FCR_CTF  0x04    // Clear transmit FIFO
#define FCR_DMA  0x08    // DMA mode select
#define FCR_F64  0x20    // Enable 64 byte fifo (16750+)
#define FCR_RT_2_LESS_THAN_FULL  0xC0    // Set Rx trigger at 14
#define FCR_RT_HALF              0x80    // Set Rx trigger at 8
#define FCR_RT_QUARTER           0x40    // Set Rx trigger at 4
#define FCR_RT1                  0x00    // Set Rx trigger at 1
#define FCR_TT_HALF_FULL         0x30    // Set Tx Empty trigger at 1/2 full
#define FCR_TT_QUARTER_FULL      0x20    // Set Tx Empty trigger at 1/4 full
#define FCR_TT_2_IN_FIFO         0x10    // Set Tx Empty trigger at 2
#define FCR_TT_EMPTY             0x00    // Set Tx Empty trigger at 0


static unsigned char select_word_length[] = {
    LCR_WL5,    // 5 bits / word (char)
    LCR_WL6,
    LCR_WL7,
    LCR_WL8
};

static unsigned char select_stop_bits[] = {
    0,
    LCR_SB1,    // 1 stop bit
    LCR_SB1_5,  // 1.5 stop bit
    LCR_SB2     // 2 stop bits
};

static unsigned char select_parity[] = {
    LCR_PN,     // No parity
    LCR_PE,     // Even parity
    LCR_PO,     // Odd parity
    LCR_PM,     // Mark parity
    LCR_PS,     // Space parity
};

typedef enum rx_threshold_e{
    RX_THRESHOLD_E_1 = 0,
    RX_THRESHOLD_E_QUARTER,
    RX_THRESHOLD_E_HALF,
    RX_THRESHOLD_E_2_LESS_THAN_FULL,
}RX_THRESHOLD_E;

typedef enum tx_threshold_e{
    TX_THRESHOLD_E_EMPTY = 0,
    TX_THRESHOLD_E_2_IN_FIFO,
    TX_THRESHOLD_E_QUARTER_FULL,
    TX_THRESHOLD_E_HALF_FULL,
}TX_THRESHOLD_E;

typedef struct {
    unsigned char           *data;
    volatile int             put;
    volatile int             get;
    int                      len;
    volatile int             nb;          // count of bytes currently in buffer
                                          // For rx: max buffer used before flow unthrottled
    cyg_drv_cond_t           wait;
    cyg_drv_mutex_t          lock;
    volatile bool            waiting;
    volatile bool            abort;       // Set by an outsider to kill processing
    volatile unsigned int    timeout;     // tick of timeout in op(read/write)
} bp2016_cbuf_t;

#define BP2016_CBUF_INIT(_data, _len) \
   {_data, 0, 0, _len}



#endif // CYGONCE_ARM_BP2016_SERIAL_H
