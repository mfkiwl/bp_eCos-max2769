#ifndef _DMA_IF_H_
#define _DMA_IF_H_

#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/regs/dma.h>

#define DMAC_LLI_NODE_MAX (0x8)
#define DMAC_UART_BLK_MAX (HW_DMAC_CH_BLK_MAX)

typedef struct lli_node {
    U32 sar;
    U32 sar_upper;
    U32 dar;
    U32 dar_upper;
    U32 blk_ts;
    U32 resv0;
    U32 llp;
    U32 llp_upper;
    U32 ctl;
    U32 ctl_upper;
    U32 sstat;
    U32 dstat;
    U32 llp_status;
    U32 llp_status_upper;
    U32 resv1;
    U32 resv2;
} LLI_NODE_ST;

typedef enum {
    DMAC_ERR_E_OK = 0,
    DMAC_ERR_E_NOT_INIT = 1,
    DMAC_ERR_E_CH_INVALID = 2,
    DMAC_ERR_E_CH_BUSY = 3,
    DMAC_ERR_E_CH_TIMEOUT = 4,
    DMAC_ERR_E_PARA_INVALID,
    DMAC_ERR_E_NO_VALID_CH,
    DMAC_ERR_E_FREE_NOT_ALLOC_CH,
    DMAC_ERR_E_NOT_END,
    DMAC_ERR_E_RESETTING,
    DMAC_ERR_E_MAX
} DMAC_ERR_E;

typedef enum {
    DMA_TRANS_TYPE_E_M2M = 0,
	DMA_TRANS_TYPE_E_QSPIFLASH,
    DMA_TRANS_TYPE_E_OTHER
}DMA_TRANS_TYPE_E;

/**
 * enum dma_transfer_direction - dma transfer mode and direction indicator
 * @DMA_MEM_TO_MEM: Async/Memcpy mode
 * @DMA_MEM_TO_DEV: Slave mode & From Memory to Device
 * @DMA_DEV_TO_MEM: Slave mode & From Device to Memory
 * @DMA_DEV_TO_DEV: Slave mode & From Device to Device
 */
typedef enum dma_transfer_direction {
    DMA_MEM_TO_MEM,
    DMA_MEM_TO_DEV,
    DMA_DEV_TO_MEM,
    DMA_DEV_TO_DEV,
    DMA_TRANS_NONE,
}DMA_TRANS_DIR_E;

// DMAC Peripheral enum
typedef enum {
    DMAC_SLAVE_ID_E_QSPI = 0,
    DMAC_SLAVE_ID_E_SPI0,
    DMAC_SLAVE_ID_E_SPI3,
    DMAC_SLAVE_ID_E_UART6,
    DMAC_SLAVE_ID_E_UART7,
    DMAC_SLAVE_ID_E_I2C0,
    DMAC_SLAVE_ID_E_I2C1,
    DMAC_SLAVE_ID_E_UART1,
    DMAC_SLAVE_ID_E_MEM,
    DMAC_SLAVE_ID_E_UNKNOWN,
}DMAC_SLAVE_ID_E;


// Number of data items to be transferred                                                                                                
typedef enum {
    DMAC_CTL_MSIZE_E_1 = 0,
    DMAC_CTL_MSIZE_E_4,
    DMAC_CTL_MSIZE_E_8,
    DMAC_CTL_MSIZE_E_16,
    DMAC_CTL_MSIZE_E_32,
    DMAC_CTL_MSIZE_E_64,
    DMAC_CTL_MSIZE_E_128,
    DMAC_CTL_MSIZE_E_256,
    DMAC_CTL_MSIZE_E_512,
    DMAC_CTL_MSIZE_E_1024,
    DMAC_CTL_MSIZE_E_ERR
}DMAC_CTL_MSIZE_E;

// Source/Destination Transfer Width
typedef enum {
    DMAC_CTL_TR_WIDTH_E_8 = 0,
    DMAC_CTL_TR_WIDTH_E_16,
    DMAC_CTL_TR_WIDTH_E_32,
    DMAC_CTL_TR_WIDTH_E_64,
    DMAC_CTL_TR_WIDTH_E_128,
    DMAC_CTL_TR_WIDTH_E_256,
    DMAC_CTL_TR_WIDTH_E_512,
    DMAC_CTL_TR_WIDTH_E_ERR
}DMAC_CTL_TR_WIDTH_E;

// function declare
void * DMA_alloc(DMAC_SLAVE_ID_E slave_id);
cyg_uint32 DMA_free(void * dma_handle);
cyg_uint32 DMA_wait_ch_done(void * dma_handle);
bool DMA_m2m_set_config(void * dma_handle, cyg_uint32 src, cyg_uint32 dst, cyg_uint32 len, bool sync);
bool DMA_m2p_set_config(void * dma_handle, cyg_uint32 src, 
                    cyg_uint32 dst, cyg_uint32 len, 
                    DMAC_CTL_TR_WIDTH_E dst_width, 
                    DMAC_CTL_MSIZE_E dst_burst, bool sync);
bool DMA_p2m_set_config(void * dma_handle, cyg_uint32 src, 
                    cyg_uint32 dst, cyg_uint32 len, 
                    DMAC_CTL_TR_WIDTH_E src_width, 
                    DMAC_CTL_MSIZE_E src_burst, bool sync);
int DMA_trans(void * dma_handle);
bool DMA_abort(void * dma_handle);


#endif
