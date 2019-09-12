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
cyg_handle_t simple_th_H_4, simple_th_M_6, simple_th_L_10;

/* and now variables for the procedure which is the thread */
cyg_thread_entry_t simple_program;
cyg_thread_entry_t simple_program_B;

/* and now a mutex to protect calls to the C library */
cyg_mutex_t m1_lock;
cyg_mutex_t m2_lock;

#define THREAD_DEBUG

#ifdef THREAD_DEBUG
#define th_printf(fmt, args...)		\
	diag_printf("[Thread test] %s:%d	" fmt "\n", __func__, __LINE__, ##args);
#else
#define th_printf(fmt, ...)
#endif

/* we install our own startup routine which sets up threads */
//int thread_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
void cyg_user_start(void)
{
  th_printf("Entering!");

  cyg_mutex_init(&m1_lock);
  cyg_mutex_init(&m2_lock);

  th_printf("create mutex and Thread!");

  cyg_thread_create(4, simple_program, (cyg_addrword_t) 0,
		    "Thread H_pri_4", (void *) stack[0], DEF_THREAD_SIZE,
		    &simple_th_H_4, &thread_s[0]);
  cyg_thread_create(6, simple_program_B, (cyg_addrword_t) 1,
		    "Thread M_pri_6", (void *) stack[1], DEF_THREAD_SIZE,
		    &simple_th_M_6, &thread_s[1]);
  cyg_thread_create(10, simple_program, (cyg_addrword_t) 2,
		    "Thread L_pri_10", (void *) stack[2], DEF_THREAD_SIZE,
		    &simple_th_L_10, &thread_s[2]);


  th_printf("resume thread_2.");
  cyg_thread_resume(simple_th_L_10);
}

extern void boot_ap_fw_ecos2linux(void);
extern void show_all_thread_infor(void);
/* this is a simple program which runs in a thread */
void simple_program(cyg_addrword_t data)
{
    int message = (int) data;

    u64   s, e;
    u64   lock_t = 0;
    th_printf("Beginning execution; thread_%d\n", message);

    switch (message) {
        case 0:
            th_printf("%d: Thread_0 L_pri_4 get m1 lock\n", message);
            break;
        case 2:
            show_all_thread_infor();
            th_printf("%d: Thread_2 L_pri_10 get m1 lock\n", message);
            break;
    }

    s = arch_counter_get_cntpct();
    cyg_mutex_lock(&m1_lock); 
    e = arch_counter_get_cntpct();
    lock_t =  (e - s);
    switch(message) {
        case 0:
            th_printf("%d: Thread_0 M_pri_4:", message);
            break;
        case 1:
            th_printf("%d: Thread_1 M_pri_6:", message);
            break;
        case 2:
            th_printf("%d: resume Thread_1 M_pri_6\n", message);
            cyg_thread_resume(simple_th_M_6);
            th_printf("%d: Thread_2 L_pri_10 get m2 lock\n", message);
			cyg_mutex_lock(&m2_lock); 
            th_printf("%d: Thread_2 L_pri_10\n", message);
			cyg_mutex_unlock(&m2_lock); 
            break;
        default:
            break;
    }
    th_printf("Thread_%d unlock m1 interval: %d\n\n", message, (u32)lock_t );
    cyg_mutex_unlock(&m1_lock);
	th_printf("Thread_%d: unlock!\n", message);
	th_printf("show all threads info after unlock Thread_%d", message);
	show_all_thread_infor();
    cyg_thread_delay(1000000);
}


void simple_program_B(cyg_addrword_t data)
{
    int message = (int) data;

    u64   s, e;
    u64   lock_t = 0;
    th_printf("Beginning execution; thread_%d\n", message);

    switch (message) {
        case 1:
            th_printf("%d: Thread_1 L_pri_6 get m2 lock\n", message);
            break;
		default:
			th_printf("error! exit ...");
            break;
    }

    s = arch_counter_get_cntpct();
    cyg_mutex_lock(&m2_lock); 
    e = arch_counter_get_cntpct();
    lock_t =  (e - s);
    switch(message) {
        case 1:
            th_printf("%d: Thread_1 L_pri_6:, delay 100 and enter sleep state\n", message);
            cyg_thread_delay(100);
            th_printf("%d: resume Thread_0 M_pri_4\n", message);
            cyg_thread_resume(simple_th_H_4);
            break;
        default:
            break;
    }
    th_printf("Thread_%d unlock m2 interval: %d\n", message, (u32)lock_t );
    th_printf("Check all thread priority *************:\n");
	show_all_thread_infor();
    cyg_mutex_unlock(&m2_lock);
	th_printf("Thread_%d: unlock!\n", message);
	th_printf("show all threads info after unlock Thread_%d", message);
	show_all_thread_infor();
    cyg_thread_delay(1000000);
}
