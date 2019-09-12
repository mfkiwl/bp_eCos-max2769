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
// Date:         2017-01-10
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

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016
#include <cyg/hal/regs/spi.h>
#include <cyg/io/spi/spi_bp2016.h>

#ifdef CYGHWR_HAL_ASIC_CLK
#include <cyg/hal/clk/clk.h>
#endif

#ifdef CYGPKG_DEVS_SPI_ENABLE_DMA
#include <cyg/hal/api/dma_api.h>
extern inline void spi_set_dma_tdlr(HWP_SPI_T *hw_spi, u32 tdlr);
extern inline void spi_set_dma_rdlr(HWP_SPI_T *hw_spi, u32 rdlr);
#endif /* CYGPKG_DEVS_SPI_ENABLE_DMA */

extern inline void dw_spi_baud(HWP_SPI_T *hw_spi, const u32 baud);
extern inline void dw_spi_tmod(HWP_SPI_T *hw_spi, const u32 tmod);
extern inline void dw_spi_cs_enable(HWP_SPI_T *hw_spi, const u32 cs);
extern inline void dw_spi_ndf(HWP_SPI_T *hw_spi, const u32 ndf);
extern inline void dw_spi_cs_disable(HWP_SPI_T *hw_spi);
extern inline int dw_spi_get_cs(HWP_SPI_T *hw_spi);
extern inline void dw_spi_enable(HWP_SPI_T *hw_spi, cyg_uint32 en);
extern inline cyg_uint16 dw_spi_read_16bit(HWP_SPI_T *hw_spi);
extern inline cyg_uint8 dw_spi_read_8bit(HWP_SPI_T *hw_spi);
extern inline int dw_spi_read_32bit(HWP_SPI_T *hw_spi);
extern inline void dw_spi_write_8bit(HWP_SPI_T *hw_spi, cyg_uint8 ret);
extern inline void dw_spi_write_16bit(HWP_SPI_T *hw_spi, cyg_uint16 ret);
extern inline void dw_spi_write_32bit(HWP_SPI_T *hw_spi, cyg_uint32 ret);
extern inline void spi_rx_full_inten(HWP_SPI_T *hw_spi, int en);
extern inline void spi_tx_empty_inten(HWP_SPI_T *hw_spi, int en);
extern inline void spi_rx_underflow_inten(HWP_SPI_T *hw_spi, int en);
extern inline void spi_rx_overflow_inten(HWP_SPI_T *hw_spi, int en);
extern inline void spi_tx_overflow_inten(HWP_SPI_T *hw_spi, int en);
extern inline void dw_spi_set_rx_fifo_level(HWP_SPI_T *hw_spi, int rft);
extern inline void dw_spi_set_tx_fifo_level(HWP_SPI_T *hw_spi, int tft);
extern inline int dw_spi_get_txfifo_valid_num(HWP_SPI_T *hw_spi);
extern inline int dw_spi_get_rxfifo_valid_num(HWP_SPI_T *hw_spi);
extern inline u32 dw_spi_clr_all_interrupts(HWP_SPI_T *hw_spi);
extern inline int dw_spi_polling_raw_status(HWP_SPI_T *hw_spi);
extern inline void dw_spi_cpmode(HWP_SPI_T *hw_spi, const u32 cpha, const u32 cpol);
extern inline u32 dw_spi_get_isr_status(HWP_SPI_T *hw_spi);
extern inline void spi_disable_all_int(HWP_SPI_T *hw_spi);
extern inline void spi_set_dma_ctrl(HWP_SPI_T *hw_spi, u8 enable);
extern inline void dw_spi_bus_width(HWP_SPI_T *hw_spi, u32 width);
extern inline bool dw_spi_is_busy(HWP_SPI_T *hw_spi);

// this is for spi2 controler cs num less than board
extern int scm_gpiob_en(int index);
extern int scm_spi2_en(int cs0, int cs1, int cs2, int cs3, int cs4, int cs5, int cs6, int cs7);
bool gpio_set_output_high(int gpio_num);
bool gpio_set_output_low(int gpio_num);
// end

static cyg_uint32 spi_bp2016_ISR(cyg_vector_t vec, cyg_addrword_t data);
static void spi_bp2016_DSR(cyg_vector_t vec, cyg_ucount32 count, cyg_addrword_t data);
static int spi_bp2016_get_config(cyg_spi_device *device, cyg_uint32 key, void *buf,
                                 cyg_uint32 *len);
static int spi_bp2016_set_config(cyg_spi_device *device, cyg_uint32 key, const void *buf,
                                 cyg_uint32 *len);
static void spi_bp2016_transaction_begin(cyg_spi_device *device);
static void spi_bp2016_transaction_transfer(cyg_spi_device *device, cyg_bool polled,
                                            cyg_uint32 count, const cyg_uint8 *tx_data,
                                            cyg_uint8 *rx_data, cyg_bool drop_cs);
static void spi_bp2016_transaction_tick(cyg_spi_device *device, cyg_bool polled, cyg_uint32 count);
static void spi_bp2016_transaction_end(cyg_spi_device *device);

enum transfer_state {
    TRANS_ING = 0,
    TRANS_TX_DONE,
    TRANS_RX_DONE,
};

