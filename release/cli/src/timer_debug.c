//==========================================================================
//
//        timer_test.c
//
//        timer performance test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2018-02-09
// Description:   timer performance test for bp2016
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
// static HWP_TIMER_AP_T *timer1 = hwp_apTimer1;
// static HWP_TIMER_AP_T *timer2 = hwp_apTimer2;
// static HWP_TIMER_AP_T *timer3 = hwp_apTimer3;

static cyg_int32 hal_ostimer_enable1(cyg_uint32 instance, cyg_uint32 load_val, cyg_uint32 irq_mode)
{
    HWP_TIMER_AP_T *hwp_timer = NULL;
    switch (instance) {
    case 0:
        hwp_timer = hwp_apTimer0;
        break;
    case 1:
        hwp_timer = hwp_apTimer1;
        break;
    case 2:
        hwp_timer = hwp_apTimer2;
        break;
    case 3:
        hwp_timer = hwp_apTimer3;
        break;
    default:
        return -1;
    }

    hwp_timer->ctrl = 0;

    if (load_val != 0)
        hwp_timer->loadval = load_val;

    hwp_timer->irq_eoi = TIMER_AP_IRQ_CAUSE;

    if (irq_mode == IRQ_MODE)
        hwp_timer->irq_enable = TIMER_AP_IRQ_ENABLE;
    else
        hwp_timer->irq_enable = 0;

    hwp_timer->ctrl = TIMER_AP_ENABLE | TIMER_AP_REPEAT | TIMER_AP_LOAD;
    return 0;
}

static cyg_int32 hal_ostimer_disable1(cyg_uint32 instance)
{
    HWP_TIMER_AP_T *hwp_timer = NULL;
    switch (instance) {
    case 0:
        hwp_timer = hwp_apTimer0;
        break;
    case 1:
        hwp_timer = hwp_apTimer1;
        break;
    case 2:
        hwp_timer = hwp_apTimer2;
        break;
    case 3:
        hwp_timer = hwp_apTimer3;
        break;
    default:
        return -1;
    }

    hwp_timer->irq_enable = 0;
    hwp_timer->irq_eoi = TIMER_AP_IRQ_CAUSE;
    hwp_timer->ctrl = 0;
    return 0;
}

#define MAX_COMM_MSG_LEN 460
// in case of stack overflow, define huge array as global
volatile int iComm = 0, jComm = 0, CommFlag = 0;
extern unsigned int RdssPhaseTeQ[2];
int MaxLenMsg[2][2][2] = {{{142, 110}, {142, 62}}, {{220, 188}, {220, 140}}};
unsigned int data[7];
unsigned char CommMsgStr1[MAX_COMM_MSG_LEN];
unsigned char CommMsgStr[MAX_COMM_MSG_LEN];
unsigned char crc_buf[32];

unsigned int bbsrc[53] = {
    0x646d91b6, 0x46d91b64, 0x6d91b646, 0xd91b646d, 0x91b646d9, 0x1b646d91, 0xb646d91b, 0x646d91b6,
    0x46d91b64, 0x6d91b646, 0xd91b646d, 0x91b646d9, 0x1b646d91, 0xb646d91b, 0x646d91b6, 0x46d91b64,
    0x6d91b646, 0xd91b646d, 0x91b646d9, 0x1b646d91, 0xb646d91b, 0x646d91b6, 0x46d91b64, 0x6d91b646,
    0xd91b646d, 0x91b646d9, 0x1b646d91, 0xb646d91b, 0x646d91b6, 0x46d91b64, 0x6d91b646, 0xd91b646d,
    0x91b646d9, 0x1b646d91, 0xb646d91b, 0x646d91b6, 0x46d91b64, 0x6d91b646, 0xd91b646d, 0x91b646d9,
    0x1b646d91, 0xb646d91b, 0x646d91b6, 0x46d91b64, 0x6d91b646, 0xd91b646d, 0x91b646d9, 0x1b646d91,
    0xb646d91b, 0x646d91b6, 0x46d91b64, 0x6d91b646, 0xd91b0000};
u8 bbdst[256];

