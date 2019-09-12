#include <cyg/infra/diag.h>
#include <cyg/hal/hal_diag.h>

#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/flash/qspi_dev.h>
#include <cyg/hal/regs/qspi.h>
#include "qspi_hw.h"
#include "qspi_wrapper.h"
#include "qspi_dbg.h"
#include "fl_common_cmd.h"
#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
#include <cyg/hal/api/dma_api.h>
#include <string.h>
#endif

static cyg_uint8 *qspi_rq_dbuf;
static int qspi_rq_dlen;
static int irq_rw_dlen;
static int qspi_rd_thr_8;
static int qspi_rd_thr_en = 1;
static int qspi_rd_dfs = 0;

// static int dma_init = 0;

#define QSPI_RX 0
#define QSPI_TX 1

#ifdef QSPI_HW_STAT
static unsigned long long timer_tick_save = 0;

u64 qspi_get_timer_tick_save(void)
{
    return timer_tick_save;
}
#endif

void qspi_dsr_lock(void);
void qspi_dsr_unlock_cond_wait(void);

#define QSPI_CTL_BUSY_TIMEOUT	(1000000)
static bool wait_qspi_hw_idle(void)
{
	cyg_uint32 tm = 0;
    while (qspi_is_busy() == true)
	{
		HAL_DELAY_US(50);
		tm += 50;

		if(tm > QSPI_CTL_BUSY_TIMEOUT)
		{
			break;
		}
	}

	return qspi_is_busy();
}

inline void qspi_wren(cyg_uint8 cmd)
{
	cyg_uint32 intrst=0;
	cyg_uint32 imr=0;

    if(true == wait_qspi_hw_idle())
	{
		qspi_printf("controller is busy! return ...");
		return;
	}
	imr = mask_all_interrupts();
    qspi_tx_init();
    qspi_reg_enable();
    qspi_push_data_8bit(cmd);
    asm volatile("ISB");
    asm volatile("DMB");
    asm volatile("DSB");

    if(true == wait_qspi_hw_idle())
	{
		qspi_printf("controller is busy! return ...");
		return;
	}

	intrst = qspi_get_interrupt_status();	
	qspi_clr_all_interrupts();
	set_interrupts_mask(imr);

	if(0 != intrst)
		qspi_printf("intrrupts status 0x%x", intrst);
}

static void qspi_xfer_init(cyg_uint8 *dbuf, int dlen, int flags)
{
    qspi_rq_dlen = dlen;
    qspi_rq_dbuf = dbuf;
    irq_rw_dlen = 0;
    qspi_rd_thr_8 = 0;

	if (flags == QSPI_RX)
		qspi_rx_full_inten(1);

	if (flags == QSPI_TX)
		qspi_tx_empty_inten(1);
}

int qspi_xfer_tx_process(void)
{
    int tx_e_count = 0, i;
    int rw_len = irq_rw_dlen;
    tx_e_count = 256 - qspi_get_tx_fifo_d_num();
	//disable cpsr I
#ifdef CYGPKG_DEVS_QSPI_DMA_EN_0
	cyg_interrupt_disable();
#endif
    for (i = 0; i < tx_e_count; i++) {
        qspi_push_data_8bit(qspi_rq_dbuf[rw_len++]);
        if (rw_len == qspi_rq_dlen) {
			//qspi_tx_fifo_flush();
			//qspi_cache_flush();
			qspi_tx_empty_inten(0);
			qspi_debug("Had writen %d, qspi tx: tx_e_count = %d, qspi_rq_dlen = %d\n", irq_rw_dlen,
					tx_e_count, qspi_rq_dlen);
            irq_rw_dlen = rw_len;
#ifdef CYGPKG_DEVS_QSPI_DMA_EN_0
			cyg_interrupt_enable();
#endif
            return TRNF_OVER;
        }
    }
#ifdef CYGPKG_DEVS_QSPI_DMA_EN_0
	cyg_interrupt_enable();
#endif
    irq_rw_dlen = rw_len;

	return TRNF_NOVER;
}