#define spi_printf(fmt, args...)    \
        diag_printf("\n[SPI ERROR] %s: line: %d, " fmt, __func__, __LINE__, ##args)

#if (CYGPKG_DEVS_SPI_ARM_BP2016_DEBUG_LEVEL > 0)
#define spi_debug(fmt, args...)    \
        diag_printf("\n[SPI DBG] %s: line: %d, " fmt, __func__, __LINE__, ##args)
#else
#define spi_debug(fmt, args...)
#endif

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS0
cyg_spi_bp2016_bus_t cyg_spi_bp2016_bus0 = {
    .spi_bus.spi_transaction_begin = spi_bp2016_transaction_begin,
    .spi_bus.spi_transaction_transfer = spi_bp2016_transaction_transfer,
    .spi_bus.spi_transaction_tick = spi_bp2016_transaction_tick,
    .spi_bus.spi_transaction_end = spi_bp2016_transaction_end,
    .spi_bus.spi_get_config = spi_bp2016_get_config,
    .spi_bus.spi_set_config = spi_bp2016_set_config,

    .count = 0,
    .msg_idx = 0,
    .msg_tx_idx = 0,
    .msg_rx_idx = 0,
    .mode = 0,
    .msgs_num = 0,
    .dma_use = CYGPKG_SPI_BP2016_BUS0_DMA_SUPPORT, // spi dma original choice
    .hw_spi = (void *) hwp_ap_Spi0,
    .spi_vector = SYS_IRQ_ID_SPI0,
    .spi_prio = INT_PRI_DEFAULT,
};
CYG_SPI_DEFINE_BUS_TABLE(cyg_spi_bp2016_dev_t, 0);
#endif

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS1
cyg_spi_bp2016_bus_t cyg_spi_bp2016_bus1 = {
    .spi_bus.spi_transaction_begin = spi_bp2016_transaction_begin,
    .spi_bus.spi_transaction_transfer = spi_bp2016_transaction_transfer,
    .spi_bus.spi_transaction_tick = spi_bp2016_transaction_tick,
    .spi_bus.spi_transaction_end = spi_bp2016_transaction_end,
    .spi_bus.spi_get_config = spi_bp2016_get_config,
    .spi_bus.spi_set_config = spi_bp2016_set_config,

    .count = 0,
    .msg_idx = 0,
    .msg_tx_idx = 0,
    .msg_rx_idx = 0,
    .mode = 0,
    .msgs_num = 0,
    .dma_use = CYGPKG_SPI_BP2016_BUS1_DMA_SUPPORT,
    .hw_spi = (void *) hwp_ap_Spi1,
    .spi_vector = SYS_IRQ_ID_SPI1,
    .spi_prio = INT_PRI_DEFAULT,
};
CYG_SPI_DEFINE_BUS_TABLE(cyg_spi_bp2016_dev_t, 1);
#endif

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS2
cyg_spi_bp2016_bus_t cyg_spi_bp2016_bus2 = {
    .spi_bus.spi_transaction_begin = spi_bp2016_transaction_begin,
    .spi_bus.spi_transaction_transfer = spi_bp2016_transaction_transfer,
    .spi_bus.spi_transaction_tick = spi_bp2016_transaction_tick,
    .spi_bus.spi_transaction_end = spi_bp2016_transaction_end,
    .spi_bus.spi_get_config = spi_bp2016_get_config,
    .spi_bus.spi_set_config = spi_bp2016_set_config,

    .count = 0,
    .msg_idx = 0,
    .msg_tx_idx = 0,
    .msg_rx_idx = 0,
    .mode = 0,
    .msgs_num = 0,
    .dma_use = CYGPKG_SPI_BP2016_BUS2_DMA_SUPPORT,
    .hw_spi = (void *) hwp_ap_Spi2,
    .spi_vector = SYS_IRQ_ID_SPI2,
    .spi_prio = INT_PRI_DEFAULT,
};
CYG_SPI_DEFINE_BUS_TABLE(cyg_spi_bp2016_dev_t, 2);
#endif

static inline void spi_check_idle(HWP_SPI_T *hw_spi)
{
    unsigned int timeout = 5000000;

    do {
        if(dw_spi_is_busy(hw_spi) == false)
            return;
        HAL_DELAY_US(10);
    } while (timeout--);
}

static int dw_spi_xfer_rx_process(cyg_spi_bp2016_bus_t *bus)
{
    HWP_SPI_T *hw_spi = bus->hw_spi;
    cyg_uint32 rx_count = dw_spi_get_rxfifo_valid_num(hw_spi);
    int i = 0;
    int ret = TRANS_ING;
    struct spi_msg *msg = bus->msgs;
    int loops = 0;

    spi_debug("rx valid = %d, index = %d, msg_num = %d", rx_count, bus->msg_rx_idx, bus->msgs_num);

    while (bus->msg_rx_idx < bus->msgs_num) {
        if (!(msg[bus->msg_rx_idx].flags & SPI_M_READ)) {
            bus->msg_rx_idx++;
            continue;
        }

        if (bus->status == STATUS_IDLE) {
            bus->count = msg[bus->msg_rx_idx].len / bus->n_bytes; // todo: check 32bit data
            bus->rx = msg[bus->msg_rx_idx].buf;
        }
        loops = (bus->count > rx_count) ? rx_count : bus->count;

        spi_debug("buf= 0x%x, count = %d, msgs_num = %d, idx = %d", (u32) bus->rx, bus->count,
                  bus->msgs_num, bus->msg_rx_idx);

        if (bus->spi_dfs <= 8) {
            for (i = 0; i < loops; i++) {
                bus->rx[i] = dw_spi_read_8bit(hw_spi);
            }
        } else if (bus->spi_dfs <= 16) {
            cyg_uint16 *d16_ = (cyg_uint16 *) (bus->rx);
            for (i = 0; i < loops; i++) {
                d16_[i] = dw_spi_read_16bit(hw_spi);
                // spi_debug("\nloops = %d, bus->tx[%d] = 0x%x\n", loops, i, bus->tx[i]);
            }
        } else {
            cyg_uint32 *d32_ = (cyg_uint32 *) (bus->rx);
            for (i = 0; i < loops; i++) {
                d32_[i] = dw_spi_read_32bit(hw_spi);
                // spi_debug("\nloops = %d, bus->tx[%d] = 0x%x\n", loops, i, bus->tx[i]);
            }
        }

        bus->status = STATUS_IN_PROGRESS;
        bus->rx += loops * bus->n_bytes;
        bus->count -= loops;
        spi_debug("\ncount = %d, loops = %d\n", bus->count, loops);

        if (bus->count == 0) {
            spi_rx_full_inten(hw_spi, 0);
            spi_rx_underflow_inten(hw_spi, 0);
            // rx over
            ret = TRANS_RX_DONE;
            bus->status = STATUS_IDLE;
            bus->msg_rx_idx++;
        }

        break;
    }

    return ret;
}

static int dw_spi_xfer_tx_process(cyg_spi_bp2016_bus_t *bus)
{
    HWP_SPI_T *hw_spi = bus->hw_spi;
    cyg_uint32 tx_count = dw_spi_get_txfifo_valid_num(hw_spi);
    int i = 0;
    int ret = TRANS_ING;
    struct spi_msg *msg = bus->msgs;
    int loops = tx_count;

    for (; bus->msg_tx_idx < bus->msgs_num;) {
        if (!(msg[bus->msg_tx_idx].flags & SPI_M_WRITE)) {
            bus->msg_tx_idx++;
            continue;
        }

        if (bus->status == STATUS_IDLE) {
            bus->count = msg[bus->msg_tx_idx].len / bus->n_bytes; // todo: check 32bit data
            bus->tx = msg[bus->msg_tx_idx].buf;
        }
        loops = (bus->count > tx_count) ? tx_count : bus->count;

        cyg_interrupt_disable();
        if (bus->spi_dfs <= 8) {
            for (i = 0; i < loops; i++) {
                dw_spi_write_8bit(hw_spi, bus->tx[i]);
                // spi_debug("\nloops = %d, bus->tx[%d] = 0x%x\n", loops, i, bus->tx[i]);
            }
        } else if (bus->spi_dfs <= 16) {
            cyg_uint16 *d16_ = (cyg_uint16 *) bus->tx;
            for (i = 0; i < loops; i++) {
                dw_spi_write_16bit(hw_spi, d16_[i]);
                // spi_debug("\ndfs16 loops = %d, bus->tx[%d] = 0x%x\n", loops, i, d16_[i]);
            }
        } else {
            cyg_uint32 *d32_ = (cyg_uint32 *) bus->tx;
            for (i = 0; i < loops; i++) {
                dw_spi_write_32bit(hw_spi, d32_[i]);
            }
        }
        cyg_interrupt_enable();

        bus->tx += loops * bus->n_bytes;
        bus->count -= loops;
        bus->status = STATUS_IN_PROGRESS;
        spi_debug("count = %d, loops = %d", bus->count, loops);

        if (bus->count == 0) {
            bus->status = STATUS_IDLE;
            bus->msg_tx_idx++;
        }
        break;
    }

    spi_debug("buf= 0x%x, msgs_num = %d, idx = %d", (u32) bus->tx, bus->msgs_num, bus->msg_tx_idx);

    if (bus->msg_tx_idx == bus->msgs_num) {
        spi_tx_empty_inten(hw_spi, 0);
        spi_tx_overflow_inten(hw_spi, 0);
        ret = TRANS_TX_DONE;
        spi_debug("disable tx irq, clear index = %d", bus->msg_tx_idx);
    }

    return ret;
}

/*
 * Interrupt routine
 * read & write the next byte until count reaches zero
 */
static cyg_uint32
spi_bp2016_ISR(cyg_vector_t vec, cyg_addrword_t data)
{
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) data;

    cyg_drv_interrupt_mask(bus->spi_vector);
    cyg_drv_interrupt_acknowledge(bus->spi_vector);
    return CYG_ISR_HANDLED | CYG_ISR_CALL_DSR;
}

static void
spi_bp2016_DSR(cyg_vector_t vec, cyg_ucount32 count, cyg_addrword_t data)
{
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) data;
    HWP_SPI_T *hw_spi = bus->hw_spi;
    cyg_uint32 status = dw_spi_get_isr_status(hw_spi);
    cyg_uint32 ret = 0;

    dw_spi_clr_all_interrupts(hw_spi);
    spi_debug("hw_spi = 0x%x, status = 0x%x\n", (cyg_uint32) hw_spi, status);

    if (status & SPI_INT_RXUI) {
        spi_rx_underflow_inten(hw_spi, 0);
        spi_rx_full_inten(hw_spi, 0);
        spi_rx_overflow_inten(hw_spi, 0);
        bus->error = ERR_RX_UNDERFLOW_ERROR;
        spi_printf("rx underflow hw_spi = 0x%x, status = 0x%x\n", (cyg_uint32) hw_spi, status);
        cyg_drv_cond_signal(&bus->spi_wait);
    }

    if (status & SPI_INT_TXOI) {
        spi_tx_overflow_inten(hw_spi, 0);
        spi_tx_empty_inten(hw_spi, 0);
        bus->error = ERR_TX_OVERFLOW_ERROR;
        cyg_drv_cond_signal(&bus->spi_wait);
        spi_printf("tx overflow hw_spi = 0x%x, status = 0x%x\n", (cyg_uint32) hw_spi, status);
    }

    if (status & SPI_INT_RXOI) {
        spi_rx_underflow_inten(hw_spi, 0);
        spi_rx_full_inten(hw_spi, 0);
        spi_rx_overflow_inten(hw_spi, 0);
        bus->error = ERR_RX_OVERFLOW_ERROR;
        spi_printf("rx overflow hw_spi = 0x%x, status = 0x%x\n", (cyg_uint32) hw_spi, status);
        cyg_drv_cond_signal(&bus->spi_wait);
    }

    if (status & SPI_INT_TXEI) {
        ret = dw_spi_xfer_tx_process(bus);
    }

    if (status & SPI_INT_RXFI) {
        ret = dw_spi_xfer_rx_process(bus);
    }

    switch (ret) {
    case TRANS_TX_DONE:
        spi_tx_empty_inten(hw_spi, 0);
        spi_tx_overflow_inten(hw_spi, 0);
        if (!(bus->mode & SPI_RX_ONLY)) {   //only tx
            cyg_drv_cond_signal(&bus->spi_wait);
        }
        break;
    case TRANS_RX_DONE:      //only rx or tx + rx
        spi_rx_full_inten(hw_spi, 0);
        spi_rx_underflow_inten(hw_spi, 0);
        spi_rx_overflow_inten(hw_spi, 0);
        cyg_drv_cond_signal(&bus->spi_wait);
        break;
    default:
        break;
    }

    if (bus->error != SPI_OK) {
        // todo: abort dma, dma coding is not ready, waiting
        dw_spi_enable(hw_spi, false);   // disable spi controller
    }

    cyg_drv_interrupt_unmask(bus->spi_vector);
}

