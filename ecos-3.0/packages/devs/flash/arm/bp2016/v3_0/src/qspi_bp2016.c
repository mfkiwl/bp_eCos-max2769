//==========================================================================
//
//      qspi_bp2016.c
//
//      Qspi driver for BP2016
//
//==========================================================================
// -------------------------------------------
// ####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    jinyingwu
// Date:         2018-01-31
// Purpose:
// Description:
//
//####DESCRIPTIONEND####
//
//==========================================================================

//==========================================================================
//                                 INCLUDES
//==========================================================================

#include <cyg/infra/diag.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/drv_api.h>

#include <pkgconf/devs_qspi_bp2016.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/kernel/kapi.h>
#include <errno.h>
#include <string.h>

#include <cyg/io/flash_dev.h>
#include <cyg/io/flash.h>

#include <cyg/io/flash/qspi_bp2016.h>
#include "qspi_hw.h"
#include "qspi_wrapper.h"
#include "qspi_dbg.h"
#include "fl_common_cmd.h"

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
#include <cyg/hal/api/dma_api.h>
#endif

void *qspi_dma_tx_chan = NULL;
//#define QSPI_DFS (8)
cyg_drv_cond_t qspi_wait;

#ifdef CYGPKG_DEVS_QSPI_QUAD_ENABLE_1
static int quad_enable = 1;
//static int rd_cmd = SPINOR_OP_READ_1_1_4;
static int rd_cmd = SPINOR_OP_READ_QUAD_IO;
#else
static int quad_enable = 0;
//static int rd_cmd = SPINOR_OP_READ;
static int rd_cmd = SPINOR_OP_READ_1_1_2;
#endif

void qspi_dsr_lock(void)
{
    cyg_drv_dsr_lock();
}

void qspi_dsr_unlock_cond_wait(void)
{
    cyg_drv_cond_wait(&qspi_wait);
    cyg_drv_dsr_unlock();
}

static cyg_uint32 bp2016_qspi_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_drv_interrupt_mask(vector);
    cyg_drv_interrupt_acknowledge(vector);
    return (CYG_ISR_CALL_DSR);
}

//==========================================================================
// DSR signals data
//==========================================================================
static void bp2016_qspi_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
	int ret=0;
    //cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) data;

	//qspi_debug("enter!");
    cyg_uint32 st = qspi_irq_read_status();

    if (st == QSPI_SR_RX_FULL) {
        ret = qspi_xfer_rx_process();
    }

    if (st == QSPI_SR_TX_EMPTY) {
        ret = qspi_xfer_tx_process();
    }

	if(TRNF_OVER == ret)
		cyg_drv_cond_signal(&qspi_wait);

    cyg_drv_interrupt_unmask(vector);
}

int bp2016_qspi_init(struct cyg_flash_dev *dev)
{
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;

	qspi_debug("enter!");
    //disable qspi, qspi is init in bootrom, if not disable at here
	//it may rise interrupt after install interrupt handle
	qspi_hw_init();

    cyg_drv_mutex_init(&qspi_dev->qspi_lock);
    cyg_drv_cond_init(&qspi_wait, &qspi_dev->qspi_lock);
    cyg_drv_interrupt_create(qspi_dev->qspi_isrvec, qspi_dev->qspi_isrpri,
                             (cyg_addrword_t) qspi_dev, &bp2016_qspi_isr, &bp2016_qspi_dsr,
                             &(qspi_dev->qspi_interrupt_handle), &(qspi_dev->qspi_interrupt_data));
    cyg_drv_interrupt_attach(qspi_dev->qspi_interrupt_handle);
    cyg_drv_interrupt_unmask(qspi_dev->qspi_isrvec);
	qspi_debug("done!");

    return ENOERR;
}

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
static int qspi_dma_init(cyg_qspi_flash_dev *bus)
{
    int res = 0;

    qspi_debug("ENTER");
    qspi_printf("qspi work at dma mode!\n");

    if (NULL != (bus->dma_rx_ch = DMA_alloc(DMAC_SLAVE_ID_E_QSPI))) {
        qspi_debug("spi read get valid channel %d!\n", *((cyg_uint32 *) bus->dma_rx_ch + 1));
    } else {
        qspi_printf("no valid read channel!\n");
        res = QSPI_OP_FAILED;
        goto exit;
    }

    if (NULL != (bus->dma_tx_ch = DMA_alloc(DMAC_SLAVE_ID_E_QSPI))) {
        qspi_debug("spi write get valid channel %d!\n", *((cyg_uint32 *) bus->dma_tx_ch + 1));
        qspi_dma_tx_chan = bus->dma_tx_ch;
        goto exit;
    } else {
        qspi_printf("no valid write channel!\n");
        res = QSPI_OP_FAILED;
        goto free_rxchan;
    }

free_rxchan:
    DMA_free(bus->dma_rx_ch);

exit:
    return res;
}
#endif

