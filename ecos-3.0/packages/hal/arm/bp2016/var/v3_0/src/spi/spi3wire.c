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
#include <cyg/hal/regs/spi3wire.h>

inline void spi_3wire_bus_width(HWP_SPI_3WIRE_T *hw_spi, const u32 addr_width, const u32 data_width)
{
    u32 tmpwidth = 0;
    tmpwidth = (addr_width << 16) & 0xffff0000;
    tmpwidth |= data_width & 0xffff;
    hw_spi->length = tmpwidth;
}

inline void spi_3wire_div(HWP_SPI_3WIRE_T *hw_spi, const u32 div)
{
    hw_spi->clkdiv = div;
}

static inline void spi_3wire_set_cs(HWP_SPI_3WIRE_T *hw_spi, const u32 cs)
{
    switch (cs) {
        case 0:
            hw_spi->ctrl = SPI_3WIRE_CTRL_CS0;
            break;
        case 1:
            hw_spi->ctrl = SPI_3WIRE_CTRL_CS1;
            break;
        default:
            break;
    }
}

inline bool spi_3wire_is_busy(HWP_SPI_3WIRE_T *hw_spi)
{
    return ((hw_spi->busy & SPI_3WIRE_BUSY) != SPI_3WIRE_BUSY) ? true : false;
}

inline int spi_3wire_cmd_read(HWP_SPI_3WIRE_T *hw_spi, const u32 cs, const u32 addr)
{
    int ret = 0;
    hw_spi->addrin = addr;

    switch (cs) {
        case 0:
            hw_spi->ctrl = (SPI_3WIRE_CTRL_READ | SPI_3WIRE_CTRL_START | SPI_3WIRE_CTRL_CS0);
            break;
        case 1:
            hw_spi->ctrl = (SPI_3WIRE_CTRL_READ | SPI_3WIRE_CTRL_START | SPI_3WIRE_CTRL_CS1);
            break;
        case 2:
            hw_spi->ctrl = (SPI_3WIRE_CTRL_READ | SPI_3WIRE_CTRL_START | SPI_3WIRE_CTRL_CS2);
            break;
        default:
            ret = -1;
            break;
    }
    return (ret);
}

inline u32 spi_3wire_read_value(HWP_SPI_3WIRE_T *hw_spi)
{
    u32 rdata = (hw_spi->rdata);
    return rdata;
}

inline int spi_3wire_writereg(HWP_SPI_3WIRE_T *hw_spi, const u32 cs, const u32 addr, const int value)
{
    hw_spi->addrin = addr;
    hw_spi->wdata = value;

    switch (cs) {
        case 0:
            hw_spi->ctrl = (SPI_3WIRE_CTRL_WRITE | SPI_3WIRE_CTRL_START | SPI_3WIRE_CTRL_CS0);
            break;
        case 1:
            hw_spi->ctrl = (SPI_3WIRE_CTRL_WRITE | SPI_3WIRE_CTRL_START | SPI_3WIRE_CTRL_CS1);
            break;
        case 2:
            hw_spi->ctrl = (SPI_3WIRE_CTRL_WRITE | SPI_3WIRE_CTRL_START | SPI_3WIRE_CTRL_CS2);
            break;
        default:
            return -1;
            break;
    }
    return 0;
}

#endif /* CYGPKG_DEVS_SPI_ARM_BP2016 */

