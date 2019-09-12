//==========================================================================
//
//      ser_arm_bp2016_generic_16x5x.inl
//
//      ARM Industrial Module BP2016 Serial I/O Interface Module
//      (interrupt driven) for use with 16x5x driver.
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
// Author(s):    jlarmour
// Contributors: 
// Date:         2001-06-08
// Purpose:      Serial I/O module (interrupt driven version)
// Description: 
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <cyg/hal/hal_intr.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/plf/chip_config.h>
#include <cyg/hal/regs/uart.h>
//-----------------------------------------------------------------------------
// Baud rate specification, based on raw 95MHz clock
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
     //UBRDIV_50,    // 50
     //UBRDIV_75,    // 75
             0,    // 50
             0,    // 75
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

//#############################################################################
#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL0
static pc_serial_info bp2016_16x5x_serial_info0 = {REG_AP_UART0_BASE,
                                         CYGNUM_HAL_INTERRUPT_UART0};
#if CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL0_BUFSIZE > 0
static unsigned char bp2016_16x5x_serial_out_buf0[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL0_BUFSIZE];
static unsigned char bp2016_16x5x_serial_in_buf0[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL0_BUFSIZE];

static SERIAL_CHANNEL_USING_INTERRUPTS(bp2016_16x5x_serial_channel0,
                                       pc_serial_funs, 
                                       bp2016_16x5x_serial_info0,
                                       CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL0_BAUD),
                                       CYG_SERIAL_STOP_DEFAULT,
                                       CYG_SERIAL_PARITY_DEFAULT,
                                       CYG_SERIAL_WORD_LENGTH_DEFAULT,
                                       CYG_SERIAL_FLAGS_DEFAULT,
                                       &bp2016_16x5x_serial_out_buf0[0], sizeof(bp2016_16x5x_serial_out_buf0),
                                       &bp2016_16x5x_serial_in_buf0[0], sizeof(bp2016_16x5x_serial_in_buf0)
    );
#else
static SERIAL_CHANNEL(bp2016_16x5x_serial_channel0,
                      pc_serial_funs, 
                      bp2016_16x5x_serial_info0,
                      CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL0_BAUD),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );
#endif

DEVTAB_ENTRY(bp2016_16x5x_serial_io0, 
             CYGDAT_IO_SERIAL_ARM_BP2016_16X5X_SERIAL0_NAME,
             0,                     // Does not depend on a lower level interface
             &cyg_io_serial_devio, 
             pc_serial_init, 
             pc_serial_lookup,     // Serial driver may need initializing
             &bp2016_16x5x_serial_channel0
    );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL0

//#############################################################################
#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL1
static pc_serial_info bp2016_16x5x_serial_info1 = {REG_AP_UART1_BASE,
                                         CYGNUM_HAL_INTERRUPT_UART1};
#if CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL1_BUFSIZE > 0
static unsigned char bp2016_16x5x_serial_out_buf1[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL1_BUFSIZE];
static unsigned char bp2016_16x5x_serial_in_buf1[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL1_BUFSIZE];

static SERIAL_CHANNEL_USING_INTERRUPTS(bp2016_16x5x_serial_channel1,
                                       pc_serial_funs, 
                                       bp2016_16x5x_serial_info1,
                                       CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL1_BAUD),
                                       CYG_SERIAL_STOP_DEFAULT,
                                       CYG_SERIAL_PARITY_DEFAULT,
                                       CYG_SERIAL_WORD_LENGTH_DEFAULT,
                                       CYG_SERIAL_FLAGS_DEFAULT,
                                       &bp2016_16x5x_serial_out_buf1[0], sizeof(bp2016_16x5x_serial_out_buf1),
                                       &bp2016_16x5x_serial_in_buf1[0], sizeof(bp2016_16x5x_serial_in_buf1)
    );
#else
static SERIAL_CHANNEL(bp2016_16x5x_serial_channel1,
                      pc_serial_funs, 
                      bp2016_16x5x_serial_info1,
                      CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL1_BAUD),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );
