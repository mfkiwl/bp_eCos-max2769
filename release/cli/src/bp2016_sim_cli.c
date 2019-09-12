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
#include <cyg/kernel/kapi.h>
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/infra/diag.h>

#ifdef CYGPKG_IO_COMMON_SIM_ARM_BP2016
#include <pkgconf/io_common_sim_arm_bp2016.h>
#include <cyg/hal/api/sim_api.h>
#include <cyg/hal/api/iomux_api.h>

// this is for debug trace,
#if (CYGPKG_DEVS_SIM_ARM_BP2016_DEBUG_LEVEL > 0)
#define sim_printf printf
#else
#define sim_printf(fmt, ...)
#endif

static cyg_uint8 buffer[1024] = {0};
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

int do_sim_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int res = 0;
    cyg_uint32 len = 0;
    cyg_uint32 timeout = 0;
    cyg_uint32 i = 0;
    cyg_uint32 s, e;
    static cyg_uint32 index = 2, etu = 372;

    if (argc > 2) {
        index = simple_strtoul(argv[2], NULL, 16);
        if (index > 2) {
            index = 2;
        }
    }
    if (argc > 3) {
        etu = simple_strtoul(argv[3], NULL, 10);
        if (!((etu == 372) || (etu == 32))) {
            etu = 372;
        }
    }

    sim_printf("\n-------------sim testing in --------------------\nsim index = %d, etu = %d\n",
            index, etu);

    if (strcmp(argv[1], "a") == 0) {
        switch (index) {
        case 0:
            hal_iomux_en(IOMUX_SIM0);
            break;
        case 1:
            hal_iomux_en(IOMUX_SIM1);
            break;
        case 2:
            hal_iomux_en(IOMUX_SIM2);
            break;
        }
        SimCardInit(index);
        sim_printf("\n---------------sim atr api start ------------------------\n");
        len = 40;
        timeout = 100;
        res = SimCardAtr(index, etu, 1, buffer, &len, timeout);
        if ((res == (-ETIMEDOUT)) || (res == ENOERR)) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\n---------------sim atr api end ------------------------\n");
    }

    if (strcmp(argv[1], "c") == 0) {
        sim_printf("\n-----------------1 READ check start--------------------\n");
        timeout = 50;
        len = 40;
        SimCardWrite(index, selfcheck_txbuf, &len);

        timeout = 100;
        len = 0x6d;
        res = SimCardRead(index, buffer, &len, timeout);
        if ((res == (-ETIMEDOUT)) || (res == ENOERR)) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\n-----------------1 READ check end-----------------------\n");
    }

    if (strcmp(argv[1], "u") == 0) {
        etu = 32;
        sim_printf("\n-----------------2 READ user info start ---------------\n");
        timeout = 50;
        len = 5;
        SimCardWrite(index, userinfo_txbuf, &len);

        timeout = 200;
        len = 0x25d;
        res = SimCardRead(index, buffer, &len, timeout);
        if ((res == (-ETIMEDOUT)) || (res == ENOERR)) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\n-----------------2 READ user info stop ---------------\n");
    }

    if (strcmp(argv[1], "get") == 0) {
        sim_printf("\n-----------------get config start ---------------\n");
        len = 12;
        SimCardGetConfig(index, buffer, &len);
        dump_sim_buffer(buffer, len);
        sim_printf("\n-----------------get config stop ---------------\n");
    }

    if (strcmp(argv[1], "set") == 0) {
        sim_printf("\n-----------------set config start ---------------\n");
        SimCardSetetu(index, etu);
        sim_printf("\n-----------------set config stop ---------------\n");
    }

    if (strcmp(argv[1], "auto") == 0) {
        sim_printf("\n---------------sim atr api start ------------------------\n");
        len = 11;
        timeout = 100;
        SimCardAtr(index, etu, 1, buffer, &len, timeout);
        dump_sim_buffer(buffer, len);
        sim_printf("\n---------------sim atr api end ------------------------\n");
        cyg_thread_delay(30);

        sim_printf("\n-----------------1 READ check start--------------------\n");
        timeout = 50;
        len = 40;
        SimCardWrite(index, selfcheck_txbuf, &len);

        timeout = 100;
        len = 0x6d;
        res = SimCardRead(index, buffer, &len, timeout);
        if ((res == (-ETIMEDOUT)) || (res == ENOERR)) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\n-----------------1 READ check end-----------------------\n");
        cyg_thread_delay(30);

        sim_printf("\n-----------------2 READ user info start ---------------\n");
        timeout = 50;
        len = 5;
        SimCardWrite(index, userinfo_txbuf, &len);

        timeout = 200;
        len = 0x25d;
        res = SimCardRead(index, buffer, &len, timeout);
        if ((res == (-ETIMEDOUT)) || (res == ENOERR)) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\n-----------------2 READ user info stop ---------------\n");
        cyg_thread_delay(30);

        sim_printf("\n-----------------get config start ---------------\n");
        len = 12;
        SimCardGetConfig(index, buffer, &len);
        dump_sim_buffer(buffer, len);
        sim_printf("\n-----------------get config stop ---------------\n");
    }

    if (strcmp(argv[1], "polling") == 0) {
        SimCardPollingInit(index);

        sim_printf("\n----------polling atr start\n");
        len = 11;
        res = SimCardPollingAtr(index, etu, 1, buffer, &len, 100);
        sim_printf("\nres = 0x%x\n", res);
        if ((res == (-ETIMEDOUT)) || (res == ENOERR)) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\natr end len = 0x%x\n", len);
        sim_printf("\n----------polling atr end\n");

        sim_printf("\n----------polling check start\n");
        i = 0;
        len = 40;
        timeout = 500;
        while (1) {
            res = SimCardPollingWrite(index, selfcheck_txbuf[i]);
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
            res = SimCardPollingRead(index, (cyg_int8*)&buffer[i]);
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
        if ((res == (-ETIMEDOUT)) || (res == ENOERR)) {
            dump_sim_buffer(buffer, len);
        }
        sim_printf("\ncheck end len = 0x%x\n", len);
        sim_printf("\n----------polling check end\n");
    }

    return res;
}

#endif
