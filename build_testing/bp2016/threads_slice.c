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
cyg_thread_entry_t entry_thread_1;
cyg_thread_entry_t entry_thread_2;

#define THREAD_DEBUG

#ifdef THREAD_DEBUG
#define th_printf(fmt, args...)		\
	diag_printf("[Thread slice] %s:%d	" fmt "\n", __func__, __LINE__, ##args);
#else
#define th_printf(fmt, ...)
#endif

extern void boot_ap_fw_ecos2linux(void);
extern void show_all_thread_infor(void);
/* we install our own startup routine which sets up threads */
//int thread_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
void cyg_user_start(void)
{
  u64   s, e;
  th_printf("create Thread!");

  cyg_thread_create(6, entry_thread_1, (cyg_addrword_t) 0,
		    "Thread_1 pri_6", (void *) stack[0], DEF_THREAD_SIZE,
		    &simple_th_1, &thread_s[0]);
  cyg_thread_create(6, entry_thread_2, (cyg_addrword_t) 1,
		    "Thread_2 pri_6", (void *) stack[1], DEF_THREAD_SIZE,
		    &simple_th_2, &thread_s[1]);

  th_printf("resume thread_1.");
  s = cyg_current_time();
  cyg_thread_resume(simple_th_1);
#if 0
  e = cyg_current_time();
  th_printf("thread_1 resume time %llu", e-s);

  th_printf("resume thread_2.");
  s = cyg_current_time();
#endif
  cyg_thread_resume(simple_th_2);
  e = cyg_current_time();
  th_printf("thread_2 resume time %llu", e-s);
  show_all_thread_infor();
}

static u64   time_th1=0, time_th2=0;
static th1_flag=0, th2_flag=0;
static u64 time_th1_tmp, time_th1_tmp1;
static u64 time_th2_tmp, time_th2_tmp1;

/* this is a simple program which runs in a thread */
void entry_thread_1(cyg_addrword_t data)
{
	th_printf("#### tick %llu", cyg_current_time());
	while(1)
	{   
		//th_printf("time thread 1: %llu", time_th1);
		time_th1 = cyg_current_time();
		if(!th1_flag)
		{
			time_th1_tmp = time_th1;
			time_th2_tmp = time_th2;

			if(time_th2_tmp > time_th2_tmp1)
			{
				th_printf("time thread 2 tick: %llu", time_th2_tmp - time_th2_tmp1);
			}else{
				th_printf("time thread 2 tick: %llu", time_th2_tmp1 - time_th2_tmp);
			}

			th1_flag = 1;
			th2_flag = 0;
		}
	}
}

void entry_thread_2(cyg_addrword_t data)
{
	th_printf("#### tick %llu", cyg_current_time());
	while(1)
	{
		time_th2 = cyg_current_time();
		if(!th2_flag)
		{
			time_th1_tmp1 = time_th1;
			time_th2_tmp1 = time_th2;
			if(time_th1_tmp1 > time_th1_tmp)
			{
				th_printf("time thread 1 tick: %llu", time_th1_tmp1 - time_th1_tmp);
			}else{
				th_printf("time thread 1 tick: %llu", time_th1_tmp - time_th1_tmp1);
			}

			th2_flag = 1;
			th1_flag = 0;
		}
		//th_printf("time thread 2: %llu", time_th2);
	}
}