#endif

DEVTAB_ENTRY(bp2016_16x5x_serial_io1, 
             CYGDAT_IO_SERIAL_ARM_BP2016_16X5X_SERIAL1_NAME,
             0,                     // Does not depend on a lower level interface
             &cyg_io_serial_devio, 
             pc_serial_init, 
             pc_serial_lookup,     // Serial driver may need initializing
             &bp2016_16x5x_serial_channel1
    );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL1

//#############################################################################
#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL2
static pc_serial_info bp2016_16x5x_serial_info2 = {REG_AP_UART2_BASE,
                                         CYGNUM_HAL_INTERRUPT_UART2};
#if CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL2_BUFSIZE > 0
static unsigned char bp2016_16x5x_serial_out_buf2[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL2_BUFSIZE];
static unsigned char bp2016_16x5x_serial_in_buf2[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL2_BUFSIZE];

static SERIAL_CHANNEL_USING_INTERRUPTS(bp2016_16x5x_serial_channel2,
                                       pc_serial_funs, 
                                       bp2016_16x5x_serial_info2,
                                       CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL2_BAUD),
                                       CYG_SERIAL_STOP_DEFAULT,
                                       CYG_SERIAL_PARITY_DEFAULT,
                                       CYG_SERIAL_WORD_LENGTH_DEFAULT,
                                       CYG_SERIAL_FLAGS_DEFAULT,
                                       &bp2016_16x5x_serial_out_buf2[0], sizeof(bp2016_16x5x_serial_out_buf2),
                                       &bp2016_16x5x_serial_in_buf2[0], sizeof(bp2016_16x5x_serial_in_buf2)
    );
#else
static SERIAL_CHANNEL(bp2016_16x5x_serial_channel2,
                      pc_serial_funs, 
                      bp2016_16x5x_serial_info2,
                      CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL2_BAUD),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );
#endif

DEVTAB_ENTRY(bp2016_16x5x_serial_io2, 
             CYGDAT_IO_SERIAL_ARM_BP2016_16X5X_SERIAL2_NAME,
             0,                     // Does not depend on a lower level interface
             &cyg_io_serial_devio, 
             pc_serial_init, 
             pc_serial_lookup,     // Serial driver may need initializing
             &bp2016_16x5x_serial_channel2
    );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL2

//#############################################################################
#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL3
static pc_serial_info bp2016_16x5x_serial_info3 = {REG_AP_UART3_BASE,
                                         CYGNUM_HAL_INTERRUPT_UART3};
#if CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL3_BUFSIZE > 0
static unsigned char bp2016_16x5x_serial_out_buf3[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL3_BUFSIZE];
static unsigned char bp2016_16x5x_serial_in_buf3[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL3_BUFSIZE];

static SERIAL_CHANNEL_USING_INTERRUPTS(bp2016_16x5x_serial_channel3,
                                       pc_serial_funs, 
                                       bp2016_16x5x_serial_info3,
                                       CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL3_BAUD),
                                       CYG_SERIAL_STOP_DEFAULT,
                                       CYG_SERIAL_PARITY_DEFAULT,
                                       CYG_SERIAL_WORD_LENGTH_DEFAULT,
                                       CYG_SERIAL_FLAGS_DEFAULT,
                                       &bp2016_16x5x_serial_out_buf3[0], sizeof(bp2016_16x5x_serial_out_buf3),
                                       &bp2016_16x5x_serial_in_buf3[0], sizeof(bp2016_16x5x_serial_in_buf3)
    );
#else
static SERIAL_CHANNEL(bp2016_16x5x_serial_channel3,
                      pc_serial_funs, 
                      bp2016_16x5x_serial_info3,
                      CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL3_BAUD),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );
#endif