static bool qspi_xfer_read_32bit_fifo_complete(int dlen)
{
    int i;
    cyg_uint32 *d32_ = (cyg_uint32 *) qspi_rq_dbuf;
    for (i = 0; i < dlen; i++)
        d32_[irq_rw_dlen++] = qspi_get_data_32bit();
    if (irq_rw_dlen * 4 == qspi_rq_dlen)
        return true;
    return false;
}

static bool qspi_xfer_read_16bit_fifo_complete(int dlen)
{
    int i;
    cyg_uint16 *d16_ = (cyg_uint16 *) qspi_rq_dbuf;
    for (i = 0; i < dlen; i++)
        d16_[irq_rw_dlen++] = qspi_get_data_16bit();
    if (irq_rw_dlen * 2 == qspi_rq_dlen)
        return true;
    return false;
}

static bool qspi_xfer_read_8bit_fifo_complete(int dlen)
{
    int i;
    for (i = 0; i < dlen; i++)
        qspi_rq_dbuf[irq_rw_dlen++] = qspi_get_data_8bit();
    if (irq_rw_dlen == qspi_rq_dlen)
        return true;
    return false;
}

static bool qspi_xfer_read_complete(int dlen)
{
    if (qspi_rd_dfs == 8)
        return qspi_xfer_read_8bit_fifo_complete(dlen);
    if (qspi_rd_dfs == 16)
        return qspi_xfer_read_16bit_fifo_complete(dlen);
    if (qspi_rd_dfs == 32)
        return qspi_xfer_read_32bit_fifo_complete(dlen);
    return false;
}

int qspi_xfer_rx_process(void)
{
    int rx_count = 0;
    rx_count = qspi_get_rx_fifo_d_num();
#ifdef QSPI_HW_STAT
    timer_tick_save = HAL_GET_COUNTER;
#endif
    if (qspi_rd_thr_8 > 0)
        rx_count = qspi_rd_thr_8;
	qspi_debug("qspi rx: rx_f_count = %d, qspi_rq_dlen = %d\n", rx_count, qspi_rq_dlen);

    if (qspi_xfer_read_complete(rx_count) == true) {
		qspi_rx_full_inten(0);
		qspi_debug("qspi rx: rx_f_count = %d, qspi_rq_dlen = %d\n", rx_count, qspi_rq_dlen);
        return TRNF_OVER;
    }

	return TRNF_NOVER;
}

static void qspi_xfer_wait_done(void)
{
    if(true == wait_qspi_hw_idle())
	{
		qspi_printf("controller is busy! return ...");
		return;
	}
}

void qspi_ahb_read_enable(cyg_uint8 rd_cmd, cyg_uint8 dfs)
{
    if(true == wait_qspi_hw_idle())
	{
		qspi_printf("controller is busy! return ...");
		return;
	}
    qspi_rx_init(rd_cmd, dfs);
    qspi_reg_enable();
    qspi_rx_fifo_flush();
    qspi_ahb_enable();
}

void qspi_ahb_write_enable(cyg_uint8 wren_cmd)
{
    qspi_wren(wren_cmd);
    qspi_ahb_enable();
}

int qspi_send_xx_data(cyg_uint8 *tx_dbuf, int tx_len)
{
	qspi_debug("qspi_send_xx_data, tx_len = %d\n", tx_len);
    if(true == wait_qspi_hw_idle())
	{
		qspi_printf("controller is busy! return ...");
		return 0;
	}
	qspi_dsr_lock();
    qspi_tx_init();
    qspi_set_tx_fifo_level(248);
    qspi_xfer_init(tx_dbuf, tx_len, QSPI_TX);
    qspi_reg_enable();

	qspi_dsr_unlock_cond_wait();
    qspi_xfer_wait_done();

    return tx_len;
}

