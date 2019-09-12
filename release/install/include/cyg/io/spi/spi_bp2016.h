#ifndef CYG_DEVS_SPI_ARM_BP2016_H
#define CYG_DEVS_SPI_ARM_BP2016_H

//==========================================================================
//
//      spi_bp2016.h
//
//      SPI driver for
//
//==========================================================================
//
//#####DESCRIPTIONBEGIN####
//
// Author(s):    huyue<huyue@haigebeidou.cn>
// Contributors: @BSP Team
// Date:         2018-01-17
// Purpose:
// Description:
//
//####DESCRIPTIONEND####
//
//==========================================================================
#include <pkgconf/hal.h>
#include <pkgconf/io_spi.h>
#include <pkgconf/devs_spi_arm_bp2016.h>

#include <cyg/infra/cyg_type.h>
#include <cyg/hal/drv_api.h>
#include <cyg/io/spi.h>
#include <cyg/hal/regs/spi.h>

#ifdef CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4
#include <cyg/hal/regs/spi3wire.h>
#endif /* CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4 */

#define     SPI_MAX_BAUDRATE    190000000

struct spi_msg {
    cyg_uint32 flags;	/* read : 0, write : 1 */
#define SPI_M_READ		0x0001	/* read data, from slave to master */
#define SPI_M_WRITE		0x0002	/* write data, from master to slave*/
    cyg_uint32 len;		/* msg length				*/
    cyg_uint8 *buf;		/* pointer to msg data			*/
};

typedef struct {
    cyg_spi_bus spi_bus;

    cyg_interrupt spi_intr;
    cyg_handle_t spi_intr_handle;
    cyg_vector_t spi_vector;
    cyg_priority_t spi_prio;

    cyg_drv_mutex_t spi_lock;
    cyg_drv_cond_t spi_wait;

    void *hw_spi;

    struct spi_msg msgs[2];
    cyg_uint32 msgs_num;
    cyg_uint32 msg_idx;
    cyg_uint32 msg_tx_idx;
    cyg_uint32 msg_rx_idx;
    cyg_uint32 mode;    //todo: maybe no use, necessary?
#define SPI_TR_MODE			0X0
#define SPI_RX_ONLY			0X1
#define SPI_TX_ONLY			0X2
#define SPI_EEPROM_MODE 	0X3

#ifdef CYGPKG_DEVS_SPI_ENABLE_DMA
    void *dma_rx_ch; // read channel
    void *dma_tx_ch; // write channel
    cyg_uint32 dma_threshold;
#endif

    cyg_uint32 error;
    cyg_uint32 spi_dfs;
    cyg_uint32 n_bytes;

    cyg_uint32 status;
#define STATUS_IDLE           0x0
#define STATUS_IN_PROGRESS    0x1
    cyg_uint32 dma_use;	// whether spi use dma
#define SPI_USE_DMA		0x0001	/* use dma mode */
#define SPI_USE_CPU	    0x0000	/* use cpu mode */

    volatile cyg_uint32 count;
    volatile cyg_uint8 *tx;
    volatile cyg_uint8 *rx;
} cyg_spi_bp2016_bus_t;

typedef struct {
    cyg_spi_device spi_device;

    cyg_uint32 error;
    cyg_uint32 spi_dfs;
    cyg_uint32 spi_tmod;
    cyg_uint32 spi_cs_num;
    cyg_uint32 gpio_num;
    union {
        struct {
            cyg_uint32 cpol;
            cyg_uint32 cpha;
            cyg_uint32 baud;
            cyg_uint32 bus_width;
        } spi_dw_cfg;

        struct {
            cyg_uint32 baud;
            cyg_uint32 addr_width;
            cyg_uint32 data_width;
        } spi3wire_cfg;
    } spi_cfg;
} cyg_spi_bp2016_dev_t;

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS0
externC cyg_spi_bp2016_bus_t cyg_spi_bp2016_bus0;
#endif

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS1
externC cyg_spi_bp2016_bus_t cyg_spi_bp2016_bus1;
#endif

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016_BUS2
externC cyg_spi_bp2016_bus_t cyg_spi_bp2016_bus2;
#endif

#ifdef CYGPKG_DEVS_SPI3WIRE_ARM_BP2016_BUS4
externC cyg_spi_bp2016_bus_t cyg_spi3wire_bp2016_bus4;
#endif

enum spi_error_e {
    SPI_OK = 0,
    ERR_TIMEOUT = -10,
    ERR_CS_ERROR,
    ERR_MODE_ERROR,
    ERR_USERCFG_ERROR,
    ERR_COUNT_ERROR,
    ERR_RX_UNDERFLOW_ERROR,
    ERR_RX_OVERFLOW_ERROR,
    ERR_TX_OVERFLOW_ERROR,
    ERR_DMA_NO_CHANNEL_ERROR,
    ERR_DMA_CONFIG_ERROR,
};
#endif /* CYG_DEVS_SPI_ARM_BP2016_H */

