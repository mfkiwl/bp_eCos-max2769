//==========================================================================
//
//      io/serial/arm/bp2016_serial.c
//
//      HG BP2016 Serial I/O Interface Module (interrupt driven)
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
// Author(s):    michael anburaj <michaelanburaj@hotmail.com>
// Contributors: michael anburaj <michaelanburaj@hotmail.com>
// Date:         2003-08-01
// Purpose:      BP2016 Serial I/O module (interrupt driven version)
// Description: 
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/system.h>
#include <pkgconf/io_serial.h>
#include <pkgconf/io.h>
#include <cyg/io/io.h>
#include <cyg/hal/hal_intr.h>           // interrupt
#include <cyg/hal/hal_io.h>             // register base
#include <cyg/io/devtab.h>
#include <cyg/io/serial.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/regs/uart.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <string.h>
//#include <cyg/serial/bp2016_serial.h>
#include <cyg/hal/clk/clk.h>

#ifdef CYGPKG_IO_SERIAL_ARM_BP2016

#include "bp2016_serial.h"

typedef struct bp2016_uart_priv {
    CYG_ADDRWORD   base;
    CYG_WORD       int_num;
    CYG_WORD       int_pri;
    bool           auto_flow_ctl;
    unsigned int   hw_fifo_size;  // channel fifo depth
    unsigned int   rx_threshold;  // rx water level
    unsigned int   tx_threshold;  // tx water level
    cyg_interrupt  serial_interrupt;
    cyg_handle_t   serial_interrupt_handle;
    volatile unsigned int tx_fifo_avail;
} bp2016_uart_priv;

typedef struct bp2016_uart_channel {
    void               *dev_priv;  // bp2016_uart_priv
    cyg_serial_info_t   config;    // Current configuration
    bool                init;
    bp2016_cbuf_t       out_cbuf;
    bp2016_cbuf_t       in_cbuf;
}bp2016_uart_channel;


// Initialization macro for serial channel
#define BP2016_UART_CHANNEL_USING_INTERRUPTS(_l,                             \
        _dev_priv,                                       \
        _baud, _stop, _parity, _word_length, _flags,     \
        _out_buf, _out_buflen,                           \
        _in_buf, _in_buflen)                             \
bp2016_uart_channel _l = {                                                   \
    &(_dev_priv),                                                       \
    CYG_SERIAL_INFO_INIT(_baud, _stop, _parity, _word_length, _flags),  \
    false,                                                              \
    BP2016_CBUF_INIT(_out_buf, _out_buflen),                                   \
    BP2016_CBUF_INIT(_in_buf, _in_buflen)                                     \
};

static Cyg_ErrNo bp2016_serial_write(cyg_io_handle_t handle, const void *buf, cyg_uint32 *len);
static Cyg_ErrNo bp2016_serial_read(cyg_io_handle_t handle, void *buf, cyg_uint32 *len);
static cyg_bool  bp2016_serial_select(cyg_io_handle_t handle, cyg_uint32 which, CYG_ADDRWORD info);
static Cyg_ErrNo bp2016_serial_get_config(cyg_io_handle_t handle, cyg_uint32 key, void *buf, cyg_uint32 *len);
static Cyg_ErrNo bp2016_serial_set_config(cyg_io_handle_t handle, cyg_uint32 key, const void *buf, cyg_uint32 *len);
static void bp2016_serial_chan_init(bp2016_uart_channel *chan);
static void bp2016_serial_xmt_char(bp2016_uart_channel *chan);
static void bp2016_serial_rcv_char(bp2016_uart_channel *chan, unsigned char c);

DEVIO_TABLE(cyg_io_bp2016_serial_devio,
        bp2016_serial_write,
        bp2016_serial_read,
        bp2016_serial_select,
        bp2016_serial_get_config,
        bp2016_serial_set_config
        );

static bool bp2016_serial_init(struct cyg_devtab_entry *tab);
static Cyg_ErrNo bp2016_serial_lookup(struct cyg_devtab_entry **tab, 
        struct cyg_devtab_entry *sub_tab,
        const char *name);
static inline void bp2016_uart_start_xmit(bp2016_uart_priv *chan);
static inline void bp2016_uart_stop_xmit(bp2016_uart_priv *chan);

static cyg_uint32 bp2016_serial_ISR(cyg_vector_t vector, cyg_addrword_t data);
static void       bp2016_serial_DSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data);


#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL0
static bp2016_uart_priv bp2016_uart_priv0 = {(cyg_uint32)REG_AP_UART0_BASE,
    CYGNUM_HAL_INTERRUPT_UART0,
    INT_PRI_DEFAULT,
    CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL0_AUTO_FLOW_CONTROL,
    CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL0_FIFOSIZE,
    CYGPKG_IO_SERIAL_BP2016_SERIAL0_FIFO_RX_THRESHOLD,
    CYGPKG_IO_SERIAL_BP2016_SERIAL0_FIFO_TX_THRESHOLD,
};
#if CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL0_BUFSIZE > 0
static unsigned char bp2016_serial_out_buf0[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL0_BUFSIZE];
static unsigned char bp2016_serial_in_buf0[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL0_BUFSIZE];

static BP2016_UART_CHANNEL_USING_INTERRUPTS(bp2016_uart_channel0,
        bp2016_uart_priv0,
        (CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL0_BAUD),
        CYG_SERIAL_STOP_DEFAULT,
        CYG_SERIAL_PARITY_DEFAULT,
        CYG_SERIAL_WORD_LENGTH_DEFAULT,
        CYG_SERIAL_FLAGS_DEFAULT,
        &bp2016_serial_out_buf0[0], sizeof(bp2016_serial_out_buf0),
        &bp2016_serial_in_buf0[0], sizeof(bp2016_serial_in_buf0)
        );