int qspi_get_rx_ndf(cyg_uint8 cmd, int rx_len, int dfs)
{
    if (dfs == 8)
        return rx_len; /*Byte Number*/

    switch (cmd) {
    case SPINOR_OP_READ_1_1_2:
    case SPINOR_OP_READ_1_1_4:
    case SPINOR_OP_READ_DUAL_IO:
    case SPINOR_OP_READ_QUAD_IO:
    case SPINOR_OP_READ_FAST:
        return rx_len; /*Byte Number*/

    case SPINOR_OP_READ:
        if (dfs == 16)
            return rx_len / 2;
        if (dfs == 32)
            return rx_len / 4;
        break;
    default:
        break;
    }
    return 0;
}

int qspi_get_rx_thr(int rx_len, int dfs)
{
    if (dfs == 8)
        return rx_len;
    if (dfs == 16)
        return rx_len / 2;
    if (dfs == 32)
        return rx_len / 4;

    return 0;
}

/*
 *  only for read page operations, using dma mode
 */

int qspi_dma_read_start(cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *rx_dbuf, int rx_len,
                        int dma_rdlr)
{
    int rx_ndf = 0, rx_thr;

    if(rx_len%4)
    {
        rx_ndf = qspi_get_rx_ndf(cmd, rx_len, 8);
        rx_thr = qspi_get_rx_thr(rx_len, 8);
    }else{
        rx_ndf = qspi_get_rx_ndf(cmd, rx_len, 32);
        rx_thr = qspi_get_rx_thr(rx_len, 32);
    }

	qspi_debug("cmd = %02x, addr = 0x%x, rx_len = %d, ndf = %d, thr = %d\n", cmd, addr, rx_len,
			rx_ndf, rx_thr);

    if ((rx_len > 256) || (rx_ndf == 0))
        return 0;

    if(true == wait_qspi_hw_idle())
	{
		qspi_printf("controller is busy! return ...");
		return 0;
	}
    qspi_set_dma_ctrl(0);
    
    if(rx_len%4)
    {
        qspi_rx_init(cmd, 8);
    }else{
        qspi_rx_init(cmd, 32);
    }

    qspi_set_rx_fifo_level(rx_thr - 1);
    qspi_set_rx_ndf(rx_ndf - 1);

    if(rx_len%4)
    {
        qspi_set_dma_rdlr(0);
    }else{
        qspi_set_dma_rdlr(dma_rdlr);
    }

    qspi_set_dma_ctrl(1);
    asm volatile("isb" : : : "memory");
    qspi_reg_enable();

    if (rx_len%4) {
        cyg_interrupt_disable();
        qspi_push_data_8bit(cmd);
        qspi_push_data_8bit((addr >> 16) & 0xff);
        qspi_push_data_8bit((addr >> 8) & 0xff);
        qspi_push_data_8bit(addr & 0xff);
        cyg_interrupt_enable();
    }else{
        qspi_push_data_32bit((cmd << 24) | addr);
    }

    return 1;
}

extern void v7_dma_inv_range(cyg_uint32, cyg_uint32);
#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
//#include "cp15.h"
//#include "dma_if.h"
extern inline cyg_uint32 get_qspiflash_dr(void);
extern int dma_qspiflash_read_polling(cyg_uint32, cyg_uint32 channel_id, cyg_uint32 flash_addr,
                                      cyg_uint32 dst_addr, cyg_uint32 length, bool syn_flag);
extern void dma_wait_channel_done_polling(cyg_uint32, cyg_uint32 channel_id, bool syn_flag);
extern inline void dmac_enable(cyg_uint32);
extern bool is_dma_init(cyg_uint32);

