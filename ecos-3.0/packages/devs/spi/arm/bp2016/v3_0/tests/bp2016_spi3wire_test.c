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
#include <pkgconf/devs_spi_arm_bp2016.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <stdio.h>

#ifdef CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4
#include <cyg/hal/regs/spi3wire.h>
#include <cyg/io/spi/spi_bp2016.h>
#include <cyg/hal/api/spi_api.h>

#define DEV_SPI3WIRE_RX3605_TEST_ENABLE 0
#define DEV_SPI3WIRE_RX3601_TEST_ENABLE 0
#define DEV_SPI3WIRE_RX3902_TEST_ENABLE 1

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
    cyg_uint32 stack[1024 * 8];
    cyg_handle_t hdl;
} thread_data_t;

//===========================================================================
//                              LOCAL DATA
//===========================================================================
#if (DEV_SPI3WIRE_RX3605_TEST_ENABLE > 0)
#define THREAD0_PRIOIRTY 14
cyg_thread_entry_t spi_thread0;
thread_data_t spi_thread0_data;
#endif /* DEV_SPI3WIRE_RX3605_TEST_ENABLE */

#if (DEV_SPI3WIRE_RX3601_TEST_ENABLE > 0)
#define THREAD1_PRIOIRTY 15
cyg_thread_entry_t spi_thread1;
thread_data_t spi_thread1_data;
#endif /* DEV_SPI3WIRE_RX3601_TEST_ENABLE */

#if (DEV_SPI3WIRE_RX3902_TEST_ENABLE > 0)
#define THREAD2_PRIOIRTY 16
cyg_thread_entry_t spi_thread2;
thread_data_t spi_thread2_data;
#endif /* DEV_SPI3WIRE_RX3902_TEST_ENABLE */

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

#if (DEV_SPI3WIRE_RX3605_TEST_ENABLE > 0)
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

static spi_dev_config_t spi3wire_3605_config = {
    .spi3wire_cfg.baud = 80000,
    .spi3wire_cfg.addr_width = 5,
    .spi3wire_cfg.data_width = 20,
};

void spi_thread0(cyg_addrword_t data)
{
    cyg_uint32 txlen = 1;
    cyg_uint32 i = 0;
    spi_printf("\n-------------spi thread0 in----------------------\n");
    spi_printf("\n3605 baud = %d, addr = 0x%x, data = 0x%x\n", spi3wire_3605_config.spi3wire_cfg.baud, spi3wire_3605_config.spi3wire_cfg.addr_width, spi3wire_3605_config.spi3wire_cfg.data_width);
    spi_printf("\n\n");

    SpiInit(4, 0, &spi3wire_3605_config);
    SpiRead(4, 0, &rxbuf[0], &txlen);
    SpiRead(4, 0, &rxbuf[1], &txlen);
    SpiRead(4, 0, &rxbuf[2], &txlen);
    SpiRead(4, 0, &rxbuf[3], &txlen);
    SpiRead(4, 0, &rxbuf[4], &txlen);
    SpiRead(4, 0, &rxbuf[5], &txlen);
    SpiRead(4, 0, &rxbuf[6], &txlen);
    SpiRead(4, 0, &rxbuf[7], &txlen);
    SpiRead(4, 0, &rxbuf[8], &txlen);
    SpiRead(4, 0, &rxbuf[9], &txlen);
    SpiRead(4, 0, &rxbuf[10], &txlen);
    SpiRead(4, 0, &rxbuf[11], &txlen);
    SpiRead(4, 0, &rxbuf[12], &txlen);
    SpiRead(4, 0, &rxbuf[13], &txlen);
    SpiRead(4, 0, &rxbuf[14], &txlen);
    SpiRead(4, 0, &rxbuf[15], &txlen);
    SpiRead(4, 0, &rxbuf[16], &txlen);
    SpiRead(4, 0, &rxbuf[17], &txlen);
    SpiRead(4, 0, &rxbuf[18], &txlen);
    SpiRead(4, 0, &rxbuf[19], &txlen);
    SpiRead(4, 0, &rxbuf[20], &txlen);
    spi_printf("\n%s: \n", __FUNCTION__);
    for (i = 0; i < 21; i++) {
        spi_printf("%02d: 0x%05x\n\n", i, rxbuf[i] & 0xfffff);
    }

    while (1) {
        spi_printf("\n-------------rx3605 thread loop in----------------------\n");
        SpiWrite(4, 0, &txbuf[0], &txlen);
        rxbuf[8] = 0x800000;
        SpiRead(4, 0, &rxbuf[8], &txlen);
        spi_printf("reg 08: 0x%05x\n\n", (rxbuf[8] & 0xfffff));
        cyg_thread_delay(200);
        SpiWrite(4, 0, &txbuf[1], &txlen);
        rxbuf[8] = 0x800000;
        SpiRead(4, 0, &rxbuf[8], &txlen);
        spi_printf("reg 08: 0x%05x\n\n", (rxbuf[8] & 0xfffff));
        spi_printf("\n-------------rx3605 thread loop end---------------------\n");
        cyg_thread_delay(200);
    }
}
#endif /* DEV_SPI3WIRE_RX3605_TEST_ENABLE */