/*
 * if baud > 95Mhz, double is 190MHZ, near 200Mhz,
 * spi controller should less than 200Mhz,
 * so controller would not divider, only use scm
 */
static u32 spi_scm_clk[] = {
    // 76000000           // f
    // 81000000,          // e
    // 87000000,          // d
    95000000,          // c
    103000000,
    114000000,
    126000000,
    142000000,
    162000000,
    190000000,          // 6
    // 228000000,          // 5
    // 285000000,          // 4
    // 380000000,          // 3
    // 570000000,          // 2
    // 1140000000,         // 1
};

// this is for asic set input to 1140MHz, fpga is not compitibal
#ifdef CYGHWR_HAL_ASIC_CLK
static int clk_set_spi(HWP_SPI_T *hw_spi, int rate_kHz)
{
    switch ((cyg_uint32) hw_spi) {
    case (cyg_uint32) hwp_ap_Spi0:
        hal_clk_set_rate_kHz(CLK_SPI0, rate_kHz);
        break;
    case (cyg_uint32) hwp_ap_Spi1:
        hal_clk_set_rate_kHz(CLK_SPI1, rate_kHz);
        break;
    case (cyg_uint32) hwp_ap_Spi2:
        hal_clk_set_rate_kHz(CLK_SPI2, rate_kHz);
        break;
    }

    return 0;
}
#else
static int clk_set_spi(HWP_SPI_T *hw_spi, int rate_kHz)
{
    return 0;
}
#endif