int qspi_dma_read_page(void *dma_rx_ch, cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *rx_dbuf, int rx_len)
{
    cyg_uint32 width = DMAC_CTL_TR_WIDTH_E_32;
    cyg_uint32 burst = DMAC_CTL_MSIZE_E_16;
    int res;

    qspi_debug("ENTER!");
    v7_dma_inv_range((cyg_uint32) rx_dbuf, (cyg_uint32)(rx_dbuf + rx_len));

    if(rx_len%4)
    {
        width = DMAC_CTL_TR_WIDTH_E_8;
        burst = DMAC_CTL_MSIZE_E_1;
    }

    res = DMA_p2m_set_config(dma_rx_ch, get_qspiflash_dr(), (u32)rx_dbuf, rx_len, width, burst,
            false);
    if (true != res) {
        qspi_printf("set config error!");
        res = QSPI_OP_FAILED;
        goto free_channel;
    }

    if(!qspi_dma_read_start(cmd, addr, rx_dbuf, rx_len, 15))
	{
        goto free_channel;
	}

    DMA_trans(dma_rx_ch);
    res = DMA_wait_ch_done(dma_rx_ch);

    if(res)
    {
        qspi_printf("dma read failed! ret = %d", res);
        return res;
    }
    
    qspi_hw_init();

    return rx_len;

free_channel:
    qspi_printf("free_channel!");
    DMA_free(dma_rx_ch);
    return 0;
}
#endif

/*
 *  only for read page operations
 */
int qspi_read_page_data(cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *rx_dbuf, int rx_len, int dfs)
{
    int rx_thr, rx_ndf = 0;
    cyg_uint32 d = 0;

    qspi_rd_dfs = dfs;
    rx_thr = qspi_get_rx_thr(rx_len, dfs);
    rx_ndf = qspi_get_rx_ndf(cmd, rx_len, dfs);

	qspi_debug("cmd = %02x, addr = 0x%x, rx_len = %d, ndf = %d, thr = %d\n", cmd, addr, rx_len,
			rx_ndf, rx_thr);

    if (rx_len > 256)
        return 0;
    if ((rx_ndf == 0) || (rx_thr == 0))
        return 0;

    if(true == wait_qspi_hw_idle())
	{
		qspi_printf("controller is busy! return ...");
		return 0;
	}
	qspi_dsr_lock();
    qspi_rx_init(cmd, dfs);
    qspi_set_rx_ndf(rx_ndf - 1);

    qspi_set_rx_fifo_level(rx_thr - 1);
    qspi_xfer_init(rx_dbuf, rx_len, QSPI_RX);

    if (qspi_rd_thr_en > 0) {
        if (rx_len == 256) {
            qspi_set_rx_fifo_level(rx_thr / 8 - 1);
            qspi_rd_thr_8 = rx_thr / 8;
        }
    }

    qspi_reg_enable();

    qspi_rx_fifo_flush();
    asm volatile("ISB");
    asm volatile("DMB");
    asm volatile("DSB");

	cyg_interrupt_disable();
    if (dfs == 8) {
        qspi_push_data_8bit(cmd);
        qspi_push_data_8bit((addr >> 16) & 0xff);
        qspi_push_data_8bit((addr >> 8) & 0xff);
        qspi_push_data_8bit(addr & 0xff);
    } else if (dfs == 16) {
        d = cmd;
        d = d << 8;
        d |= (addr >> 16) & 0xff;
        qspi_push_data_16bit(d);
        qspi_push_data_16bit(addr & 0xffff);
    } else if (dfs == 32) {
        d = cmd;
        d = d << 24;
        d |= addr;
        qspi_push_data_32bit(d);
    }
	cyg_interrupt_enable();

	qspi_dsr_unlock_cond_wait();
    qspi_xfer_wait_done();
    return rx_len;
}

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
//#include "dma_if.h"
__attribute__((aligned(64))) char dma_qspiflash_write_buf[320] = {0};
extern int dma_qspiflash_write_page_polling(cyg_uint32, cyg_uint32 channel_id,
                                            cyg_uint32 flash_addr, cyg_uint32 src_addr,
                                            cyg_uint32 len, bool syn_flag);