#if (DEV_SPI3WIRE_RX3601_TEST_ENABLE > 0)
cyg_uint32 txbuf1[4] = {0x611111, 0x6aaaaa};
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

static spi_dev_config_t spi3wire_3601_config = {
    .spi3wire_cfg.baud = 80000,
    .spi3wire_cfg.addr_width = 5,
    .spi3wire_cfg.data_width = 20,
};

void spi_thread1(cyg_addrword_t data)
{
    cyg_uint32 txlen = 1;
    cyg_uint32 i = 0;
    spi_printf("\n-------------spi thread1 in----------------------\n");
    spi_printf("\nrx3601 baud = %d, addr = 0x%x, data = 0x%x\n", spi3wire_3601_config.spi3wire_cfg.baud, spi3wire_3601_config.spi3wire_cfg.addr_width, spi3wire_3601_config.spi3wire_cfg.data_width);
    spi_printf("\n\n");
    cyg_thread_delay(500);

    SpiInit(4, 1, &spi3wire_3601_config);
    SpiRead(4, 1, &rxbuf1[0], &txlen);
    SpiRead(4, 1, &rxbuf1[1], &txlen);
    SpiRead(4, 1, &rxbuf1[2], &txlen);
    SpiRead(4, 1, &rxbuf1[3], &txlen);
    SpiRead(4, 1, &rxbuf1[4], &txlen);
    SpiRead(4, 1, &rxbuf1[5], &txlen);
    SpiRead(4, 1, &rxbuf1[6], &txlen);
    SpiRead(4, 1, &rxbuf1[7], &txlen);
    SpiRead(4, 1, &rxbuf1[8], &txlen);
    SpiRead(4, 1, &rxbuf1[9], &txlen);
    SpiRead(4, 1, &rxbuf1[10], &txlen);
    SpiRead(4, 1, &rxbuf1[11], &txlen);
    SpiRead(4, 1, &rxbuf1[12], &txlen);
    SpiRead(4, 1, &rxbuf1[13], &txlen);
    SpiRead(4, 1, &rxbuf1[14], &txlen);
    SpiRead(4, 1, &rxbuf1[15], &txlen);
    SpiRead(4, 1, &rxbuf1[16], &txlen);
    SpiRead(4, 1, &rxbuf1[17], &txlen);
    SpiRead(4, 1, &rxbuf1[18], &txlen);
    SpiRead(4, 1, &rxbuf1[19], &txlen);
    SpiRead(4, 1, &rxbuf1[20], &txlen);
    spi_printf("\n%s: \n", __FUNCTION__);
    for (i = 0; i < 21; i++) {
        spi_printf("%02d: 0x%05x\n\n", i, rxbuf1[i] & 0xfffff);
    }

    while (1) {
        spi_printf("\n-------------rx3601 thread loop in----------------------\n");
        SpiWrite(4, 1, &txbuf1[0], &txlen);
        rxbuf1[6] = 0x600000;
        SpiRead(4, 1, &rxbuf1[6], &txlen);
        spi_printf("reg 06: 0x%05x\n\n", (rxbuf1[6] & 0xfffff));
        cyg_thread_delay(200);
        SpiWrite(4, 1, &txbuf1[1], &txlen);
        rxbuf1[6] = 0x600000;
        SpiRead(4, 1, &rxbuf1[6], &txlen);
        spi_printf("reg 06: 0x%05x\n\n", (rxbuf1[6] & 0xfffff));
        spi_printf("\n-------------rx3601 thread loop end---------------------\n");
        cyg_thread_delay(200);
    }
}
#endif /* DEV_SPI3WIRE_RX3601_TEST_ENABLE */

#if (DEV_SPI3WIRE_RX3902_TEST_ENABLE > 0)
cyg_uint32 txbuf2[4] = {0xa99999, 0xa66666};
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

static spi_dev_config_t spi3wire_3902_config = {
    .spi3wire_cfg.baud = 80000,
    .spi3wire_cfg.addr_width = 6,
    .spi3wire_cfg.data_width = 20,
};

