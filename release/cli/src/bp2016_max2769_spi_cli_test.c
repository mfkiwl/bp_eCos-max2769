//==========================================================================
//
//        bp2016_spi_cli_test.c
//
//        spi performance cli test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2018-01-26
// Description:   spi performance cli test for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#if 0
#include <pkgconf/devs_spi_arm_bp2016.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <stdio.h>
#include "common.h"

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016
#include <cyg/hal/regs/spi.h>
#include <cyg/io/spi/spi_bp2016.h>
#include <cyg/hal/api/spi_api.h>

// this is for debug trace,
#if (CYGPKG_DEVS_SPI_ARM_BP2016_DEBUG_LEVEL > 0)
#define spi_printf printf
#else
#define spi_printf(fmt, ...)
#endif

static void dump_spi_buffer(void *buffer, cyg_uint32 len)
{
    cyg_uint32 i = 0;
    cyg_uint8 *buf = (cyg_uint8 *) buffer;

    printf("\n");
    for (i = 0; i < len; i++) {
        printf("%02x ", *(buf + i));
    }
    printf("\n");
}

spi_dev_config_t spi_2769_config = {
    .spi_dw_cfg.cpol = 0,
    .spi_dw_cfg.cpha = 0,
    .spi_dw_cfg.baud = 50000,
    .spi_dw_cfg.bus_width = 32,
};

spi_dev_config_t spi_lo_config = {
    .spi_dw_cfg.cpol = 0,
    .spi_dw_cfg.cpha = 0,
    .spi_dw_cfg.baud = 50000,
    .spi_dw_cfg.bus_width = 24,
};

extern int spi_max2769_config_all(void);
extern int spi_lo_config_all(cyg_uint32 gpiocs8, cyg_uint32 gpiocs9);
extern int spi_xn112_test(const unsigned char argc, const char **argv);

int do_spi_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int res = 0;
    cyg_uint32 len = 0;
    cyg_uint32 timeout = 0;
    cyg_uint32 i = 0;
    static cyg_uint32 gpiocs8 = 8;
    static cyg_uint32 gpiocs9 = 7;

    spi_printf("\n-------------spi testing in --------------------\n");

    if (strcmp(argv[1], "init") == 0) {
        SpiInit(2, 0, &spi_2769_config);
        SpiInit(2, 1, &spi_2769_config);
        SpiInit(2, 2, &spi_2769_config);
        SpiInit(2, 3, &spi_2769_config);
        SpiInit(2, 4, &spi_2769_config);
        SpiInit(2, 5, &spi_2769_config);
        SpiInit(2, 6, &spi_2769_config);
        SpiInit(2, 7, &spi_2769_config);

        SpiInit(2, 8, &spi_lo_config);
        SpiInit(2, 9, &spi_lo_config);
        spi_printf("\n---------------spi init start ------------------------\n");
    }

    if (strcmp(argv[1], "max2769") == 0) {
        spi_max2769_config_all();
        spi_printf("\n-----------------spi config max2769 start--------------------\n");
    }

    if (strcmp(argv[1], "gpio") == 0) {
        if (argc > 3) {
            gpiocs8 = simple_strtoul(argv[2], NULL, 10);
            gpiocs9 = simple_strtoul(argv[3], NULL, 10);
        }
        spi_printf("\n-----------------spi config gpio gpiocs8 = %d, gpiocs9 = %d   --------------------\n", gpiocs8, gpiocs9);
    }

    if (strcmp(argv[1], "lo") == 0) {
        spi_lo_config_all(8, 7);
        spi_printf("\n---------------spi config lo------------------------\n");
    }

    if (strcmp(argv[1], "auto") == 0) {
        spi_max2769_config_all();
        spi_lo_config_all(8, 7);
        spi_printf("\n----------spi config max2769 and lo end\n");
    }

    if (strcmp(argv[1], "xn112") == 0) {
        spi_xn112_test(argc, argv);
        spi_printf("\n----------spi config xn112 end\n");
    }

    return res;
}

#endif
#endif
