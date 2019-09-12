/*==========================================================================
//
//        kmbox1.cxx
//
//        Kernel Mbox test 1
//
//==========================================================================
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
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author:        dsm
// Contributors:    dsm
// Date:          1998-06-02
// Description:   Tests basic mbox functionality.
//####DESCRIPTIONEND####
*/
#if 0
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL

#include <cyg/kernel/kapi.h>

//#include <cyg/infra/testcase.h>

#define NTHREADS 2
#define STACKSIZE CYGNUM_HAL_STACK_SIZE_TYPICAL

#define THREAD_DEBUG

#ifdef THREAD_DEBUG
#define th_printf(fmt, args...)		\
	diag_printf("[Thread mbox] %s:%d	" fmt "\n", __func__, __LINE__, ##args);
#else
#define th_printf(fmt, ...)
#endif

static cyg_handle_t thread[NTHREADS];

static cyg_thread thread_obj[NTHREADS];
static char stack[NTHREADS][STACKSIZE];

static cyg_handle_t m0, m1, m2;
static cyg_mbox mbox0, mbox1, mbox2;

static cyg_atomic q = 0;

#ifndef CYGMTH_MBOX_PUT_CAN_WAIT
#define cyg_mbox_PUT cyg_mbox_tryput
#endif

static void entry0( cyg_addrword_t data )
{
    cyg_count8 u,i,j;

    th_printf("Testing put() and tryput() without wakeup");
	// Queries to see if other threads are waiting to receive a message in the specified message box. 
	// If so, TRUE is returned; otherwise, FALSE is returned
	th_printf("cyg_mbox_waiting_to_get returned flase");
    CYG_TEST_CHECK(!cyg_mbox_waiting_to_get(m0), "mbox not initialized properly");

	th_printf("cyg_mbox_peek return 0");
	//Returns the number of messages in the specified message box
    CYG_TEST_CHECK(0==cyg_mbox_peek(m0), "mbox not initialized properly");

	th_printf("cyg_mbox_peek_item return NULL");
	//Checks to see if a message is available in the specified message box. 
	//If a message is present, the address of the data is returned without removing the message from the message box. 
	//If no message is available, NULL is returned
    CYG_TEST_CHECK(NULL==cyg_mbox_peek_item(m0), "mbox not initialized properly");

	th_printf("cyg_mbox_PUT 55");
    cyg_mbox_PUT(m0, (void *)55);
    CYG_TEST_CHECK(1==cyg_mbox_peek(m0), "peek() wrong");
    CYG_TEST_CHECK(55==(cyg_count8)cyg_mbox_peek_item(m0), "peek_item() wrong");
    for(u=1; cyg_mbox_tryput(m0, (void*)u); u++) {
		th_printf("cyg_mbox_tryput u = %d", u);
        CYG_TEST_CHECK(55==(cyg_count8)cyg_mbox_peek_item(m0), "peek_item() wrong");
        CYG_TEST_CHECK(u+1==cyg_mbox_peek(m0), "peek() wrong");
    }
	th_printf("u=%d CYGNUM_KERNEL_SYNCH_MBOX_QUEUE_SIZE=%d", u, CYGNUM_KERNEL_SYNCH_MBOX_QUEUE_SIZE);
    CYG_TEST_CHECK(u == CYGNUM_KERNEL_SYNCH_MBOX_QUEUE_SIZE, "mbox not configured size");

    // m0 now contains ( 55 1 2 .. u-1 )
	th_printf("cyg_mbox_peek return %d ", u);
    CYG_TEST_CHECK(u==cyg_mbox_peek(m0), "peek() wrong");
    CYG_TEST_CHECK(55==(cyg_count8)cyg_mbox_peek_item(m0), "peek_item() wrong");

    th_printf("Testing get(), tryget()");
    
    i = (cyg_count8)cyg_mbox_tryget(m0); //remove the message from the mbox
    CYG_TEST_CHECK( 55 == i, "Got wrong message" );
    for(j=1; j<u;j++) {
        CYG_TEST_CHECK( j == (cyg_count8)cyg_mbox_peek_item(m0), "peek_item()" );
        CYG_TEST_CHECK( cyg_mbox_peek(m0) == u - j, "peek() wrong" );
        i = (cyg_count8)cyg_mbox_get(m0);
		th_printf("j=%d, Mbox value %d", j, u);
        CYG_TEST_CHECK( j == i, "Got wrong message" );
    }
    
	th_printf("Checks if the Mbox is NULL, at this time the mbox is NULL");
    CYG_TEST_CHECK( NULL == cyg_mbox_peek_item(m0), "peek_item()" );
    CYG_TEST_CHECK( 0 == cyg_mbox_peek(m0), "peek()");
    
    // m0 now empty

	th_printf("cyg_mbox_waiting_to_put, must be FALSE");
    CYG_TEST_CHECK(!cyg_mbox_waiting_to_put(m0), "waiting_to_put()");
	th_printf("cyg_mbox_waiting_to_get, must be FALSE");
    CYG_TEST_CHECK(!cyg_mbox_waiting_to_get(m0), "waiting_to_get()");

    th_printf("Testing get(), blocking");
    
	th_printf("check synchronization, q=0");
    CYG_TEST_CHECK(0==q++, "bad synchronization");

	th_printf("cyg_mbox_PUT 99");
    cyg_mbox_PUT(m1, (void*)99);                  // wakes t1

	th_printf("cyg_mbox_get(m0) , m0 is NULL, wait t1 wakeup");
    i = (cyg_count8)cyg_mbox_get(m0);          // sent by t1
    CYG_TEST_CHECK(3==i, "Recieved wrong message");
	th_printf("check synchronization, q=2");
    CYG_TEST_CHECK(2==q++, "bad synchronization");

#ifdef CYGFUN_KERNEL_THREADS_TIMER
	th_printf("cyg_mbox_timed_get(m0, cyg_current_time()+10)");
    CYG_TEST_CHECK(NULL==cyg_mbox_timed_get(m0, cyg_current_time()+10),
                   "unexpectedly found message");
	th_printf("check synchronization, q=3");
    CYG_TEST_CHECK(3==q++, "bad synchronization");
    // Allow t1 to run as this get times out
    // t1 must not be waiting...
	th_printf("cyg_mbox_waiting_to_get return TRUE");
    CYG_TEST_CHECK(cyg_mbox_waiting_to_get(m0), "waiting_to_get()");

	th_printf("cyg_mbox_PUT(m0, (void*)7)");
    cyg_mbox_PUT(m0, (void*)7);                   // wake t1 from timed get
#ifdef CYGMTH_MBOX_PUT_CAN_WAIT
    q=10;
    while(cyg_mbox_tryput(m0, (void*)6))          // fill m0's queue
        ;
    // m0 now contains ( 6 ... 6 )
	th_printf("check synchronization, q=10");
    CYG_TEST_CHECK(10==q++, "bad synchronization");
    cyg_mbox_put(m1, (void*)4);                   // wake t1
    CYG_TEST_CHECK(!cyg_mbox_timed_put(m0, (void*)8, cyg_current_time()+10),
                   "timed put() unexpectedly worked");
	th_printf("check synchronization, q=12");
    CYG_TEST_CHECK(12==q++, "bad synchronization");
    // m0 still contains ( 6 ... 6 )
    cyg_mbox_put(m0, (void*)9);
	th_printf("check synchronization, q=13");
    CYG_TEST_CHECK(13==q++, "bad synchronization");
#endif
#endif
    i=(cyg_count8)cyg_mbox_get(m2);
    CYG_TEST_FAIL_FINISH("Not reached");
}

static void entry1( cyg_addrword_t data )
{
    cyg_count8 i;
	th_printf("enter!");
    i = (cyg_count8)cyg_mbox_get(m1);
	th_printf("cyg_mbox_get i=%d", i);
	th_printf("check synchronization, q=1");
    CYG_TEST_CHECK(1==q++, "bad synchronization");

	th_printf("cyg_mbox_PUT 3");
    cyg_mbox_PUT(m0, (void *)3);                  // wake t0

#if defined(CYGFUN_KERNEL_THREADS_TIMER)
    th_printf("Testing timed functions");
    CYG_TEST_CHECK(7==(cyg_count8)cyg_mbox_timed_get(m0,cyg_current_time()+20),
                   "timed get()");
	th_printf("check synchronization, q=4");
    CYG_TEST_CHECK(4==q++, "bad synchronization");
#ifdef CYGMTH_MBOX_PUT_CAN_WAIT
    CYG_TEST_CHECK(4==(cyg_count8)cyg_mbox_get(m1));
	th_printf("4==(cyg_count8)cyg_mbox_get(m1)");

	th_printf("check synchronization, q=11");
    CYG_TEST_CHECK(11==q++, "bad synchronization");
    thread[0]->delay(20);    // allow t0 to reach put on m1
	th_printf("check synchronization, q=14");
    CYG_TEST_CHECK(14==q++, "bad synchronization");

	th_printf("cyg_mbox_waiting_to_put(m0), waiting_to_put()");
    CYG_TEST_CHECK(cyg_mbox_waiting_to_put(m0), "waiting_to_put()");
    do {
        // after first get m0 contains ( 6 .. 6 9 )
        i=(cyg_count8)cyg_mbox_tryget(m0);
		th_printf("cyg_mbox_tryget(m0) i=%d", i);
    } while(6==i);
	
	th_printf("check if i(%d)==9", i);
    CYG_TEST_CHECK(9==i,"put gone awry");
#endif
#endif
    CYG_TEST_PASS_FINISH("Kernel C API Mbox 1 OK");
}

void cyg_user_start( void )
{
	th_printf("enter!");

    cyg_mbox_create( &m0, &mbox0 );
    cyg_mbox_create( &m1, &mbox1 );
    cyg_mbox_create( &m2, &mbox2 );

    cyg_thread_create(4, entry0 , (cyg_addrword_t)0, "kmbox1-0",
        (void *)stack[0], STACKSIZE, &thread[0], &thread_obj[0]);
    cyg_thread_resume(thread[0]);

    cyg_thread_create(5, entry1 , (cyg_addrword_t)1, "kmbox1-1",
        (void *)stack[1], STACKSIZE, &thread[1], &thread_obj[1]);
    cyg_thread_resume(thread[1]);
}

/* EOF kmbox1.c */
#else

#include "common.h"
#include <cyg/kernel/kapi.h>

#define cyg_mbox_PUT cyg_mbox_tryput


static cyg_handle_t m0, m1, m2;
static cyg_mbox mbox0, mbox1, mbox2;
int do_mbox_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
	unsigned int data;
	int i = 0;
	int m0_cnt;
    cyg_mbox_create( &m0, &mbox0);
    cyg_mbox_create( &m1, &mbox1);
    cyg_mbox_create( &m2, &mbox2);

    cyg_mbox_delete(m0);

    if(0!=cyg_mbox_peek(m0)){
    	printf("mbox0 Init error!\n");
    	return -1;
    }
//	if (0 != cyg_mbox_peek(m1)) {
//		printf("mbox1 Init error!\n");
//		return -1;
//	}

    while(cyg_mbox_tryput(m0, (void*)(100+i))){          // fill m0's queue
    	i++;
	}
	printf("\nmbox0 put total %d item, peek %d!\n", i, cyg_mbox_peek(m0));

	m0_cnt = cyg_mbox_peek(m0);
	if (i != m0_cnt) {
		printf("mbox0 peek error!\n");
		return -1;
	}

	for(i = 0; i < m0_cnt; i++){
		data = (unsigned int)cyg_mbox_tryget(m0);
		printf("-%d-mbox0 get item:%d!\n", i, data);
	}

	for(i = 0; i < 3; i++){
		data = (unsigned int)cyg_mbox_tryget(m0);
		if(0 != data){
			printf("-%d-should be 0, but %d!\n", i, data);
		}
		printf("get more -%d- mbox0 get item:%d!\n", i, data);
	}

    return 0;
}

#endif