DEVTAB_ENTRY(bp2016_16x5x_serial_io3, 
             CYGDAT_IO_SERIAL_ARM_BP2016_16X5X_SERIAL3_NAME,
             0,                     // Does not depend on a lower level interface
             &cyg_io_serial_devio, 
             pc_serial_init, 
             pc_serial_lookup,     // Serial driver may need initializing
             &bp2016_16x5x_serial_channel3
    );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL3

//#############################################################################
#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL4
static pc_serial_info bp2016_16x5x_serial_info4 = {REG_AP_UART4_BASE,
                                         CYGNUM_HAL_INTERRUPT_UART4};
#if CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL4_BUFSIZE > 0
static unsigned char bp2016_16x5x_serial_out_buf4[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL4_BUFSIZE];
static unsigned char bp2016_16x5x_serial_in_buf4[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL4_BUFSIZE];

static SERIAL_CHANNEL_USING_INTERRUPTS(bp2016_16x5x_serial_channel4,
                                       pc_serial_funs, 
                                       bp2016_16x5x_serial_info4,
                                       CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL4_BAUD),
                                       CYG_SERIAL_STOP_DEFAULT,
                                       CYG_SERIAL_PARITY_DEFAULT,
                                       CYG_SERIAL_WORD_LENGTH_DEFAULT,
                                       CYG_SERIAL_FLAGS_DEFAULT,
                                       &bp2016_16x5x_serial_out_buf4[0], sizeof(bp2016_16x5x_serial_out_buf4),
                                       &bp2016_16x5x_serial_in_buf4[0], sizeof(bp2016_16x5x_serial_in_buf4)
    );
#else
static SERIAL_CHANNEL(bp2016_16x5x_serial_channel4,
                      pc_serial_funs, 
                      bp2016_16x5x_serial_info4,
                      CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL4_BAUD),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );
#endif

DEVTAB_ENTRY(bp2016_16x5x_serial_io4, 
             CYGDAT_IO_SERIAL_ARM_BP2016_16X5X_SERIAL4_NAME,
             0,                     // Does not depend on a lower level interface
             &cyg_io_serial_devio, 
             pc_serial_init, 
             pc_serial_lookup,     // Serial driver may need initializing
             &bp2016_16x5x_serial_channel4
    );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL4

//#############################################################################
#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL5
static pc_serial_info bp2016_16x5x_serial_info5 = {REG_AP_UART5_BASE,
                                         CYGNUM_HAL_INTERRUPT_UART5};
#if CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL5_BUFSIZE > 0
static unsigned char bp2016_16x5x_serial_out_buf5[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL5_BUFSIZE];
static unsigned char bp2016_16x5x_serial_in_buf5[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL5_BUFSIZE];

static SERIAL_CHANNEL_USING_INTERRUPTS(bp2016_16x5x_serial_channel5,
                                       pc_serial_funs, 
                                       bp2016_16x5x_serial_info5,
                                       CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL5_BAUD),
                                       CYG_SERIAL_STOP_DEFAULT,
                                       CYG_SERIAL_PARITY_DEFAULT,
                                       CYG_SERIAL_WORD_LENGTH_DEFAULT,
                                       CYG_SERIAL_FLAGS_DEFAULT,
                                       &bp2016_16x5x_serial_out_buf5[0], sizeof(bp2016_16x5x_serial_out_buf5),
                                       &bp2016_16x5x_serial_in_buf5[0], sizeof(bp2016_16x5x_serial_in_buf5)
    );
#else
static SERIAL_CHANNEL(bp2016_16x5x_serial_channel5,
                      pc_serial_funs, 
                      bp2016_16x5x_serial_info5,
                      CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL5_BAUD),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );
#endif

DEVTAB_ENTRY(bp2016_16x5x_serial_io5, 
             CYGDAT_IO_SERIAL_ARM_BP2016_16X5X_SERIAL5_NAME,
             0,                     // Does not depend on a lower level interface
             &cyg_io_serial_devio, 
             pc_serial_init, 
             pc_serial_lookup,     // Serial driver may need initializing
             &bp2016_16x5x_serial_channel5
    );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL5