static void spi_bp2016_set_baud(cyg_spi_bp2016_bus_t *bus, cyg_uint32 baud)
{
    cyg_uint32 div = 0;
    cyg_int32 i = 5;
    HWP_SPI_T *hw_spi = bus->hw_spi;

    if (baud > SPI_MAX_BAUDRATE) {
        spi_printf("baud.error\n");
        bus->error = -EINVAL;
        return;
    }

    if (baud <= CONFIG_APB_CLOCK) {
        clk_set_spi(hw_spi, (CONFIG_APB_CLOCK / 1000));
        div = ((CONFIG_APB_CLOCK / baud) + 1) & 0xFFFE; // even value
    } else {
        for (i = 0; i < (sizeof(spi_scm_clk) / sizeof(spi_scm_clk[0])); i++) {
            if(spi_scm_clk[i] <= baud) {

            } else {
                break;
            }
        }
        i = i - 1;

        if (i > (sizeof(spi_scm_clk) / sizeof(spi_scm_clk[0]))) {
            bus->error = -EINVAL;
            return; //error
        }
        clk_set_spi(hw_spi, (spi_scm_clk[i] / 1000));
        div = 0;
        spi_debug("spi_scm_clk[%d] = %d, div = %d", i, spi_scm_clk[i], div);
    }

    dw_spi_baud(hw_spi, div);
}

/*
 * get/set configuration
 */
static int
spi_bp2016_get_config(cyg_spi_device *device, cyg_uint32 key, void *buf, cyg_uint32 *len)
{
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) device;
    // cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;

    switch (key) {
    case CYG_IO_GET_CONFIG_SPI_CLOCKRATE:
        if (*len == sizeof(cyg_uint32)) {
            cyg_uint32 *b = (cyg_uint32 *) buf;
            *b = dev->spi_cfg.spi_dw_cfg.baud;
        } else
            return -EINVAL;
        break;
    default:
        return -EINVAL;
    }

    return ENOERR;
}

static int
spi_bp2016_set_config(cyg_spi_device *device, cyg_uint32 key, const void *buf,
        cyg_uint32 *len)
{
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) device;
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;
    HWP_SPI_T *hw_spi = bus->hw_spi;

    switch (key) {
    case CYG_IO_SET_CONFIG_SPI_CLOCKRATE:
        if (*len == sizeof(cyg_uint32)) {
            dev->spi_cfg.spi_dw_cfg.baud = *(cyg_uint32 *) buf;
            dw_spi_enable(hw_spi, false);
            spi_bp2016_set_baud(bus, dev->spi_cfg.spi_dw_cfg.baud);
            dw_spi_enable(hw_spi, true);
        } else
            return -EINVAL;
        break;
    default:
        return -EINVAL;
    }

    return ENOERR;
}