extern struct qspi_fl_info * fl_rescan(void);

size_t bp2016_qspi_query(struct cyg_flash_dev *dev, void *data, size_t len)
{
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = NULL; 
    static char query[] = "HG BP2016 Qspi Flash";
    memcpy(data, query, sizeof(query));

	qspi_debug("enter!");

    cyg_drv_mutex_lock(&qspi_dev->qspi_lock);

    qspi_hw_init();
#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
    qspi_dma_init(qspi_dev);
#endif

    fl_dev = fl_rescan();

    if(NULL == fl_dev)
	{
		qspi_printf("scan qspi device failed!");
		cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);
		return -EINVAL;
	}else{
		qspi_dev->fl_dev = fl_dev;
		qspi_printf("scan done!");
	}

	qspi_printf("block_size %u  block num %u", fl_dev->blk_size, fl_dev->blk_num);
	
	if(fl_dev->reset)
		fl_dev->reset();

	qspi_printf("reset done!");

#if 1
    if (fl_dev->config_quad(quad_enable) != 0) {
		qspi_printf("config quad mode failed!");
		cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);
        return -ENOENT;
    }
#endif

    dev->num_block_infos = 1;
    qspi_dev->block_info[0].block_size = fl_dev->blk_size;
    qspi_dev->block_info[0].blocks = fl_dev->blk_num;

	dev->end = QSPI_MEM_ADDR + fl_dev->blk_size*fl_dev->blk_num - 1;

    cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);

    return ENOERR;
}

int bp2016_qspi_erase_block(struct cyg_flash_dev *dev, cyg_flashaddr_t block_base, int erase_type)
{
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;
    int status;
    cyg_flashaddr_t offset = block_base - dev->start;


	qspi_debug("erase addr 0x%x", block_base);

    cyg_drv_mutex_lock(&qspi_dev->qspi_lock);

	fl_dev->config_4byte_extend(offset, offset&0x3000000);

    if(HG_FLASH_SECTOR_ERASE == erase_type)
    {
        if(fl_dev->sector_size != 0)
        {
            status = fl_dev->erase(SPINOR_OP_BE_4K, offset);
        }else{
            status = QSPI_OP_FAILED; 
        }
    }else if(HG_FLASH_BLOCK_ERASE == erase_type)
    {
        if(4096 == fl_dev->blk_size) //block size is 4K, sector erase
        {
            status = fl_dev->erase(SPINOR_OP_BE_4K, offset);
        }else if(32768 == fl_dev->blk_size){	//block size is 32KiB
            status = fl_dev->erase(SPINOR_OP_BE_32K, offset);
        }else{ //other, 64KB erase, for Spansion this command do 256KB erase
            status = fl_dev->erase(SPINOR_OP_BE_64K, offset);
        }
    }

	//fl_dev->read_status();
    cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);

    if (status != QSPI_OP_SUCCESS)
	{
		qspi_printf("erase failed! addr 0x%x, ret=%d", block_base - QSPI_MEM_ADDR, status);
        return -1;
	}
    return ENOERR;
}

int bp2016_qspi_program(struct cyg_flash_dev *dev, cyg_flashaddr_t base, const void *data,
                        size_t len)
{
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;
    cyg_flashaddr_t offset = base - dev->start;
    int status, i=0, t_len=0;
	cyg_uint8 *ptr = NULL;
	int ret = ENOERR;

	qspi_debug("base 0x%x len 0x%x", offset, len);
    cyg_drv_mutex_lock(&qspi_dev->qspi_lock);

#ifdef QSPI_DEBUG
	fl_dev->read_status();
#endif

	ptr = (cyg_uint8 *)data;
	for(i=0; i<len;)
	{
		if(offset%256)
		{
			t_len = 256 - offset%256;	
			if(t_len>len)
				t_len = len;
		}else{
			if((len-i) >= 256)
			{
				t_len=256;
			}else{
				t_len=len-i;
			}
		}

        fl_dev->config_4byte_extend(offset, offset&0x3000000);

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
        //cyg_drv_interrupt_mask(qspi_dev->qspi_isrvec);
		status = fl_dev->dma_write_page(qspi_dev->dma_tx_ch, offset, ptr, t_len);
        //cyg_drv_interrupt_unmask(qspi_dev->qspi_isrvec);
#else
		status = fl_dev->write_page(offset, ptr, t_len);
#endif

		if(QSPI_OP_SUCCESS != status)
		{
			ret = -EIO;
			break;
		}

		i += t_len;
		ptr += t_len;
		offset += t_len;
	}

    cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);

    if (status != QSPI_OP_SUCCESS)
        return -1;

    return ret;
}

