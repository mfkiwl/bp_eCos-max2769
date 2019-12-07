#include <pkgconf/devs_spi_arm_bp2016.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/io/devtab.h>

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016
#include <cyg/hal/regs/spi.h>
#include <cyg/io/spi/spi_bp2016.h>
#include <cyg/hal/api/spi_api.h>

#ifdef CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4
#include <cyg/hal/regs/spi3wire.h>
#endif

/* Flash opcodes. */
#define SPINOR_OP_WREN		0x06	/* Write enable */
#define SPINOR_OP_WRDI		0x04	/* Write disable */
#define SPINOR_OP_RDSR		0x05	/* Read status register */
#define SPINOR_OP_READ		0x03	/* Read data bytes (low frequency) */
#define SPINOR_OP_PP		0x02	/* Page program (up to 256 bytes) */
#define SPINOR_OP_SE		0xd8	/* Sector erase (usually 64KiB) */

#define spi_printf(fmt, args...)    \
        diag_printf("\n[SPI] %s: line: %d, " fmt, __func__, __LINE__, ##args)

#if (CYGPKG_DEVS_SPI_ARM_BP2016_DEBUG_LEVEL > 0)
#define spi_debug(fmt, args...)    \
        diag_printf("\n[SPI DBG] %s: line: %d, " fmt, __func__, __LINE__, ##args)
#else
#define spi_debug(fmt, args...)
#endif

// todo: static
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS0
cyg_spi_bp2016_dev_t spi_bus0_dev[CYGPKG_IO_SPI_ARM_BP2016_BUS0_CS_NUM];
#endif
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS1
cyg_spi_bp2016_dev_t spi_bus1_dev[CYGPKG_IO_SPI_ARM_BP2016_BUS1_CS_NUM];
#endif
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS2
cyg_spi_bp2016_dev_t spi_bus2_dev[CYGPKG_IO_SPI_ARM_BP2016_BUS2_CS_NUM_ALL];
#endif
#ifdef CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4
cyg_spi_bp2016_dev_t spi_bus4_dev[CYGPKG_IO_SPI3WIRE_ARM_BP2016_BUS4_CS_NUM];
#endif

static cyg_spi_bp2016_dev_t *SpiGetdev(cyg_uint32 index, cyg_uint32 cs)
{
    cyg_spi_bp2016_dev_t *dev = NULL;

    switch (index) {
        case SPI_INDEX_0:
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS0
            if (cs < CYGPKG_IO_SPI_ARM_BP2016_BUS0_CS_NUM)
                dev = &spi_bus0_dev[cs];
#endif
            break;
        case SPI_INDEX_1:
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS1
            if (cs < CYGPKG_IO_SPI_ARM_BP2016_BUS1_CS_NUM)
                dev = &spi_bus1_dev[cs];
#endif
            break;
        case SPI_INDEX_2:
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS2
            if (cs < CYGPKG_IO_SPI_ARM_BP2016_BUS2_CS_NUM_ALL)
                dev = &spi_bus2_dev[cs];
#endif
            break;
        case SPI_INDEX_4:
#ifdef CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4
            if (cs < CYGPKG_IO_SPI3WIRE_ARM_BP2016_BUS4_CS_NUM)
                dev = &spi_bus4_dev[cs];
#endif
            break;
        default:
            break;
    }

    return dev;
}

static Cyg_ErrNo SpiMatchBusnew(cyg_uint32 index, cyg_spi_bp2016_dev_t *dev)
{
    switch (index) {
        case SPI_INDEX_0:
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS0
            dev->spi_device.spi_bus = (cyg_spi_bus*)(&cyg_spi_bp2016_bus0);
            spi_debug("bus = 0x%x", (cyg_uint32)(dev->spi_device.spi_bus));
#else
            return (-ENODEV);
#endif
            break;
        case SPI_INDEX_1:
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS1
            dev->spi_device.spi_bus = (cyg_spi_bus*)(&cyg_spi_bp2016_bus1);
            spi_debug("bus = 0x%x", (cyg_uint32)(dev->spi_device.spi_bus));
#else
            return (-ENODEV);
#endif
            break;
        case SPI_INDEX_2:
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS2
            dev->spi_device.spi_bus = (cyg_spi_bus*)(&cyg_spi_bp2016_bus2);
            spi_debug("bus = 0x%x", (cyg_uint32)(dev->spi_device.spi_bus));
#else
            return (-ENODEV);
#endif
            break;
        case SPI_INDEX_4:
#ifdef CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4
            dev->spi_device.spi_bus = (cyg_spi_bus*)(&cyg_spi3wire_bp2016_bus4);
            spi_debug("bus = 0x%x", (cyg_uint32)(dev->spi_device.spi_bus));
#else
            return (-ENODEV);
#endif
            break;
        default:
            return (-ENODEV);
            break;
    }
    return ENOERR;
}

static Cyg_ErrNo SpiConfigDev(cyg_uint32 index, cyg_uint32 cs, spi_dev_config_t *spi_dev)
{
    Cyg_ErrNo res = ENOERR;
    cyg_spi_bp2016_dev_t *dev = SpiGetdev(index, cs);
    spi_debug("dev = 0x%x, cs = %d\n", (cyg_uint32)dev, cs);
    switch (index) {
        case SPI_INDEX_0:
        case SPI_INDEX_1:
        case SPI_INDEX_2:
            dev->spi_cfg.spi_dw_cfg.cpol = spi_dev->spi_dw_cfg.cpol;
            dev->spi_cfg.spi_dw_cfg.cpha = spi_dev->spi_dw_cfg.cpha;
            dev->spi_cfg.spi_dw_cfg.baud = spi_dev->spi_dw_cfg.baud;
            dev->spi_cfg.spi_dw_cfg.bus_width = spi_dev->spi_dw_cfg.bus_width;
            dev->spi_cfg.spi_dw_cfg.sample_delay = spi_dev->spi_dw_cfg.sample_delay;
            spi_debug("cpol = %d, cpha = %d, baud = %d, buswidth = %d, sample_delay = %d\n",
                       dev->spi_cfg.spi_dw_cfg.cpol, dev->spi_cfg.spi_dw_cfg.cpha,
                       dev->spi_cfg.spi_dw_cfg.baud, dev->spi_cfg.spi_dw_cfg.bus_width,
                       dev->spi_cfg.spi_dw_cfg.sample_delay);
            dev->spi_cs_num = cs;
            dev->spi_dfs = spi_dev->spi_dw_cfg.bus_width;
            dev->gpio_num = spi_dev->spi_dw_cfg.gpio_num;
            break;
        case SPI_INDEX_4:
            dev->spi_cfg.spi3wire_cfg.baud = spi_dev->spi3wire_cfg.baud;
            dev->spi_cfg.spi3wire_cfg.addr_width = spi_dev->spi3wire_cfg.addr_width;
            dev->spi_cfg.spi3wire_cfg.data_width = spi_dev->spi3wire_cfg.data_width;
            dev->spi_cs_num = cs;
            dev->spi_dfs =
                dev->spi_cfg.spi3wire_cfg.addr_width + dev->spi_cfg.spi3wire_cfg.data_width;
            spi_debug("baud = %d, addr = %d, data = %d\n",
                       dev->spi_cfg.spi3wire_cfg.baud, dev->spi_cfg.spi3wire_cfg.addr_width,
                       dev->spi_cfg.spi3wire_cfg.data_width);
            break;
        default:
            res = -ENODEV;
            break;
    }

    res = SpiMatchBusnew(index, dev);
    return res;
}

static Cyg_ErrNo SpiCheckIndex(cyg_uint32 index)
{
    Cyg_ErrNo res = ENOERR;

    switch (index) {
        case SPI_INDEX_0:
        case SPI_INDEX_1:
        case SPI_INDEX_2:
        case SPI_INDEX_4:
            break;
        default:
            res = (-ENODEV);
            break;
    }

    return res;
}

// init bus(controller) level variables
void *SpiInit(spi_dev_config_t *dev_config)
{
    Cyg_ErrNo res = ENOERR;
    cyg_uint32 index = *(cyg_uint32 *) dev_config;
    cyg_uint32 cs = *((cyg_uint32 *) dev_config + 1);

    spi_debug("index = %d, cs = %d", index, cs);

    res = SpiCheckIndex(index);
    if (res != ENOERR) {
        return (NULL);
    }
    cyg_spi_bp2016_dev_t *dev = SpiGetdev(index, cs);
    if (!dev)
        return NULL;
        // create and get merge
    res = SpiConfigDev(index, cs, dev_config);
    return dev;
}

Cyg_ErrNo SpiWrite(void * handler, const cyg_uint8 *wbuf, cyg_uint32 *wlen)
{
    Cyg_ErrNo res = ENOERR;

    if ((wbuf == NULL) || (*wlen == 0)) {
        return (-EINVAL);
    }

    if (handler == NULL) {
        return (-ENODEV);
    }

    spi_debug("wbuf addr = 0x%x, len = %d\n", (cyg_uint32)wbuf, *wlen);

    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) handler;

    spi_debug("&dev->spi_device = 0x%x\n", (cyg_uint32)(&dev->spi_device));
    // begin
    cyg_spi_transaction_begin(&dev->spi_device);
    // transfer
    cyg_spi_transaction_transfer(&dev->spi_device, 0, *wlen, wbuf, (cyg_uint8 *) NULL, true);
    res = dev->error;   //use spi io mutex, safe
    // end
    cyg_spi_transaction_end(&dev->spi_device);
    spi_debug("error = 0x%x\n", dev->error);
    return res;
}

Cyg_ErrNo SpiRead(void *handler, cyg_uint8 *rbuf, cyg_uint32 *rlen)
{
    Cyg_ErrNo res = ENOERR;

    if ((rbuf == NULL) || (*rlen == 0)) {
        return (-EINVAL);
    }

    if (handler == NULL) {
        return (-ENODEV);
    }

    spi_debug("rbuf addr = 0x%x, rlen = 0x%x\n", (cyg_uint32)rbuf, *rlen);
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) handler;

    spi_debug("&dev->spi_device = 0x%x\n", (cyg_uint32)(&dev->spi_device));
    // begin
    cyg_spi_transaction_begin(&dev->spi_device);
    // transfer
    cyg_spi_transaction_transfer(&dev->spi_device, 0, *rlen, (cyg_uint8 *) NULL, rbuf, true);
    res = dev->error;   //use spi io mutex, safe
    // end
    cyg_spi_transaction_end(&dev->spi_device);
    spi_debug("error = 0x%x\n", dev->error);
    return res;
}

