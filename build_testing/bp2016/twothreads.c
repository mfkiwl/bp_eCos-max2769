#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>

#define  DEF_THREAD_SIZE        (8192)

/* now declare (and allocate space for) some kernel objects,
   like the two threads we will use */
cyg_thread thread_s[3];		/* space for two thread objects */

char stack[3][DEF_THREAD_SIZE];		/* space for two 4K stacks */

/* now the handles for the threads */
cyg_handle_t simple_threadA, simple_threadB, simple_threadC;

/* and now variables for the procedure which is the thread */
cyg_thread_entry_t simple_program;

/* and now a mutex to protect calls to the C library */
cyg_mutex_t cliblock;

/* we install our own startup routine which sets up threads */
void cyg_user_start(void)
{
  printf("Entering twothreads' cyg_user_start() function\n");

  cyg_mutex_init(&cliblock);
  printf("Entering twothreads' cyg_mutex_init() function\n");

  cyg_thread_create(10, simple_program, (cyg_addrword_t) 0,
		    "Thread L_pri_10", (void *) stack[0], DEF_THREAD_SIZE,
		    &simple_threadA, &thread_s[0]);
  cyg_thread_create(6, simple_program, (cyg_addrword_t) 1,
		    "Thread M_pri_6", (void *) stack[1], DEF_THREAD_SIZE,
		    &simple_threadB, &thread_s[1]);
  cyg_thread_create(4, simple_program, (cyg_addrword_t) 2,
		    "Thread H_pri_4", (void *) stack[2], DEF_THREAD_SIZE,
		    &simple_threadC, &thread_s[2]);


  printf("Entering twothreads' create three thread, then resume its\n");
  cyg_thread_resume(simple_threadA);
}

extern void boot_ap_fw_ecos2linux(void);
extern void show_all_thread_infor(void);
/* this is a simple program which runs in a thread */
void simple_program(cyg_addrword_t data)
{
    int message = (int) data;

    u64   s, e;
    u64   lock_t = 0;
    printf("Beginning execution; thread data is %d\n", message);

    switch (message) {
        case 0:
            show_all_thread_infor();
            printf("%d: Thread L_pri_10 get lock\n", message);
            break;
        case 1:
            printf("%d: resume Thread H_pri_4\n", message);
            cyg_thread_resume(simple_threadC);
            printf("%d: Thread M_pri_6 get lock\n", message);
            break;
        case 2:
            printf("%d: delay 10, and mid thread can get lock\n", message);
            cyg_thread_delay(10);
            printf("%d: Thread H_pri_4 get lock\n", message);
            break;
    }

    s = arch_counter_get_cntpct();
    cyg_mutex_lock(&cliblock); 
    e = arch_counter_get_cntpct();
    lock_t =  (e - s);
    switch(message) {
        case 0:
            printf("%d: resume Thread M_pri_6\n", message);
            cyg_thread_resume(simple_threadB);
            printf("%d: Thread L_pri_10 :, delay 100 and enter sleep state\n", message);
            cyg_thread_delay(100);
            printf("%d: Thread L_pri_10:", message);
            break;
        case 1:
            printf("%d: Thread M_pri_6:", message);
            break;
        case 2:
            printf("%d: Thread H_pri_4:", message);
            break;
        default:
            break;
    }
    printf("get lock interval: %d\n\n", (u32)lock_t );
    cyg_mutex_unlock(&cliblock);
	printf("%d: unlock!\n", message);
    cyg_thread_delay(1000000);
}
