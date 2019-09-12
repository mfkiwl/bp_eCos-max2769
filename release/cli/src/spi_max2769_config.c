#define CONFIG_SPI_MAX2769_TEST
#ifdef CONFIG_SPI_MAX2769_TEST

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

#define writel_relaxed(v,c)	((*(volatile u32 *) (c)) = (v))
#define readl_relaxed(c)	(*(volatile u32 *) (c))

#define NOUSE_GPIO  0XFF
#define CS8_GPIO    13
#define CS9_GPIO    14

static spi_dev_config_t lo_cfg[2] = {
    {
        .spi_dw_cfg.index = 2,
        .spi_dw_cfg.cs = 8,
        .spi_dw_cfg.cpol = 1,
        .spi_dw_cfg.cpha = 1,
        .spi_dw_cfg.baud = 50000,
        .spi_dw_cfg.bus_width = 24,
        .spi_dw_cfg.gpio_num = CS8_GPIO,
    },

    {
        .spi_dw_cfg.index = 2,
        .spi_dw_cfg.cs = 9,
        .spi_dw_cfg.cpol = 1,
        .spi_dw_cfg.cpha = 1,
        .spi_dw_cfg.baud = 50000,
        .spi_dw_cfg.bus_width = 24,
        .spi_dw_cfg.gpio_num = CS9_GPIO,
    }
};

static spi_dev_config_t max2769_cfg[8] = {
    {
        .spi_dw_cfg.index = 2,
        .spi_dw_cfg.cs = 0,
        .spi_dw_cfg.cpol = 1,
        .spi_dw_cfg.cpha = 1,
        .spi_dw_cfg.baud = 50000,
        .spi_dw_cfg.bus_width = 32,
        .spi_dw_cfg.gpio_num = NOUSE_GPIO,
    },

    {
        .spi_dw_cfg.index = 2,
        .spi_dw_cfg.cs = 1,
        .spi_dw_cfg.cpol = 1,
        .spi_dw_cfg.cpha = 1,
        .spi_dw_cfg.baud = 50000,
        .spi_dw_cfg.bus_width = 32,
        .spi_dw_cfg.gpio_num = NOUSE_GPIO,
    },

    {
        .spi_dw_cfg.index = 2,
        .spi_dw_cfg.cs = 2,
        .spi_dw_cfg.cpol = 1,
        .spi_dw_cfg.cpha = 1,
        .spi_dw_cfg.baud = 50000,
        .spi_dw_cfg.bus_width = 32,
        .spi_dw_cfg.gpio_num = NOUSE_GPIO,
    },

    {
        .spi_dw_cfg.index = 2,
        .spi_dw_cfg.cs = 3,
        .spi_dw_cfg.cpol = 1,
        .spi_dw_cfg.cpha = 1,
        .spi_dw_cfg.baud = 50000,
        .spi_dw_cfg.bus_width = 32,
        .spi_dw_cfg.gpio_num = NOUSE_GPIO,
    },

    {
        .spi_dw_cfg.index = 2,
        .spi_dw_cfg.cs = 4,
        .spi_dw_cfg.cpol = 1,
        .spi_dw_cfg.cpha = 1,
        .spi_dw_cfg.baud = 50000,
        .spi_dw_cfg.bus_width = 32,
        .spi_dw_cfg.gpio_num = NOUSE_GPIO,
    },

    {
        .spi_dw_cfg.index = 2,
        .spi_dw_cfg.cs = 5,
        .spi_dw_cfg.cpol = 1,
        .spi_dw_cfg.cpha = 1,
        .spi_dw_cfg.baud = 50000,
        .spi_dw_cfg.bus_width = 32,
        .spi_dw_cfg.gpio_num = NOUSE_GPIO,
    },

    {
        .spi_dw_cfg.index = 2,
        .spi_dw_cfg.cs = 6,
        .spi_dw_cfg.cpol = 1,
        .spi_dw_cfg.cpha = 1,
        .spi_dw_cfg.baud = 50000,
        .spi_dw_cfg.bus_width = 32,
        .spi_dw_cfg.gpio_num = NOUSE_GPIO,
    },

    {
        .spi_dw_cfg.index = 2,
        .spi_dw_cfg.cs = 7,
        .spi_dw_cfg.cpol = 1,
        .spi_dw_cfg.cpha = 1,
        .spi_dw_cfg.baud = 50000,
        .spi_dw_cfg.bus_width = 32,
        .spi_dw_cfg.gpio_num = NOUSE_GPIO,
    },
};