#endif

    DEVTAB_ENTRY(bp2016_serial_io0, 
            CYGDAT_IO_SERIAL_ARM_BP2016_SERIAL0_NAME,
            0,                          // Does not depend on a lower level interface
            &cyg_io_bp2016_serial_devio, 
            bp2016_serial_init, 
            bp2016_serial_lookup,     // Serial driver may need initializing
            &bp2016_uart_channel0
            );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL0

#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL1
    static bp2016_uart_priv bp2016_uart_priv1 = {(cyg_uint32)REG_AP_UART1_BASE,
        CYGNUM_HAL_INTERRUPT_UART1,
        INT_PRI_DEFAULT,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL1_AUTO_FLOW_CONTROL,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL1_FIFOSIZE,
        CYGPKG_IO_SERIAL_BP2016_SERIAL1_FIFO_RX_THRESHOLD,
        CYGPKG_IO_SERIAL_BP2016_SERIAL1_FIFO_TX_THRESHOLD,
    };
#if CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL1_BUFSIZE > 0
static unsigned char bp2016_serial_out_buf1[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL1_BUFSIZE];
static unsigned char bp2016_serial_in_buf1[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL1_BUFSIZE];

static BP2016_UART_CHANNEL_USING_INTERRUPTS(bp2016_uart_channel1,
        bp2016_uart_priv1,
        (CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL1_BAUD),
        CYG_SERIAL_STOP_DEFAULT,
        CYG_SERIAL_PARITY_DEFAULT,
        CYG_SERIAL_WORD_LENGTH_DEFAULT,
        CYG_SERIAL_FLAGS_DEFAULT,
        &bp2016_serial_out_buf1[0], sizeof(bp2016_serial_out_buf1),
        &bp2016_serial_in_buf1[0], sizeof(bp2016_serial_in_buf1)
        );
#endif

    DEVTAB_ENTRY(bp2016_serial_io1, 
            CYGDAT_IO_SERIAL_ARM_BP2016_SERIAL1_NAME,
            0,                          // Does not depend on a lower level interface
            &cyg_io_bp2016_serial_devio, 
            bp2016_serial_init, 
            bp2016_serial_lookup,     // Serial driver may need initializing
            &bp2016_uart_channel1
            );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL1

#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL2
    static bp2016_uart_priv bp2016_uart_priv2 = {(cyg_uint32)REG_AP_UART2_BASE,
        CYGNUM_HAL_INTERRUPT_UART2,
        INT_PRI_DEFAULT,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL2_AUTO_FLOW_CONTROL,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL2_FIFOSIZE,
        CYGPKG_IO_SERIAL_BP2016_SERIAL2_FIFO_RX_THRESHOLD,
        CYGPKG_IO_SERIAL_BP2016_SERIAL2_FIFO_TX_THRESHOLD,
    };
#if CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL2_BUFSIZE > 0
static unsigned char bp2016_serial_out_buf2[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL2_BUFSIZE];
static unsigned char bp2016_serial_in_buf2[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL2_BUFSIZE];

static BP2016_UART_CHANNEL_USING_INTERRUPTS(bp2016_uart_channel2,
        bp2016_uart_priv2,
        (CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL2_BAUD),
        CYG_SERIAL_STOP_DEFAULT,
        CYG_SERIAL_PARITY_DEFAULT,
        CYG_SERIAL_WORD_LENGTH_DEFAULT,
        CYG_SERIAL_FLAGS_DEFAULT,
        &bp2016_serial_out_buf2[0], sizeof(bp2016_serial_out_buf2),
        &bp2016_serial_in_buf2[0], sizeof(bp2016_serial_in_buf2)
        );
#endif

    DEVTAB_ENTRY(bp2016_serial_io2, 
            CYGDAT_IO_SERIAL_ARM_BP2016_SERIAL2_NAME,
            0,                          // Does not depend on a lower level interface
            &cyg_io_bp2016_serial_devio, 
            bp2016_serial_init, 
            bp2016_serial_lookup,     // Serial driver may need initializing
            &bp2016_uart_channel2
            );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL2

#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL3
    static bp2016_uart_priv bp2016_uart_priv3 = {(cyg_uint32)REG_AP_UART3_BASE,
        CYGNUM_HAL_INTERRUPT_UART3,
        INT_PRI_DEFAULT,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL3_AUTO_FLOW_CONTROL,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL3_FIFOSIZE,
        CYGPKG_IO_SERIAL_BP2016_SERIAL3_FIFO_RX_THRESHOLD,
        CYGPKG_IO_SERIAL_BP2016_SERIAL3_FIFO_TX_THRESHOLD,
    };
#if CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL3_BUFSIZE > 0
static unsigned char bp2016_serial_out_buf3[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL3_BUFSIZE];
static unsigned char bp2016_serial_in_buf3[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL3_BUFSIZE];

static BP2016_UART_CHANNEL_USING_INTERRUPTS(bp2016_uart_channel3,
        bp2016_uart_priv3,
        (CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL3_BAUD),
        CYG_SERIAL_STOP_DEFAULT,
        CYG_SERIAL_PARITY_DEFAULT,
        CYG_SERIAL_WORD_LENGTH_DEFAULT,
        CYG_SERIAL_FLAGS_DEFAULT,
        &bp2016_serial_out_buf3[0], sizeof(bp2016_serial_out_buf3),
        &bp2016_serial_in_buf3[0], sizeof(bp2016_serial_in_buf3)
        );
