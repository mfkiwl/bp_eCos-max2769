// #define CONFIG_SPI_XN112_TEST
#ifdef CONFIG_SPI_XN112_TEST

#include <pkgconf/devs_spi_arm_bp2016.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/hal/hal_diag.h>
#include <stdio.h>

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016
#include <cyg/hal/regs/spi.h>
#include <cyg/io/spi/spi_bp2016.h>
#include <cyg/hal/api/spi_api.h>

#if (CYGPKG_DEVS_SPI_ARM_BP2016_DEBUG_LEVEL > 0)
#define spi_printf printf
#else
#define spi_printf(fmt, ...)
#endif

#define writel_relaxed(v,c)	((*(volatile u32 *) (c)) = (v))
#define readl_relaxed(c)	(*(volatile u32 *) (c))

#define R0 0x0
#define R1 0x1
#define R2 0x2
#define R3 0x3
#define R4 0x4
#define R5 0x5
#define R6 0x6
#define R7 0x7
#define R8 0x8
#define R9 0x9
#define R10 10
#define R11 11
#define R12 12
#define R13 13

static int spi_write_xn112(int reg, const int arg)
{
    int value = 0;
    int txlen = 1;
    value = (1 << 20) | ((reg & 0xf) << 16) | (arg & 0xffff);
    SpiWrite(2, 0, (cyg_uint8 *)&value, &txlen);
    return 0;
}

spi_dev_config_t spi_xn112_config = {
    .spi_dw_cfg.cpol = 0,
    .spi_dw_cfg.cpha = 0,
    .spi_dw_cfg.baud = 50000,
    .spi_dw_cfg.bus_width = 21,
};

int spi_xn112_test(const unsigned char argc, const char **argv)
{
    int res = 0;
    u32 reg = 0;
    u32 value = 0;
    u32 i = 0;

    if (strcmp(argv[2], "init") == 0) {
        SpiInit(2, 0, &spi_xn112_config);
        spi_write_xn112(R8, 0x8e85);
        spi_write_xn112(R11, 0x9e85);
        printf("xn112 init finished\n");
        spi_printf("\n%s: argc = %d, argv[2] = %s\n", __FUNCTION__, argc, argv[2]);
        return res;
    }

    if (strcmp(argv[2], "write") == 0) {
        reg = simple_strtoul(argv[3], NULL, 10);
        value = simple_strtoul(argv[4], NULL, 16);
        spi_printf("\nreg = %d, value = 0x%x\n", reg, value);
        spi_write_xn112(reg, value & 0xffff);
        return res;
    }

    if (strcmp(argv[2], "test") == 0) {
        for (i = 0; i < 10; i++) {
            spi_write_xn112(R8, 0x8e81);
            cyg_thread_delay(50);
            spi_write_xn112(R8, 0x8e85);
            cyg_thread_delay(50);
        }
        return res;
    }

    // common
    {
        spi_write_xn112(R4, 0x2300);
        spi_write_xn112(R5, 0x9a84);
        spi_write_xn112(R7, 0x808f);
        spi_write_xn112(R8, 0x8e81);
        spi_write_xn112(R9, 0x6010);
        spi_write_xn112(R10, 0x8088);
        spi_write_xn112(R11, 0x9e81);
        spi_write_xn112(R12, 0x6212);
        spi_write_xn112(R13, 0x8900);
    }

    if (strcmp(argv[2], "b1b3") == 0) {
        spi_write_xn112(R1, 0xc51f);
        spi_write_xn112(R2, 0x5027);
        spi_write_xn112(R6, 0x6004);
        spi_printf("\n%s: argc = %d, argv[2] = %s\n", __FUNCTION__, argc, argv[2]);
    }

    if (strcmp(argv[2], "b1b2") == 0) {
        spi_write_xn112(R1, 0xc51f);
        spi_write_xn112(R2, 0x9877);
        spi_write_xn112(R6, 0xe004);
        spi_printf("\n%s: argc = %d, argv[2] = %s\n", __FUNCTION__, argc, argv[2]);
    }

    if (strcmp(argv[2], "l1ca") == 0) {
        spi_write_xn112(R1, 0xc67f);
        spi_write_xn112(R2, 0x5027);
        spi_write_xn112(R6, 0x6004);
        spi_printf("\n%s: argc = %d, argv[2] = %s\n", __FUNCTION__, argc, argv[2]);
    }

    if (strcmp(argv[2], "l1f") == 0) {
        spi_write_xn112(R1, 0xca3f);
        spi_write_xn112(R2, 0x502b);
        spi_write_xn112(R6, 0x6004);
        spi_printf("\n%s: argc = %d, argv[2] = %s\n", __FUNCTION__, argc, argv[2]);
    }

    if (strcmp(argv[2], "l1p") == 0) {
        spi_write_xn112(R1, 0xc67f);
        spi_write_xn112(R2, 0x9b3f);
        spi_write_xn112(R6, 0xe004);
        spi_printf("\n%s: argc = %d, argv[2] = %s\n", __FUNCTION__, argc, argv[2]);
    }

    return res;
}

#endif /* CYGPKG_DEVS_SPI_ARM_BP2016 */

#endif /* CONFIG_SPI_XN112_TEST */