Cyg_ErrNo SpiWritethenRead(void *handler, const cyg_uint8 *wbuf, cyg_uint32 *wlen, cyg_uint8 *rbuf, cyg_uint32 *rlen)
{
    Cyg_ErrNo res = ENOERR;

    if ((wbuf == NULL) || (*wlen == 0)) {
        return (-EINVAL);
    }

    if ((rbuf == NULL) || (*rlen == 0)) {
        return (-EINVAL);
    }

    if (handler == NULL) {
        return (-ENODEV);
    }

    spi_debug("wbuf addr = 0x%x, wlen = 0x%x, rbuf addr = 0x%x, rlen = 0x%x\n", (cyg_uint32)wbuf, *wlen, (cyg_uint32)rbuf, *rlen);
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) handler;

    spi_debug("&dev->spi_device = 0x%x\n", (cyg_uint32)(&dev->spi_device));
    // begin
    cyg_spi_transaction_begin(&dev->spi_device);
    // transfer
    cyg_spi_transaction_transfer(&dev->spi_device, 0, *wlen, (cyg_uint8 *) wbuf, (cyg_uint8 *) NULL, false);
    cyg_spi_transaction_transfer(&dev->spi_device, 0, *rlen, (cyg_uint8 *) NULL, rbuf, true);
    res = dev->error;   //use spi io mutex, safe
    // end
    cyg_spi_transaction_end(&dev->spi_device);
    spi_debug("error = 0x%x\n", dev->error);
    return res;
}

