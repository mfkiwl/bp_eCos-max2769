//==========================================================================
//
//        bp2016_spi3wire_test.c
//
//        spi 3wire performance test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2018-01-29
// Description:   spi 3wire test demo for bp2016
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
#include <cyg/hal/regs/spi3wire.h>
#include <cyg/io/spi/spi_bp2016.h>
#include <cyg/hal/api/spi_api.h>

// this is for debug trace,
#if (CYGPKG_DEVS_SPI_ARM_BP2016_DEBUG_LEVEL > 0)
#define spi_printf printf
#else
#define spi_printf(fmt, ...)
#endif
#if 0
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
#endif

cyg_uint32 txbuf[4] = {0x855555, 0x8ccccc};
cyg_uint32 rxbuf[21] = {
    0x0055555,
    0x0155555,
    0x0255555,
    0x0355555,
    0x0455555,
    0x0555555,
    0x0655555,
    0x0755555,
    0x0855555,
    0x0955555,
    0x0a55555,
    0x0b55555,
    0x0c55555,
    0x0d55555,
    0x0e55555,
    0x0f55555,
    0x1055555,
    0x1155555,
    0x1255555,
    0x1355555,
    0x1455555,
};

cyg_uint32 txbuf1[4] = {0x633333, 0x666666};
cyg_uint32 rxbuf1[21] = {
    0x0055555,
    0x0155555,
    0x0255555,
    0x0355555,
    0x0455555,
    0x0555555,
    0x0655555,
    0x0755555,
    0x0855555,
    0x0955555,
    0x0a55555,
    0x0b55555,
    0x0c55555,
    0x0d55555,
    0x0e55555,
    0x0f55555,
    0x1055555,
    0x1155555,
    0x1255555,
    0x1355555,
    0x1455555,
};

#if 0
static spi_dev_config_t spi3wire_3605_config = {
    .index = 4,
    .cs = 2,
    .spi3wire_cfg.baud = 80000,
    .spi3wire_cfg.addr_width = 5,
    .spi3wire_cfg.data_width = 20,
};

static spi_dev_config_t spi3wire_3601_config = {
    .index = 4,
    .cs = 1,
    .spi3wire_cfg.baud = 80000,
    .spi3wire_cfg.addr_width = 5,
    .spi3wire_cfg.data_width = 20,
};
static spi_dev_config_t spi4_3902_config = {
    .index = 4,
    .cs = 0,
    .spi3wire_cfg.baud = 80000,
    .spi3wire_cfg.addr_width = 5,
    .spi3wire_cfg.data_width = 20,
};

static void *spi4_handler[3];
#endif

cyg_uint32 spibaud[3] = {0};

extern int spi_init_rx3902(void);
extern int spi_read_rx3902_default(void);
extern int spi_rx3902_config(void);
extern int spi_rx3902_test(void);
extern int spi_read_rx3902(int argc, const char **argv);
extern int spi_write_rx3902(int argc, const char **argv);

int do_spi3wire_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int res = 0;
    cyg_uint32 i = 0;
    spi_printf("\n-------------spi 3wire testing in----------------------\n");

#if 0
    if (strcmp(argv[1], "init") == 0) {
        spi4_handler[0] = SpiInit(&spi4_3902_config);
        spi4_handler[1] = SpiInit(&spi3wire_3601_config);
        spi4_handler[2] = SpiInit(&spi3wire_3605_config);
    }

    if (strcmp(argv[1], "rx3605") == 0) {
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[0], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[1], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[2], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[3], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[4], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[5], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[6], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[7], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[8], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[9], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[10], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[11], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[12], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[13], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[14], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[15], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[16], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[17], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[18], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[19], &txlen);
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[20], &txlen);

        spi_printf("\n%s: \n", __FUNCTION__);
        for (i = 0; i < 21; i++) {
            spi_printf("%02d: 0x%05x\n", i, rxbuf[i] & 0xfffff);
        }
    }

    if (strcmp(argv[1], "rx3601") == 0) {
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[0], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[1], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[2], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[3], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[4], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[5], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[6], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[7], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[8], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[9], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[10], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[11], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[12], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[13], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[14], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[15], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[16], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[17], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[18], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[19], &txlen);
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[20], &txlen);

        spi_printf("\n%s: \n", __FUNCTION__);
        for (i = 0; i < 21; i++) {
            spi_printf("%02d: 0x%05x\n", i, rxbuf1[i] & 0xfffff);
        }
    }

    if (strcmp(argv[1], "auto") == 0) {
        spi_printf("\n-------------rx3605 auto in----------------------\n");
        SpiWrite(4, 0, (cyg_uint8 *)&txbuf[0], &txlen);
        rxbuf[8] = 0x800000;
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[8], &txlen);
        spi_printf("reg 08: 0x%05x\n\n", (rxbuf[8] & 0xfffff));
        SpiWrite(4, 0, (cyg_uint8 *)&txbuf[1], &txlen);
        rxbuf[8] = 0x800000;
        SpiRead(4, 0, (cyg_uint8 *)&rxbuf[8], &txlen);
        spi_printf("reg 08: 0x%05x\n\n", (rxbuf[8] & 0xfffff));
        spi_printf("\n-------------rx3605 auto end---------------------\n");

        spi_printf("\n-------------rx3601 auto in----------------------\n");
        SpiWrite(4, 1, (cyg_uint8 *)&txbuf1[0], &txlen);
        rxbuf1[6] = 0x600000;
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[6], &txlen);
        spi_printf("reg 06: 0x%05x\n\n", (rxbuf1[6] & 0xfffff));
        SpiWrite(4, 1, (cyg_uint8 *)&txbuf1[1], &txlen);
        rxbuf1[6] = 0x600000;
        SpiRead(4, 1, (cyg_uint8 *)&rxbuf1[6], &txlen);
        spi_printf("reg 06: 0x%05x\n\n", (rxbuf1[6] & 0xfffff));
        spi_printf("\n-------------rx3601 auto end---------------------\n");
    }
#endif

    if (strcmp(argv[1], "init3902") == 0) {
        spi_init_rx3902();
    }

    if (strcmp(argv[1], "read3902") == 0) {
        spi_read_rx3902_default();
    }

    if (strcmp(argv[1], "config3902") == 0) {
        spi_rx3902_config();
    }

    if (strcmp(argv[1], "test3902") == 0) {
        for (i = 0; i < 10; i++) {
            spi_rx3902_test();
        }
    }

    if (strcmp(argv[1], "read") == 0) {
        spi_read_rx3902(argc, argv);
    }
    if (strcmp(argv[1], "write") == 0) {
        spi_write_rx3902(argc, argv);
    }
#if 0
    if (strcmp(argv[1], "set") == 0) {
        spibaud[0] = 66666;
        spibaud[1] = 77777;
        SpiSetBaud(4, 0, &spibaud[0]);
        SpiSetBaud(4, 1, &spibaud[1]);
    }

    if (strcmp(argv[1], "get") == 0) {
        SpiGetBaud(4, 0, &spibaud[0]);
        SpiGetBaud(4, 1, &spibaud[1]);
    }
#endif
    return res;
}

#endif /* CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4 */
