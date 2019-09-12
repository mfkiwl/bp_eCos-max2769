//==========================================================================
//
//        spi_test.c
//
//        spi performance test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2018-01-10
// Description:   spi performance test for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <pkgconf/devs_spi_arm_bp2016.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <stdio.h>

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

//===========================================================================
//                               DATA TYPES
//===========================================================================
typedef struct st_thread_data {
    cyg_thread obj;
    cyg_uint32 stack[4096];
    cyg_handle_t hdl;
} thread_data_t;

//===========================================================================
//                              LOCAL DATA
//===========================================================================
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS0
#define THREAD0_PRIOIRTY 14
cyg_thread_entry_t spi_thread0;
thread_data_t spi_thread0_data;
#endif
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS1
#define THREAD1_PRIOIRTY 13
cyg_thread_entry_t spi_thread1;
thread_data_t spi_thread1_data;
#endif
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS2
#define THREAD2_PRIOIRTY 12
cyg_thread_entry_t spi_thread2;
thread_data_t spi_thread2_data;
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

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS0
void spi_thread0(cyg_addrword_t data)
{
    spi_printf("\n-------------spi thread in----------------------\n");

    while (1) {
        cyg_thread_delay(500);
        spi_printf("\n-------------spi thread 0 in----------------------\n");
    }
}
#endif

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS1
void spi_thread1(cyg_addrword_t data)
{
    spi_printf("\n-------------spi thread in----------------------\n");

    while (1) {
        cyg_thread_delay(500);
        spi_printf("\n-------------spi thread 1 in----------------------\n");
    }
}
#endif

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS2
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

u32 tx_buf[10] = {0x55555555, 0x33333333, 0x0f0f0f0f, 0xcccccccc};

extern int spi_max2769_test_gpsl1(void);
extern int spi_max2769_config_all(void);
extern int spi_lo_config_all(void);

void spi_thread2(cyg_addrword_t data)
{
    spi_printf("\n-------------spi thread in----------------------\n");
    cyg_uint32 txlen = 1;

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

    spi_lo_config_all();
    spi_max2769_config_all();
    while (1) {
        spi_printf("\n-------------spi thread 2 in----------------------\n");
        // spi_max2769_test_gpsl1();
        cyg_thread_delay(5000);
        spi_printf("\n-------------spi thread 2 out ----------------------\n");
    }
}
#endif

void cyg_start(void)
{
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS0
    cyg_thread_create(THREAD0_PRIOIRTY, spi_thread0, (cyg_addrword_t) 0, "bp2016_spi_thread0",
                      (void *) spi_thread0_data.stack, 1024 * sizeof(long), &spi_thread0_data.hdl,
                      &spi_thread0_data.obj);
    cyg_thread_resume(spi_thread0_data.hdl);
#endif

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS1
    cyg_thread_create(THREAD1_PRIOIRTY, spi_thread1, (cyg_addrword_t) 0, "bp2016_spi_thread1",
                      (void *) spi_thread1_data.stack, 1024 * sizeof(long), &spi_thread1_data.hdl,
                      &spi_thread1_data.obj);
    cyg_thread_resume(spi_thread1_data.hdl);
#endif

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS2
    cyg_thread_create(THREAD2_PRIOIRTY, spi_thread2, (cyg_addrword_t) 0, "bp2016_spi_thread2",
                      (void *) spi_thread2_data.stack, 1024 * sizeof(long), &spi_thread2_data.hdl,
                      &spi_thread2_data.obj);
    cyg_thread_resume(spi_thread2_data.hdl);
#endif

    cyg_scheduler_start();
}

#endif