Cyg_ErrNo SpiReadFlash(void *handler, cyg_uint32 addr, cyg_uint8 *rbuf, cyg_uint32 *rlen)
{
    Cyg_ErrNo res = ENOERR;
    cyg_uint8 wbuf[4] = {0};
    cyg_uint32 wlen = 4;

    wbuf[0] = SPINOR_OP_READ;
    wbuf[1] = (addr >> 16) & 0xff;
    wbuf[2] = (addr >> 8) & 0xff;
    wbuf[3] = addr & 0xff;

    if ((rbuf == NULL) || (*rlen == 0)) {
        return (-EINVAL);
    }

    if (handler == NULL) {
        return (-ENODEV);
    }

    spi_debug("rbuf addr = 0x%x, rlen = 0x%x\n", (cyg_uint32)rbuf, *rlen);
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) handler;

    spi_debug("&dev->spi_device = 0x%x\n", (cyg_uint32)(&dev->spi_device));
    // begin
    cyg_spi_transaction_begin(&dev->spi_device);
    // transfer
    cyg_spi_transaction_transfer(&dev->spi_device, 0, wlen, (cyg_uint8 *) wbuf, (cyg_uint8 *) NULL, false);
    cyg_spi_transaction_transfer(&dev->spi_device, 0, *rlen, (cyg_uint8 *) NULL, rbuf, true);
    res = dev->error;   //use spi io mutex, safe
    // end
    cyg_spi_transaction_end(&dev->spi_device);
    spi_debug("error = 0x%x\n", dev->error);
    return res;
}

