#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>


/* now declare (and allocate space for) some kernel objects,
   like the two threads we will use */
cyg_thread thread_s[2];		/* space for two thread objects */

char stack[2][4096];		/* space for two 4K stacks */
u32  test_buf[8192];

/* now the handles for the threads */
cyg_handle_t simple_threadA, simple_threadB;

/* and now variables for the procedure which is the thread */
cyg_thread_entry_t simple_program, simple_program1;

/* and now a mutex to protect calls to the C library */
cyg_mutex_t cliblock;

/* we install our own startup routine which sets up threads */
void cyg_user_start(void)
{
  printf("Entering twothreads' cyg_user_start() function\n");

  cyg_mutex_init(&cliblock);
  printf("Entering twothreads' cyg_mutex_init() function\n");

  cyg_thread_create(4, simple_program, (cyg_addrword_t) 0,
		    "Thread A", (void *) stack[0], 4096,
		    &simple_threadA, &thread_s[0]);
  cyg_thread_create(10, simple_program1, (cyg_addrword_t) 1,
		    "Thread B", (void *) stack[1], 4096,
		    &simple_threadB, &thread_s[1]);

  printf("Entering twothreads' create two thread, then resume its\n");
  cyg_thread_resume(simple_threadA);
  cyg_thread_resume(simple_threadB);
}

void simple_program1(cyg_addrword_t data)
{
  int message = (int) data;
  int delay;

  u64   s, e;
  u32   counter = 0;

 
  cyg_thread_delay(200);

  printf("Beginning execution; thread data is %d\n", message);
  counter = 0;

  for (;;) {
    cyg_thread_delay(200);
    hal_interrupt_stats_display();
  }
}

void iram_testing(int loops)
{
    u64  s, e;
    u32  us, i, j;
    u32 * va_s, count;
    
    va_s = (u32*)test_buf;
    count = 8192;
   
    for(j = 0; j < loops; j++) {
        cyg_interrupt_disable(); 
        s = arch_counter_get_cntpct();
        i = 0;
        for(i = 0; i< count; i++)
            va_s[i] = i;
        e = arch_counter_get_cntpct();

        us = ((u32)(e-s))/20;
        cyg_interrupt_enable(); 
        printf("%d: iram testing need %d us\n", j, us);
    }
    printf("\n\n");
}


void bb_ram_testing(int loops)
{
    u64  s, e;
    u32  us, i, j;
    u32 * va_s, count;
    
    va_s = (u32*)0x600000;
    count = 8192;
   
    for(j = 0; j < loops; j++) {
        cyg_interrupt_disable(); 
        s = arch_counter_get_cntpct();
        i = 0;
        for(i = 0; i< count; i++)
            va_s[i] = i;
        e = arch_counter_get_cntpct();

        us = ((u32)(e-s))/20;
        cyg_interrupt_enable(); 
        printf("%d: bb ram testing need %d us\n", j, us);
    }
    printf("\n\n");
}
void print_ttb_info(int size)
{
   	int i = 0;
   	u32   addr;
    u32 * ttb = (u32*) hal_get_pagetable_base();
    addr = (u32)ttb;
    printf("************ttb*************\n");
	for(i = 0; i < size; i++) {
		if(i % 4 == 0) 
			printf("\r\n%08x: ", addr + i * 4);
		printf("%08x ", *ttb ++); 
	}
	printf("\r\n");

}
/* this is a simple program which runs in a thread */
void simple_program(cyg_addrword_t data)
{
    int message = (int) data;

    printf("Beginning execution; thread data is %d\n", message);

    cyg_thread_delay(400);

    print_ttb_info(20);
    bb_ram_testing(10);
    iram_testing(10);
    modify_bbram_mmu_section();
    printf("modify bbram_mmu\n");
    print_ttb_info(20);
    bb_ram_testing(10);
    iram_testing(10);

    for (;;) {
        cyg_thread_delay(100);
    }
}
