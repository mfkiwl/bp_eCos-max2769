//==========================================================================
//
//        bp2016_flash_spi_cli_test.c
//
//        spi flash cli test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2018-04-26
// Description:   spi cli test flash for bp2016
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

#define     SPI_FLASH_CLI_PRINTF
#ifdef     SPI_FLASH_CLI_PRINTF
#define spi_printf(fmt, args...)    \
        diag_printf("\n[SPI] %s: line: %d, " fmt, __func__, __LINE__, ##args)
#define spi_debug(fmt, args...)    \
        diag_printf("\n[SPI DBG] %s: line: %d, " fmt, __func__, __LINE__, ##args)

#else
#define spi_debug(fmt, args...)
#define spi_printf(fmt, args...)
#endif

extern Cyg_ErrNo SpiReadFlash(void *handler, cyg_uint32 addr, cyg_uint8 *rbuf, cyg_uint32 *rlen);
extern Cyg_ErrNo SpiEraseFlash(void *handler, cyg_uint32 addr);

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

spi_dev_config_t spi_flash_8bit_config = {
    .spi_dw_cfg.index = 0,
    .spi_dw_cfg.cs = 0,
    .spi_dw_cfg.cpol = 1,
    .spi_dw_cfg.cpha = 1,
    .spi_dw_cfg.baud = 3000000,
    .spi_dw_cfg.bus_width = 8,
};

spi_dev_config_t spi_flash_16bit_config = {
    .spi_dw_cfg.index = 0,
    .spi_dw_cfg.cs = 1,
    .spi_dw_cfg.cpol = 1,
    .spi_dw_cfg.cpha = 1,
    .spi_dw_cfg.baud = 3000000,
    .spi_dw_cfg.bus_width = 16,
};

spi_dev_config_t spi_flash_32bit_config = {
    .spi_dw_cfg.index = 2,
    .spi_dw_cfg.cs = 4,
    .spi_dw_cfg.cpol = 1,
    .spi_dw_cfg.cpha = 1,
    .spi_dw_cfg.baud = 3000000,
    .spi_dw_cfg.bus_width = 32,
};

static cyg_uint8 wrbuf[260] = {0};
static cyg_uint8 rdbuf[256] = {0};
static void *handler = NULL;

int do_flash_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int res = 0;
    cyg_uint32 i = 0;
    cyg_uint32 txlen = 0;
    cyg_uint32 rxlen = 0;
    cyg_uint32 addr = 0;

    spi_printf("\n-------------spi flash testing in --------------------\n");

    if (strcmp(argv[1], "init") == 0) {
        handler = SpiInit(&spi_flash_8bit_config);
        if (handler == NULL) {
            spi_printf("\n%s: handler NULL error\n", __FUNCTION__);
        }
        spi_printf("\n---------------spi init start ------------------------\n");
    }

    if (strcmp(argv[1], "program") == 0) {
        if (argc > 2) {
            addr = simple_strtoul(argv[2], NULL, 16);
        }
        txlen = 1;
        wrbuf[0] = 0x06;
        SpiWrite(handler, wrbuf, &txlen);

        txlen = 260;
        wrbuf[0] = 0x02;
        wrbuf[1] = (addr >> 16) & 0xff;
        wrbuf[2] = (addr >> 8) & 0xff;
        wrbuf[3] = addr & 0xff;
        for(i = 0; i < 256; i++) {
            wrbuf[i + 4] = i % 256 + 0x30;
        }

        SpiWrite(handler, wrbuf, &txlen);
        spi_printf("\n-----------------spi program--------------------\n");
    }

    if (strcmp(argv[1], "read") == 0) {
        if (argc > 2) {
            addr = simple_strtoul(argv[2], NULL, 16);
        }
        rxlen = 256;
        memset(rdbuf, 0x22, sizeof(rdbuf));
        SpiReadFlash(handler, addr, rdbuf, &rxlen);
        dump_spi_buffer(rdbuf, 256);
        spi_printf("\n-----------------spi read--------------------\n");
    }

    if (strcmp(argv[1], "bit32") == 0) {
        cyg_uint32 buf32bit[4] = {0x30313233, 0x34353637, 0x38393a3b, 0x3c3d3e3f};
        handler = SpiInit(&spi_flash_32bit_config);
        txlen = sizeof(buf32bit);
        spi_printf("\ntxlen = %d\n", txlen);

        SpiWrite(handler, (cyg_uint8 *)buf32bit, &txlen);
        spi_printf("\n-----------------spi test 32bit--------------------\n");
    }

    if (strcmp(argv[1], "bit16") == 0) {
        cyg_uint16 buf16bit[4] = {0x1122, 0x3344, 0x5566, 0x7788};
        handler = SpiInit(&spi_flash_16bit_config);
        txlen = sizeof(buf16bit);
        spi_printf("\ntxlen = %d\n", txlen);

        res = SpiWrite(handler, (cyg_uint8 *)buf16bit, &txlen);
        if (res != 0) {
            diag_printf("\nbit16 error = %d\n", res);
        }
        spi_printf("\n-----------------spi test 16bit--------------------\n");
    }

    if (strcmp(argv[1], "erase") == 0) {
        if (argc > 2) {
            addr = simple_strtoul(argv[2], NULL, 16);
        }
        txlen = 1;
        wrbuf[0] = 0x06;
        SpiWrite(handler, wrbuf, &txlen);

        SpiEraseFlash(handler, addr);
        spi_printf("\n-----------------spi erase--------------------\n");
    }

    if (strcmp(argv[1], "get") == 0) {
        cyg_uint32 baud = 0;

        res = SpiGetBaud(handler, &baud);
        if (res != 0) {
            diag_printf("\nget baud error = %d\n", res);
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
            diag_printf("\nset baud error = %d\n", res);
        }
    }

    return res;
}

#endif

