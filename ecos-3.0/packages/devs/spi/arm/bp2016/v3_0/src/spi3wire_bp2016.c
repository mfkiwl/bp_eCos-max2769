//==========================================================================
//
//      spi_bp2016.c
//
//      SPI driver for BP2016
//
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    huyue<huyue@haigebeidou.cn>
// Contributors: @BSP Team
// Date:         2017-01-29
// Purpose:
// Description:
//
//####DESCRIPTIONEND####
//
//==========================================================================
#include <pkgconf/devs_spi_arm_bp2016.h>
#include <pkgconf/io_spi.h>

#include <cyg/kernel/kapi.h>
#include <cyg/io/devtab.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/io/spi.h>
#include <string.h>

#ifdef CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4
#include <cyg/hal/regs/spi3wire.h>
#include <cyg/io/spi/spi_bp2016.h>

#ifdef CYGHWR_HAL_ASIC_CLK
#include <cyg/hal/clk/clk.h>
#endif

extern inline int spi_3wire_cmd_read(HWP_SPI_3WIRE_T *hw_spi, const u32 cs, const u32 addr);
extern inline u32 spi_3wire_read_value(HWP_SPI_3WIRE_T *hw_spi);
extern inline int spi_3wire_writereg(HWP_SPI_3WIRE_T *hw_spi, const u32 cs, const u32 addr, const int value);
extern inline bool spi_3wire_is_busy(HWP_SPI_3WIRE_T *hw_spi);
extern inline void spi_3wire_div(HWP_SPI_3WIRE_T *hw_spi, const u32 div);
extern inline void spi_3wire_bus_width(HWP_SPI_3WIRE_T *hw_spi, const u32 addr_width, const u32 data_width);

static int spi3wire_bp2016_get_config(cyg_spi_device *device, cyg_uint32 key, void *buf, cyg_uint32 *len);
static int spi3wire_bp2016_set_config(cyg_spi_device *device, cyg_uint32 key, const void *buf, cyg_uint32 *len);
static void spi3wire_bp2016_transaction_begin(cyg_spi_device *device);
static void spi3wire_bp2016_transaction_transfer(cyg_spi_device *device, cyg_bool polled,
        cyg_uint32 count, const cyg_uint8 *tx_data, cyg_uint8 *rx_data, cyg_bool drop_cs);
static void spi3wire_bp2016_transaction_tick(cyg_spi_device *device, cyg_bool polled, cyg_uint32 count);
static void spi3wire_bp2016_transaction_end(cyg_spi_device *device);

// this is for debug trace,
#if (CYGPKG_DEVS_SPI_ARM_BP2016_DEBUG_LEVEL > 0)
#define spi_printf diag_printf
#else
#define spi_printf(fmt, ...)
#endif

#ifdef CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4
cyg_spi_bp2016_bus_t cyg_spi3wire_bp2016_bus4 = {
    .spi_bus.spi_transaction_begin = spi3wire_bp2016_transaction_begin,
    .spi_bus.spi_transaction_transfer = spi3wire_bp2016_transaction_transfer,
    .spi_bus.spi_transaction_tick = spi3wire_bp2016_transaction_tick,
    .spi_bus.spi_transaction_end = spi3wire_bp2016_transaction_end,
    .spi_bus.spi_get_config = spi3wire_bp2016_get_config,
    .spi_bus.spi_set_config = spi3wire_bp2016_set_config,
    .hw_spi = (void*)hwp_ap_Spi4,
};
CYG_SPI_DEFINE_BUS_TABLE(cyg_spi_bp2016_dev_t, 4);
#endif

// spi4 and spi2 pinmux, use spi2 scm clk divider
#ifdef CYGHWR_HAL_ASIC_CLK
static int clk_set_spi_3wire(int rate_kHz)
{
    hal_clk_set_rate_kHz(CLK_SPI2, rate_kHz);
    return 0;
}
#else
static int clk_set_spi_3wire(int rate_kHz)
{
    return 0;
}
#endif

static void bp2016_set_spi3wire_baud(cyg_spi_bp2016_bus_t *bus, cyg_uint32 baud)
{
    HWP_SPI_3WIRE_T *hw_spi = (HWP_SPI_3WIRE_T *)bus->hw_spi;
    cyg_uint32 div = 0;
    clk_set_spi_3wire(CONFIG_APB_CLOCK / 1000);
    div = (CONFIG_APB_CLOCK / baud);
    spi_3wire_div(hw_spi, div);
}

