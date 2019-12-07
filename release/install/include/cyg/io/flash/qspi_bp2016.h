#ifndef __QSPI_DEV_BP2016_H__
#define __QSPI_DEV_BP2016_H__

#include <cyg/io/flash/qspi_dev.h>

#define CYGNUM_HAL_INT_PRIO_QSPI (1)
#define QSPI_MEM_ADDR (0x10000000)
#define QSPI_MEM_SIZE (0x2000000) // 32MB


typedef struct cyg_qspi_dev {
    cyg_flash_block_info_t block_info[1];
    struct qspi_fl_info *fl_dev;

    cyg_drv_mutex_t qspi_lock;
//    cyg_drv_cond_t qspi_wait;
    cyg_vector_t qspi_isrvec;
    int qspi_isrpri;
    cyg_handle_t qspi_interrupt_handle; // For initializing the interrupt
    cyg_interrupt qspi_interrupt_data;

#ifdef CYGPKG_DEVS_QSPI_DMA_EN
    void *dma_rx_ch; // read channel
    void *dma_tx_ch; // write channel
#endif

} cyg_qspi_flash_dev;

#endif //__QSPI_DEV_BP2016_H__