#endif

    DEVTAB_ENTRY(bp2016_serial_io3, 
            CYGDAT_IO_SERIAL_ARM_BP2016_SERIAL3_NAME,
            0,                          // Does not depend on a lower level interface
            &cyg_io_bp2016_serial_devio, 
            bp2016_serial_init, 
            bp2016_serial_lookup,     // Serial driver may need initializing
            &bp2016_uart_channel3
            );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL3

#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL4
    static bp2016_uart_priv bp2016_uart_priv4 = {(cyg_uint32)REG_AP_UART4_BASE,
        CYGNUM_HAL_INTERRUPT_UART4,
        INT_PRI_DEFAULT,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL4_AUTO_FLOW_CONTROL,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL4_FIFOSIZE,
        CYGPKG_IO_SERIAL_BP2016_SERIAL4_FIFO_RX_THRESHOLD,
        CYGPKG_IO_SERIAL_BP2016_SERIAL4_FIFO_TX_THRESHOLD,
    };
#if CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL4_BUFSIZE > 0
static unsigned char bp2016_serial_out_buf4[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL4_BUFSIZE];
static unsigned char bp2016_serial_in_buf4[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL4_BUFSIZE];

static BP2016_UART_CHANNEL_USING_INTERRUPTS(bp2016_uart_channel4,
        bp2016_uart_priv4,
        (CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL4_BAUD),
        CYG_SERIAL_STOP_DEFAULT,
        CYG_SERIAL_PARITY_DEFAULT,
        CYG_SERIAL_WORD_LENGTH_DEFAULT,
        CYG_SERIAL_FLAGS_DEFAULT,
        &bp2016_serial_out_buf4[0], sizeof(bp2016_serial_out_buf4),
        &bp2016_serial_in_buf4[0], sizeof(bp2016_serial_in_buf4)
        );
#endif

    DEVTAB_ENTRY(bp2016_serial_io4, 
            CYGDAT_IO_SERIAL_ARM_BP2016_SERIAL4_NAME,
            0,                          // Does not depend on a lower level interface
            &cyg_io_bp2016_serial_devio, 
            bp2016_serial_init, 
            bp2016_serial_lookup,     // Serial driver may need initializing
            &bp2016_uart_channel4
            );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL4

#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL5
    static bp2016_uart_priv bp2016_uart_priv5 = {(cyg_uint32)REG_AP_UART5_BASE,
        CYGNUM_HAL_INTERRUPT_UART5,
        INT_PRI_DEFAULT,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL5_AUTO_FLOW_CONTROL,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL5_FIFOSIZE,
        CYGPKG_IO_SERIAL_BP2016_SERIAL5_FIFO_RX_THRESHOLD,
        CYGPKG_IO_SERIAL_BP2016_SERIAL5_FIFO_TX_THRESHOLD,
    };
#if CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL5_BUFSIZE > 0
static unsigned char bp2016_serial_out_buf5[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL5_BUFSIZE];
static unsigned char bp2016_serial_in_buf5[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL5_BUFSIZE];

static BP2016_UART_CHANNEL_USING_INTERRUPTS(bp2016_uart_channel5,
        bp2016_uart_priv5,
        (CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL5_BAUD),
        CYG_SERIAL_STOP_DEFAULT,
        CYG_SERIAL_PARITY_DEFAULT,
        CYG_SERIAL_WORD_LENGTH_DEFAULT,
        CYG_SERIAL_FLAGS_DEFAULT,
        &bp2016_serial_out_buf5[0], sizeof(bp2016_serial_out_buf5),
        &bp2016_serial_in_buf5[0], sizeof(bp2016_serial_in_buf5)
        );
#endif

    DEVTAB_ENTRY(bp2016_serial_io5, 
            CYGDAT_IO_SERIAL_ARM_BP2016_SERIAL5_NAME,
            0,                          // Does not depend on a lower level interface
            &cyg_io_bp2016_serial_devio, 
            bp2016_serial_init, 
            bp2016_serial_lookup,     // Serial driver may need initializing
            &bp2016_uart_channel5
            );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL5

#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL6
    static bp2016_uart_priv bp2016_uart_priv6 = {(cyg_uint32)REG_AP_UART6_BASE,
        CYGNUM_HAL_INTERRUPT_UART6,
        INT_PRI_DEFAULT,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL6_AUTO_FLOW_CONTROL,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL6_FIFOSIZE,
        CYGPKG_IO_SERIAL_BP2016_SERIAL6_FIFO_RX_THRESHOLD,
        CYGPKG_IO_SERIAL_BP2016_SERIAL6_FIFO_TX_THRESHOLD,
    };
#if CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL6_BUFSIZE > 0
static unsigned char bp2016_serial_out_buf6[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL6_BUFSIZE];
static unsigned char bp2016_serial_in_buf6[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL6_BUFSIZE];

static BP2016_UART_CHANNEL_USING_INTERRUPTS(bp2016_uart_channel6,
        bp2016_uart_priv6,
        (CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL6_BAUD),
        CYG_SERIAL_STOP_DEFAULT,
        CYG_SERIAL_PARITY_DEFAULT,
        CYG_SERIAL_WORD_LENGTH_DEFAULT,
        CYG_SERIAL_FLAGS_DEFAULT,
        &bp2016_serial_out_buf6[0], sizeof(bp2016_serial_out_buf6),
        &bp2016_serial_in_buf6[0], sizeof(bp2016_serial_in_buf6)
        );
#endif

    DEVTAB_ENTRY(bp2016_serial_io6, 
            CYGDAT_IO_SERIAL_ARM_BP2016_SERIAL6_NAME,
            0,                          // Does not depend on a lower level interface
            &cyg_io_bp2016_serial_devio, 
            bp2016_serial_init, 
            bp2016_serial_lookup,     // Serial driver may need initializing
            &bp2016_uart_channel6
            );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL6