static cyg_uint32 assert_usercfg(cyg_spi_bp2016_dev_t *dev)
{
    spi_debug("cpol = 0x%x, cpha = 0x%x, baud = %d, width = %d\n", \
            dev->spi_cfg.spi_dw_cfg.cpol, dev->spi_cfg.spi_dw_cfg.cpha, \
            dev->spi_cfg.spi_dw_cfg.baud, dev->spi_cfg.spi_dw_cfg.bus_width);

    if (dev->spi_cfg.spi_dw_cfg.cpol > 1) {
        spi_printf("cpol.error\n");
        return ERR_USERCFG_ERROR;
    }

    if (dev->spi_cfg.spi_dw_cfg.cpha > 1) {
        spi_printf("cpha.error\n");
        return ERR_USERCFG_ERROR;
    }

    if (dev->spi_cfg.spi_dw_cfg.baud > SPI_MAX_BAUDRATE) {
        spi_printf("baud.error\n");
        return ERR_USERCFG_ERROR;
    }

    if (dev->spi_cfg.spi_dw_cfg.bus_width > 32) {
        spi_printf("width.error\n");
        return ERR_USERCFG_ERROR;
    }
    return SPI_OK;
}

static cyg_uint32 config_cs(cyg_spi_bp2016_dev_t *dev)
{
    cyg_uint32 res = 0;
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;
    HWP_SPI_T *hw_spi = bus->hw_spi;

    spi_debug("dev->cs = %d", dev->spi_cs_num);

    switch ((int)hw_spi) {
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS0
        case (int)hwp_ap_Spi0:
            if (dev->spi_cs_num >= CYGPKG_IO_SPI_ARM_BP2016_BUS0_CS_NUM) {
                res = ERR_USERCFG_ERROR;
            } else {
                dw_spi_cs_enable(hw_spi, dev->spi_cs_num);
            }
            break;
#endif /* CYGPKG_DEVS_SPI_ARM_BP2016_BUS0 */
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS1
        case (int)hwp_ap_Spi1:
            if (dev->spi_cs_num >= CYGPKG_IO_SPI_ARM_BP2016_BUS1_CS_NUM) {
                res = ERR_USERCFG_ERROR;
            } else {
                dw_spi_cs_enable(hw_spi, dev->spi_cs_num);
            }
            break;
#endif /* CYGPKG_DEVS_SPI_ARM_BP2016_BUS1 */
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS2
        case (int)hwp_ap_Spi2:
            if (dev->spi_cs_num >= CYGPKG_IO_SPI_ARM_BP2016_BUS2_CS_NUM_ALL) {
                res = ERR_USERCFG_ERROR;
            } else if (dev->spi_cs_num >= CYGPKG_IO_SPI_ARM_BP2016_BUS2_CS_NUM) {
                dw_spi_cs_enable(hw_spi, 0);
            } else {
                dw_spi_cs_enable(hw_spi, dev->spi_cs_num);
            }
            break;
#endif /* CYGPKG_DEVS_SPI_ARM_BP2016_BUS2 */
        default:
            res = ERR_USERCFG_ERROR;
            break;
    }

    return res;
}

static int spi_xfer_init(cyg_spi_bp2016_bus_t *bus)
{
    int res = 0;
    HWP_SPI_T *hw_spi = bus->hw_spi;

    switch (bus->mode) {
    case SPI_RX_ONLY:
        if (SPI_USE_DMA != bus->dma_use) {
            spi_rx_full_inten(hw_spi, 1);
        }
        spi_rx_underflow_inten(hw_spi, 1);
        spi_rx_overflow_inten(hw_spi, 1);
        break;
    case SPI_TX_ONLY:
        if (SPI_USE_DMA != bus->dma_use) {
            spi_tx_empty_inten(hw_spi, 1);
        }
        spi_tx_overflow_inten(hw_spi, 1);
        spi_debug("tmod == TO");
        break;
    case SPI_EEPROM_MODE:
        if (SPI_USE_DMA != bus->dma_use) {
            spi_tx_empty_inten(hw_spi, 1);
            spi_rx_full_inten(hw_spi, 1);
        }
        spi_rx_underflow_inten(hw_spi, 1);
        spi_rx_overflow_inten(hw_spi, 1);
        spi_tx_overflow_inten(hw_spi, 1);
        spi_debug("tmod == eeprom");
        break;
    default:
        return ERR_MODE_ERROR;
        break;
    }

    return res;
}