//#############################################################################
#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL6
static pc_serial_info bp2016_16x5x_serial_info6 = {REG_AP_UART6_BASE,
                                         CYGNUM_HAL_INTERRUPT_UART6};
#if CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL6_BUFSIZE > 0
static unsigned char bp2016_16x5x_serial_out_buf6[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL6_BUFSIZE];
static unsigned char bp2016_16x5x_serial_in_buf6[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL6_BUFSIZE];

static SERIAL_CHANNEL_USING_INTERRUPTS(bp2016_16x5x_serial_channel6,
                                       pc_serial_funs, 
                                       bp2016_16x5x_serial_info6,
                                       CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL6_BAUD),
                                       CYG_SERIAL_STOP_DEFAULT,
                                       CYG_SERIAL_PARITY_DEFAULT,
                                       CYG_SERIAL_WORD_LENGTH_DEFAULT,
                                       CYG_SERIAL_FLAGS_DEFAULT,
                                       &bp2016_16x5x_serial_out_buf6[0], sizeof(bp2016_16x5x_serial_out_buf6),
                                       &bp2016_16x5x_serial_in_buf6[0], sizeof(bp2016_16x5x_serial_in_buf6)
    );
#else
static SERIAL_CHANNEL(bp2016_16x5x_serial_channel6,
                      pc_serial_funs, 
                      bp2016_16x5x_serial_info6,
                      CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL6_BAUD),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );
#endif

DEVTAB_ENTRY(bp2016_16x5x_serial_io6, 
             CYGDAT_IO_SERIAL_ARM_BP2016_16X5X_SERIAL6_NAME,
             0,                     // Does not depend on a lower level interface
             &cyg_io_serial_devio, 
             pc_serial_init, 
             pc_serial_lookup,     // Serial driver may need initializing
             &bp2016_16x5x_serial_channel6
    );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL6

//#############################################################################
#ifdef CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL7
static pc_serial_info bp2016_16x5x_serial_info7 = {REG_AP_UART7_BASE,
                                         CYGNUM_HAL_INTERRUPT_UART7};
#if CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL7_BUFSIZE > 0
static unsigned char bp2016_16x5x_serial_out_buf7[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL7_BUFSIZE];
static unsigned char bp2016_16x5x_serial_in_buf7[CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL7_BUFSIZE];

static SERIAL_CHANNEL_USING_INTERRUPTS(bp2016_16x5x_serial_channel7,
                                       pc_serial_funs, 
                                       bp2016_16x5x_serial_info7,
                                       CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL7_BAUD),
                                       CYG_SERIAL_STOP_DEFAULT,
                                       CYG_SERIAL_PARITY_DEFAULT,
                                       CYG_SERIAL_WORD_LENGTH_DEFAULT,
                                       CYG_SERIAL_FLAGS_DEFAULT,
                                       &bp2016_16x5x_serial_out_buf7[0], sizeof(bp2016_16x5x_serial_out_buf7),
                                       &bp2016_16x5x_serial_in_buf7[0], sizeof(bp2016_16x5x_serial_in_buf7)
    );
#else
static SERIAL_CHANNEL(bp2016_16x5x_serial_channel7,
                      pc_serial_funs, 
                      bp2016_16x5x_serial_info7,
                      CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL7_BAUD),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );
#endif

DEVTAB_ENTRY(bp2016_16x5x_serial_io7, 
             CYGDAT_IO_SERIAL_ARM_BP2016_16X5X_SERIAL7_NAME,
             0,                     // Does not depend on a lower level interface
             &cyg_io_serial_devio, 
             pc_serial_init, 
             pc_serial_lookup,     // Serial driver may need initializing
             &bp2016_16x5x_serial_channel7
    );
#endif //  CYGPKG_IO_SERIAL_ARM_BP2016_16X5X_SERIAL7

// EOF ser_arm_bp2016_16x5x.inl
