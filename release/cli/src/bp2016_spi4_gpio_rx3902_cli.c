//==========================================================================
//
//        bp2016_spi4_gpio_rx3902_cli.c
//
//        spi4 bitbang cli test case for rx3902
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2019-05-05
// Description:   spi4 gpio cli test rx3902 rf device for bp2016 oem
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <stdio.h>
#include "common.h"

#include <cyg/hal/api/spi4_bitbang_api.h>
#include <cyg/hal/api/iomux_api.h>

#define     SPI4_GPIO_DATA      (32 * 2 + 29)   //GPIO_C29
#define     SPI4_GPIO_SCLK      (32 * 2 + 28)   //GPIO_C28
#define     SPI4_GPIO_CS1       (32 * 2 + 27)   //GPIO_C27
#define     SPI4_GPIO_CS2       (32 * 2 + 26)   //GPIO_C26

spi4_gpio_dev_info rx3902_id0 = {
    .cs = SPI4_GPIO_CS1,
    .sdata = SPI4_GPIO_DATA,
    .sclk = SPI4_GPIO_SCLK,
    .baud = 100000,
    .addr_width = 6,
    .data_width = 20
};

spi4_gpio_dev_info rx3902_id1 = {
    .cs = SPI4_GPIO_CS2,
    .sdata = SPI4_GPIO_DATA,
    .sclk = SPI4_GPIO_SCLK,
    .baud = 100000,
    .addr_width = 6,
    .data_width = 20
};

int do_spi4_gpio_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int res = 0;
    cyg_uint32 cs = 1, addr = 0, value = 0;
    int i = 0;
    spi4_gpio_dev_info *handler = NULL;

    printf("\n-------------spi4 gpio bitbang testing in --------------------\n");

    if (argc > 2) {
        cs = simple_strtoul(argv[2], NULL, 10);
        if (argc > 3) {
            addr = simple_strtoul(argv[3], NULL, 10);
            if (argc > 4) {
                value = simple_strtoul(argv[4], NULL, 16);
            }
        }
    }

    if (cs == 1) {
        handler = &rx3902_id0;
    } else if (cs == 2) {
        handler = &rx3902_id1;
    } else {
        printf("\nerror: cs invalid\n");
    }

    if (strcmp(argv[1], "init") == 0) {
        hal_iomux_gpio_en(GPIO_GROUP_ID_C, 29);
        hal_iomux_gpio_en(GPIO_GROUP_ID_C, 28);
        hal_iomux_gpio_en(GPIO_GROUP_ID_C, 27);
        hal_iomux_gpio_en(GPIO_GROUP_ID_C, 26);
        spi4_bitbang_api_init();
    }

    if (strcmp(argv[1], "read") == 0) {
        res = spi4_bitbang_api_rx(handler, addr, &value);
        if (res != 0) {
            printf("invalid parameters res = 0x%x\n", res);
            return DVT_CASE_RESULT_FAIL;
        }
        printf("cs = %d, addr = %d, value = 0x%05x\n", handler->cs, addr, value);
    }

    if (strcmp(argv[1], "write") == 0) {
        res = spi4_bitbang_api_tx(handler, addr, value);
        if (res != 0) {
            printf("invalid parameters res = 0x%x\n", res);
            return DVT_CASE_RESULT_FAIL;
        }
    }

    if (strcmp(argv[1], "read3902") == 0) {
        for(i = 0; i <= 43; i++) {
            addr = i;
            res = spi4_bitbang_api_rx(handler, addr, &value);
            if (res != 0) {
                printf("invalid parameters res = 0x%x\n", res);
                return DVT_CASE_RESULT_FAIL;
            }
            printf("cs: %d %s %02d %s 0x%05x %s\n", handler->cs, "|", addr, "|", value, "|");
        }
    }

    return res;
}

