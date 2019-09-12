/*
 * Copyright (c) 2017, HG, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//! @addtogroup cortexa7
//! @{

/*!
 * @file  arm_cp_registers.h
 * @brief Definitions for ARM coprocessor registers.
 */

#ifndef __ARM_CP_REGISTERS_H__
#define __ARM_CP_REGISTERS_H__

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/*      
 * CR1 bits (CP#15 CR1)
 */
#define CR_M    (1 << 0)        /* MMU enable                           */
#define CR_A    (1 << 1)        /* Alignment abort enable               */
#define CR_C    (1 << 2)        /* Dcache enable                        */
#define CR_W    (1 << 3)        /* Write buffer enable                  */
#define CR_P    (1 << 4)        /* 32-bit exception handler             */
#define CR_D    (1 << 5)        /* 32-bit data address range            */
#define CR_L    (1 << 6)        /* Implementation defined               */
#define CR_B    (1 << 7)        /* Big endian                           */
#define CR_S    (1 << 8)        /* System MMU protection                */
#define CR_R    (1 << 9)        /* ROM MMU protection                   */
#define CR_F    (1 << 10)       /* Implementation defined               */
#define CR_Z    (1 << 11)       /* Implementation defined               */
#define CR_I    (1 << 12)       /* Icache enable                        */
#define CR_V    (1 << 13)       /* Vectors relocated to 0xffff0000      */
#define CR_RR   (1 << 14)       /* Round Robin cache replacement        */
#define CR_L4   (1 << 15)       /* LDR pc can set T bit                 */
#define CR_DT   (1 << 16)
#define CR_IT   (1 << 18)
#define CR_ST   (1 << 19)
#define CR_FI   (1 << 21)       /* Fast interrupt (lower latency mode)  */
#define CR_U    (1 << 22)       /* Unaligned access operation           */
#define CR_XP   (1 << 23)       /* Extended page tables                 */
#define CR_VE   (1 << 24)       /* Vectored interrupts                  */
#define CR_EE   (1 << 25)       /* Exception (Big) Endian               */
#define CR_TRE  (1 << 28)       /* TEX remap enable                     */
#define CR_AFE  (1 << 29)       /* Access flag enable                   */
#define CR_TE   (1 << 30)       /* Thumb exception enable               */

/*
 *  ACTLR
 */ 
#define ACTLR_SMP (1 << 6)

/*
 * DFSR
 */
#define DFSR_WNR (1 << 11)   //!< Write not Read bit. 0=read, 1=write.
#define DFSR_FS4 (0x400)      //!< Fault status bit 4..
#define DFSR_FS (0xf)      //!< Fault status bits [3:0].

#endif // __ARM_CP_REGISTERS_H__
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////

