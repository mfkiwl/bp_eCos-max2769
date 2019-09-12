/*
 * Copyright (c) 2012, Freescale Semiconductor, Inc.
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

#include <pkgconf/hal.h>
#include <cyg/hal/hal_if.h>
#include <cyg/hal/var_intr.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/regs/scm.h>
#include <cyg/hal/plf/chip_config.h>
////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

extern char *__exception_handlers;
extern void mmu_setup(void);
extern void hal_interrupt_init(void);
extern int arch_timer_starting_cpu(void);

void hal_misc_init(void)
{
   //1. for fastboot 
#ifdef CYGHWR_HAL_BP2018_SUPPORT_FAST_BOOT
    hwp_apSCM->sw_bp_magic = CONFIG_BP_FASTBOOT_JUMP_MAGIC;
    hwp_apSCM->sw_bp_jaddr = &__exception_handlers; 
#endif
}

void platform_init(void)
{
    mmu_setup();

    hal_interrupt_init();

    arch_timer_starting_cpu();      /*for archtimer init*/

    hal_if_init();
    hal_misc_init();
}

void hal_hardware_init(void)
{
    platform_init();
}

////////////////////////////////////////////////////////////////////////////////