int bp2016_qspi_read(struct cyg_flash_dev *dev, const cyg_flashaddr_t base, void *data, size_t len)
{
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;
    cyg_flashaddr_t offset = base - dev->start;
    cyg_uint32 rx_count;
	cyg_uint8 *ptr = NULL;
    int i=0, t_len=0, ret = ENOERR;
#if (CYGPKG_DEVS_QSPI_DMA_EN == 0)
	cyg_uint32 qspi_dfs = 32;
#endif

	qspi_debug("enter! read_base 0x%x len %d", base, len);
	qspi_debug("base 0x%x len 0x%x", offset, len);

    cyg_drv_mutex_lock(&qspi_dev->qspi_lock);

	ptr = (cyg_uint8 *)data;

	for(i=0; i<len;)
	{
		if(offset%256)
		{
			t_len = 256 - offset%256;	
			if(t_len > len)
				t_len = len;
		}else{
			if((len-i) >= 256)
			{
				t_len=256;
			}else{
				t_len=len-i;
			}
		}

#if (CYGPKG_DEVS_QSPI_DMA_EN == 0)
		if((offset%4) || (t_len%4))
		{
			qspi_dfs = 8;
			qspi_printf("set dfs to 8!");
		}else{
			qspi_dfs = 32;
		}
#endif

        fl_dev->config_4byte_extend(offset, offset&0x3000000);

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
        cyg_drv_interrupt_mask(qspi_dev->qspi_isrvec);
		rx_count = fl_dev->dma_read_page(qspi_dev->dma_rx_ch, rd_cmd, offset, ptr, t_len);
        qspi_clr_all_interrupts();
        cyg_drv_interrupt_unmask(qspi_dev->qspi_isrvec);
#else
		rx_count = fl_dev->read_page(rd_cmd, offset, ptr, t_len, qspi_dfs);
#endif
		if(rx_count != t_len)
		{
			ret = -EIO;
			break;
		}
		i += t_len;
		ptr += t_len;
		offset += t_len;
	}
    cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);

    return ret;
}

int bp2016_qspi_block_lock(struct cyg_flash_dev *dev, const cyg_flashaddr_t block_base)
{
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;
    cyg_flashaddr_t offset = block_base - dev->start;
	int ret = ENOERR;

    cyg_drv_mutex_lock(&qspi_dev->qspi_lock);

	fl_dev->config_4byte_extend(offset, offset&0x3000000);
	if(fl_dev->block_lock)
	{
		ret = fl_dev->get_block_lock_status(offset);
		if(ret < 0)
		{
			ret = -EIO;
		}else{
			if(BLOCK_UNPROTECTED == ret)
			{
				ret = fl_dev->block_lock(offset);
			}else{
				ret = 0;
			}
		}
	}else{
		qspi_printf("block_lock function pointer is NULL!");
	}

    cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);

    return ret;
}

int bp2016_qspi_block_unlock(struct cyg_flash_dev *dev, const cyg_flashaddr_t block_base)
{
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;
    cyg_flashaddr_t offset = block_base - dev->start;
	int ret = ENOERR;

    cyg_drv_mutex_lock(&qspi_dev->qspi_lock);

	fl_dev->config_4byte_extend(offset, offset&0x3000000);
	if(fl_dev->block_unlock)
	{
		ret = fl_dev->get_block_lock_status(offset);
		if(ret < 0)
		{
			ret = -EIO;
		}else{
			if(BLOCK_PROTECTED == ret)
			{
				ret = fl_dev->block_unlock(offset);
			}else{
				ret = 0;
			}
		}
	}else{
		qspi_printf("block_unlock function pointer is NULL!");
	}

    cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);

    return ret;
}

int	bp2016_qspi_region_lock(struct cyg_flash_dev *dev, cyg_uint32 region_num)
{
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;
	int ret = ENOERR;
	int status = 0;

	qspi_debug("enter!");
    cyg_drv_mutex_lock(&qspi_dev->qspi_lock);

#ifdef QSPI_DEBUG
	fl_dev->read_status();
#endif
	status = fl_dev->config_prot_region(region_num, 1);

    cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);

	if(0 != status)
	{
		ret = -ENOENT; 
	}
	qspi_debug("done!");
    return ret;
}

