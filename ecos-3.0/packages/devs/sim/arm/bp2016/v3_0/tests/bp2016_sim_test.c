//==========================================================================
//
//        sim_test.c
//
//        sim performance test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2017-10-18
// Description:   sim performance test for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <pkgconf/io_common_sim_arm_bp2016.h>
#include <cyg/kernel/kapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/infra/diag.h>

#ifdef CYGPKG_IO_COMMON_SIM_ARM_BP2016
#include <cyg/hal/api/sim_api.h>

// this is for debug trace,
#if (CYGPKG_DEVS_SIM_ARM_BP2016_DEBUG_LEVEL > 0)
#define sim_printf printf
#else
#define sim_printf(fmt, ...)
#endif

//===========================================================================
//                               DATA TYPES
//===========================================================================
typedef struct st_thread_data {
    cyg_thread obj;
    cyg_uint32 stack[4096];
    cyg_handle_t hdl;
} thread_data_t;

#define SIM_POLLING_TEST
#ifdef SIM_POLLING_TEST
cyg_thread thread_stack; /* space for two thread objects */
char stack[4096];
cyg_handle_t simple_thread;
cyg_thread_entry_t sim_polling_thread;
#endif

//===========================================================================
//                              LOCAL DATA
//===========================================================================
cyg_thread_entry_t sim_thread;
thread_data_t sim_thread_data;

static cyg_uint8 buffer[1024] = {0};
static cyg_uint8 timetmp = 0;
static cyg_uint8 selfcheck_txbuf[40] = {0x00, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe1, 0x7d, 0x51,
                                        0x30, 0xb1, 0xc2, 0x2a, 0x7d, 0x5f, 0xa9, 0x62, 0x46, 0x4e,
                                        0x72, 0x0f, 0xbe, 0x5b, 0xdb, 0x1f, 0x7d, 0x98, 0x67, 0x58,
                                        0x26, 0x12, 0x5d, 0x81, 0x10, 0xac, 0x22, 0xa5, 0x29, 0x1f};

static cyg_uint8 userinfo_txbuf[5] = {0x00, 0x03, 0x06, 0x20, 0x25};

static void dump_sim_buffer(void *buffer, cyg_uint32 len)
{
    cyg_uint32 i = 0;
    cyg_uint8 *buf = (cyg_uint8 *) buffer;

    printf("\n");
    for (i = 0; i < len; i++) {
        printf("%02x ", *(buf + i));
    }
    printf("\n");
}

void sim_thread(cyg_addrword_t data)
{
    int res = 0;
    cyg_uint32 len = 11;
    cyg_uint32 i = 0;
    cyg_uint32 timeout;

    sim_printf("\n-------------sim thread in----------------------\n");

    SimCardInit(1);

    while (1) {
        sim_printf("\n---------------sim atr api start ------------------------\n");

        len = 12;
        res = SimCardAtr(1, 32, 1, buffer, &len, 100);
        sim_printf("\natr res = 0x%x\n", res);
        if ((res == 0) || (res == (-ETIMEDOUT))) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\n---------------sim atr api end ------------------------\n");
        cyg_thread_delay(100);
        sim_printf("\n-----------------1 READ check start--------------------\n");
        timeout = 50;
        len = 40;
        SimCardWrite(1, selfcheck_txbuf, &len);

        timeout = 100;
        len = 0x6d;
        res = SimCardRead(1, buffer, &len, timeout);
        if (res == 0) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\n-----------------1 READ check end-----------------------\n");
        cyg_thread_delay(30);

        sim_printf("\n-----------------2 READ user info start ---------------\n");
        timeout = 50;
        len = 5;
        SimCardWrite(1, userinfo_txbuf, &len);
        timeout = 200;
        len = 0x25d;
        res = SimCardRead(1, buffer, &len, timeout);
        if (res == 0) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\n-----------------2 READ user info stop ---------------\n");
        cyg_thread_delay(100);

        sim_printf("\n-----------------2 READ check start--------------------\n");
        timeout = 50;
        len = 40;
        SimCardWrite(1, selfcheck_txbuf, &len);
        timeout = 100;
        len = 2;
        SimCardRead(1, buffer, &len, timeout);
        len = 0x6b;
        res = SimCardRead(1, (buffer + 2), &len, timeout);
        if (res == 0) {
            dump_sim_buffer(buffer, 0x6d);
        }
        sim_printf("\n-----------------2 READ check end-----------------------\n");
        cyg_thread_delay(30);

        sim_printf("\n-----------------get config start ---------------\n");
        len = 12;
        res = SimCardGetConfig(1, buffer, &len);
        if (res == 0) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\n-----------------get config stop ---------------\n");
        cyg_thread_delay(100);
    }
}

void cyg_start(void)
{
    cyg_thread_create(10, sim_thread, (cyg_addrword_t) 0, "bp2016_sim_thread",
                      (void *) sim_thread_data.stack, 1024 * sizeof(long), &sim_thread_data.hdl,
                      &sim_thread_data.obj);
    cyg_thread_resume(sim_thread_data.hdl);

#ifdef SIM_POLLING_TEST
    cyg_thread_create(3, sim_polling_thread, (cyg_addrword_t) 1, "Thread", (void *) stack, 4096,
                      &simple_thread, &thread_stack);
    cyg_thread_resume(simple_thread);
#endif

    cyg_scheduler_start();
}

#ifdef SIM_POLLING_TEST
void sim_polling_thread(cyg_addrword_t data)
{
    int res = 0;
    cyg_uint32 len = 11;
    cyg_uint32 i = 0;
    cyg_uint32 timeout;
    cyg_uint32 s, e;
    cyg_int8 rdata = 0;

    // SimCardPollingInit(1);

    for (;;) {
        sim_printf("\n----------polling atr start\n");
        len = 11;
        res = SimCardPollingAtr(1, 32, 1, buffer, &len, 100);
        sim_printf("\nres = 0x%x\n", res);
        if (res == 0) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\natr end len = 0x%x\n", len);
        cyg_thread_delay(100);
        sim_printf("\n----------polling atr end\n");
#if 0
        sim_printf("\n----------polling set etu start \n");
        SimCardPollingSetetu(1, 372);
        sim_printf("\n----------polling set etu end\n");
#endif
        sim_printf("\n----------polling check start\n");
        i = 0;
        len = 40;
        timeout = 500;
        while (1) {
            res = SimCardPollingWrite(1, selfcheck_txbuf[i]);
            if (res == (-ENOSUPP)) {
                break;
            }
            if (res == ENOERR) {
                i++;
            }
            if (i == len) {
                break;
            }
        }
        i = 0;
        len = 0x6d;
        timeout = 500;
        s = arch_counter_get_cntpct();
        while (1) {
            res = SimCardPollingRead(1, &buffer[i]);
            if (res == (-ENOSUPP)) {
                break;
            }
            if (res == ENOERR) {
                i++;
                if (i == len) {
                    break;
                }
            }

            e = arch_counter_get_cntpct();
            if (((unsigned long) (e - s) / (CONFIG_COUNTER_FREQ / 1000)) >= timeout) {
                len = i;
                break;
            }
        }
        if (res == 0) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\ncheck end len = 0x%x\n", len);
        sim_printf("\n----------polling check end\n");
        cyg_thread_delay(100);
    }
}
#endif

#endif