#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL7
    static bp2016_uart_priv bp2016_uart_priv7 = {(cyg_uint32)REG_AP_UART7_BASE,
        CYGNUM_HAL_INTERRUPT_UART7,
        INT_PRI_DEFAULT,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL7_AUTO_FLOW_CONTROL,
        CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL7_FIFOSIZE,
        CYGPKG_IO_SERIAL_BP2016_SERIAL7_FIFO_RX_THRESHOLD,
        CYGPKG_IO_SERIAL_BP2016_SERIAL7_FIFO_TX_THRESHOLD,
    };
#if CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL7_BUFSIZE > 0
static unsigned char bp2016_serial_out_buf7[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL7_BUFSIZE];
static unsigned char bp2016_serial_in_buf7[CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL7_BUFSIZE];

static BP2016_UART_CHANNEL_USING_INTERRUPTS(bp2016_uart_channel7,
        bp2016_uart_priv7,
        (CYGNUM_IO_SERIAL_ARM_BP2016_SERIAL7_BAUD),
        CYG_SERIAL_STOP_DEFAULT,
        CYG_SERIAL_PARITY_DEFAULT,
        CYG_SERIAL_WORD_LENGTH_DEFAULT,
        CYG_SERIAL_FLAGS_DEFAULT,
        &bp2016_serial_out_buf7[0], sizeof(bp2016_serial_out_buf7),
        &bp2016_serial_in_buf7[0], sizeof(bp2016_serial_in_buf7)
        );
#endif

    DEVTAB_ENTRY(bp2016_serial_io7, 
            CYGDAT_IO_SERIAL_ARM_BP2016_SERIAL7_NAME,
            0,                          // Does not depend on a lower level interface
            &cyg_io_bp2016_serial_devio, 
            bp2016_serial_init, 
            bp2016_serial_lookup,     // Serial driver may need initializing
            &bp2016_uart_channel7
            );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_SERIAL7

static unsigned int bp2016_calc_divisor(U32 clk, U32 baud_rate)
{
    unsigned int baud_divisor;

    baud_divisor = clk / 16 / baud_rate;

    if (clk > (baud_divisor * 16 * baud_rate)) {
        baud_divisor +=
            ((clk / 16 - baud_divisor * baud_rate) * 2 > baud_rate) ? 1 : 0;
    }

    return baud_divisor;
}

static cyg_uint32 uart_input_clk_setting(cyg_uint32 baud, bp2016_uart_priv *priv)
{
    #define  HIGH_BAUD_921600_APB_CLK     (162857142)
    #define  HIGH_BAUD_1875000_APB_CLK    (30000000)
    #define  HIGH_BAUD_2850000_APB_CLK    (45600000)
    cyg_uint32 apb_clk = CONFIG_APB_CLOCK;
    CLK_ID_TYPE_T  uart_id = CLK_CPU0;

    switch(priv->base){
    case REG_AP_UART6_BASE:
        uart_id = CLK_UART6;
        break;
    case REG_AP_UART7_BASE:
        uart_id = CLK_UART7;
        break;
    default:
        break;
    }

    if(CLK_CPU0 != uart_id){
        switch(baud){
        case 921600:
            hal_clk_set_rate_kHz(uart_id, (HIGH_BAUD_921600_APB_CLK/1000));
            apb_clk = HIGH_BAUD_921600_APB_CLK;
            break;
        case 1875000:
            hal_clk_set_rate_kHz(uart_id, (HIGH_BAUD_1875000_APB_CLK/1000));
            apb_clk = HIGH_BAUD_1875000_APB_CLK;
            break;
        case 2850000:
            hal_clk_set_rate_kHz(uart_id, (HIGH_BAUD_2850000_APB_CLK/1000));
            apb_clk = HIGH_BAUD_2850000_APB_CLK;
            break;
        default:
            // other baudrate, set input_clk to 95000000
            hal_clk_set_rate_kHz(uart_id, (CONFIG_APB_CLOCK/1000));
            break;
        }
    }

    return apb_clk;
}

    // Internal function to actually configure the hardware to desired baud rate, etc.