static void *max2769_handler[8];
static void *lo_handler[2];

u32 const GPSL1_BKP[10] = {
    0xA2550F90,
    0x85522a81,
    0xD4FF15C2,
    0x98C00083,
    0x18b21404,
    0x80000705,
    0x80000006,
    0x10061B27,
    0x1E0F4018,
    0x14C04029
};

u32 const GPSL1[10] = {
    0xA2550F90,
    0x85522a81,
    // 0x85522881,
    0xD4FF15C2,
    0x98C00083,
    0x18A21404,
    0x80000705,
    0x80000006,
    0x10061B27,
    0x1E0F4018,
    0x14C04029
};

u32 const BD2B1[10] = {
    0xA2550F90,
    0x85522881,
    0xD4FF15C2,
    0x98C00083,
    0x185C1404,
    0x80000705,
    0x80000006,
    0x10061B27,
    0x1E0F4018,
    0x14C04029
};

u32 const GLONL1[10] = {
    0xA2550FD0,
    0x85522881,
    0xD4FF15C2,
    0x98C00083,
    0x19001404,
    0x80000705,
    0x80000006,
    0x10061B27,
    0x1E0F4018,
    0x14C04029
};

u32 const GPSL2[10] = {
    0xA2550F90,
    0x85522881,
    0xD4FF15C2,
    0x98C00083,
    0x18EA1404,
    0x80000705,
    0x80000006,
    0x10061B27,
    0x1E0F4018,
    0x14C04029
};

u32 const BD2B2[10] = {
    0xA2550F90,
    // 0x85522881,
    0x85522a81,
    0xD4FF15C2,
    0x98C00083,
    0x18BA1404,
    0x80000705,
    0x80000006,
    0x10061B27,
    0x1E0F4018,
    0x14C04029
};

u32 const GLONL2[10] = {
    0xA2550FD0,
    0x85522881,
    0xD4FF15C2,
    0x98C00083,
    0x18CE1404,
    0x80000705,
    0x80000006,
    0x10061B27,
    0x1E0F4018,
    0x14C04029
};

u32 const BD2B3[10] = {
    0xA2550F90,
    0x85522881,
    0xD4FF15C2,
    0x98C00083,
    0x19281404,
    0x80000705,
    0x80000006,
    0x10061B27,
    0x1E0F4018,
    0x14C04029
};

u32 const GPSL5[10] = {
    0xA2550F90,
    0x85522881,
    0xD4FF15C2,
    0x98C00083,
    0x183A1404,
    0x80000705,
    0x80000006,
    0x10061B27,
    0x1E0F4018,
    0x14C04029
};

u32 const LO2[3] = {
    0x013124,
    0x017502,
    0x000051
};

u32 const LO1[3] = {
    0x013124,
    0x015702,
    0x000051
};

static int spi_max2769_config(void *handler, const cyg_uint32 *wbuf)
{
    Cyg_ErrNo res = ENOERR;
    int i = 0;
    cyg_uint32 txlen = 4;

    for(i = 0; i < 10; i++) {
        SpiWrite(handler, (cyg_uint8 *)&wbuf[i], &txlen);
        cyg_thread_delay(1);
    }
    return res;
}

void spi_max2769_test_gpsl1(void)
{
    int i = 0, loops = 10;

    for (i = 0; i < loops; i++) {
        spi_printf("gpsl1 test 1MHz start...");
        spi_max2769_config(max2769_handler[1], GPSL1);
        cyg_thread_delay(500);
        spi_printf("gpsl1 test 1MHz finished\n");

        spi_printf("gpsl1 test 5MHz start.....");
        spi_max2769_config(max2769_handler[1], GPSL1_BKP);
        cyg_thread_delay(500);
        spi_printf("gpsl1 test 5MHz finished\n");
    }
}