void CommMsgPro(unsigned char *pDst, unsigned int *pSrc, int cnt, int type)
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
                if(pSrc[i] != bbsrc[i])
                {
                    printf("error 1 ~~~~~~~\n");
                }	
            }

            return ;	
        }		
    }	
    printf("OK\n");
}

#define TIMER_INT_LOOP_VAL 95000
static cyg_sem_t synchro;
static cyg_interrupt timer_interrupt[2];
static cyg_handle_t timer_interrupt_handle[2];
static volatile bool test_stop_flag = false;
static volatile bool test_init_flag = false;
static cyg_uint32 timer_ISR(cyg_vector_t vector, cyg_addrword_t data)
{
    hal_ostimer_disable1(data);
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);

    // printf("\nvector:%d, data:%d\n", vector, data);
    if (!test_stop_flag) {
        if (data == 2) {
            hal_ostimer_enable1(data, TIMER_INT_LOOP_VAL, 1);
        }
        if (data == 3) {
            hal_ostimer_enable1(3, TIMER_INT_LOOP_VAL * 2 / 3, 1);
        }
    }

    cyg_interrupt_unmask(vector);
    return CYG_ISR_CALL_DSR; // Cause DSR to be run
}

static void timer2_DSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    cyg_semaphore_post(&synchro);
    // printf("\ntimer 2 dsr, post\n");
}

static void timer3_DSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    // cyg_semaphore_post(&synchro[data]);
    // printf("\ntimer 3 dsr\n");
}

int do_timer_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    // cyg_uint32 len = 0;
    cyg_uint32 time_out = 10;
    // cyg_uint32 i = 0;

    u64 s, e;
    unsigned int us;

    if (argc >= 2)
        time_out = simple_strtoul(argv[1], NULL, 10);

    printf("\n-------------timer testing %d sec --------------------\n", time_out);

    if (strcmp(argv[1], "bb") == 0) {
        s = arch_counter_get_cntpct();
        CommMsgPro(CommMsgStr, bbsrc, 1680, 1);
        e = arch_counter_get_cntpct();
        us = tick_to_us((unsigned long) (e - s));
        printf("timer need %d us (%d ms)\n", us, us / 1000);
        return 0;
    }

    test_stop_flag = false;
    if (test_init_flag != true) {
        cyg_semaphore_init(&synchro, 0);

        cyg_interrupt_create(2,   // timer2
                             160, // Priority - unused
                             2,   //  Data item passed to interrupt handler
                             timer_ISR, timer2_DSR, &timer_interrupt_handle[0],
                             &timer_interrupt[0]);
        cyg_interrupt_attach(timer_interrupt_handle[0]);
        cyg_interrupt_unmask(2);

        cyg_interrupt_create(3,   // timer3
                             160, // Priority - unused
                             3,   //  Data item passed to interrupt handler
                             timer_ISR, timer3_DSR, &timer_interrupt_handle[1],
                             &timer_interrupt[1]);
        cyg_interrupt_attach(timer_interrupt_handle[1]);
        cyg_interrupt_unmask(3);
        test_init_flag = true;
    }

    hal_ostimer_enable1(2, TIMER_INT_LOOP_VAL, 1);
    hal_ostimer_enable1(3, TIMER_INT_LOOP_VAL * 2 / 3, 1);

    s = arch_counter_get_cntpct();

    while (1) {
        // cyg_thread_delay(1);
#if 1
        cyg_semaphore_wait(&synchro);
        printf("w\n");

        memset(CommMsgStr, 0, MAX_COMM_MSG_LEN);
        memset(CommMsgStr1, 0, MAX_COMM_MSG_LEN);
        CommMsgPro(CommMsgStr1, bbsrc, 1680, 0);
        memset(crc_buf, 0, 32);
        CommMsgPro(CommMsgStr, bbsrc, 1680, 1);

        e = arch_counter_get_cntpct();
        if (((U32)(e - s)) / 20000000 >= time_out) {
            test_stop_flag = true;
            printf("\n-------------timer testing out--------------------\n");
            break;
        }
#endif
    }

    return 0;
}