int	bp2016_qspi_region_unlock(struct cyg_flash_dev *dev, cyg_uint32 region_num)
{
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;
	int ret = ENOERR;
	int status = 0;

	qspi_debug("enter!");
    cyg_drv_mutex_lock(&qspi_dev->qspi_lock);

#ifdef QSPI_DEBUG
	fl_dev->read_status();
#endif
	status = fl_dev->config_prot_region(region_num, 0);

    cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);

	if(0 != status)
	{
		ret = -ENOENT; 
	}
	qspi_debug("done!");

    return ret;
}

int	bp2016_qspi_global_lock(struct cyg_flash_dev *dev)
{
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;

	qspi_debug("enter!");

	if(!fl_dev->global_lock)
	{
		qspi_printf("global_lock is NULL!");
		return -EINVAL;
	}

    cyg_drv_mutex_lock(&qspi_dev->qspi_lock);

	fl_dev->read_status();
	fl_dev->global_lock();

    cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);

	qspi_debug("done!");
    return ENOERR;
}

int	bp2016_qspi_global_unlock(struct cyg_flash_dev *dev)
{
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;

	qspi_debug("enter!");

	if(!fl_dev->global_unlock)
	{
		qspi_printf("global_unlock is NULL!");
		return -EINVAL;
	}

    cyg_drv_mutex_lock(&qspi_dev->qspi_lock);

	fl_dev->read_status();
	fl_dev->global_unlock();

    cyg_drv_mutex_unlock(&qspi_dev->qspi_lock);

	qspi_debug("done!");

    return ENOERR;
}

static cyg_qspi_flash_dev bp2016_qspi_dev = {
    qspi_isrvec : SYS_IRQ_ID_QSPI,
    qspi_isrpri : CYGNUM_HAL_INT_PRIO_QSPI,
	fl_dev		: NULL,
};

// wwzz add bp2016_qspi_read_id, bp2016_qspi_chip_erase  20181024
int	bp2016_qspi_read_id(struct cyg_flash_dev *dev)
{
    cyg_uint32 jedid = 0;
    cyg_uint32 id_len = 0;
    cyg_uint8 id_buf[4];
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;

    cyg_mutex_lock(&qspi_dev->qspi_lock);
    id_len = fl_dev->read_rdid(SPINOR_OP_RDID, 3, id_buf);
    cyg_mutex_unlock(&qspi_dev->qspi_lock);
    if(0 == id_len){
        //diag_printf("bp2016_qspi_read_id error!\n");
        return 0;
    }
    jedid = (id_buf[0] << 16) | (id_buf[1] << 8) | (id_buf[2]);

    return jedid;
}

int	bp2016_qspi_chip_erase(struct cyg_flash_dev *dev)
{
	int ret = ENOERR;
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;

    cyg_mutex_lock(&qspi_dev->qspi_lock);
    ret = fl_dev->erase_chip();
    cyg_mutex_unlock(&qspi_dev->qspi_lock);
    return ret;
}

unsigned int bp2016_qspi_get_sector_size(struct cyg_flash_dev *dev)
{
	int ret = ENOERR;
    cyg_qspi_flash_dev *qspi_dev = (cyg_qspi_flash_dev *) dev->priv;
    struct qspi_fl_info *fl_dev = qspi_dev->fl_dev;

    cyg_mutex_lock(&qspi_dev->qspi_lock);
    ret = fl_dev->sector_size;
    cyg_mutex_unlock(&qspi_dev->qspi_lock);
    return ret;
}

// wwzz add bp2016_qspi_read_id, bp2016_qspi_chip_erase  20181024
static const CYG_FLASH_FUNS(bp2016_qspi_dev_funs, &bp2016_qspi_init, &bp2016_qspi_query,
                            &bp2016_qspi_erase_block, &bp2016_qspi_program, &bp2016_qspi_read,
                            &bp2016_qspi_block_lock, &bp2016_qspi_block_unlock,
							&bp2016_qspi_region_lock, &bp2016_qspi_region_unlock,
							&bp2016_qspi_global_lock, &bp2016_qspi_global_unlock,
							&bp2016_qspi_read_id, &bp2016_qspi_chip_erase,
                            &bp2016_qspi_get_sector_size);

CYG_FLASH_DRIVER(bp2016_qspi_entry, &bp2016_qspi_dev_funs, 0, QSPI_MEM_ADDR,
                 0,	//init end in bp2016_qspi_query 
                 0, // number of block_info's, filled in by init
                 bp2016_qspi_dev.block_info, &bp2016_qspi_dev);