static bool
bp2016_uart_config_port(bp2016_uart_channel *chan, cyg_serial_info_t *new_config, bool init)
{
    bp2016_uart_priv *bp2016_priv = (bp2016_uart_priv *)chan->dev_priv;
    CYG_ADDRWORD base = bp2016_priv->base;
    //unsigned short baud_divisor = select_baud[new_config->baud];
    unsigned short baud_divisor = 0;
    cyg_uint32 _lcr, count = 0;
    cyg_uint8  _ier, _mcr = 0, _usr,  _fcr_thresh = 0;
    cyg_uint32 apb_clk;

    // diag_printf("base:0x%x set baud:%d\n", base, new_config->baud);
    if ((new_config->baud < 9600) || (new_config->baud > 2850000)) return false;

    apb_clk = uart_input_clk_setting(new_config->baud, bp2016_priv);

    baud_divisor = (unsigned short)bp2016_calc_divisor(apb_clk, new_config->baud);

    if (baud_divisor == 0) return false;
    // Disable port interrupts while changing hardware
    HAL_READ_UINT32(base+REG_ier, _ier);
    HAL_WRITE_UINT32(base+REG_ier, 0);

    // enable and reset FIFO -- 20171208, fix bug, reset fifo here
    // disable reset fifo func, set config will change the threshold setting if open this codes.
    //HAL_WRITE_UINT32(base+REG_fcr, FCR_FE | FCR_CRF | FCR_CTF); 
    hal_delay_us(1000);

    // check busy status--usr[0]
    HAL_READ_UINT32(base+REG_usr, _usr);
    while(((_usr & 0x1) != 0) && (count < 1000))
    {
        count++;
        hal_delay_us(1000);
        HAL_READ_UINT32(base+REG_usr, _usr);
    }
    if(1000 == count)
    {
        diag_printf("uart line busy!\n");
        HAL_WRITE_UINT32(base+REG_ier, _ier);
        return false;
    }

    _lcr = select_word_length[new_config->word_length - CYGNUM_SERIAL_WORD_LENGTH_5] | 
        select_stop_bits[new_config->stop] |
        select_parity[new_config->parity];
    HAL_WRITE_UINT32(base+REG_lcr, _lcr | LCR_DL);
    HAL_WRITE_UINT32(base+REG_mdl, baud_divisor >> 8);
    HAL_WRITE_UINT32(base+REG_ldl, baud_divisor & 0xFF);
    HAL_WRITE_UINT32(base+REG_lcr, _lcr);

    if (init) {
        switch(bp2016_priv->rx_threshold) {
            default:
            case RX_THRESHOLD_E_1:  // one character in fifo
                _fcr_thresh = FCR_RT1;
                break;
            case RX_THRESHOLD_E_QUARTER:  // 1/4 fifo full
                _fcr_thresh = FCR_RT_QUARTER;
                break;
            case RX_THRESHOLD_E_HALF:  // 1/2 fifo full
                _fcr_thresh = FCR_RT_HALF;
                break;
            case RX_THRESHOLD_E_2_LESS_THAN_FULL:  // 2 characters space to be full
                _fcr_thresh = FCR_RT_2_LESS_THAN_FULL;
                break;
        }

        switch(bp2016_priv->tx_threshold) {
            default:
            case TX_THRESHOLD_E_EMPTY:  // acturally empty
                _fcr_thresh |= FCR_TT_EMPTY;
                bp2016_priv->tx_fifo_avail = bp2016_priv->hw_fifo_size;
                break;
            case TX_THRESHOLD_E_2_IN_FIFO:  // 2 characters in tx fifo
                _fcr_thresh |= FCR_TT_2_IN_FIFO;
                bp2016_priv->tx_fifo_avail = bp2016_priv->hw_fifo_size - 2;
                break;
            case TX_THRESHOLD_E_QUARTER_FULL:  // 1/4 fifo depth
                _fcr_thresh |= FCR_TT_QUARTER_FULL;
                bp2016_priv->tx_fifo_avail = bp2016_priv->hw_fifo_size / 4;
                break;
            case TX_THRESHOLD_E_HALF_FULL:  // 1/2 fifo depth
                _fcr_thresh |= FCR_TT_HALF_FULL;
                bp2016_priv->tx_fifo_avail = bp2016_priv->hw_fifo_size / 2;
                break;
        }
      //  _fcr_thresh|=FCR_FE|FCR_CRF|FCR_CTF;
        _fcr_thresh|=FCR_FE;
        // Enable and clear FIFO
        HAL_WRITE_UINT32(base+REG_fcr, _fcr_thresh); 

        if(true == bp2016_priv->auto_flow_ctl) {
            _mcr = MCR_AFCE | MCR_DTR | MCR_RTS;
        }
        //UART modem control register
        HAL_WRITE_UINT32(base+REG_mcr, _mcr);

        // diag_printf("base=0x%x, tx_fifo_avail=%d\n", base,  bp2016_priv->tx_fifo_avail);
        _ier = IER_RCV;
    }
    // reset FIFO
    //HAL_WRITE_UINT32(base+REG_fcr, FCR_FE | FCR_CRF | FCR_CTF); 

    HAL_WRITE_UINT32(base+REG_ier, _ier);

    if (new_config != &chan->config) {
        memcpy(&chan->config, new_config, sizeof(cyg_serial_info_t));
    }
    return true;
}

static void bp2016_chan_init(bp2016_uart_channel *chan)
{
    bp2016_uart_priv *bp2016_priv;
    if (chan->init) return; // add for multi init--API UartOpen in different threads.
    bp2016_priv = (bp2016_uart_priv *)chan->dev_priv;
    bp2016_serial_chan_init(chan);  // init buff and mutex, conditions
    cyg_drv_interrupt_create(bp2016_priv->int_num,
            bp2016_priv->int_pri,                    // Priority - unused
            (cyg_addrword_t)chan, //  Data item passed to interrupt handler
            bp2016_serial_ISR,
            bp2016_serial_DSR,
            &bp2016_priv->serial_interrupt_handle,
            &bp2016_priv->serial_interrupt);
    cyg_drv_interrupt_attach(bp2016_priv->serial_interrupt_handle);
    bp2016_uart_config_port(chan, &chan->config, true);
    cyg_drv_interrupt_unmask(bp2016_priv->int_num);
}

// Function to initialize the device.  Called at bootstrap time.
static bool 
bp2016_serial_init(struct cyg_devtab_entry *tab)
{
#if 0
    bp2016_uart_channel *chan = (bp2016_uart_channel *)tab->priv;
    bp2016_chan_init(chan);
    return true;
#endif
    return true;
}

// This routine is called when the device is "looked" up (i.e. attached)
static Cyg_ErrNo 
bp2016_serial_lookup(struct cyg_devtab_entry **tab, 
        struct cyg_devtab_entry *sub_tab,
        const char *name)
{
#if 0
    bp2016_uart_channel *chan = (bp2016_uart_channel *)(*tab)->priv;
    bp2016_serial_chan_init(chan);  // init buff and mutex, conditions
    return ENOERR;
#endif
    bp2016_uart_channel *chan = (bp2016_uart_channel *)(*tab)->priv;
    bp2016_chan_init(chan);
    return ENOERR;
}

