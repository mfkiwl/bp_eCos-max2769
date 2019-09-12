//==========================================================================
//
//        i2c_test.c
//
//        i2c base interface test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2018-10-30
// Description:   i2c base interface test for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <cyg/kernel/kapi.h>
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/infra/diag.h>

#ifdef CYGPKG_DEVS_BP2016_I2C_DW

#include <cyg/hal/api/i2c_api.h>
#include <cyg/hal/api/iomux_api.h>

//===========================================================================
//                               DATA TYPES
//===========================================================================

static void dump_i2c_buffer(void *buffer, cyg_uint32 len)
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
cyg_uint8 i2c_read_buf[64] = {
    0,
};


// wwzz add global var for testing
static void * i2c_handle = NULL;
int do_i2c_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int res = 0;
    cyg_uint32 i = 0, len = 0;

    // wwzz add test codes
    cyg_uint8 slv_addr, bus_id, t_mode;
    cyg_uint16 e_addr;

    printf("\n-------------i2c testing in----------------------\n");

    // wwzz add test case
#define TEST_MAX2112_I2C_SLV_ADDR (0x60)
#define TEST_EEPROM_I2C_SLV_ADDR (0x50)
    if (strcmp(argv[1], "init") == 0) {
        hal_iomux_en(IOMUX_I2C0);
        if (argc < 5) {
            printf("para error!\r\n	cmd: init must have 3 input paras\n");
            return ENOERR;
        }
        bus_id = (cyg_uint8) atoi(argv[2]);
        slv_addr = (cyg_uint8) atoi(argv[3]);
        t_mode = (cyg_uint8) atoi(argv[4]);
        i2c_handle = I2C_dev_init(bus_id, slv_addr, t_mode);
        if(NULL == i2c_handle){
            printf("Init failed! ");
        }else{
            printf("Init success! ");
        }
        printf("--para: bus_id(%d), slv_addr(0x%x), mode(%d)\r\n", bus_id, slv_addr, t_mode);
        return ENOERR;
    }

    if (strcmp(argv[1], "deinit") == 0) {
        I2C_dev_deinit(i2c_handle);
    }

    if (strcmp(argv[1], "read") == 0) {
        if (argc < 4) {
            printf("command error!\r\n");
            return ENOERR;
        }

        cyg_uint8 w_addr[4];

        e_addr = (cyg_uint16) atoi(argv[2]);
        len = (cyg_uint32) atoi(argv[3]);
        printf("read addr:%d len:%d\n", e_addr, len);
        memset(i2c_read_buf, 0, 16);

        w_addr[0] = ((e_addr >> 8) & 0xFF);
        w_addr[1] = (e_addr & 0xFF);

        res = I2C_dev_read(i2c_handle, w_addr, 2, i2c_read_buf, len);
        if(res == len)
            dump_i2c_buffer(i2c_read_buf, res);
        else
            printf("read data err:%d\n", res);
        return ENOERR;
    }

    if (strcmp(argv[1], "write") == 0) {
        if (argc < 4) {
            printf("command error!\r\n	cmd: max2112 dread addr len\n");
            return ENOERR;
        }

        cyg_uint8 w_data[34];

        e_addr = (cyg_uint16) atoi(argv[2]);
        len = (cyg_uint32) atoi(argv[3]);
        if(len > 34) len = 34;
        printf("read addr:%d len:%d\n", e_addr, len);

        w_data[0] = ((e_addr >> 8) & 0xFF);
        w_data[1] = (e_addr & 0xFF);
        for(i = 2; i < 34; i++){
            w_data[i] = w_data[1] + i;
        }

        res = I2C_dev_write(i2c_handle, w_data, 34);
        if(res == 34)
            dump_i2c_buffer(w_data, res);
        else
            printf("write data err:%d\n", res);

        printf("\n\n");
        return ENOERR;
    }

    return ENOERR;
}

#endif