// warning: begin cannot modify msgs_num, end clear msgs_num
static int begin_spi_trans(cyg_spi_bp2016_dev_t *dev)
{
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;
    HWP_SPI_T *hw_spi = bus->hw_spi;

    bus->error = SPI_OK;
    dev->error = SPI_OK;

    dev->error = assert_usercfg(dev);
    if (dev->error != SPI_OK) {
        return ERR_USERCFG_ERROR;
    }

    if (dev->gpio_num > 128) {
        return ERR_USERCFG_ERROR;
    }
    // this is for spi cs num, board cs = 10, controller cs is 8.
    if (dev->spi_cs_num >= 8) {
        scm_gpiob_en(4);    // disable spi2 cs0 iomux
        gpio_set_output_low(dev->gpio_num);
    }

    spi_debug("begin hw_spi = 0x%x\n", (cyg_uint32) hw_spi);
    spi_debug("gpio_num = 0x%x\n", dev->gpio_num);
    bus->spi_dfs = dev->spi_dfs;
    spi_debug("dev->error = 0x%x, bus->error = 0x%x, bus->spi_dfs = 0x%x\n", dev->error, bus->error,
              bus->spi_dfs);

    if (bus->spi_dfs <= 8) {
        bus->n_bytes = 1;
    } else if (bus->spi_dfs <= 16) {
        bus->n_bytes = 2;
    } else {
        bus->n_bytes = 4;
    }
    spi_check_idle(hw_spi);

    dw_spi_enable(hw_spi, false);
    spi_disable_all_int(hw_spi);
    spi_set_dma_ctrl(hw_spi, SPI_DMACR_DISABLE_ALL);
    dw_spi_cpmode(hw_spi, dev->spi_cfg.spi_dw_cfg.cpha, dev->spi_cfg.spi_dw_cfg.cpol);
    spi_bp2016_set_baud(bus, dev->spi_cfg.spi_dw_cfg.baud);
    dw_spi_bus_width(hw_spi, dev->spi_cfg.spi_dw_cfg.bus_width);
    dev->error = config_cs(dev);

    switch (bus->mode) {
    case SPI_RX_ONLY:
        dw_spi_ndf(hw_spi, bus->msgs[0].len); // todo: set ndf
        dw_spi_tmod(hw_spi, SPI_CTRL0_TMOD_RO);
        break;
    case SPI_TX_ONLY:
        dw_spi_tmod(hw_spi, SPI_CTRL0_TMOD_TO);
        spi_debug("tmod == TO");
        break;
    case SPI_EEPROM_MODE:
        dw_spi_ndf(hw_spi, bus->msgs[1].len); // todo: set ndf
        dw_spi_tmod(hw_spi, SPI_CTRL0_TMOD_EPROMREAD);
        spi_debug("tmod == eeprom");
        break;
    default:
        return ERR_MODE_ERROR; // todo: error
        break;
    }

    if (SPI_USE_DMA == bus->dma_use) {
        spi_set_dma_ctrl(hw_spi, bus->mode);
        asm volatile("isb" : : : "memory");
        dw_spi_enable(hw_spi, true);
    } else {
        dw_spi_set_tx_fifo_level(hw_spi, SPI_INT_TX_THRESHOLD);
        dw_spi_set_rx_fifo_level(hw_spi, 0);
    }
    spi_xfer_init(bus);
    return SPI_OK;
}

// reset bus variables
static void end_spi_trans(cyg_spi_bp2016_dev_t *dev)
{
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;
    HWP_SPI_T *hw_spi = bus->hw_spi;

    bus->msgs_num = 0;
    bus->msg_idx = 0;
    bus->msg_tx_idx = 0;
    bus->msg_rx_idx = 0;
    bus->mode = 0;

    bus->count = 0;

    bus->tx = NULL;
    bus->rx = NULL;
    spi_check_idle(hw_spi);     // check tx and rx transfer done

    if (dev->spi_cs_num >= 8) {
        scm_spi2_en(1, 1, 1, 1, 1, 1, 1, 1);    //enable spi2 
        gpio_set_output_high(dev->gpio_num);
    }
}

// this begin dma part
#ifdef CYGPKG_DEVS_SPI_ENABLE_DMA
static cyg_uint32 get_dma_valid_threshold(cyg_uint32 count)
{
    cyg_uint32 valid = CYGPKG_DEVS_SPI_DMA_MAX_BURST;

    while ((count % valid) != 0) {
        valid = valid >> 1;
    }

    if (valid == 2) // todo: dma controller limited
        valid = 1;
    return valid;
}

static cyg_uint32 get_dma_burst(cyg_uint32 valid)
{
    cyg_uint32 burst = DMAC_CTL_MSIZE_E_1;

    switch (valid) {
    case 32:
        burst = DMAC_CTL_MSIZE_E_32;
        break;
    case 16:
        burst = DMAC_CTL_MSIZE_E_16;
        break;
    case 8:
        burst = DMAC_CTL_MSIZE_E_8;
        break;
    case 4:
        burst = DMAC_CTL_MSIZE_E_4;
        break;
    default:
        burst = DMAC_CTL_MSIZE_E_1;
        break;
    }

    return burst;
}

static cyg_uint32 get_dma_width(cyg_uint32 n_bytes)
{
    cyg_uint32 width = DMAC_CTL_TR_WIDTH_E_8;

    switch (n_bytes) {
    case 1:
        width = DMAC_CTL_TR_WIDTH_E_8;
        break;
    case 2:
        width = DMAC_CTL_TR_WIDTH_E_16;
        break;
    case 4:
        width = DMAC_CTL_TR_WIDTH_E_32;
        break;
    default:
        width = DMAC_CTL_TR_WIDTH_E_8;
        break;
    }

    return width;
}

static int spi_dma_init(cyg_spi_bp2016_bus_t *bus)
{
    int res = 0;

    if (NULL != (bus->dma_rx_ch = DMA_alloc(DMAC_SLAVE_ID_E_SPI0))) {
        spi_debug("spi read get valid channel %d!\n", *((cyg_uint32 *) bus->dma_rx_ch + 1));
    } else {
        spi_printf("no valid read channel!\n");
        res = ERR_DMA_NO_CHANNEL_ERROR;
        goto exit;
    }

    if (NULL != (bus->dma_tx_ch = DMA_alloc(DMAC_SLAVE_ID_E_SPI0))) {
        spi_debug("spi write get valid channel %d!\n", *((cyg_uint32 *) bus->dma_tx_ch + 1));
        goto exit;
    } else {
        spi_printf("no valid write channel!\n");
        res = ERR_DMA_NO_CHANNEL_ERROR;
        goto free_rxchan;
    }

free_rxchan:
    DMA_free(bus->dma_rx_ch);

exit:
    return res;
}