// Serial I/O - low level interrupt handler (ISR)
static cyg_uint32 
bp2016_serial_ISR(cyg_vector_t vector, cyg_addrword_t data)
{
    bp2016_uart_channel *chan = (bp2016_uart_channel *)data;
    bp2016_uart_priv *bp2016_priv = (bp2016_uart_priv *)chan->dev_priv;
    cyg_drv_interrupt_mask(bp2016_priv->int_num);
    cyg_drv_interrupt_acknowledge(bp2016_priv->int_num);
    return CYG_ISR_CALL_DSR;  // Cause DSR to be run
}

// Serial I/O - high level interrupt handler (DSR)
static void       
bp2016_serial_DSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    bp2016_uart_channel *chan = (bp2016_uart_channel *)data;
    bp2016_uart_priv *bp2016_priv = (bp2016_uart_priv *)chan->dev_priv;
    CYG_ADDRWORD base = bp2016_priv->base;
    unsigned char c;
    cyg_uint8 _isr, _lsr;

    HAL_READ_UINT32(base+REG_isr, _isr);

    // add busy int process first
    if((_isr & 0xF) == 0x7){
        HAL_READ_UINT32(base+REG_lsr, _lsr);
        diag_printf("int %d busy int,isr:0x%x, lsr:0x%x\n", vector, _isr, _lsr);
        return;
    }
    while ((_isr & ISR_nIP) == 0) {
        switch (_isr&0xE) {
            case ISR_Rx:
            case ISR_RxTO:
                {
                    HAL_READ_UINT32(base+REG_lsr, _lsr);
                    while(_lsr & LSR_RSR) {
                        HAL_READ_UINT32(base+REG_rhr, c);
                        bp2016_serial_rcv_char(chan, c);
                        HAL_READ_UINT32(base+REG_lsr, _lsr);
                    }
                    break;
                }
            case ISR_Tx:
                bp2016_serial_xmt_char(chan);
                break;

            default:
                // Yes, this assertion may well not be visible. *But*
                // if debugging, we may still successfully hit a breakpoint
                // on cyg_assert_fail, which _is_ useful
                diag_printf("unhandled serial interrupt state\n");
        }

        HAL_READ_UINT32(base+REG_isr, _isr);
    } // while

    cyg_drv_interrupt_unmask(bp2016_priv->int_num);
}

static inline void bp2016_cbuf_init(bp2016_cbuf_t *cbuf)
{
    cbuf->waiting = false;
    cbuf->abort = false;
    cyg_drv_mutex_init(&cbuf->lock);
    cyg_drv_cond_init(&cbuf->wait, &cbuf->lock);
}

    static void
bp2016_serial_chan_init(bp2016_uart_channel *chan)
{
    if (chan->init) return;

    bp2016_cbuf_init(&chan->out_cbuf);
    bp2016_cbuf_init(&chan->in_cbuf);
    chan->init = true;
}


static inline void bp2016_uart_stop_xmit(bp2016_uart_priv *chan)
{
    cyg_addrword_t base = chan->base;
    cyg_uint8 _ier;

    HAL_READ_UINT32(base+REG_ier, _ier);
    _ier &= ~IER_XMT;                   // Disable xmit interrupt
    HAL_WRITE_UINT32(base+REG_ier, _ier);
}

static inline void bp2016_uart_start_xmit(bp2016_uart_priv *chan)
{
    cyg_addrword_t base = chan->base;
    cyg_uint8 _ier;

    HAL_READ_UINT32(base+REG_ier, _ier);
    _ier |= IER_XMT;                    // Enable xmit interrupt
    HAL_WRITE_UINT32(base+REG_ier, _ier);
}

static inline void
bp2016_serial_rcv_char(bp2016_uart_channel *chan, unsigned char c)
{
    bp2016_cbuf_t *cbuf = &chan->in_cbuf;
    // If the flow control is not enabled/sufficient and the buffer is
    // already full, just throw new characters away.
    if ( cbuf->nb < cbuf->len ) {
        cbuf->data[cbuf->put++] = c;
        if (cbuf->put == cbuf->len){
            cbuf->put = 0;
        }
        cbuf->nb++;
    } // note trailing else

    if (cbuf->waiting) {
        cbuf->waiting = false;
        cyg_drv_cond_broadcast(&cbuf->wait);
    }
}

static inline void
bp2016_serial_xmt_char(bp2016_uart_channel *chan)
{
    bp2016_cbuf_t *cbuf = &chan->out_cbuf;
    bp2016_uart_priv *bp2016_priv = (bp2016_uart_priv *)chan->dev_priv;
    CYG_ADDRWORD base = bp2016_priv->base;
    unsigned char c;
    int space, i;

    space = (bp2016_priv->tx_fifo_avail >= cbuf->nb) ? cbuf->nb : bp2016_priv->tx_fifo_avail;
    for(i = 0; i < space; i++){
        c = cbuf->data[cbuf->get];
        HAL_WRITE_UINT32(base+REG_thr, c);  // write data to fifo
        cbuf->get++;
        if (cbuf->get == cbuf->len) cbuf->get = 0;
    }
    cbuf->nb -= space;  // update number of bytes in buffer

    if(cbuf->nb == 0){
        // no data to send , disable tx empty int
        bp2016_uart_stop_xmit(bp2016_priv);
    }
    // signal waiters
    if (cbuf->waiting) {
        cbuf->waiting = false;
        cyg_drv_cond_broadcast(&cbuf->wait);
    }
}