void spi_max2769_config_bd2b2(void)
{
    spi_max2769_config(max2769_handler[5], BD2B2);
    cyg_thread_delay(500);
}

int spi_max2769_config_all(void)
{
    int res = 0;

    spi_max2769_config(max2769_handler[0], GLONL1);
    cyg_thread_delay(5);
    spi_max2769_config(max2769_handler[1], GPSL1);
    cyg_thread_delay(5);
    spi_max2769_config(max2769_handler[2], BD2B3);
    cyg_thread_delay(5);
    spi_max2769_config(max2769_handler[3], GLONL2);
    cyg_thread_delay(5);
    spi_max2769_config(max2769_handler[4], GPSL2);
    cyg_thread_delay(5);
    spi_max2769_config(max2769_handler[5], BD2B2);
    cyg_thread_delay(5);
    spi_max2769_config(max2769_handler[6], GPSL5);
    cyg_thread_delay(5);
    spi_max2769_config(max2769_handler[7], BD2B1);
    cyg_thread_delay(5);

    return res;
}

static void spi_lo_config(void *handler, const cyg_uint32 *wbuf)
{
    u32 cnt = 0;
    cyg_uint32 txlen = 3;

    for (cnt = 0; cnt < 3; cnt++) {
        SpiWrite(handler, (cyg_uint8 *)&wbuf[cnt], &txlen);
        cyg_thread_delay(2);
    }
    spi_printf("lo1 config finished\n");
}

int spi_lo_config_all(cyg_uint32 gpiocs8, cyg_uint32 gpiocs9)
{
    int res = 0;

    spi_lo_config(lo_handler[0], LO1);
    spi_lo_config(lo_handler[1], LO2);

    return res;
}

int do_max2769_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{

    spi_printf("\n-------------spi w5500 testing start --------------------\n");

    if (strcmp(argv[1], "init") == 0) {
        max2769_handler[0] = SpiInit(&max2769_cfg[0]);
        max2769_handler[1] = SpiInit(&max2769_cfg[1]);
        max2769_handler[2] = SpiInit(&max2769_cfg[2]);
        max2769_handler[3] = SpiInit(&max2769_cfg[3]);
        max2769_handler[4] = SpiInit(&max2769_cfg[4]);
        max2769_handler[5] = SpiInit(&max2769_cfg[5]);
        max2769_handler[6] = SpiInit(&max2769_cfg[6]);
        max2769_handler[7] = SpiInit(&max2769_cfg[7]);
        lo_handler[0] = SpiInit(&lo_cfg[0]);
        lo_handler[1] = SpiInit(&lo_cfg[1]);

        if ((max2769_handler[0] == NULL) || (max2769_handler[1] == NULL) ||
                (max2769_handler[2] == NULL) || (max2769_handler[3] == NULL) ||
                (max2769_handler[4] == NULL) || (max2769_handler[5] == NULL) ||
                (max2769_handler[6] == NULL) || (max2769_handler[7] == NULL) ||
                (lo_handler[0] == NULL) || (lo_handler[1] == NULL)) {
            spi_printf("\n%s: handler NULL error\n", __FUNCTION__);
        }
        spi_printf("\n---------------max2769 init ------------------------\n");
    }

    if (strcmp(argv[1], "bd2") == 0) {
        spi_max2769_config_bd2b2();
    }

    if (strcmp(argv[1], "gps") == 0) {
        spi_max2769_test_gpsl1();
    }

    if (strcmp(argv[1], "all") == 0) {
        spi_max2769_config_all();
        spi_printf("\n-----------------spi config max2769 start--------------------\n");
    }

    if (strcmp(argv[1], "lo") == 0) {
        spi_lo_config_all(8, 7);
        spi_printf("\n---------------spi config lo------------------------\n");
    }

    spi_printf("\n-------------spi w5500 testing end--------------------\n");

    return 0;
}

#endif /* CYGPKG_DEVS_SPI_ARM_BP2016 */
#endif /* CONFIG_SPI_MAX2769_TEST */