Cyg_ErrNo SpiReadFlashStatus(void *handler, cyg_uint8 *rbuf)
{
    Cyg_ErrNo res = ENOERR;
    cyg_uint8 wbuf[1] = {0};
    cyg_uint32 wlen = 1;

    wbuf[0] = SPINOR_OP_RDSR;

    if (rbuf == NULL) {
        return (-EINVAL);
    }

    if (handler == NULL) {
        return (-ENODEV);
    }

    spi_debug("rbuf addr = 0x%x\n", (cyg_uint32) rbuf);
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) handler;

    spi_debug("&dev->spi_device = 0x%x\n", (cyg_uint32)(&dev->spi_device));
    // begin
    cyg_spi_transaction_begin(&dev->spi_device);
    // transfer
    cyg_spi_transaction_transfer(&dev->spi_device, 0, wlen, (cyg_uint8 *) wbuf, (cyg_uint8 *) NULL, false);
    cyg_spi_transaction_transfer(&dev->spi_device, 0, wlen, (cyg_uint8 *) NULL, rbuf, true);
    res = dev->error;   //use spi io mutex, safe
    // end
    cyg_spi_transaction_end(&dev->spi_device);
    spi_debug("error = 0x%x\n", dev->error);
    return res;
}

Cyg_ErrNo SpiEraseFlash(void *handler, cyg_uint32 addr)
{
    Cyg_ErrNo res = ENOERR;
    cyg_uint8 wbuf[4] = {0};
    cyg_uint32 wlen = 4;
#if 0
    if (addr )
        return (-EINVAL);
    }
#endif
    wbuf[0] = SPINOR_OP_SE;
    wbuf[1] = (addr >> 16) & 0xff;
    wbuf[2] = (addr >> 8) & 0xff;
    wbuf[3] = addr & 0xff;

    if (handler == NULL) {
        return (-ENODEV);
    }

    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) handler;

    spi_debug("&dev->spi_device = 0x%x\n", (cyg_uint32)(&dev->spi_device));
    // begin
    cyg_spi_transaction_begin(&dev->spi_device);
    // transfer
    cyg_spi_transaction_transfer(&dev->spi_device, 0, wlen, (cyg_uint8 *) wbuf, (cyg_uint8 *) NULL, true);
    res = dev->error;   //use spi io mutex, safe
    // end
    cyg_spi_transaction_end(&dev->spi_device);
    spi_debug("error = 0x%x\n", dev->error);
    return res;
}

Cyg_ErrNo SpiDone(void *handler)
{
    if (handler == NULL) {
        return (-ENODEV);
    }
    return ENOERR;
}

Cyg_ErrNo SpiGetBaud(void *handler, cyg_uint32 *baud)
{
    Cyg_ErrNo res = ENOERR;
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) handler;
    cyg_uint32 key = CYG_IO_GET_CONFIG_SPI_CLOCKRATE;
    cyg_uint32 len = sizeof(cyg_uint32);

    if (handler == NULL) {
        return (-ENODEV);
    }

    if (baud == NULL) {
        return -EINVAL;
    }

    res = cyg_spi_get_config(&dev->spi_device, key, baud, &len);
    if (res != ENOERR) {
        return -EINVAL;
    }
    return res;
}

Cyg_ErrNo SpiSetBaud(void *handler, cyg_uint32 *baud)
{
    Cyg_ErrNo res = ENOERR;
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) handler;
    cyg_uint32 key = CYG_IO_SET_CONFIG_SPI_CLOCKRATE;
    cyg_uint32 len = sizeof(cyg_uint32);

    if (handler == NULL) {
        return (-ENODEV);
    }

    if ((baud == NULL) || (*baud > SPI_MAX_BAUDRATE)) {
        return -EINVAL;
    }

    res = cyg_spi_set_config(&dev->spi_device, key, baud, &len);
    if (res != ENOERR) {
        return -EINVAL;
    }
    return res;
}

#endif /* CYGPKG_DEVS_SPI_ARM_BP2016 */
