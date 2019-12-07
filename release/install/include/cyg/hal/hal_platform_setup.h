#ifndef CYGONCE_HAL_PLATFORM_SETUP_H
#define CYGONCE_HAL_PLATFORM_SETUP_H

/*=============================================================================
//
//      hal_platform_setup.h
//
//      Platform specific support for HAL
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 1998, 1999, 2000, 2001, 2002, 2006 Free Software Foundation, Inc.
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
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   Mike Jones
// Contributors:Ant Micro, ITR-GmbH
// Date:        2013-08-08
// Purpose:     Freescale iMXQ platform specific support routines
// Description:
// Usage:       #include <cyg/hal/hal_platform_setup.h>
//
//####DESCRIPTIONEND####
//
//===========================================================================*/

#include <cyg/hal/var_io.h>
#include <cyg/hal/hal_mmu.h>
#include <pkgconf/hal_arm.h>
#include <cyg/hal/mem_map.h>

v7_inv_l1_l2_dcache_:
   /* Invalidate Data/Unified Caches */
   
   MRC     p15, 1, r0, c0, c0, 1      /* Read CLIDR r0 = 0x0a200023 */
   ANDS    r3, r0, #0x07000000        /* Extract coherency level    */
   MOV     r3, r3, LSR #23            /* Total cache levels << 1    */
   BEQ     Finished                   /* If 0, no need to clean     */
   
   MOV     r10, #0                    /* R10 holds current cache level << 1 */
Loop1:
   ADD     r2, r10, r10, LSR #1       /* R2 holds cache "Set" position      */
   MOV     r1, r0, LSR r2             /* Bottom 3 bits are the Cache-type for this level */
   AND     r1, r1, #7                 /* Isolate those lower 3 bits */
   CMP     r1, #2
   BLT     Skip                       /* No cache or only instruction cache at this level */
   
   MCR     p15, 2, r10, c0, c0, 0     /* Write the Cache Size selection register */
   ISB                                /* ISB to sync the change to the CacheSizeID reg */
   MRC     p15, 1, r1, c0, c0, 0      /* Reads current Cache Size ID register*/
   AND     r2, r1, #7                 /* Extract the line length field */
   ADD     r2, r2, #4                 /* Add 4 for the line length offset (log2 16 bytes)*/
   LDR     r4, =0x3FF
   ANDS    r4, r4, r1, LSR #3         /* R4 is the max number on the way size (right aligned)*/
   CLZ     r5, r4                     /* R5 is the bit position of the way size increment */
   LDR     r7, =0x7FFF
   ANDS    r7, r7, r1, LSR #13        /* R7 is the max number of the index size (right aligned) */

Loop2:
   MOV     r9, r4                     /* R9 working copy of the max way size (right aligned)*/

Loop3:
    ORR     r11, r10, r9, LSL r5       /* Factor in the Way number and cache number into R11*/
    ORR     r11, r11, r7, LSL r2       /* Factor in the Set number */
    MCR     p15, 0, r11, c7, c6, 2     /* Invalidate by Set/Way */
    SUBS    r9, r9, #1                 /* Decrement the Way number */
    BGE     Loop3
    SUBS    r7, r7, #1                 /* Decrement the Set number */
    BGE     Loop2
Skip:
    ADD     r10, r10, #2               /* increment the cache number */
    CMP     r3, r10
    BGT     Loop1

Finished:
    mov     pc, lr


   .macro _setup
    nop

        // Disable interuptss and set to SVC mode
	mrs	r0,cpsr
    bic     r0, r0, #0x1f
    bic     r0, r0, #(CPSR_THUMB_ENABLE) 
    orr     r0, r0, #(CPSR_IRQ_DISABLE|CPSR_FIQ_DISABLE|CPSR_SUPERVISOR_MODE)
    msr     cpsr, r0

    // Set SPSR
    mov     r0, #(CPSR_IRQ_DISABLE|CPSR_FIQ_DISABLE|CPSR_SUPERVISOR_MODE)
    msr     spsr_cxsf, r0
    mrs     r0, spsr

	/*
	 * disable MMU and d-caches, enable alignment
         * enable normal exception vectors/i-cache/branch prediction
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 13)	@ clear bits 13 (--V-)
	bic	r0, r0, #0x00000007	@ clear bits 2:0 (-CAM)
	orr	r0, r0, #(1 << 1)	@ set bit 1 (--A-) Align
	orr	r0, r0, #(1 << 11)	@ set bit 11 (Z---) BTB
	orr	r0, r0, #(1 << 12)	@ set bit 12 (I) I-cache
	mcr	p15, 0, r0, c1, c0, 0
	isb
    dsb

    /*
	 * Invalidate L1 I/D
	 */
	mov	r0, #0			        @ set up for MCR
	mcr	p15, 0, r0, c8, c7, 0	@ invalidate TLBs
	mcr	p15, 0, r0, c7, c5, 0	@ invalidate icache
	mcr	p15, 0, r0, c7, c5, 6	@ invalidate BP array
	mcr p15, 0, r0, c7, c10, 4	@ DSB
	mcr p15, 0, r0, c7, c5, 4	@ ISB

    mov sp, lr
    bl v7_inv_l1_l2_dcache_
    mov lr, sp
	/* 
         * Set vector address in CP15 VBAR register 
         */
	mrc 	p15, 0, r0, c1, c0, 0   @ Read CP15 SCTLR Register
	bic 	r0, #(1 << 13) 	/* V = 0 */
	mcr 	p15, 0, r0, c1, c0, 0   @ Write CP15 SCTLR Register

	ldr 	r0, =__exception_handlers
	mcr 	p15, 0, r0, c12, c0, 0  @Set VBAR

	/*
	 * Enable VFP / NEON
     */
   	mrc 	p15, 0, r0, c1, c0, 2 @ Read CP Access register
   	orr 	r0, r0, #0x00f00000   @ Enable full access to NEON/VFP (Coprocessors 10 and 11)
   	mcr 	p15, 0, r0, c1, c0, 2 @ Write CP Access register
   	isb
   	mov 	r0, #0x40000000       @ Switch on the VFP and NEON hardware
   	vmsr 	fpexc, r0            @ Set EN bit in FPEXC
	isb
    dsb

	/*
	 * Enable actlr.SMP so we can use MMU and D$
	 */
	mrc	p15, 0, r0, c1, c0, 1
	tst	r0, #(1 << 6)		@ SMP/nAMP mode enabled?
	orreq	r0, r0, #(1 << 6)	@ Enable SMP/nAMP mode
	mcreq	p15, 0, r0, c1, c0, 1

/*
    ldr     sp,.__exception_stack
    bl      mmu_setup
*/    
    .endm

#define PLATFORM_SETUP1 _setup

    .globl  _platform_resvered_data
    .macro  _platform_vectors
    .balign 1024
_platform_resvered_data:
    .rept   CONFIG_RSV_DATA_COUNT
    .long   0       // Board Control register shadow
    .endr
    .endm                                        

#define PLATFORM_VECTORS         _platform_vectors

//-----------------------------------------------------------------------------
// end of hal_platform_setup.h
#endif // CYGONCE_HAL_PLATFORM_SETUP_H