void spi_thread2(cyg_addrword_t data)
{
    cyg_uint32 txlen = 1;
    cyg_uint32 i = 0;
    spi_printf("\n-------------spi thread2 in----------------------\n");
    spi_printf("\nrx3902 baud = %d, addr = 0x%x, data = 0x%x\n", spi3wire_3902_config.spi3wire_cfg.baud, spi3wire_3902_config.spi3wire_cfg.addr_width, spi3wire_3902_config.spi3wire_cfg.data_width);

    SpiInit(4, 0, &spi3wire_3902_config);
    SpiRead(4, 0, &rxbuf2[0], &txlen);
    SpiRead(4, 0, &rxbuf2[1], &txlen);
    SpiRead(4, 0, &rxbuf2[2], &txlen);
    SpiRead(4, 0, &rxbuf2[3], &txlen);
    SpiRead(4, 0, &rxbuf2[4], &txlen);
    SpiRead(4, 0, &rxbuf2[5], &txlen);
    SpiRead(4, 0, &rxbuf2[6], &txlen);
    SpiRead(4, 0, &rxbuf2[7], &txlen);
    SpiRead(4, 0, &rxbuf2[8], &txlen);
    SpiRead(4, 0, &rxbuf2[9], &txlen);
    SpiRead(4, 0, &rxbuf2[10], &txlen);
    SpiRead(4, 0, &rxbuf2[11], &txlen);
    SpiRead(4, 0, &rxbuf2[12], &txlen);
    SpiRead(4, 0, &rxbuf2[13], &txlen);
    SpiRead(4, 0, &rxbuf2[14], &txlen);
    SpiRead(4, 0, &rxbuf2[15], &txlen);
    SpiRead(4, 0, &rxbuf2[16], &txlen);
    SpiRead(4, 0, &rxbuf2[17], &txlen);
    SpiRead(4, 0, &rxbuf2[18], &txlen);
    SpiRead(4, 0, &rxbuf2[19], &txlen);
    SpiRead(4, 0, &rxbuf2[20], &txlen);
    SpiRead(4, 0, &rxbuf2[21], &txlen);
    SpiRead(4, 0, &rxbuf2[22], &txlen);
    SpiRead(4, 0, &rxbuf2[23], &txlen);
    SpiRead(4, 0, &rxbuf2[24], &txlen);
    SpiRead(4, 0, &rxbuf2[25], &txlen);
    SpiRead(4, 0, &rxbuf2[26], &txlen);
    SpiRead(4, 0, &rxbuf2[27], &txlen);
    SpiRead(4, 0, &rxbuf2[28], &txlen);
    SpiRead(4, 0, &rxbuf2[29], &txlen);
    SpiRead(4, 0, &rxbuf2[30], &txlen);
    SpiRead(4, 0, &rxbuf2[31], &txlen);
    SpiRead(4, 0, &rxbuf2[32], &txlen);
    SpiRead(4, 0, &rxbuf2[33], &txlen);
    SpiRead(4, 0, &rxbuf2[34], &txlen);
    SpiRead(4, 0, &rxbuf2[35], &txlen);
    SpiRead(4, 0, &rxbuf2[36], &txlen);
    SpiRead(4, 0, &rxbuf2[37], &txlen);
    SpiRead(4, 0, &rxbuf2[38], &txlen);
    spi_printf("\n%s: \n", __FUNCTION__);
    for (i = 0; i < 39; i++) {
        spi_printf("%02d: 0x%05x\n", i, rxbuf2[i] & 0xfffff);
    }

    while (1) {
        spi_printf("\n-------------rx3902 thread loop in----------------------\n");
        SpiWrite(4, 0, &txbuf2[0], &txlen);
        rxbuf2[10] = 0xa00000;
        SpiRead(4, 0, &rxbuf2[10], &txlen);
        spi_printf("reg 06: 0x%05x\n\n", (rxbuf2[10] & 0xfffff));
        cyg_thread_delay(200);
        SpiWrite(4, 0, &txbuf2[1], &txlen);
        rxbuf2[10] = 0xa00000;
        SpiRead(4, 0, &rxbuf2[10], &txlen);
        spi_printf("reg 06: 0x%05x\n\n", (rxbuf2[10] & 0xfffff));
        spi_printf("\n-------------rx3902 thread loop end---------------------\n");
        cyg_thread_delay(200);
    }
}
#endif /* DEV_SPI3WIRE_RX3902_TEST_ENABLE */

void cyg_start(void)
{
#if (DEV_SPI3WIRE_RX3605_TEST_ENABLE > 0)
    cyg_thread_create(THREAD0_PRIOIRTY, spi_thread0, (cyg_addrword_t) 0, "bp2016_spi_thread0",
                      (void *) spi_thread0_data.stack, sizeof(spi_thread2_data.stack), &spi_thread0_data.hdl,
                      &spi_thread0_data.obj);
    cyg_thread_resume(spi_thread0_data.hdl);
#endif /* DEV_SPI3WIRE_RX3605_TEST_ENABLE */

#if (DEV_SPI3WIRE_RX3601_TEST_ENABLE > 0)
    cyg_thread_create(THREAD1_PRIOIRTY, spi_thread1, (cyg_addrword_t) 0, "bp2016_spi_thread1",
                      (void *) spi_thread1_data.stack, sizeof(spi_thread2_data.stack), &spi_thread1_data.hdl,
                      &spi_thread1_data.obj);
    cyg_thread_resume(spi_thread1_data.hdl);
#endif /* DEV_SPI3WIRE_RX3601_TEST_ENABLE */

#if (DEV_SPI3WIRE_RX3902_TEST_ENABLE > 0)
    cyg_thread_create(THREAD2_PRIOIRTY, spi_thread2, (cyg_addrword_t) 0, "bp2016_spi_thread2",
                      (void *) spi_thread2_data.stack, sizeof(spi_thread2_data.stack), &spi_thread2_data.hdl,
                      &spi_thread2_data.obj);
    cyg_thread_resume(spi_thread2_data.hdl);
#endif /* DEV_SPI3WIRE_RX3902_TEST_ENABLE */

    cyg_scheduler_start();
}

#endif
