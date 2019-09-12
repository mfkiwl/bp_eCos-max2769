/*=================================================================
//
//        kmutex1.c
//
//        Kernel C API Mutex test 1
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
// Author(s):     dsm
// Contributors:    dsm
// Date:          1998-03-23
// Description:   Tests basic mutex functionality.
// Omissions:     Timed wait.
//####DESCRIPTIONEND####
*/

#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL

#include <cyg/kernel/kapi.h>

#include <cyg/infra/testcase.h>

#define CHECK(b) CYG_TEST_CHECK(b,#b)

#define NTHREADS 3
#define STACKSIZE CYGNUM_HAL_STACK_SIZE_TYPICAL

#define THREAD_DEBUG

#ifdef THREAD_DEBUG
#define th_printf(fmt, args...)		\
	diag_printf("[Thread cond] %s:%d	" fmt "\n", __func__, __LINE__, ##args);
#else
#define th_printf(fmt, ...)
#endif

static cyg_handle_t thread[NTHREADS];

static cyg_thread thread_obj[NTHREADS];
static char stack[NTHREADS][STACKSIZE];


static cyg_mutex_t m0, m1;
static cyg_cond_t cvar0, cvar1, cvar2;

static cyg_ucount8 m0d=0, m1d=0;

static void finish( cyg_ucount8 t )
{
	th_printf("LOCK m1! Thread_%d", t);
    cyg_mutex_lock( &m1 ); {
        m1d |= 1<<t;
        if( 0x7 == m1d )
            CYG_TEST_PASS_FINISH("Kernel C API Mutex 1 OK");
		th_printf("cyg_cond_wait wait cvar2!");
        cyg_cond_wait( &cvar2 );
		th_printf("get cvar2!");
    } /* cyg_mutex_unlock( &m1 ); */
    CYG_TEST_FAIL_FINISH("Not reached");    
}

static void entry0( cyg_addrword_t data )
{
	th_printf("LOCK m0!");
    cyg_mutex_lock( &m0 ); {
        /*
         *
         *CHECK expand as this
         do { (void)(( ! cyg_mutex_trylock( &m0 ) ) || ( cyg_test_output(CYGNUM_TEST_FAIL, "! cyg_mutex_trylock( &m0 )", 101, "kmutex1.c") , cyg_test_exit(), 1)); } while (0);
         *
         */
		//cyg_mutex_trylock Attempts to change the state of a mutex to the locked state. 
		//This function call returns immediately. 
		//TRUE is returned if the mutex has been locked, or FALSE if it has not been locked. 
		//this time , the return value is FALSE, and the CHECK pass
        CHECK( ! cyg_mutex_trylock( &m0 ) );
		th_printf("LOCK m1!");
        cyg_mutex_lock( &m1 ); {
            CHECK( ! cyg_mutex_trylock( &m0 ) );
		th_printf("UNLOCK m1!");
        } cyg_mutex_unlock( &m1 );
    } 
	th_printf("UNLOCK m0!");
	cyg_mutex_unlock( &m0 );

	th_printf("LOCK m0!");
    cyg_mutex_lock( &m0 ); {
        while ( 0 == m0d )
		{
			th_printf("cyg_cond_wait wait cvar0!");
			//unlock cvar0, and this thread go to sleep
            cyg_cond_wait( &cvar0 );
			th_printf("get cvar0!");
		}
        CHECK( 1 == m0d++ );
		th_printf("cyg_cond_signal signal cvar0!");
        cyg_cond_signal( &cvar0 );
        while ( 4 != m0d )
		{
			th_printf("wait cvar1!");
            cyg_cond_wait( &cvar1 );
			th_printf("get cvar1!");
		}
        CHECK( 4 == m0d );
    } 
	th_printf("UNLOCK m0!");
	cyg_mutex_unlock( &m0 );

    finish( (cyg_ucount8)data );
}

static void entry1( cyg_addrword_t data )
{
	th_printf("LOCK m0!");
    cyg_mutex_lock( &m0 ); {
        th_printf("CHECK trylock m1, if return flase, exit");
        CHECK( cyg_mutex_trylock( &m1 ) ); {
        } 
        th_printf("UNLOCK m1");
        cyg_mutex_unlock( &m1 );
      
        th_printf("UNLOCK m0");
    } cyg_mutex_unlock( &m0 );

	th_printf("LOCK m0!");
    cyg_mutex_lock( &m0 ); {
	th_printf("CHECK m0d(%d)  == 0!, if m0d is not 0, exit", m0d);
        CHECK( 0 == m0d++ );
     
        th_printf("cyg_cond_broadcast cvar0!");
        cyg_cond_broadcast( &cvar0 );
    } 
    th_printf("UNLOCK m0!");
    cyg_mutex_unlock( &m0 );
    
	th_printf("LOCK m0!");
    cyg_mutex_lock( &m0 ); {
        while( 1 == m0d )
        {
            th_printf("cyg_cond_wait cvar0!");
            cyg_cond_wait( &cvar0 );
        }
        th_printf("CHECK m0d(%d) == 2, if not, exit!");
        CHECK( 2 == m0d++ );
        th_printf("cyg_cond_signal cvar0!");
        cyg_cond_signal( &cvar0 );
        while( 3 == m0d )
        {
            th_printf("cyg_cond_wait cvar1!");
            cyg_cond_wait( &cvar1 );
        }
    } 
    th_printf("UNLOCK m0!");
    cyg_mutex_unlock( &m0 );

    finish( (cyg_ucount8)data );
}

static void entry2( cyg_addrword_t data )
{
    th_printf("LOCK m0!");
    cyg_mutex_lock( &m0 ); {
        while( 3 != m0d ) {
            th_printf("cyg_cond_wait cvar0!");
            cyg_cond_wait( &cvar0 );
        }
        th_printf("CHECK m0d(%d) == 3, if not, exit!");
        CHECK( 3 == m0d++ );
        th_printf("cyg_cond_broadcast cvar1!");
        cyg_cond_broadcast( &cvar1 );
    }
    th_printf("UNLOCK m0!");
    cyg_mutex_unlock( &m0 );

    finish( (cyg_ucount8)data );
}

void kmutex1_main( void )
{
    cyg_mutex_init( &m0 );
    cyg_mutex_init( &m1 );

    cyg_cond_init( &cvar0, &m0 );
    cyg_cond_init( &cvar1, &m0 );
    cyg_cond_init( &cvar2, &m1 );

    cyg_thread_create(4, entry0 , (cyg_addrword_t)0, "kmutex1-0",
        (void *)stack[0], STACKSIZE, &thread[0], &thread_obj[0]);
    cyg_thread_resume(thread[0]);

    cyg_thread_create(4, entry1 , (cyg_addrword_t)1, "kmutex1-1",
        (void *)stack[1], STACKSIZE, &thread[1], &thread_obj[1]);
    cyg_thread_resume(thread[1]);

    cyg_thread_create(4, entry2 , (cyg_addrword_t)2, "kmutex1-2",
        (void *)stack[2], STACKSIZE, &thread[2], &thread_obj[2]);
    cyg_thread_resume(thread[2]);

    //CYG_TEST_FAIL_FINISH("Not reached");
}

extern void show_all_thread_infor(void);
void cyg_user_start( void )
{ 
	th_printf("enter!");

    kmutex1_main();
	show_all_thread_infor();
}

/* EOF kmutex1.c */