static void spi_dma_exit(cyg_spi_bp2016_bus_t *bus)
{
    // todo: destory TX transfer
    DMA_free(bus->dma_tx_ch);
    // todo: destory RX transfer
    DMA_free(bus->dma_rx_ch);
}

static int dma_xfer_msgs(cyg_spi_bp2016_bus_t *bus)
{
    int res = 0;
    struct spi_msg *msg = bus->msgs;
    HWP_SPI_T *hw_spi = bus->hw_spi;
    cyg_uint32 dev_addr_spi = (cyg_uint32) &hw_spi->dr;
    cyg_uint32 width = DMAC_CTL_TR_WIDTH_E_8;
    cyg_uint32 burst = DMAC_CTL_MSIZE_E_1;
    cyg_uint32 len = 0;
    cyg_uint8 *buf = NULL;

    res = spi_dma_init(bus);
    if (res != 0) {
        return res;
    }

    width = get_dma_width(bus->n_bytes);

    for (; bus->msg_idx < bus->msgs_num; bus->msg_idx++) {
        len = msg[bus->msg_idx].len / bus->n_bytes; // todo: check 32bit data
        buf = msg[bus->msg_idx].buf;
        bus->dma_threshold = get_dma_valid_threshold(len); // todo: check 32bit data
        burst = get_dma_burst(bus->dma_threshold);

        spi_debug("buf= 0x%x, msgs_num = %d, idx = %d, len = %d, threshold = %d, burst = %d",
                  (u32) buf, bus->msgs_num, bus->msg_idx, len, bus->dma_threshold, burst);

        if ((msg[bus->msg_idx].flags & SPI_M_READ)) {
            spi_set_dma_rdlr(hw_spi, bus->dma_threshold - 1);

            v7_dma_inv_range((u32) buf, (u32) buf + len);
            res = DMA_p2m_set_config(bus->dma_rx_ch, dev_addr_spi, (u32) buf, len, width, burst,
                                     false);
            if (true != res) {
                spi_printf("set config error!");
                res = ERR_DMA_CONFIG_ERROR;
                goto free_channel;
            }
        }

        if ((msg[bus->msg_idx].flags & SPI_M_WRITE)) {
            spi_set_dma_tdlr(hw_spi, bus->dma_threshold);

            v7_dma_clean_range((u32) buf, (u32) buf + len);
            res = DMA_m2p_set_config(bus->dma_tx_ch, (u32) buf, dev_addr_spi, len, width, burst,
                                     true);
            if (true != res) {
                spi_printf("set config error!");
                res = ERR_DMA_CONFIG_ERROR;
                goto free_channel;
            }
        }
    }

    if (bus->mode & SPI_RX_ONLY) {
        DMA_trans(bus->dma_rx_ch);
    }
    if (bus->mode & SPI_TX_ONLY) {
        DMA_trans(bus->dma_tx_ch);
    }

    if (bus->mode & SPI_RX_ONLY) {
        DMA_wait_ch_done(bus->dma_rx_ch);
        spi_debug("wait read finished");
    }

free_channel:
    spi_dma_exit(bus);

    return res;
}
#endif

static void spi_dma_transfer(cyg_spi_device *device, cyg_bool polled, cyg_uint32 count,
                             const cyg_uint8 *tx_data, cyg_uint8 *rx_data, cyg_bool drop_cs)
{
#ifdef CYGPKG_DEVS_SPI_ENABLE_DMA
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) device;
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;

    struct spi_msg *msg;
    int mode = 0;
    u8 *msgbuf = NULL;
    cyg_uint32 flags = 0;

    msg = &bus->msgs[bus->msgs_num];

    spi_debug("bus->msgs_num = %d", bus->msgs_num);
    if (tx_data != NULL) {
        msgbuf = (u8 *) tx_data;
        mode |= SPI_TX_ONLY;
        flags = SPI_M_WRITE;
    }

    if (rx_data != NULL) {
        msgbuf = rx_data;
        mode |= SPI_RX_ONLY;
        flags = SPI_M_READ;
    }

    if (mode == SPI_EEPROM_MODE) {
        dev->error = ERR_MODE_ERROR;
        return; // todo with error, not support TR mode
    }

    msg->buf = msgbuf;
    msg->len = count;
    msg->flags = flags;
    bus->mode |= mode;
    bus->msgs_num++;

    spi_debug("bus->mode = 0x%x", bus->mode);
    if (!drop_cs) { // tx: 0, rx: 1
        return;
    }

    if (bus->msgs_num == 2) {
        if (bus->mode != SPI_EEPROM_MODE) {
            dev->error = ERR_MODE_ERROR;
            return; // todo: with error
        }
    }

    begin_spi_trans(dev);
    dma_xfer_msgs(bus); // this must send all msgs
    end_spi_trans(dev);

    return;
#endif
}

/*
 * Transfer a buffer to a device,
 * fill another buffer with data from the device
 */
