//==========================================================================
//
//        spi_w5500_cli_test.c
//
//        spi w5500 cli test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2018-12-05
// Description:   spi cli test w5500 net device for bp2016
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
    diag_printf("\n[SPI] %s: line: %d, " fmt, __func__, __LINE__, ##args)
#define spi_debug(fmt, args...)    \
    diag_printf("\n[SPI DBG] %s: line: %d, " fmt, __func__, __LINE__, ##args)

#elif (CYGPKG_DEVS_SPI_ARM_BP2016_DEBUG_LEVEL > 0)
#define spi_debug(fmt, args...)    \
    diag_printf("\n[SPI DBG] %s: line: %d, " fmt, __func__, __LINE__, ##args)
#define spi_printf(fmt, args...)

#else
#define spi_debug(fmt, args...)
#define spi_printf(fmt, args...)
#endif

#define BP2016_OEM
#ifdef BP2016_OEM
#define W5500_RESET_GPIO_NUM        (32 * 3 + 9)
#define W5500_RESET_GPIO_ID         (9)
#define CHANGE_SPI_MUX_GPIO15       (15)
#define CHANGE_SPI_MUX_GPIO16       (16)
#else
#define W5500_RESET_GPIO_NUM        20
#endif

static void dump_spi_buffer(void *buffer, cyg_uint32 len)
{
    cyg_uint32 i = 0;
    cyg_uint8 *buf = (cyg_uint8 *) buffer;

    diag_printf("\n");
    for (i = 0; i < len; i++) {
        if (i % 16 == 0) {
            diag_printf("\n");
        }
        diag_printf("%02x ", *(buf + i));
    }
    diag_printf("\n");
}

spi_dev_config_t w5500_8bit_config = {
    .spi_dw_cfg.index = 0,
    .spi_dw_cfg.cs = 0,
    .spi_dw_cfg.cpol = 1,
    .spi_dw_cfg.cpha = 1,
    .spi_dw_cfg.baud = 10000000,
    .spi_dw_cfg.bus_width = 8,
    .spi_dw_cfg.gpio_num = 0,
};

#ifdef BP2016_OEM
void w5500_ext_pin_init(GPIO_GROUP_ID_T group_id, int id)
{
    printf("ext pin group num = %d\n", group_id);
    hal_iomux_gpio_en(group_id, id);
    gpio_api_init();
    gpio_api_set_output((group_id*32) + id);
    gpio_api_set_high((group_id*32) + id);
}
#endif

static cyg_uint8 wrbuf[20] = {0};
static cyg_uint8 rdbuf[20] = {0};
static void *handler = NULL;
static unsigned int gpio_num = W5500_RESET_GPIO_NUM;

int do_w5500_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int res = 0;
    cyg_uint32 txlen = 0;
    cyg_uint32 rxlen = 0;

    spi_printf("\n-------------spi w5500 testing in --------------------\n");
    if (strcmp(argv[1], "reset") == 0) {
#ifdef BP2016_OEM
        w5500_ext_pin_init(GPIO_GROUP_ID_A, CHANGE_SPI_MUX_GPIO15);
        w5500_ext_pin_init(GPIO_GROUP_ID_A, CHANGE_SPI_MUX_GPIO16);
        hal_iomux_gpio_en(GPIO_GROUP_ID_D, W5500_RESET_GPIO_ID);
#endif
        gpio_api_init();
        gpio_api_set_output(gpio_num);
        gpio_api_set_low(gpio_num);
        cyg_thread_delay(10);
        gpio_api_set_high(gpio_num);
    }

    if (strcmp(argv[1], "init") == 0) {
        //hal_iomux_spi0_en(1);
        handler = SpiInit(&w5500_8bit_config);
        if (handler == NULL) {
            spi_printf("\n%s: handler NULL error\n", __FUNCTION__);
        }
        spi_printf("\n---------------spi init start ------------------------\n");
    }

    if (strcmp(argv[1], "write") == 0) {
        txlen = 9;
        wrbuf[0] = 0x00;
        wrbuf[1] = 0x09;
        wrbuf[2] = 0x04;
        srand((u32) arch_counter_get_cntpct());
        wrbuf[3] = (u8) rand();
        wrbuf[4] = (u8) rand();
        wrbuf[5] = (u8) rand();
        wrbuf[6] = (u8) rand();
        wrbuf[7] = (u8) rand();
        wrbuf[8] = (u8) rand();

        SpiWrite(handler, wrbuf, &txlen);
        dump_spi_buffer((wrbuf+3), 6);
        spi_printf("\n-----------------spi write --------------------\n");
    }

    if (strcmp(argv[1], "read") == 0) {
        rxlen = 6;
        wrbuf[0] = 0x00;
        wrbuf[1] = 0x09;
        wrbuf[2] = 0x00;
        txlen = 3;
        memset(rdbuf, 0x22, sizeof(rdbuf));

        SpiWritethenRead(handler, wrbuf, &txlen, rdbuf, &rxlen);

        dump_spi_buffer(rdbuf, 6);
        spi_printf("\n-----------------spi read--------------------\n");
    }

    if (strcmp(argv[1], "get") == 0) {
        cyg_uint32 baud = 0;

        res = SpiGetBaud(handler, &baud);
        if (res != 0) {
            diag_printf("\nbit16 error = %d\n", res);
        }
        spi_printf("\nbaud = %d\n", baud);
    }

    if (strcmp(argv[1], "set") == 0) {
        cyg_uint32 setbaud = 95000;
        if (argc > 2) {
            setbaud = simple_strtoul(argv[2], NULL, 10);
        }
        diag_printf("\nbaud = %d\n", setbaud);

        res = SpiSetBaud(handler, &setbaud);
        if (res != 0) {
            diag_printf("\nbit16 error = %d\n", res);
        }
    }

    return res;
}

#endif