static int bp2016_spi3wire_is_busy(HWP_SPI_3WIRE_T *hw_spi)
{
    cyg_uint32 timeout = 100;

    do {
        if (spi_3wire_is_busy(hw_spi) != true)
            return 0;
        HAL_DELAY_US(10);
    } while (timeout--);

    return -ETIMEDOUT;
}

static void bp2016_spi3wire_write(HWP_SPI_3WIRE_T *hw_spi, const u32 cs, const u32 addr, const u32 value)
{
    bp2016_spi3wire_is_busy(hw_spi);
    spi_3wire_writereg(hw_spi, cs, addr, value);
    bp2016_spi3wire_is_busy(hw_spi);
}

static u32 bp2016_spi3wire_read(HWP_SPI_3WIRE_T *hw_spi, const u32 cs, const u32 addr)
{
    u32 ret = 0;
    bp2016_spi3wire_is_busy(hw_spi);
    spi_3wire_cmd_read(hw_spi, cs, addr);
    bp2016_spi3wire_is_busy(hw_spi);
    ret = spi_3wire_read_value(hw_spi);
    return ret;
}

/*
 * get/set configuration
 */
static int
spi3wire_bp2016_get_config(cyg_spi_device *device, cyg_uint32 key, void *buf, cyg_uint32 *len)
{
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) device;
    // cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;
    // HWP_SPI_3WIRE_T *hw_spi = (HWP_SPI_3WIRE_T *)bus->hw_spi;

    switch (key) {
    case CYG_IO_GET_CONFIG_SPI_CLOCKRATE:
        if (*len == sizeof(cyg_uint32)) {
            cyg_uint32 *b = (cyg_uint32 *) buf;
            *b = dev->spi_cfg.spi3wire_cfg.baud;
        } else
            return -EINVAL;
        break;
    default:
        return -EINVAL;
    }

    return ENOERR;
}

static int
spi3wire_bp2016_set_config(cyg_spi_device *device, cyg_uint32 key, const void *buf,
                                 cyg_uint32 *len)
{
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) device;
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;

    switch (key) {
        case CYG_IO_SET_CONFIG_SPI_CLOCKRATE:
            if (*len == sizeof(cyg_uint32)) {
                dev->spi_cfg.spi_dw_cfg.baud = *(cyg_uint32 *) buf;
                bp2016_set_spi3wire_baud(bus, dev->spi_cfg.spi3wire_cfg.baud);
            } else
                return -EINVAL;
            break;
        default:
            return -EINVAL;
    }

    return ENOERR;
}

/*
 * Begin transaction
 * configure bus for device and drive CS by calling device cs() function
 */
static void
spi3wire_bp2016_transaction_begin(cyg_spi_device *device)
{
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) device;
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;
    HWP_SPI_3WIRE_T *hw_spi = (HWP_SPI_3WIRE_T *)bus->hw_spi;

    //todo: add spi init
    if (dev->spi_cs_num < CYGPKG_IO_SPI3WIRE_ARM_BP2016_BUS4_CS_NUM) {
        spi_printf("%s: hw_spi = 0x%x, cs = %d\n", __FUNCTION__, (cyg_uint32)hw_spi, dev->spi_cs_num);
    } else {
        return;
    }
    spi_printf("\n%s: baud = %d, addr = %d, data  = %d\n", __FUNCTION__,
            dev->spi_cfg.spi3wire_cfg.baud,
            dev->spi_cfg.spi3wire_cfg.addr_width,
            dev->spi_cfg.spi3wire_cfg.data_width);

    spi_3wire_bus_width(hw_spi, dev->spi_cfg.spi3wire_cfg.addr_width, dev->spi_cfg.spi3wire_cfg.data_width);
    bp2016_set_spi3wire_baud(bus, dev->spi_cfg.spi3wire_cfg.baud);
}

static void spi3wire_convert_format(cyg_uint8* data, cyg_uint32 addr_width, cyg_uint32 data_width, cyg_uint32 *addr, cyg_uint32 *value)
{
    cyg_uint32 tmp = (1 << data_width) - 1;
    cyg_uint32 dat_tmp = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    spi_printf("\n--- dat_tmp = 0x%x, --- rx_data[0] = 0x%x [1] = 0x%x, [2] = 0x%x, [3] = 0x%x---\n",
            dat_tmp, data[0], data[1], data[2], data[3]);
    *addr = dat_tmp >> data_width;
    *value = dat_tmp & tmp;
}

