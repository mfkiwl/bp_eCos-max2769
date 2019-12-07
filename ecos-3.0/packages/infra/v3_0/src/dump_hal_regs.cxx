//==========================================================================
//
//      abort.cxx
//
//      Dummy abort()
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2003 Free Software Foundation, Inc.                        
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
// Author(s):   nickg
// Date:        2003-04-02
// Purpose:     provide a dummy abort() function
// Description: Parts of the C and C++ compiler runtime systems have 
//              references to abort() built in to them. This definition
//              satisfies these references. Note that it is not expected
//              that this function will ever be called.
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/hal_arm.h>
#include <pkgconf/system.h>
#include <pkgconf/infra.h>

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_trac.h>         // tracing macros
#include <cyg/infra/cyg_ass.h>          // assertion macros

#include <pkgconf/hal.h>                // HAL configury
#include <cyg/infra/diag.h>             // HAL polled output
#include <cyg/hal/hal_arch.h>           // architectural stuff for...
#include <cyg/hal/hal_intr.h>           // interrupt control

#ifdef CYGPKG_KERNEL
#include <pkgconf/kernel.h>             // kernel configury
#include <cyg/kernel/thread.hxx>        // thread id to print
#include <cyg/kernel/sched.hxx>         // ancillaries for above
#include <cyg/kernel/thread.inl>        // ancillaries for above
#endif

#ifdef CYGDBG_HG_DUMP_ALL_THREADS_INFO

//==========================================================================
externC void
cyg_hg_trace_dump(void);

//externC void cyg_hg_dump_frame(unsigned char *frame,  EXCEPTION_ENUM ex);
externC bool kernel_text_address(cyg_uint32 addr);
externC bool kernel_sp_addr_cross(cyg_uint32 addr);

void cyg_hg_dump_frame_ss(unsigned char *frame, CYG_ADDRESS sp_base, cyg_uint32 size)
{
    HAL_SavedRegisters *rp = (HAL_SavedRegisters *)frame;
    int i;

    diag_printf("Registers:\n");
    for (i = 0;  i <= 10;  i++) {
        if ((i == 0) || (i == 6)) diag_printf("R%d: ", i);
        diag_printf("%08X ", rp->d[i]);
        if ((i == 5) || (i == 10)) diag_printf("\n");
    }
    diag_printf("FP: %08X, SP: %08X, LR: %08X, PC: %08X, PSR: %08X\n SVC_LR: %08X, SVC_SP: %08X\n",
            rp->fp, rp->sp, rp->lr, rp->pc, rp->cpsr, rp->svc_lr, rp->svc_sp);

	if((rp->sp >= sp_base) && (rp->sp <= (sp_base + size))) {
		if(kernel_sp_addr_cross(rp->sp) == true) 
			diag_printf("SP is in kernel space\r\n");
		else
			diag_printf("SP is in thread space\r\n");
	}
	else 
		diag_printf("SP invalid\r\n");

    if(kernel_text_address(rp->pc) == false)
        diag_printf("PC is invalid address\r\n");
    else
        diag_printf("PC is valid\r\n");

    if(kernel_text_address(rp->lr) == false)
        diag_printf("LR is invalid address\r\n");
    else
        diag_printf("LR is valid\r\n");

    diag_printf("\r\n");
}

void
cyg_hg_trace_dump(void)
{
    HAL_SavedRegisters *regs;
#if defined(CYGPKG_KERNEL)
    {
        diag_printf("\nScheduler:\n\n");

        Cyg_Scheduler *sched = &Cyg_Scheduler::scheduler;

        diag_printf("Lock:                %d\n",sched->get_sched_lock() );

# ifdef CYGVAR_KERNEL_THREADS_NAME
    
        diag_printf("Current Thread:      %s\n",sched->get_current_thread()->get_name());

# else

        diag_printf("Current Thread:    %d\n",sched->get_current_thread()->get_unique_id());
    
# endif

    }
    
   diag_printf("\nThe following show all Threads' informations:\n\n");
# ifdef CYGVAR_KERNEL_THREADS_LIST
    {
        Cyg_Thread *t = Cyg_Thread::get_list_head();

    
        while( NULL != t )
        {
            diag_printf("\n###############################################################\n");
            cyg_uint32 state = t->get_state();
            static char *(reasons[8]) =
            {
                "NONE",                           // No recorded reason
                "WAIT",                           // Wait with no timeout
                "DELAY",                          // Simple time delay
                "TIMEOUT",                        // Wait with timeout/timeout expired
                "BREAK",                          // forced break out of sleep
                "DESTRUCT",                       // wait object destroyed[note]
                "EXIT",                           // forced termination
                "DONE"                            // Wait/delay complete
            };

            char * s = NULL;
            switch(state) {
            case 0:
                s = "RUNNING";
                break;
            case 1:
                s = "SLEEPING";
                break;
            case 2:
                s = "COUNTSLEEP";
                break;
            case 4:
                s = "SUSPENDED";
                break;
            case 8:
                s = "CREATING";
                break;
            case 16:
                s = "EXITED";
                break;
            case 3:
                s = "SLEEPING OR COUNTSLEEP";
                break;
            default:
                s = "Unknow";
                break;
            }

#   ifdef CYGVAR_KERNEL_THREADS_NAME
        
            diag_printf( "%20s pri = %3d state = %s id = %3d\n",
                         t->get_name(),
                         t->get_priority(),
                         s,
                         t->get_unique_id()
                );
#   else

            diag_printf( "Thread %3d        pri = %3d state = %s\n",
                         t->get_unique_id(),
                         t->get_priority(),
                         s 
                );

#   endif        
            diag_printf( "%20s stack base = %08x stack end = %08x size = %08x ctx = %08x\n",
                         "",
                         t->get_stack_base(),
                         t->get_stack_size() + t->get_stack_base(),
                         t->get_stack_size(),
                         (int) t->hg_get_saved_context()
                );

            diag_printf( "%20s sleep reason %8s wake reason %8s\n",
                         "",
                         reasons[t->get_sleep_reason()],
                         reasons[t->get_wake_reason()]
                );

            diag_printf( "%20s queue = %08x      wait info = %08x\n",
                         "",
                         (int) t->get_current_queue(),
                         t->get_wait_info()
                         );

            diag_printf( "Thread stack: \n");
            regs = t->hg_get_saved_context();

            cyg_hg_dump_frame_ss((unsigned char *)regs, t->get_stack_base(), t->get_stack_size());

            diag_printf("\n");
            t = t->get_list_next();
        }

    }
# endif // CYGVAR_KERNEL_THREADS_LIST
    
#endif // CYG_DIAG_PRINTF
    while(1){};
}

#endif //CYGDBG_HG_DUMP_ALL_THREADS_INFO
//==========================================================================
// EOF abort.cxx
