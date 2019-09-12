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
#include "common.h"
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

#define TIME_OUT (100)
cyg_uint8 read_buf[16] = {
    0,
};

cyg_uint8 test_case0[14] = {0x80,0x3e,0x06,0x00,0x00,0x01,0x28,0xcc,0x66,0x04,0x00,0x00,0x00,0x00};
cyg_uint8 test_case1[14] = {0, };

int do_max2112_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int res = 0;
    cyg_uint32 i = 0, len = 0;
    cyg_uint8 addr;

    max2112_printf("\n-------------max2112 thread in----------------------\n");

    if (strcmp(argv[1], "init") == 0) {
        if (argc < 2) {
            max2112_printf("command error!\r\n	cmd: max2112 init i2c_index");
            return ENOERR;
        }

        if(argc > 2)
        {
            i = (cyg_uint32) atoi(argv[2]);
        }else{
            i = 0;
        }
        res = Max2112_Init(i);

        if (res != ENOERR) {
            max2112_printf("error, ret = %d", res);
            return -EIO;
        }
        Max2112_config_gpio(4, 5, 6);

        res = Max2112_get_mode();
        max2112_printf("Max2112_get_mode return %d", res);
    }

    if (strcmp(argv[1], "read") == 0) {
        if (argc < 4) {
            max2112_printf("command error!\r\n	cmd: max2112 read addr len");
            return ENOERR;
        }
        addr = (cyg_uint8) atoi(argv[2]);
        len = (cyg_uint32) atoi(argv[3]);
        memset(read_buf, 0, 16);
        res = Max2112_read(addr, read_buf, len);
        dump_max2112_buffer(read_buf, res);
        return ENOERR;
    }

    if (strcmp(argv[1], "setm") == 0) {

        if (argc < 3) {
            max2112_printf("command error!\r\n	cmd: max2112 setm mode");
            return ENOERR;
        }
        addr = (cyg_uint8) atoi(argv[2]);
        res = Max2112_set_mode(addr);
        max2112_printf("Max2112_set_mode return %d", res);
        return ENOERR;
    }

    if (strcmp(argv[1], "gpio") == 0) {

        if (argc < 5) {
            max2112_printf("command error!\r\n	cmd: max2112 gpio sel gpio0 gpio1");
            return ENOERR;
        }
        addr = (cyg_uint32) atoi(argv[2]);
        len = (cyg_uint32) atoi(argv[3]);
        res = (cyg_uint32) atoi(argv[4]);

        max2112_printf("gpio set low: %d high: %d %d", addr, len, res);
        return Max2112_config_gpio(addr, len, res);
    }

    if (strcmp(argv[1], "getm") == 0) {
        res = Max2112_get_mode();
        max2112_printf("Max2112_get_mode return %d", res);
        return ENOERR;
    }

    if (strcmp(argv[1], "wr") == 0) {

        if (argc < 4) {
            max2112_printf("command error!\r\n	cmd: max2112 wr addr values");
            return ENOERR;
        }

        memset(read_buf, 0, 16);
        addr = (cyg_uint8) atoi(argv[2]);

        for (i = 0; i < argc - 3; i++) {
            read_buf[i] = (cyg_uint8) atoi(argv[3 + i]);
        }
        max2112_printf("write content: ");
        dump_max2112_buffer(read_buf, argc - 3);

        res = Max2112_write(addr, read_buf, argc - 3);

        return ENOERR;
    }

    if (strcmp(argv[1], "auto") == 0) {
        max2112_printf("\n---------------max2112 test start ------------------------\n");

        cyg_thread_delay(100);
        res = Max2112_read(0, read_buf, 14);

        if (14 != res) {
            max2112_printf("read register failed! ret %d", res);
            cyg_thread_delay(TIME_OUT);
            return -EIO;
        } else {
            dump_max2112_buffer(read_buf, res);
        }

        memset(read_buf, 0x00, 14);
        res = Max2112_write(0, read_buf, 14);
        max2112_printf("write len %d", res);
        cyg_thread_delay(100);

        res = Max2112_read(0, read_buf, 14);

        if (14 != res) {
            max2112_printf("read register failed! ret %d", res);
            cyg_thread_delay(TIME_OUT);
            return -EIO;
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
            return -EIO;
        } else {
            dump_max2112_buffer(read_buf, res);
        }
        memset(read_buf, 0xaa, 14);
        res = Max2112_write(0, read_buf, 14);
        max2112_printf("write len %d", res);
        cyg_thread_delay(100);

        res = Max2112_read(0, read_buf, 14);

        if (14 != res) {
            max2112_printf("read register failed! ret %d", res);
            cyg_thread_delay(TIME_OUT);
            return -EIO;
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
            return -EIO;
        } else {
            dump_max2112_buffer(read_buf, res);
        }
        max2112_printf("\n-----------------get config stop ---------------\n");
        cyg_thread_delay(100);

        // read data not form 0
        res = Max2112_read(5, read_buf, 9);

        if (9 != res) {
            max2112_printf("read register failed! ret %d", res);
            cyg_thread_delay(TIME_OUT);
            return -EIO;
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
            return -EIO;
        } else {
            dump_max2112_buffer(read_buf, res);
        }

        cyg_thread_delay(TIME_OUT);
    }

    if (strcmp(argv[1], "ts") == 0) {

        for(i=0; i <100; i++)
        {
            res = Max2112_write(0, test_case0, 14);

            res = Max2112_read(0, read_buf, 14);

            if (14 != res) {
                max2112_printf("read register failed! ret %d", res);
                cyg_thread_delay(TIME_OUT);
                return -EIO;
            } else {
                dump_max2112_buffer(read_buf, res);
            }

            cyg_thread_delay(10*TIME_OUT);

            res = Max2112_write(0, test_case1, 14);
            res = Max2112_read(0, read_buf, 14);

            if (14 != res) {
                max2112_printf("read register failed! ret %d", res);
                cyg_thread_delay(TIME_OUT);
                return -EIO;
            } else {
                dump_max2112_buffer(read_buf, res);
            }

            cyg_thread_delay(10*TIME_OUT);

        }
    }

    return ENOERR;
}

#endif
