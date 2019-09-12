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
cyg_sem_t sem_lock;

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

  cyg_semaphore_init(&sem_lock, 0);

  th_printf("create semaphore and Thread!");

  cyg_thread_create(4, simple_program, (cyg_addrword_t) 0,
		    "Thread H_pri_4", (void *) stack[0], DEF_THREAD_SIZE,
		    &simple_th_H_4, &thread_s[0]);
  cyg_thread_create(6, simple_program_B, (cyg_addrword_t) 1,
		    "Thread M_pri_6", (void *) stack[1], DEF_THREAD_SIZE,
		    &simple_th_M_6, &thread_s[1]);
#ifdef CYGIMP_KERNEL_SCHED_SORTED_QUEUES
  cyg_thread_create(4, simple_program, (cyg_addrword_t) 2,
		    "Thread_2 L_pri_4", (void *) stack[2], DEF_THREAD_SIZE,
		    &simple_th_L_10, &thread_s[2]);
#else
  cyg_thread_create(10, simple_program, (cyg_addrword_t) 2,
		    "Thread L_pri_10", (void *) stack[2], DEF_THREAD_SIZE,
		    &simple_th_L_10, &thread_s[2]);
#endif

  th_printf("resume thread_2.");
  cyg_thread_resume(simple_th_L_10);
}

extern void boot_ap_fw_ecos2linux(void);
extern void show_all_thread_infor(void);

static int flag_0=0, flag_1=0;

/* this is a simple program which runs in a thread */
void simple_program(cyg_addrword_t data)
{
    int message = (int) data;

    u64   s, e, u, v;
    u64   lock_t = 0;
    th_printf("Beginning execution; thread_%d\n", message);

	while(1)
	{
		switch (message) {
		case 0:
			th_printf("######### Thread_0 L_pri_4 is running! ###########");
			if(!flag_1)
			{
				th_printf("resume Thread_1 simple_th_M_6!");
				cyg_thread_resume(simple_th_M_6);
				flag_1=1;
			}
			break;
		case 2:
			th_printf("######### Thread_2 L_pri_10 is running! ###########");
			if(!flag_0)
			{
				th_printf("resume Thread_2 simple_th_H_4!");
				cyg_thread_resume(simple_th_H_4);
				flag_0=1;
			}
			break;
		}

		th_printf("Thread_%d wait the semaphore!", message);
		cyg_semaphore_wait(&sem_lock);
		th_printf("Thread_%d get the semaphore!", message);
		cyg_thread_delay(50);
	}

}

void simple_program_B(cyg_addrword_t data)
{
    int message = (int) data;
    u64   s, e,u, v;
	int i=0;

    th_printf("Beginning execution; thread_%d\n", message);

	while(1)
	{
		s = cyg_current_time();
		cyg_thread_delay(100);
		e = cyg_current_time();
		th_printf("Thread_%d post semaphore! run tick %llu", message, e-s);
		cyg_semaphore_post(&sem_lock);
	}
}
