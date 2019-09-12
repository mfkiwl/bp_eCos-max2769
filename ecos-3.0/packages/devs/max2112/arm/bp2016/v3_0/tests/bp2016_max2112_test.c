//==========================================================================
//
//        max2112_test.c
//
//        max2112 performance test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2017-10-18
// Description:   max2112 performance test for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <cyg/kernel/kapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/infra/diag.h>

#ifdef CYGPKG_BP2016_I2C_DEV_MAX2112
#include <pkgconf/bp2016_i2c_dev_max2112.h>
#include <cyg/io/max2112/max2112_bp2016.h>
#include <cyg/hal/api/max2112_api.h>

// this is for debug trace,
#if (CYGPKG_DEVS_MAX2112_ARM_BP2016_DEBUG_LEVEL > 0)
#define max2112_printf(fmt, args...)                                                               \
    diag_printf("[max2112 test] %s:%d " fmt "\n", __func__, __LINE__, ##args);
#else
#define max2112_printf(fmt, ...)
#endif

//===========================================================================
//                               DATA TYPES
//===========================================================================
typedef struct st_thread_data {
    cyg_thread obj;
    cyg_uint32 stack[4096];
    cyg_handle_t hdl;
} thread_data_t;

//===========================================================================
//                              LOCAL DATA
//===========================================================================
cyg_thread_entry_t max2112_thread;
thread_data_t max2112_thread_data;

static cyg_uint8 userinfo_txbuf[5] = {0x00, 0x03, 0x06, 0x20, 0x25};

static void dump_max2112_buffer(void *buffer, cyg_uint32 len)
{
    cyg_uint32 i = 0;
    cyg_uint8 *buf = (cyg_uint8 *) buffer;

    printf("\n");
    for (i = 0; i < len; i++) {
        printf("%02x ", *(buf + i));
    }
    printf("\n");
}

#define TIME_OUT (1000000)
cyg_uint8 read_buf[16] = {
    0,
};

void max2112_thread(cyg_addrword_t data)
{
    int res = 0;
    cyg_uint32 i = 0;
    cyg_uint32 timeout;

    max2112_printf("\n-------------max2112 thread in----------------------\n");

    res = Max2112_Init(0);

	Max2112_config_gpio(4, 5, 6);

    if (res != ENOERR) {
        max2112_printf("error, ret = %d", res);
        return;
    }

    res = Max2112_get_mode();
    max2112_printf("Max2112_get_mode return %d", res);

    res = Max2112_set_mode(I2C_MODE_FAST);
    max2112_printf("Max2112_set_mode return %d", res);

    while (1) {
        max2112_printf("\n---------------max2112 test start ------------------------\n");

        cyg_thread_delay(100);
        res = Max2112_read(0, read_buf, 14);

        if (14 != res) {
            max2112_printf("read register failed! ret %d", res);
            cyg_thread_delay(TIME_OUT);
            break;
        } else {
            dump_max2112_buffer(read_buf, res);
        }

        max2112_printf("\n---------------max2112 standard mode start ------------------------\n");
        res = Max2112_set_mode(I2C_MODE_STD);
        max2112_printf("Max2112_set_mode return %d", res);

        memset(read_buf, 0x00, 14);
        res = Max2112_write(0, read_buf, 14);
        max2112_printf("write len %d", res);
        cyg_thread_delay(100);

        res = Max2112_read(0, read_buf, 14);

        if (14 != res) {
            max2112_printf("read register failed! ret %d", res);
            cyg_thread_delay(TIME_OUT);
            break;
        } else {
            dump_max2112_buffer(read_buf, res);
        }

        memset(read_buf, 0x55, 14);
        res = Max2112_write(0, read_buf, 14);
        max2112_printf("write len %d", res);
        cyg_thread_delay(100);

        res = Max2112_read(0, read_buf, 14);

        if (14 != res) {
            max2112_printf("read register failed! ret %d", res);
            cyg_thread_delay(TIME_OUT);
            break;
        } else {
            dump_max2112_buffer(read_buf, res);
        }

        max2112_printf("\n---------------max2112 standard mode end ------------------------\n");

        res = Max2112_set_mode(I2C_MODE_FAST);
        max2112_printf("Max2112_set_mode return %d", res);

        memset(read_buf, 0xaa, 14);
        res = Max2112_write(0, read_buf, 14);
        max2112_printf("write len %d", res);
        cyg_thread_delay(100);

        res = Max2112_read(0, read_buf, 14);

        if (14 != res) {
            max2112_printf("read register failed! ret %d", res);
            cyg_thread_delay(TIME_OUT);
            break;
        } else {
            dump_max2112_buffer(read_buf, res);
        }

        memset(read_buf, 0xff, 14);
        res = Max2112_write(0, read_buf, 14);
        max2112_printf("write len %d", res);
        cyg_thread_delay(100);

        res = Max2112_read(0, read_buf, 14);

        if (14 != res) {
            max2112_printf("read register failed! ret %d", res);
            cyg_thread_delay(TIME_OUT);
            break;
        } else {
            dump_max2112_buffer(read_buf, res);
        }

        cyg_thread_delay(100);

        // read data not form 0
        res = Max2112_read(5, read_buf, 9);

        if (9 != res) {
            max2112_printf("read register failed! ret %d", res);
            cyg_thread_delay(TIME_OUT);
            break;
        } else {
            dump_max2112_buffer(read_buf, res);
        }

        memset(read_buf, 0x55, 14);
        res = Max2112_write(5, read_buf, 4);
        max2112_printf("write len %d", res);
        cyg_thread_delay(100);

        res = Max2112_write(11, read_buf, 1);
        max2112_printf("write len %d", res);
        cyg_thread_delay(100);

        // read data not form 0
        res = Max2112_read(5, read_buf, 1);

        if (1 != res) {
            max2112_printf("read register failed! ret %d", res);
            cyg_thread_delay(TIME_OUT);
            break;
        } else {
            dump_max2112_buffer(read_buf, res);
        }

        cyg_thread_delay(TIME_OUT);
    }
}

void cyg_start(void)
{
    cyg_thread_create(10, max2112_thread, (cyg_addrword_t) 0, "bp2016_max2112_thread",
                      (void *) max2112_thread_data.stack, 1024 * sizeof(long),
                      &max2112_thread_data.hdl, &max2112_thread_data.obj);
    cyg_thread_resume(max2112_thread_data.hdl);

    cyg_scheduler_start();
}

#endif