static void
spi_cpu_transfer(cyg_spi_device *device, cyg_bool polled,
                 cyg_uint32 count, const cyg_uint8 *tx_data,
                 cyg_uint8 *rx_data, cyg_bool drop_cs)
{
    cyg_uint32 res = 0;
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) device;
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;
    HWP_SPI_T *hw_spi = bus->hw_spi;

    struct spi_msg *msg;
    int mode = 0;
    u8 *msgbuf = NULL;
    cyg_uint32 flags = 0;

    msg = &bus->msgs[bus->msgs_num];

    spi_debug("bus->msgs_num = %d", bus->msgs_num);
    if (tx_data != NULL) {
        msgbuf = (u8 *) tx_data;
        mode |= SPI_TX_ONLY;
        flags = SPI_M_WRITE;
    }

    if (rx_data != NULL) {
        msgbuf = rx_data;
        mode |= SPI_RX_ONLY;
        flags = SPI_M_READ;
    }

    if (mode == SPI_EEPROM_MODE)
        return; // todo with error, not support TR mode

    msg->buf = msgbuf;
    msg->len = count;
    msg->flags = flags;
    bus->mode |= mode;
    bus->msgs_num++;

    spi_debug("bus->mode = 0x%x", bus->mode);
    if (!drop_cs) { // save msg, and wait next msg
        return;
    }

    if (bus->msgs_num == 2) {
        if (bus->mode != SPI_EEPROM_MODE)
            return; // todo: with error
    }

    begin_spi_trans(dev);

    if (dev->error)
        return;

    if (!polled) {
        cyg_drv_mutex_lock(&bus->spi_lock);

        cyg_drv_dsr_lock();
        dw_spi_enable(hw_spi, true);

        res = cyg_drv_cond_wait(&bus->spi_wait);

        dev->error = bus->error;

        spi_debug("res = 0x%x, dev->error = 0x%x", res, dev->error);
        if (res == 0) {
            dev->error = res;
            spi_printf("wait error");
        }

        cyg_drv_dsr_unlock();
        end_spi_trans(dev); // thread could switch, but mutex must wait for done

        if (drop_cs) {
            dw_spi_cs_disable(hw_spi);
        }

        cyg_drv_mutex_unlock(&bus->spi_lock);
    } else {
    }

    return;
}

/*
 * Begin transaction
 * configure bus for device and drive CS by calling device cs() function
 */
static void
spi_bp2016_transaction_begin(cyg_spi_device *device)
{
}

/*
 * attention: drop_cs = 0, it means eeprom mode, both tx and rx use drop_cs = 0
 * TO/RO/TR mode set drop_cs = 1
 */
static void
spi_bp2016_transaction_transfer(cyg_spi_device *device, cyg_bool polled,
                                cyg_uint32 count, const cyg_uint8 *tx_data,
                                cyg_uint8 *rx_data, cyg_bool drop_cs)
{
    cyg_spi_bp2016_dev_t *dev = (cyg_spi_bp2016_dev_t *) device;
    cyg_spi_bp2016_bus_t *bus = (cyg_spi_bp2016_bus_t *) dev->spi_device.spi_bus;

    if (!count) {
        dev->error = ERR_COUNT_ERROR;
        return;
    }

    if (SPI_USE_DMA == bus->dma_use) {
        spi_dma_transfer(device, polled, count, tx_data, rx_data, drop_cs);
    } else {
        spi_cpu_transfer(device, polled, count, tx_data, rx_data, drop_cs);
    }

    return;
}

/*
 * Tick
 */
static void
spi_bp2016_transaction_tick(cyg_spi_device *device, cyg_bool polled, cyg_uint32 count)
{
}

/*
 * End transaction
 * disable SPI bus, drop CS, reset transfer variables
 */
static void
spi_bp2016_transaction_end(cyg_spi_device *device)
{
}

static void is_dma_use(cyg_spi_bp2016_bus_t *bus)
{
// if disable dma package, dma_use = 0, spi use cpu mode
// else use default dma settings
#ifndef CYGPKG_DEVS_SPI_ENABLE_DMA
    bus->dma_use = SPI_USE_CPU;
#endif
}

/*
 * Driver & bus initialization
 */
static void
spi_bp2016_init_bus(cyg_spi_bp2016_bus_t *bus)
{
    cyg_drv_mutex_init(&bus->spi_lock);
    cyg_drv_cond_init(&bus->spi_wait, &bus->spi_lock);

    cyg_drv_interrupt_create(bus->spi_vector,
                             bus->spi_prio,
                             (cyg_addrword_t) bus,
                             &spi_bp2016_ISR,
                             &spi_bp2016_DSR,
                             &bus->spi_intr_handle,
                             &bus->spi_intr);
    cyg_drv_interrupt_attach(bus->spi_intr_handle);
	CYG_SPI_BUS_COMMON_INIT(&bus->spi_bus);
	is_dma_use(bus);
    cyg_drv_interrupt_unmask(bus->spi_vector);
    spi_debug("spi bus 0x%x init finished!\n", (cyg_uint32) bus->hw_spi);
}

// If C constructor with init priority functionality is not in compiler,
// rely on spi_bp2016_init.cxx to init us.
#ifndef CYGBLD_ATTRIB_C_INIT_PRI
#define CYGBLD_ATTRIB_C_INIT_PRI(x)
#endif

void CYGBLD_ATTRIB_C_INIT_PRI(CYG_INIT_BUS_SPI)
cyg_spi_bp2016_bus_init(void)
{
#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS0
    spi_bp2016_init_bus(&cyg_spi_bp2016_bus0);
#endif

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS1
    spi_bp2016_init_bus(&cyg_spi_bp2016_bus1);
#endif

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS2
    spi_bp2016_init_bus(&cyg_spi_bp2016_bus2);
#endif
}

#endif /* CYGPKG_DEVS_SPI_ARM_BP2016 */