/*
 * Transfer a buffer to a device,
 * fill another buffer with data from the device
 */
static void
spi3wire_bp2016_transaction_transfer(cyg_spi_device *device, cyg_bool polled,
        cyg_uint32 count, const cyg_uint8 *tx_data,
        cyg_uint8 *rx_data, cyg_bool drop_cs)
{
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) device;
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;
    HWP_SPI_3WIRE_T *hw_spi = (HWP_SPI_3WIRE_T *)bus->hw_spi;
    cyg_uint32 cs = dev->spi_cs_num;
    cyg_uint32 addr = 0;
    cyg_uint32 value = 0;
    cyg_uint32 readback = 0;
    cyg_uint32 width = 0;

    if (!count)
        return;

    width = dev->spi_cfg.spi3wire_cfg.addr_width + dev->spi_cfg.spi3wire_cfg.data_width;
    if (width > 16) {
        count = count >> 2;
    } else if(width > 8) {
        count = count >> 1;
    } else {
    }

    spi_printf("\n width = %d, addrwidth = %d, datawidth = %d",
            (int)width,
            dev->spi_cfg.spi3wire_cfg.addr_width,
            dev->spi_cfg.spi3wire_cfg.data_width);

    cyg_drv_mutex_lock(&bus->spi_lock);

    if (tx_data != NULL) {
        spi3wire_convert_format((cyg_uint8*)tx_data, dev->spi_cfg.spi3wire_cfg.addr_width, dev->spi_cfg.spi3wire_cfg.data_width, &addr, &value);
        spi_printf("\n%s: addr = 0x%x, data = 0x%x\n", __FUNCTION__, addr, value);
    }
    if (rx_data != NULL) {
        spi3wire_convert_format(rx_data, dev->spi_cfg.spi3wire_cfg.addr_width, dev->spi_cfg.spi3wire_cfg.data_width, &addr, &value);
        spi_printf("\n%s: addr = 0x%x, data = 0x%x\n", __FUNCTION__, addr, value);
    }

    cyg_drv_dsr_lock();

    do {
        if (tx_data) {
            bp2016_spi3wire_write(hw_spi, cs, addr, value);
            count--;
        }
        if (rx_data) {
            readback = bp2016_spi3wire_read(hw_spi, cs, addr);
            spi_printf("\nreadback = 0x%x\n", readback);
            memcpy(rx_data, (u8*)&readback, 4);
            count--;
        }
    } while (count);

    cyg_drv_dsr_unlock();
    cyg_drv_mutex_unlock(&bus->spi_lock);

    return;
}

/*
 * Tick
 */
static void
spi3wire_bp2016_transaction_tick(cyg_spi_device *device, cyg_bool polled, cyg_uint32 count)
{

}

/*
 * End transaction
 * disable SPI bus, drop CS, reset transfer variables
 */
static void
spi3wire_bp2016_transaction_end(cyg_spi_device *device)
{
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) device;
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;

    bus->count = 0;
    bus->tx = NULL;
    bus->rx = NULL;
}

/*
 * Driver & bus initialization
 */
static void
spi3wire_bp2016_init_bus(cyg_spi_bp2016_bus_t *bus)
{
    cyg_drv_mutex_init(&bus->spi_lock);
    cyg_drv_cond_init(&bus->spi_wait, &bus->spi_lock);

    CYG_SPI_BUS_COMMON_INIT(&bus->spi_bus);
    spi_printf("\n%s: spi hw_spi: 0x%x init finished!\n", __FUNCTION__, (cyg_uint32)(bus->hw_spi));
}

// If C constructor with init priority functionality is not in compiler,
// rely on spi_bp2016_init.cxx to init us.
#ifndef CYGBLD_ATTRIB_C_INIT_PRI
#define CYGBLD_ATTRIB_C_INIT_PRI(x)
#endif

void CYGBLD_ATTRIB_C_INIT_PRI(CYG_INIT_BUS_SPI)
cyg_spi3wire_bp2016_bus_init(void)
{
#ifdef CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4
    spi3wire_bp2016_init_bus(&cyg_spi3wire_bp2016_bus4);
#endif
}

#endif /* CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4 */

