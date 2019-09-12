//==========================================================================
//
//        thread_cli.c
//
//        thread test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2019-03-09
// Description:   thread test for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <cyg/kernel/kapi.h>
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/infra/diag.h>

#include <cyg/hal/regs/timer.h>
#include <cyg/hal/timer/timer_imp.h>
#include <pkgconf/hal_arm.h>

#ifdef CYGFUN_KERNEL_THREADS_CPULOAD
extern void show_all_thread_infor(void);
extern void show_all_thread_cpuload(void);
#define MAX_COMM_MSG_LEN 460
// in case of stack overflow, define huge array as global
static volatile int iComm = 0, jComm = 0, CommFlag = 0;
static unsigned char CommMsgStr1[MAX_COMM_MSG_LEN];
static unsigned char CommMsgStr[MAX_COMM_MSG_LEN];
static unsigned char crc_buf[32];

static unsigned int bb_src[53] = {
    0x646d91b6, 0x46d91b64, 0x6d91b646, 0xd91b646d, 0x91b646d9, 0x1b646d91, 0xb646d91b, 0x646d91b6,
    0x46d91b64, 0x6d91b646, 0xd91b646d, 0x91b646d9, 0x1b646d91, 0xb646d91b, 0x646d91b6, 0x46d91b64,
    0x6d91b646, 0xd91b646d, 0x91b646d9, 0x1b646d91, 0xb646d91b, 0x646d91b6, 0x46d91b64, 0x6d91b646,
    0xd91b646d, 0x91b646d9, 0x1b646d91, 0xb646d91b, 0x646d91b6, 0x46d91b64, 0x6d91b646, 0xd91b646d,
    0x91b646d9, 0x1b646d91, 0xb646d91b, 0x646d91b6, 0x46d91b64, 0x6d91b646, 0xd91b646d, 0x91b646d9,
    0x1b646d91, 0xb646d91b, 0x646d91b6, 0x46d91b64, 0x6d91b646, 0xd91b646d, 0x91b646d9, 0x1b646d91,
    0xb646d91b, 0x646d91b6, 0x46d91b64, 0x6d91b646, 0xd91b0000};

void LCommMsgPro(unsigned char *pDst, unsigned int *pSrc, int cnt, int type)
{
    int i;

    for(iComm=0,jComm=0; iComm<cnt; iComm++)
    {
        if(pSrc[iComm>>5] & (1 << (31 - (iComm%32))))	
            pDst[jComm] |= 1 << (6 - (iComm%7));
        if((iComm % 7) == 6)	
            pDst[jComm++] += 0xa0;
    }		

    for(iComm=0; iComm<jComm; iComm++)
    {
        if(pDst[iComm] != 0xd2 && pDst[iComm] != 0xbb && type == 1)
        {
            printf("error 0 ~~~~~~~\n");
            for(i=0; i<53; i++)
            {
                if(pSrc[i] != bb_src[i])
                {
                    printf("error 1 ~~~~~~~\n");
                }	
            }

            return ;	
        }		
    }	
    //printf("OK\n");
}


#define MAX_THREAD_CNT     8
static cyg_thread thread_s[MAX_THREAD_CNT];     /* space for thread objects */
static char stack[MAX_THREAD_CNT][8192];     /* space for stacks */
/* now the handles for the threads */
static cyg_handle_t handle_thread[MAX_THREAD_CNT];
/* and now variables for the procedure which is the thread */

static bool thread_pri_flag[MAX_THREAD_CNT] = {false}; 



void temp_program(cyg_uint32 id)
{
    //u64 s, e;
    //cyg_uint32 time_out = rand()%10;
    //cyg_uint32 time_out = 10;
    while (1) {
#if 1
        if(thread_pri_flag[id] == true){
            //time_out = rand()%10;
            thread_pri_flag[id] = false;
        }

        //s = arch_counter_get_cntpct();
        memset(CommMsgStr, 0, MAX_COMM_MSG_LEN);
        memset(CommMsgStr1, 0, MAX_COMM_MSG_LEN);
        LCommMsgPro(CommMsgStr1, bb_src, 1680, 0);
        memset(crc_buf, 0, 32);
        LCommMsgPro(CommMsgStr, bb_src, 1680, 1);

        //e = arch_counter_get_cntpct();
        //cyg_thread_delay(time_out+1);
#else
        printf("id:%d -- delay(%d)\n", id, time_out+50);
        cyg_thread_delay(time_out+50);
#endif
    }
}

static char name[MAX_THREAD_CNT][20];
/* we install our own startup routine which sets up threads */
void thread_list(cyg_uint32 id)
{
    cyg_uint32 pri = rand()%31;

    memset(name[id], 0, 20);
    sprintf(name[id], "Thread_%d", id);
    printf("name:%s\n", name[id]);
    cyg_thread_create(pri, temp_program, (cyg_addrword_t) id,
            name[id], (void *) (&stack[id][0]), 8192,
            &handle_thread[id], &thread_s[id]);

    cyg_thread_resume(handle_thread[id]);
    //printf("id:%d,base:0x%x,resume done.\n",id, (cyg_uint32)(&stack[id][0]));
    //show_all_thread_infor();
    show_all_thread_cpuload();
}

void thread_pri(cyg_uint32 id)
{
    cyg_uint32 pri = rand()%31;
    thread_pri_flag[id] = true;

    cyg_thread_set_priority(handle_thread[id], pri);
    cyg_thread_resume(handle_thread[id]);
}


int do_mthread_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    static bool thread_init_flag = false;
    cyg_uint32 loop = 1;
    cyg_uint32 i = 0;

    // set rand seed
    srand(arch_counter_get_cntpct() & 0xFFFFFFFF);

    if (argc == 1){
        show_all_thread_cpuload();
        show_all_thread_infor();
        return 0;
    }

    if (strcmp(argv[1], "reset") == 0) {
        cyg_thread_cpuload_reset();
        show_all_thread_cpuload();
        return 0;
    }

    if (strcmp(argv[1], "cpu") == 0) {
        show_all_thread_cpuload();

        return 0;
    }
    if (strcmp(argv[1], "info") == 0) {
        if(argc > 2){
            loop = simple_strtoul(argv[2], NULL, 10);
        }

        for(i = 0; i < loop; i++){
            cyg_thread_delay(100);
            show_all_thread_infor();
        }
    }

    if (strcmp(argv[1], "rand") == 0) {
        //show_all_thread_cpuload();
        //return 0;
        if(false == thread_init_flag){
            for(i = 0; i < MAX_THREAD_CNT; i++){
                thread_list(i);
        //show_all_thread_cpuload();
        //show_all_thread_infor();
            }
            thread_init_flag = true;
        //show_all_thread_cpuload();
        //show_all_thread_infor();
            printf("create thread done.\n");
       // show_all_thread_cpuload();
        //show_all_thread_infor();
        }else{
            for(i = 0; i < MAX_THREAD_CNT; i++){
                thread_pri(i);
            }
            printf("set thread pri done.\n");
        }
        return 0;
    }

    return 0;
}
#endif