#if 0
static Cyg_ErrNo 
bp2016_serial_write(cyg_io_handle_t handle, const void *_buf, cyg_uint32 *len)
{
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *)handle;
    bp2016_uart_channel *chan = (bp2016_uart_channel *)t->priv;
    bp2016_uart_priv *bp2016_priv = (bp2016_uart_priv *)chan->dev_priv;
    cyg_int32 left_size = *len, space = 0, copy_size, i;
    cyg_uint8 *buf = (cyg_uint8 *)_buf;
    bp2016_cbuf_t *cbuf = &chan->out_cbuf;
    Cyg_ErrNo res = ENOERR;

    if(NULL == buf || NULL == len) return -EINVAL;
    cyg_drv_mutex_lock(&cbuf->lock);
    cyg_drv_dsr_lock();  // Avoid race condition testing pointers
    cbuf->abort = false;

    while (left_size > 0) {       
        space = cbuf->len - cbuf->nb;
        copy_size = (space >= left_size) ? left_size : space;
        for(i = 0; i < copy_size; i++){
            cbuf->data[cbuf->put] = *buf++;
            cbuf->put++;
            if (cbuf->put == cbuf->len){
                cbuf->put = 0;
            }
        }
        left_size -= copy_size;  // calculate remain size to send!
        cbuf->nb += copy_size;
        // start xmit, open tx empty int
        bp2016_uart_start_xmit(bp2016_priv);

        // Buffer full - wait for space
        if ((cbuf->nb == cbuf->len) && (left_size > 0)) {
            cbuf->waiting = true;
            if( !cyg_drv_cond_wait(&cbuf->wait) ){
                cbuf->abort = true;
            }
            if (cbuf->abort) {
                // Give up!
                *len -= left_size;   // number of characters actually sent
                cbuf->abort = false;
                cbuf->waiting = false;
                res = -EINTR;
                break;
            }
        }
    }

    cyg_drv_dsr_unlock();
    cyg_drv_mutex_unlock(&cbuf->lock);
    return res;
}

static Cyg_ErrNo 
bp2016_serial_read(cyg_io_handle_t handle, void *_buf, cyg_uint32 *len)
{
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *)handle;
    bp2016_uart_channel *chan = (bp2016_uart_channel *)t->priv;
    cyg_uint8 *buf = (cyg_uint8 *)_buf;
    cyg_int32 copy_num = 0, left_num = *len, i;
    bp2016_cbuf_t *cbuf = &chan->in_cbuf;
    Cyg_ErrNo res = ENOERR;

    if(NULL == buf || NULL == len) return -EINVAL;
    cyg_drv_mutex_lock(&cbuf->lock);
    cyg_drv_dsr_lock();  // Avoid races

    cbuf->abort = false;
    while (left_num) {   
        copy_num = (cbuf->nb >= left_num) ? left_num : cbuf->nb;  // get rx data num first
        for(i = 0; i < copy_num; i++){
            *buf++ = cbuf->data[cbuf->get];
            cbuf->get++;
            if (cbuf->get == cbuf->len){
                cbuf->get = 0;
            }
        }
        left_num -= copy_num;  // update left number of bytes
        cbuf->nb -= copy_num;

        if((0 < left_num) && (0 == cbuf->nb)){
            cbuf->waiting = true;
            if( !cyg_drv_cond_wait(&cbuf->wait) )
                cbuf->abort = true;
            if (cbuf->abort) {
                // Give up!
                *len -= left_num;        // characters actually read
                cbuf->abort = false;
                cbuf->waiting = false;
                res = -EINTR;
                break;
            }
        }
    }
    cyg_drv_dsr_unlock();
    cyg_drv_mutex_unlock(&cbuf->lock);
    return res;
}
#else
static Cyg_ErrNo 
bp2016_serial_write(cyg_io_handle_t handle, const void *_buf, cyg_uint32 *len)
{
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *)handle;
    bp2016_uart_channel *chan = (bp2016_uart_channel *)t->priv;
    bp2016_uart_priv *bp2016_priv = (bp2016_uart_priv *)chan->dev_priv;
    cyg_int32 left_size = *len, space = 0, copy_size, i;
    cyg_uint8 *buf = (cyg_uint8 *)_buf;
    bp2016_cbuf_t *cbuf = &chan->out_cbuf;
    Cyg_ErrNo res = ENOERR;

    if(NULL == buf || NULL == len) return -EINVAL;
    cyg_drv_mutex_lock(&cbuf->lock);
    cyg_drv_dsr_lock();  // Avoid race condition testing pointers
    cbuf->abort = false;

    while (left_size > 0) {       
        space = cbuf->len - cbuf->nb;
        copy_size = (space >= left_size) ? left_size : space;
        for(i = 0; i < copy_size; i++){
            cbuf->data[cbuf->put] = *buf++;
            cbuf->put++;
            if (cbuf->put == cbuf->len){
                cbuf->put = 0;
            }
        }
        left_size -= copy_size;  // calculate remain size to send!
        cbuf->nb += copy_size;
        // start xmit, open tx empty int
        bp2016_uart_start_xmit(bp2016_priv);

        // Buffer full - wait for space
        if ((cbuf->nb == cbuf->len) && (left_size > 0)) {
            cbuf->waiting = true;
            if( !cyg_cond_timed_wait(&cbuf->wait, cyg_current_time() + cbuf->timeout) ){
                cbuf->abort = true;
            }
            if (cbuf->abort) {
                // Give up!
                *len -= left_size;   // number of characters actually sent
                cbuf->abort = false;
                cbuf->waiting = false;
                res = -EINTR;
                break;
            }
        }
    }

    cyg_drv_dsr_unlock();
    cyg_drv_mutex_unlock(&cbuf->lock);
    return res;
}

