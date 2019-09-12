#define CONFIG_SPI_3902_TEST
#ifdef CONFIG_SPI_3902_TEST

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

#define DEBUG_SPI4
#ifdef DEBUG_SPI4
#define spi_printf printf
#else
#define spi_printf(fmt, ...)
#endif

static spi_dev_config_t spi3wire_3902_config = {
    .spi3wire_cfg.index = 4,
    .spi3wire_cfg.cs = 0,
    .spi3wire_cfg.baud = 80000,
    .spi3wire_cfg.addr_width = 6,
    .spi3wire_cfg.data_width = 20,
};
cyg_uint32 txbuf2[4] = {0xa33333, 0xa66666};
cyg_uint32 rxbuf2[39] = {
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
    0x1555555,
    0x1655555,
    0x1755555,
    0x1855555,
    0x1955555,
    0x1a55555,
    0x1b55555,
    0x1c55555,
    0x1d55555,
    0x1e55555,
    0x1f55555,
    0x2055555,
    0x2155555,
    0x2255555,
    0x2355555,
    0x2455555,
    0x2555555,
    0x2655555,
};

u32 const ch1_cfg[12] = {
    0x26801AB,
    0x050B0C0,
    0x091DD75,
    0x0B7FFFE,
    0x0FFD662,
    0x130B0C0,
    0x144208D,
    0x1527975,
    0x177FFFE,
    0x1E0B0C0,
    0x201F375,
    0x227FFFE
};

u32 const ch2_cfg[12] = {
    0x26801AB,
    0x050B0C0,
    0x091DD75,
    0x0B7FFFE,
    0x0FFD662,
    0x130B0C0,
    0x144208D,
    0x1527975,
    0x177FFFE,
    0x1E0B0C0,
    0x201F375,
    0x227FFFE
};

u32 const ch3_cfg[12] = {
    0x26801AB,
    0x050B0C0,
    0x091DD75,
    0x0B7FFFE,
    0x0FFD662,
    0x130B0C0,
    0x144208D,
    0x1527975,
    0x177FFFE,
    0x1E0B0C0,
    0x201F375,
    0x227FFFE
};

static void *spi4_handler = NULL;

int spi_init_rx3902(void)
{
    spi4_handler = SpiInit(&spi3wire_3902_config);
    return 0;
}

int spi_read_rx3902(int argc, const char **argv)
{
    cyg_uint32 txlen = 4;
    cyg_uint32 reg_r = 5;
    cyg_uint32 value = 0x12345;

    if(argc > 2) {
        reg_r = simple_strtoul(argv[2], NULL, 16);
    }
    if (argc > 3) {
        value = simple_strtoul(argv[3], NULL, 16);
    }

    value = value | (reg_r << 20);
    SpiRead(spi4_handler, (cyg_uint8 *)&value, &txlen);
    spi_printf("read value: 0x%05x\n", value & 0xfffff);
    return 0;
}

int spi_write_rx3902(int argc, const char **argv)
{
    cyg_uint32 txlen = 4;
    cyg_uint32 reg_r = 5;
    cyg_uint32 value = 0x12345;

    if(argc > 2) {
        reg_r = simple_strtoul(argv[2], NULL, 16);
    }
    if (argc > 3) {
        value = simple_strtoul(argv[3], NULL, 16);
    }
    value = value | (reg_r << 20);
    SpiWrite(spi4_handler, (cyg_uint8 *)&value, &txlen);
    spi_printf("write value: 0x%05x\n", value& 0xfffff);
    return 0;
}

int spi_read_rx3902_default(void)
{
    cyg_uint32 txlen = 4;
    cyg_uint32 i = 0;

    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[0], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[1], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[2], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[3], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[4], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[5], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[6], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[7], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[8], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[9], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[10], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[11], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[12], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[13], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[14], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[15], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[16], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[17], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[18], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[19], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[20], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[21], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[22], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[23], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[24], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[25], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[26], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[27], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[28], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[29], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[30], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[31], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[32], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[33], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[34], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[35], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[36], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[37], &txlen);
    SpiRead(spi4_handler, (cyg_uint8 *)&rxbuf2[38], &txlen);
    spi_printf("\n%s: \n", __FUNCTION__);
    for (i = 0; i < 39; i++) {
        spi_printf("%02d: 0x%05x\n", i, rxbuf2[i] & 0xfffff);
    }

    return 0;
}

int spi_rx3902_config(void)
{
    cyg_uint32 txlen = 4;
    cyg_uint32 i = 0;
    for (i = 0; i < 12; i++) {
        SpiWrite(spi4_handler, (cyg_uint8 *)&ch1_cfg[i], &txlen);
    }
#if 0
    for (i = 0; i < 12; i++) {
        SpiWrite(4, 0, &ch2_cfg[i], &txlen);
        // delay
    }
    for (i = 0; i < 12; i++) {
        SpiWrite(4, 0, &ch3_cfg[i], &txlen);
        // delay
    }
#endif
    return 0;
}

int spi_rx3902_test(void)
{
    cyg_uint32 txlen = 4;
    cyg_uint32 rxdat = 0x1527975;
    SpiWrite(spi4_handler, (cyg_uint8 *)&rxdat, &txlen);
    cyg_thread_delay(200);
    rxdat = 0x1527f75;
    SpiWrite(spi4_handler, (cyg_uint8 *)&rxdat, &txlen);
    cyg_thread_delay(200);
    return 0;
}

#endif /* CYGPKG_DEVS_SPI_ARM_BP2016 */

#endif /* CONFIG_SPI_3902_TEST */
