#include <pkgconf/system.h>

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016
#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h> // base types
#include <cyg/hal/hal_if.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/hal_io.h>   // IO macros
#include <cyg/hal/hal_arch.h> // Register state info
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/regs/spi.h>

inline void dw_spi_cpmode(HWP_SPI_T *hw_spi, const u32 cpha, const u32 cpol)
{
    u32 tmp = hw_spi->ctrl0;
    tmp &= SPI_CTRL0_SCPH_MIDDLE_TOGGLE;
    tmp &= SPI_CTRL0_SCPOL_LOW;
    tmp |= (cpha << SPI_CTRL0_SCPH_OFFSET) | (cpol << SPI_CTRL0_SCPOL_OFFSET);
    hw_spi->ctrl0 = tmp;
}

inline void dw_spi_tmod(HWP_SPI_T *hw_spi, const u32 tmod)
{
    u32 tmp = hw_spi->ctrl0;
    tmp &= ~SPI_CTRL0_TMOD_MASK;
    tmp |= tmod;
    hw_spi->ctrl0 = tmp;
}

inline void dw_spi_bus_width(HWP_SPI_T *hw_spi, u32 width)
{
    u32 ctrl0 = hw_spi->ctrl0;
    width -= 1;
    ctrl0 &= ~SPI_CTRL0_DFS32_MASK;
    ctrl0 |= ((width << SPI_CTRL0_DFS32_OFFSET) & SPI_CTRL0_DFS32_MASK);
    hw_spi->ctrl0 = ctrl0;
}

inline void dw_spi_cs_enable(HWP_SPI_T *hw_spi, const u32 cs)
{
    if (cs > 0xf) {
        return;
    }
    hw_spi->ser = (1 << cs);
}

inline void dw_spi_cs_disable(HWP_SPI_T *hw_spi)
{
    hw_spi->ser = 0;
}

inline int dw_spi_get_cs(HWP_SPI_T *hw_spi)
{
    return hw_spi->ser;
}

inline void dw_spi_ndf(HWP_SPI_T *hw_spi, const u32 ndf)
{
    hw_spi->ctrl1 = (ndf - 1) & 0xffff;     // this is must be rx length - 1
}

inline void dw_spi_baud(HWP_SPI_T *hw_spi, const u32 baud)
{
    hw_spi->baudr = baud;
}

inline void dw_spi_sample_dly(HWP_SPI_T *hw_spi, const u32 sample_delay)
{
    hw_spi->rx_sample_dly = sample_delay;
}

inline void dw_spi_enable(HWP_SPI_T *hw_spi, cyg_uint32 en)
{
    if (en == true) {
        hw_spi->ssienr = SPI_SSIENR_ENABLE_SPI;
    }
    else {
        hw_spi->ssienr = SPI_SSIENR_DISABLE_SPI;
    }
}

inline void dw_spi_disable(HWP_SPI_T *hw_spi)
{
    hw_spi->ssienr = SPI_SSIENR_DISABLE_SPI;
}

inline int dw_spi_read_32bit(HWP_SPI_T *hw_spi)
{
    return hw_spi->dr;
}

inline cyg_uint16 dw_spi_read_16bit(HWP_SPI_T *hw_spi)
{
    return hw_spi->dr & 0xffff;
}

inline cyg_uint8 dw_spi_read_8bit(HWP_SPI_T *hw_spi)
{
    return hw_spi->dr & 0xff;
}

inline void dw_spi_write_32bit(HWP_SPI_T *hw_spi, cyg_uint32 ret)
{
    hw_spi->dr = ret;
}

inline void dw_spi_write_16bit(HWP_SPI_T *hw_spi, cyg_uint16 ret)
{
    hw_spi->dr = ret;
}

inline void dw_spi_write_8bit(HWP_SPI_T *hw_spi, cyg_uint8 ret)
{
    hw_spi->dr = ret;
}

inline bool dw_spi_is_busy(HWP_SPI_T *hw_spi)
{
    return ((hw_spi->sr & SR_BUSY) == SR_BUSY) ? true : false;
}

inline bool dw_spi_is_rx_not_empty(HWP_SPI_T *hw_spi)
{
    return ((hw_spi->sr & SR_RF_NOT_EMPT) == SR_RF_NOT_EMPT) ? true : false;
}

inline void spi_rx_full_inten(HWP_SPI_T *hw_spi, int en)
{
    if (en > 0)
        hw_spi->imr |= SPI_INT_RXFI;
    else
        hw_spi->imr &= ~SPI_INT_RXFI;
}

inline void spi_rx_underflow_inten(HWP_SPI_T *hw_spi, int en)
{
    if (en > 0)
        hw_spi->imr |= SPI_INT_RXUI;
    else
        hw_spi->imr &= ~SPI_INT_RXUI;
}

inline void spi_rx_overflow_inten(HWP_SPI_T *hw_spi, int en)
{
    if (en > 0)
        hw_spi->imr |= SPI_INT_RXOI;
    else
        hw_spi->imr &= ~SPI_INT_RXOI;
}

inline void spi_tx_empty_inten(HWP_SPI_T *hw_spi, int en)
{
    if (en > 0)
        hw_spi->imr |= SPI_INT_TXEI;
    else
        hw_spi->imr &= ~SPI_INT_TXEI;
}

inline void spi_tx_overflow_inten(HWP_SPI_T *hw_spi, int en)
{
    if (en > 0)
        hw_spi->imr |= SPI_INT_TXOI;
    else
        hw_spi->imr &= ~SPI_INT_TXOI;
}

inline void spi_disable_all_int(HWP_SPI_T *hw_spi)
{
    hw_spi->imr = SPI_INT_DISABLE_ALL;
}

inline void dw_spi_set_rx_fifo_level(HWP_SPI_T *hw_spi, int rft)
{
    hw_spi->rxftlr = rft;
}

inline void dw_spi_set_tx_fifo_level(HWP_SPI_T *hw_spi, int tft)
{
    hw_spi->txftlr = tft;
}

inline int dw_spi_get_txfifo_valid_num(HWP_SPI_T *hw_spi)
{
    int tx_count = 256 - hw_spi->txflr;
    return tx_count;
}

inline int dw_spi_get_rxfifo_valid_num(HWP_SPI_T *hw_spi)
{
    int rx_count = hw_spi->rxflr;
    return rx_count;
}

inline u32 dw_spi_clr_all_interrupts(HWP_SPI_T *hw_spi)
{
    return (hw_spi->icr);
}

inline int dw_spi_polling_raw_status(HWP_SPI_T *hw_spi)
{
    u32 irq_cause = hw_spi->risr;

    dw_spi_clr_all_interrupts(hw_spi);

    if (irq_cause & SPI_INT_RXFI) {
        return SPI_INT_RXFI;
    }

    if (irq_cause & SPI_INT_TXEI) {
        return SPI_INT_TXEI;
    }
    return 0;
}

inline u32 dw_spi_get_isr_status(HWP_SPI_T *hw_spi)
{
    return (hw_spi->isr);
}

inline void spi_set_dma_ctrl(HWP_SPI_T *hw_spi, u8 enable)
{
    hw_spi->dmacr = enable;
}

inline void spi_set_dma_tdlr(HWP_SPI_T *hw_spi, u32 tdlr)
{
    hw_spi->dmatdlr = tdlr;
}

inline void spi_set_dma_rdlr(HWP_SPI_T *hw_spi, u32 rdlr)
{
    hw_spi->dmardlr = rdlr;
}

#endif /* CYGPKG_DEVS_SPI_ARM_BP2016 */