extern inline void qspi_tx_init_4(void);
static int qspi_dma_write_start(
    /*cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 * rx_dbuf, int tx_len, */ int dma_tdlr)
{
    if(true == wait_qspi_hw_idle())
	{
		qspi_printf("controller is busy! return ...");
		return 0;
	}
    qspi_set_dma_ctrl(0);

    if(dma_tdlr == 8){
        qspi_tx_init();
    }else{
        qspi_tx_init_4();
    }

    qspi_set_tx_fifo_level(16);
    qspi_set_dma_tdlr(dma_tdlr-1);
    qspi_set_dma_ctrl(2);
    asm volatile("isb" : : : "memory");
    qspi_reg_enable();

    return 0;
}

int qspi_dma_write_page(void *dma_tx_ch, cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *tx_dbuf, int tx_len, int dfs)
{
    cyg_uint32 width;
    cyg_uint32 burst;
    int res;

    mask_all_interrupts();

    // prepare data
    if(8 == dfs){
        dma_qspiflash_write_buf[0] = cmd;
        dma_qspiflash_write_buf[1] = (addr >> 16) & 0xFF;
        dma_qspiflash_write_buf[2] = (addr >> 8) & 0xFF;
        dma_qspiflash_write_buf[3] = addr & 0xFF;
        width = DMAC_CTL_TR_WIDTH_E_8;
        burst = DMAC_CTL_MSIZE_E_16;
    }else if(32 == dfs){
        dma_qspiflash_write_buf[3] = cmd;
        dma_qspiflash_write_buf[2] = (addr >> 16) & 0xFF;
        dma_qspiflash_write_buf[1] = (addr >> 8) & 0xFF;
        dma_qspiflash_write_buf[0] = addr & 0xFF;
        width = DMAC_CTL_TR_WIDTH_E_32;
        burst = DMAC_CTL_MSIZE_E_4;
    }else{
        qspi_printf("DFS = %d is not support, return!\n", dfs);
        return -1;
    }

    if((0!=tx_len) && (NULL != tx_dbuf))
        memcpy((dma_qspiflash_write_buf + 4), (char *) tx_dbuf, tx_len);

    v7_dma_clean_range((cyg_uint32) dma_qspiflash_write_buf,
            (cyg_uint32) dma_qspiflash_write_buf + tx_len + 4);

    res = DMA_m2p_set_config(dma_tx_ch, (u32)dma_qspiflash_write_buf, get_qspiflash_dr(), tx_len + 4, width, burst,
            true);
    if (true != res) {
        qspi_printf("set config error!");
        res = QSPI_OP_FAILED;
        goto free_channel;
    }

    asm volatile("isb" : : : "memory");
    asm volatile("dsb" : : : "memory");
    qspi_dma_write_start(/*cmd, addr, tx_dbuf, tx_len, */ dfs);

    DMA_trans(dma_tx_ch);

    // close qspi-dma, no tx_req
    qspi_set_dma_ctrl(0);
    qspi_clr_all_interrupts();
    return tx_len;

free_channel:
    DMA_free(dma_tx_ch);
    return 0;
}

