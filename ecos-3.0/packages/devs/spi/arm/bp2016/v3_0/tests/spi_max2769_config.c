#define CONFIG_SPI_MAX2769_TEST
#ifdef CONFIG_SPI_MAX2769_TEST

#include <pkgconf/devs_spi_arm_bp2016.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
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

static int spi_max2769_config(cyg_uint32 index, cyg_uint32 cs, const cyg_uint32 *wbuf)
{
    Cyg_ErrNo res = ENOERR;
    int i = 0;
    cyg_uint32 txlen = 1;

    for(i = 0; i < 10; i++) {
        SpiWrite(index, cs, &wbuf[i], &txlen);
        cyg_thread_delay(1);
    }
    return res;
}

int spi_max2769_test_gpsl1(void)
{
    int i = 0, loops = 10;

    for (i = 0; i < loops; i++) {
        spi_printf("gpsl1 test 1MHz start...");
        spi_max2769_config(2, 1, GPSL1);
        cyg_thread_delay(500);
        spi_printf("gpsl1 test 1MHz finished\n");

        spi_printf("gpsl1 test 5MHz start.....");
        spi_max2769_config(2, 1, GPSL1_BKP);
        cyg_thread_delay(500);
        spi_printf("gpsl1 test 5MHz finished\n");
    }
}

int spi_max2769_config_bd2b2(void)
{
    spi_max2769_config(2, 5, BD2B2);
    cyg_thread_delay(500);
}

int spi_max2769_config_all(void)
{
    int res = 0;

    spi_max2769_config(2, 0, GLONL1);
    cyg_thread_delay(1);

    spi_max2769_config(2, 1, GPSL1);
    cyg_thread_delay(5);

    spi_max2769_config(2, 2, BD2B3);
    cyg_thread_delay(5);

    spi_max2769_config(2, 3, GLONL2);
    cyg_thread_delay(5);

    spi_max2769_config(2, 4, GPSL2);
    cyg_thread_delay(5);

    spi_max2769_config(2, 5, BD2B2);
    cyg_thread_delay(5);

    spi_max2769_config(2, 6, GPSL5);
    cyg_thread_delay(5);

    spi_max2769_config(2, 7, BD2B1);
    cyg_thread_delay(5);

    return res;
}

extern bool hal_gpio_set_output(unsigned int gpio_num);
extern bool hal_gpio_set_high(unsigned int gpio_num);
extern bool hal_gpio_set_low(unsigned int gpio_num);

static int spi_lo_config(cyg_uint32 index, cyg_uint32 cs, cyg_uint32 gpio, const cyg_uint32 *wbuf)
{
    int res = 0;
    u32 cnt = 0;
    u32 regtmp = 0, regbkp = 0;
    cyg_uint32 txlen = 1;
    regbkp = readl_relaxed(0xc01d0070);
    regtmp = regbkp | (0x3 << 20);
    spi_printf("\nreg = 0x%x\n", regtmp);

    // need iomux
    writel_relaxed(0xc01d0070, regtmp);

    hal_gpio_set_output(gpio);
    for (cnt = 0; cnt < 2; cnt++) {
        hal_gpio_set_low(gpio);
        SpiWrite(2, cs, &wbuf[cnt], &txlen);
        cyg_thread_delay(2);
        hal_gpio_set_high(gpio);
        cyg_thread_delay(1);
    }
    hal_gpio_set_low(gpio);
    cyg_thread_delay(5);
    SpiWrite(2, cs, &wbuf[2], &txlen);
    cyg_thread_delay(2);
    hal_gpio_set_high(gpio);
    spi_printf("lo1 test finished\n");
    // need reset iomux
    writel_relaxed(0xc01d0070, regbkp);
}

int spi_lo_config_all(cyg_uint32 gpiocs8, cyg_uint32 gpiocs9)
{
    int res = 0;

    spi_lo_config(2, 8, gpiocs8, LO1);
    spi_lo_config(2, 9, gpiocs9, LO2);

    return res;
}

#endif /* CONFIG_SPI_MAX2769_TEST */
#endif /* CYGPKG_DEVS_SPI_ARM_BP2016 */