static Cyg_ErrNo 
bp2016_serial_read(cyg_io_handle_t handle, void *_buf, cyg_uint32 *len)
{
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *)handle;
    bp2016_uart_channel *chan = (bp2016_uart_channel *)t->priv;
    cyg_uint8 *buf = (cyg_uint8 *)_buf;
    cyg_int32 copy_num = 0, left_num = *len, i;
    bp2016_cbuf_t *cbuf = &chan->in_cbuf;
    Cyg_ErrNo res = ENOERR;

    if(NULL == buf || NULL == len) return -EINVAL;
    cyg_drv_mutex_lock(&cbuf->lock);
    cyg_drv_dsr_lock();  // Avoid races

    cbuf->abort = false;
    while (left_num) {   
        copy_num = (cbuf->nb >= left_num) ? left_num : cbuf->nb;  // get rx data num first
        for(i = 0; i < copy_num; i++){
            *buf++ = cbuf->data[cbuf->get];
            cbuf->get++;
            if (cbuf->get == cbuf->len){
                cbuf->get = 0;
            }
        }
        left_num -= copy_num;  // update left number of bytes
        cbuf->nb -= copy_num;

        if((0 < left_num) && (0 == cbuf->nb)){
            cbuf->waiting = true;
            if( !cyg_cond_timed_wait(&cbuf->wait, cyg_current_time() + cbuf->timeout) )
                cbuf->abort = true;
            if (cbuf->abort) {
                // Give up!
                *len -= left_num;        // characters actually read
                cbuf->abort = false;
                cbuf->waiting = false;
                res = -EINTR;
                break;
            }
        }
    }
    cyg_drv_dsr_unlock();
    cyg_drv_mutex_unlock(&cbuf->lock);
    return res;
}
#endif

static cyg_bool
bp2016_serial_select(cyg_io_handle_t handle, cyg_uint32 which, CYG_ADDRWORD info)
{
    return true;
}

static Cyg_ErrNo 
bp2016_serial_get_config(cyg_io_handle_t handle, cyg_uint32 key, void *xbuf,
        cyg_uint32 *len)
{
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *)handle;
    bp2016_uart_channel *chan = (bp2016_uart_channel *)t->priv;
    Cyg_ErrNo res = ENOERR;
    bp2016_cbuf_t *out_cbuf = &chan->out_cbuf;
    bp2016_cbuf_t *in_cbuf = &chan->in_cbuf;

    if(NULL == len) return -EINVAL;
    switch (key) {
        case CYG_IO_GET_CONFIG_SERIAL_INFO:
            if (*len < sizeof(cyg_serial_info_t)) {
                return -EINVAL;
            }
            memcpy(xbuf, &chan->config, sizeof(cyg_serial_info_t));
            *len = sizeof(chan->config);
            break;       

        case CYG_IO_GET_CONFIG_SERIAL_BUFFER_INFO:
            // return rx/tx buffer sizes and counts
            {
                cyg_serial_buf_info_t *p;
                if (*len < sizeof(cyg_serial_buf_info_t))
                    return -EINVAL;

                *len = sizeof(cyg_serial_buf_info_t);
                p = (cyg_serial_buf_info_t *)xbuf;

                p->rx_bufsize = in_cbuf->len;
                if (p->rx_bufsize)
                    p->rx_count = in_cbuf->nb;
                else
                    p->rx_count = 0;

                p->tx_bufsize = out_cbuf->len;
                if (p->tx_bufsize)
                    p->tx_count = out_cbuf->nb;
                else
                    p->tx_count = 0;
            }
            break;

        case CYG_IO_GET_CONFIG_SERIAL_ABORT:
            // Abort any outstanding I/O, including blocked reads
            // Caution - assumed to be called from 'timeout' (i.e. DSR) code
            in_cbuf->abort = true;
            cyg_drv_cond_broadcast(&in_cbuf->wait);

            out_cbuf->abort = true;
            cyg_drv_cond_broadcast(&out_cbuf->wait);
            break;

        default:
            res = -EINVAL;
    }
    return res;
}

static Cyg_ErrNo 
bp2016_serial_set_config(cyg_io_handle_t handle, cyg_uint32 key, const void *xbuf, 
        cyg_uint32 *len)
{
    Cyg_ErrNo res = ENOERR;
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *)handle;
    bp2016_uart_channel *chan = (bp2016_uart_channel *)t->priv;
    bp2016_cbuf_t *out_cbuf = &chan->out_cbuf;
    bp2016_cbuf_t *in_cbuf = &chan->in_cbuf;

    if(NULL == xbuf || NULL == len) return -EINVAL;
    switch (key) {
        case CYG_IO_SET_CONFIG_SERIAL_INFO:
            {
                cyg_serial_info_t *config = (cyg_serial_info_t *)xbuf;
                if ( *len < sizeof(cyg_serial_info_t) ) {
                    return -EINVAL;
                }
                *len = sizeof(cyg_serial_info_t);
                if ( true != bp2016_uart_config_port(chan, config, false) )
                    return -EINVAL;
            }
            break;

        case CYG_IO_SET_CONFIG_SERIAL_READ_TIMEOUT:
            in_cbuf->timeout = (cyg_uint32)xbuf;
            //printf("set read timeout:%d\n", in_cbuf->timeout);
            break;

        case CYG_IO_SET_CONFIG_SERIAL_WRITE_TIMEOUT:
            out_cbuf->timeout = (cyg_uint32)xbuf;
            //printf("set write timeout:%d\n", out_cbuf->timeout);
            break;

        default:
            return -EINVAL;
    }

    return res;
}

#endif // CYGPKG_IO_SERIAL_ARM_BP2016

// EOF bp2016_serial.c