int qspi_dma_send_cmd_data(cyg_uint8 cmd, cyg_uint8 data)
{
    cyg_uint32 width;
    cyg_uint32 burst;
    int res;

    if(!qspi_dma_tx_chan)
    {
        qspi_printf("ERROR: dma tx channel is NULL!");
        return QSPI_OP_FAILED;
    }
    mask_all_interrupts();

    // prepare data
    dma_qspiflash_write_buf[0] = cmd;
    dma_qspiflash_write_buf[1] = (data & 0xFF);
    width = DMAC_CTL_TR_WIDTH_E_8;
    burst = DMAC_CTL_MSIZE_E_16;

    v7_dma_clean_range((cyg_uint32) dma_qspiflash_write_buf,
            (cyg_uint32) dma_qspiflash_write_buf + 2);

    res = DMA_m2p_set_config(qspi_dma_tx_chan, (u32)dma_qspiflash_write_buf, get_qspiflash_dr(), 2, width, burst,
            true);
    if (true != res) {
        qspi_printf("set config error!");
        res = QSPI_OP_FAILED;
        goto free_channel;
    }

    asm volatile("isb" : : : "memory");
    asm volatile("dsb" : : : "memory");
    qspi_dma_write_start(/*cmd, addr, tx_dbuf, tx_len, */ 8);

    DMA_trans(qspi_dma_tx_chan);

    // close qspi-dma, no tx_req
    qspi_set_dma_ctrl(0);
    qspi_clr_all_interrupts();
    return 2;

free_channel:
    DMA_free(qspi_dma_tx_chan);
    return 0;
}
#endif

/*
 *  only for read status/device id/readid etc. operations
 */
int qspi_read_xx_data(cyg_uint8 cmd, cyg_uint32 addr, int addr_vld, cyg_uint8 *rx_dbuf, int rx_len)
{
	qspi_debug("cmd = %02x, rx_len = %d\n", cmd, rx_len);

    qspi_rd_dfs = 8;
    if(true == wait_qspi_hw_idle())
	{
		qspi_printf("controller is busy! return ...");
		return 0;
	}
	qspi_dsr_lock();
    qspi_rx_init(SPINOR_OP_READ, 8);
    qspi_set_rx_ndf(rx_len - 1);
    qspi_set_rx_fifo_level(rx_len - 1);

    qspi_xfer_init(rx_dbuf, rx_len, QSPI_RX);
    qspi_reg_enable();

    qspi_rx_fifo_flush();

    if (addr_vld > 0) { // addr is valid
		cyg_interrupt_disable();
#if 0
        qspi_push_data_8bit(cmd);
        qspi_push_data_8bit((addr >> 16) & 0xff);
        qspi_push_data_8bit((addr >> 8) & 0xff);
        qspi_push_data_8bit(addr & 0xff);
#else
        unsigned int d;
        d = cmd;
        d = d << 24;
        d |= addr;
        qspi_push_data_32bit(d);
#endif
		cyg_interrupt_enable();
    } else
        qspi_push_data_8bit(cmd);

	qspi_dsr_unlock_cond_wait();
    qspi_xfer_wait_done();
    return rx_len;
}


/*
 *  only for spansion asp relative registers read/write operations
 */
int qspi_read_spansion_data(cyg_uint8 cmd, cyg_uint32 addr, int addr_vld, cyg_uint8 *rx_dbuf, int rx_len)
{
	qspi_debug("cmd = %02x, rx_len = %d\n", cmd, rx_len);

    qspi_rd_dfs = 8;
    if(true == wait_qspi_hw_idle())
	{
		qspi_printf("controller is busy! return ...");
		return 0;
	}
    qspi_rx_init(SPINOR_OP_READ, 8);
	qspi_dsr_lock();
    qspi_set_rx_ndf(rx_len - 1);
    qspi_set_rx_fifo_level(rx_len - 1);

    qspi_xfer_init(rx_dbuf, rx_len, QSPI_RX);
    qspi_reg_enable();

    qspi_rx_fifo_flush();

    if (addr_vld > 0) { // addr is valid
        qspi_push_data_8bit(cmd);
        qspi_push_data_8bit((addr >> 24) & 0xff);
        qspi_push_data_8bit((addr >> 16) & 0xff);
        qspi_push_data_8bit((addr >> 8) & 0xff);
        qspi_push_data_8bit(addr & 0xff);
    } else
        qspi_push_data_8bit(cmd);

	qspi_dsr_unlock_cond_wait();
    qspi_xfer_wait_done();
    return rx_len;
}
