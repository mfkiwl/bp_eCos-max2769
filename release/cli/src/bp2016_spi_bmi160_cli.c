//==========================================================================
//
//        bp2016_spi_bmi160_cli.c
//
//        spi bmi160 cli test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2019-05-05
// Description:   spi cli bmi160 device for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <stdio.h>
#include "common.h"

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016
#include <pkgconf/devs_spi_arm_bp2016.h>
#include <cyg/hal/regs/spi.h>
#include <cyg/io/spi/spi_bp2016.h>
#include <cyg/hal/api/spi_api.h>
#include <cyg/hal/api/gpio_api.h>
#include <cyg/hal/api/iomux_api.h>

#if (CYGPKG_DEVS_SPI_ARM_BP2016_DEBUG_LEVEL > 1)
#define spi_printf(fmt, args...)    \
    printf("\n[SPI] %s: line: %d, " fmt, __func__, __LINE__, ##args)
#define spi_debug(fmt, args...)    \
    printf("\n[SPI DBG] %s: line: %d, " fmt, __func__, __LINE__, ##args)

#elif (CYGPKG_DEVS_SPI_ARM_BP2016_DEBUG_LEVEL > 0)
#define spi_debug(fmt, args...)    \
    printf("\n[SPI DBG] %s: line: %d, " fmt, __func__, __LINE__, ##args)
#define spi_printf(fmt, args...)

#else
#define spi_debug(fmt, args...)
#define spi_printf(fmt, args...)
#endif

static void dump_spi_buffer(void *buffer, cyg_uint32 len)
{
    cyg_uint32 i = 0;
    cyg_uint8 *buf = (cyg_uint8 *) buffer;

    printf("\n");
    for (i = 0; i < len; i++) {
        if (i % 16 == 0) {
            printf("\n");
        }
        printf("%02x ", *(buf + i));
    }
    printf("\n");
}

static spi_dev_config_t bmi160_8bit_config = {
    .spi_dw_cfg.index = 2,
    .spi_dw_cfg.cs = 0,
    .spi_dw_cfg.cpol = 1,
    .spi_dw_cfg.cpha = 1,
    .spi_dw_cfg.baud = 1000000,
    .spi_dw_cfg.bus_width = 8,
    .spi_dw_cfg.gpio_num = 0,
};

#define     BMI160_IRQ0_GPIO_NUM        13
#define     BMI160_IRQ1_GPIO_NUM        14

static cyg_uint8 wrbuf[20] = {0};
static cyg_uint8 rdbuf[20] = {0};
static void *handler = NULL;

#define     WRITE_FLAG      0
#define     READ_FLAG       0x80

int do_bmi160_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int res = 0;
    cyg_uint32 txlen = 0;
    cyg_uint32 rxlen = 1;
    cyg_uint32 addr = 0, value = 0x33;

    spi_printf("\n-------------spi bmi160 testing in --------------------\n");

    if (strcmp(argv[1], "init") == 0) {
        hal_iomux_spi2_en(1, 0, 0, 0, 0, 0, 0, 0);
        handler = SpiInit(&bmi160_8bit_config);
        if (handler == NULL) {
            spi_printf("\n%s: handler NULL error\n", __FUNCTION__);
        }
        spi_printf("\n---------------spi init start ------------------------\n");
    }

    if (strcmp(argv[1], "write") == 0) {
        if (argc > 2) {
            addr = simple_strtoul(argv[2], NULL, 16);
            if (argc > 3) {
                value = simple_strtoul(argv[3], NULL, 16);
            }
        }

        txlen = 2;
        wrbuf[0] = addr | WRITE_FLAG;
        wrbuf[1] = value;

        res = SpiWrite(handler, wrbuf, &txlen);
        dump_spi_buffer(wrbuf, txlen);
        spi_printf("\n-----------------spi write --------------------\n");
    }

    if (strcmp(argv[1], "read") == 0) {
        if (argc > 2) {
            addr = simple_strtoul(argv[2], NULL, 16);
            if (argc > 3) {
                rxlen = simple_strtoul(argv[3], NULL, 16);
            }
        } else {
            rxlen = 1;
        }
        wrbuf[0] = addr | READ_FLAG;
        txlen = 1;
        memset(rdbuf, 0x22, sizeof(rdbuf));

        SpiWritethenRead(handler, wrbuf, &txlen, rdbuf, &rxlen);

        dump_spi_buffer(rdbuf, rxlen);
        spi_printf("\n-----------------spi read--------------------\n");
    }

    if (strcmp(argv[1], "get") == 0) {
        cyg_uint32 baud = 0;

        res = SpiGetBaud(handler, &baud);
        if (res != 0) {
            printf("\nbit16 error = %d\n", res);
        }
        spi_printf("\nbaud = %d\n", baud);
    }

    if (strcmp(argv[1], "set") == 0) {
        cyg_uint32 setbaud = 95000;
        if (argc > 2) {
            setbaud = simple_strtoul(argv[2], NULL, 10);
        }
        printf("\nbaud = %d\n", setbaud);

        res = SpiSetBaud(handler, &setbaud);
        if (res != 0) {
            printf("\nbit16 error = %d\n", res);
        }
    }

    return res;
}

#endif

