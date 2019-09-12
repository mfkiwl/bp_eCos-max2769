#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>

#define  DEF_THREAD_SIZE        (8192)

/* now declare (and allocate space for) some kernel objects,
   like the two threads we will use */
cyg_thread thread_s[2];		/* space for two thread objects */

char stack[2][DEF_THREAD_SIZE];		/* space for two 4K stacks */

/* now the handles for the threads */
cyg_handle_t simple_th_1, simple_th_2;

/* and now variables for the procedure which is the thread */
cyg_thread_entry_t entry_thread;

#define THREAD_DEBUG

#ifdef THREAD_DEBUG
#define th_printf(fmt, args...)		\
	diag_printf("[Thread slice2] %s:%d	" fmt "\n", __func__, __LINE__, ##args);
#else
#define th_printf(fmt, ...)
#endif

extern void boot_ap_fw_ecos2linux(void);
extern void show_all_thread_infor(void);
/* we install our own startup routine which sets up threads */
//int thread_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
void cyg_user_start(void)
{
  th_printf("create Thread!");

  cyg_thread_create(6, entry_thread, (cyg_addrword_t) 0,
		    "Thread_1 pri_6", (void *) stack[0], DEF_THREAD_SIZE,
		    &simple_th_1, &thread_s[0]);
  cyg_thread_create(6, entry_thread, (cyg_addrword_t) 1,
		    "Thread_2 pri_6", (void *) stack[1], DEF_THREAD_SIZE,
		    &simple_th_2, &thread_s[1]);

  th_printf("resume thread_1.");
  cyg_thread_resume(simple_th_1);

  th_printf("resume thread_2.");
  cyg_thread_resume(simple_th_2);

  show_all_thread_infor();
}


/* this is a simple program which runs in a thread */
void entry_thread(cyg_addrword_t data)
{
    int message = (int) data;
	u64   time_1=0, time_2=0;
	int i=0;

	th_printf("#### tick %llu", cyg_current_time());
	while(1)
	{   
		time_1 = cyg_current_time();

		if(time_1 != time_2)
		{
			th_printf("thread_%d tick: %llu [%llu]", message, time_1, time_2);
			time_2 = time_1;
			if((0 == message) && (time_1 > 200))
			{
				i++;
				if(i>2)
				{
					th_printf("thread_%d yield. i=%d", message, i);
					i=0;
					cyg_thread_yield();				
				}
			}
		}
	}
}
