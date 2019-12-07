#include <cyg/infra/diag.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/flash/qspi_dev.h>
#include <cyg/hal/regs/qspi.h>
#include <cyg/hal/plf/chip_config.h>
#include <errno.h>
#include <string.h>
#include "qspi_dbg.h"
#include "fl_common_cmd.h"

#define     RDSR1_BP_BITS_MASK              0x3c
#define     RDSR1_BP_BITS_SHIFT(bp)         (bp << 2)
#define     RDSR3_WPS                       0x04

#define     SR2_QUAD_EN_WINBOND             0x02

#define     SR_WIP_WINBOND                  0x0	    /*Winbond*/
#define	    SR_BP_BITS_MASK_WINBOND         0x3C
#define	    SR_BP_BITS_SHIFT_WINBOND(v)	    (v << 2)
#define     SR_TBPROT_EN_WINBOND	        0x40	/* Winbond TBProt I/O: 1=low address */

extern inline int qspi_wren(cyg_uint8 cmd);
extern void qspi_ahb_read_enable(cyg_uint8 rd_cmd, cyg_uint8 dfs);
extern void qspi_ahb_write_enable(cyg_uint8 wren_cmd);
extern int qspi_send_xx_data(cyg_uint8 *tx_dbuf, int tx_len);
extern int qspi_read_xx_data(cyg_uint8 cmd, cyg_uint32 addr, int addr_vld, cyg_uint8 *rx_dbuf,
                             int rx_len);
cyg_uint8 qspi_fl_read_status(cyg_uint8 cmd);
cyg_uint8 qspi_w25q256fv_read_status(void);
extern int qspi_dma_send_cmd_data(cyg_uint8 cmd, cyg_uint8 data);

extern int qspi_fl_wait_wel(void);
static cyg_uint32 wait_cmd = 0;
static cyg_uint8 wbuf_s[280];

static int qspi_w25q256fv_write_extend_addr(cyg_uint8 cmd, cyg_uint8 data)
{
    int ret=QSPI_OP_SUCCESS;

    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
        return ret;
    ret = qspi_fl_wait_wel();
    if(ret < 0)
        return ret;

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
    ret = qspi_dma_send_cmd_data(cmd, data);
#else
    cyg_uint8 wbuf[2];
    wbuf[0] = cmd;
    wbuf[1] = data;
    ret = qspi_send_xx_data(wbuf, 2);
#endif
	if(ret<0)
	{
		return QSPI_OP_FAILED;
	}else{
        return QSPI_OP_SUCCESS;
    }
}

/*
 * only for program/erase/write status/erase[program] security register
 */
static cyg_uint8 wait_flash_idle_extend(cyg_uint32 wip, cyg_uint32 wip_mask)
{
    cyg_uint8 st = 0;
	cyg_uint32 timeout = 0;
	cyg_uint32 tim_dec = 50;

	switch(wait_cmd)
	{
	case SPINOR_OP_BE_64K:
		timeout = tm_array[1].ers_blk64k_tm;
		tim_dec = 1000;
		break;
	case SPINOR_OP_BE_32K:
		timeout = tm_array[1].ers_blk32k_tm;
		tim_dec = 1000;
		break;
	case SPINOR_OP_BE_SECTOR:
		timeout = tm_array[1].ers_sec_tm;
		break;
	case SPINOR_OP_CHIP_ERASE:
		timeout = tm_array[1].ers_chip_tm;
		tim_dec = 1000;
		break;
	case SPINOR_OP_BP:
		timeout = tm_array[1].prg_page_tm;
		break;
	case SPINOR_OP_WRSR1:
	case SPINOR_OP_WRSR2:
	case SPINOR_OP_WRSR3:
		timeout = tm_array[1].wr_status_tm;
		break;
	default:
		timeout = tm_array[1].wr_status_tm;
		break;
	}

    while (timeout) {
        st = qspi_fl_read_status(SPINOR_OP_RDSR1);
        if ((st & wip_mask) == wip)
            break;
		HAL_DELAY_US(tim_dec);
		timeout -= tim_dec;
    }
    return st;
}

static int qspi_w25q256fv_write_status(cyg_uint8 cmd, cyg_uint8 sr_x)
{
	int ret=0;
    cyg_uint8 status;
    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
        return ret;
    ret = qspi_fl_wait_wel();
    if(ret < 0)
        return ret;

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
    ret = qspi_dma_send_cmd_data(cmd, sr_x);
#else
    cyg_uint8 wbuf[3];
    wbuf[0] = cmd;
    wbuf[1] = sr_x;
    ret = qspi_send_xx_data(wbuf, 2);
#endif
	if(ret<0)
	{
		return QSPI_OP_FAILED;
	}

	wait_cmd = cmd;
    status = wait_flash_idle_extend(0, 1);
    if (status & SR_WIP) {
        qspi_printf("write status failed, status 0x:%x\n", status);
        ret = qspi_wren(SPINOR_OP_WRDI);
        if(ret<0)
        {
            return QSPI_OP_FAILED;
        }
        qspi_printf("wren disable, status	  :0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR1));
    }
    return (int)status;
}

static int qspi_w25q256fv_4byte_extend_enable(cyg_uint32 addr)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 ext_data = 0, ext_rd;
    int ret=QSPI_OP_SUCCESS;

    ext_rd = qspi_fl_read_status(SPINOR_OP_RD_EXTADDR);
    if ((addr & (1 << 24)) > 0)
        ext_data |= SPINOR_OP_EXTR_A24;

    if (ext_rd != ext_data) {
#ifdef QSPI_DEBUG
        s = HAL_GET_COUNTER;
#endif
        ret = qspi_w25q256fv_write_extend_addr(SPINOR_OP_WR_EXTADDR, ext_data);

#ifdef QSPI_DEBUG
        e = HAL_GET_COUNTER;
        us = archtimer_tick_to_us((unsigned long) (e - s));
#endif

        qspi_debug("extend addr register      : 0x%x, wr: 0x%x\n",
               qspi_fl_read_status(SPINOR_OP_RD_EXTADDR), ext_data);
        qspi_debug("address_extend_enable need: %d us (%d ms)\n\n", us, us / 1000);

    }

    return ret;
}

static int qspi_w25q256fv_4byte_extend_disable(cyg_uint32 addr)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 ext_d = 0;
    int ret=QSPI_OP_SUCCESS;

    ext_d = qspi_fl_read_status(SPINOR_OP_RD_EXTADDR);

    if (ext_d != 0) {
#ifdef QSPI_DEBUG
        s = HAL_GET_COUNTER;
#endif
        ret = qspi_w25q256fv_write_extend_addr(SPINOR_OP_WR_EXTADDR, 0);

#ifdef QSPI_DEBUG
        e = HAL_GET_COUNTER;
        us = archtimer_tick_to_us((unsigned long) (e - s));
#endif

        qspi_debug("extend addr register       : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RD_EXTADDR));
        qspi_debug("address_extend_disable need: %d us (%d ms)\n\n", us, us / 1000);
    }

    return ret;
}

static int qspi_w25q256fv_quad_enable(void)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 sr2;
    int st, ret = QSPI_OP_SUCCESS;

    qspi_debug("quad enable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif

    sr2 = qspi_fl_read_status(SPINOR_OP_RDSR2);
    if ((sr2 & SR2_QUAD_EN_WINBOND) == SR2_QUAD_EN_WINBOND) {
        qspi_debug("quad bit had enabled\n");
        return ret;
    }

    st = qspi_w25q256fv_write_status(SPINOR_OP_WRSR2, sr2 | SR2_QUAD_EN_WINBOND);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif
    
    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }

    if(st < 0)
        ret = QSPI_OP_FAILED;

    qspi_debug("status 2 register     : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR2));
    qspi_debug("quad_enable need    : %d us (%d ms)\n\n", us, us / 1000);

    return ret;
}

int qspi_w25q256fv_quad_disable(void)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 sr2;
    int st , ret = QSPI_OP_SUCCESS;

    qspi_debug("quad enable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif

    sr2 = qspi_fl_read_status(SPINOR_OP_RDSR2);
    if ((sr2 & SR2_QUAD_EN_WINBOND) == 0) {
        qspi_debug("quad bit had disabled\n");
        return ret;
    }

    sr2 &= ~(SR2_QUAD_EN_WINBOND);
    st = qspi_w25q256fv_write_status(SPINOR_OP_WRSR2, sr2);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif

    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }

    if(st < 0)
        ret = QSPI_OP_FAILED;

    qspi_debug("status 2 register     : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR2));
    qspi_debug("quad_disable need   :%d us (%d ms)\n\n", us, us / 1000);

    return ret;
}

static int qspi_w25q256fv_prot_region_enable(int region)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 rdsr1 = 0, rdsr3 = 0;
    int st, ret = QSPI_OP_SUCCESS;

    if (region > 0xf)
        return QSPI_OP_FAILED;

    qspi_debug("\nprot enable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif
    rdsr1 = qspi_fl_read_status(SPINOR_OP_RDSR1);
    rdsr3 = qspi_fl_read_status(SPINOR_OP_RDSR3);
    if ((rdsr3 & RDSR3_WPS) > 0) {
        qspi_printf("failed! need utilize individual block lock;(wps =1)\n");
        return QSPI_OP_FAILED;
    }

    if ((rdsr1 & RDSR1_BP_BITS_MASK) == RDSR1_BP_BITS_SHIFT(region)) {
        qspi_debug("region had enabled, rdsr1 = 0x%x, rdsr3 = 0x%x\n", rdsr1, rdsr3);
        return QSPI_OP_SUCCESS;
    }

    rdsr1 |= RDSR1_BP_BITS_SHIFT(region);
    st = qspi_w25q256fv_write_status(SPINOR_OP_WRSR1, rdsr1);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif

    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }

    if(st < 0)
        ret = QSPI_OP_FAILED;

    qspi_w25q256fv_read_status();
    qspi_debug("prot_enable need    : %d us (%d ms), w:0x%x\n", us, us / 1000, rdsr1);

    return ret;
}

static int qspi_w25q256fv_prot_region_disable(int region)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 rdsr1 = 0, rdsr3 = 0;
    int st, ret = QSPI_OP_SUCCESS;

    qspi_debug("\nprot disable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif
    rdsr1 = qspi_fl_read_status(SPINOR_OP_RDSR1);
    rdsr3 = qspi_fl_read_status(SPINOR_OP_RDSR3);

    if ((rdsr3 & RDSR3_WPS) > 0) {
        qspi_printf("failed! need utilize individual block lock;(wps =1)\n");
        return QSPI_OP_FAILED;
    }

    if ((rdsr1 & RDSR1_BP_BITS_MASK) == 0) {
        qspi_debug("region had disabled, rdsr1 = 0x%x, rdsr3 = 0x%x\n", rdsr1, rdsr3);
        return QSPI_OP_SUCCESS;
    }

    rdsr1 &= ~RDSR1_BP_BITS_MASK;
    st = qspi_w25q256fv_write_status(SPINOR_OP_WRSR1, rdsr1);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif
    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }
    if(st < 0)
        ret = QSPI_OP_FAILED;

    qspi_w25q256fv_read_status();
    qspi_debug("prot_disable need    : %d us (%d ms), w:0x%x\n", us, us / 1000, rdsr1);

    return ret;
}

int qspi_w25q256fv_write_page(cyg_uint32 addr, cyg_uint8 *dbuf, int len)
{
	int ret=0;
    cyg_uint8 status;

    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
        return ret;
    ret = qspi_fl_wait_wel();
    if(ret < 0)
        return ret;

    wbuf_s[0] = SPINOR_OP_BP;
    wbuf_s[1] = (addr >> 16) & 0xff;
    wbuf_s[2] = (addr >> 8) & 0xff;
    wbuf_s[3] = addr & 0xff;

    if (len <= 256)
        memcpy(wbuf_s + 4, dbuf, len);
    ret = qspi_send_xx_data(wbuf_s, len + 4);
	if(ret<0)
	{
		return QSPI_OP_FAILED;
	}

	wait_cmd = SPINOR_OP_BP;
    status = wait_flash_idle_extend(0, 3); //wip wel

    if (status & SR_WIP) {
        qspi_printf("program addr 0x%x failed, status 0x:%x\n", addr, status);
        qspi_wren(SPINOR_OP_WRDI);
        qspi_printf("wren disable, status	  :0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR1));
        return QSPI_OP_FAILED;
    }
    return QSPI_OP_SUCCESS;
}

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
extern int qspi_dma_write_page(void *dma_tx_ch, cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *tx_dbuf, int tx_len, int dfs);
extern inline void qspi_hw_init(void);
int qspi_w25q256fv_dma_write_page(void *dma_tx_ch, cyg_uint32 addr, cyg_uint8 *dbuf, int len)
{
    cyg_uint8 status;
    int ret=0;

    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
        return ret;
    ret = qspi_fl_wait_wel();
    if(ret < 0)
        return ret;

    ret = qspi_dma_write_page(dma_tx_ch, SPINOR_OP_BP, addr, dbuf, len, 8);

    if(ret < 0)
        return QSPI_OP_FAILED;

	wait_cmd = SPINOR_OP_BP;
    status = wait_flash_idle_extend(0, 1);

    if (status & SR_WIP) {
        qspi_printf("%s:program addr 0x%x failed, status 0x:%x\n", __FUNCTION__, addr, status);
        qspi_wren(SPINOR_OP_WRDI);
        qspi_printf("%s:wren disable, status	  :0x%x\n", __FUNCTION__,
               qspi_fl_read_status(SPINOR_OP_RDSR1));
        qspi_hw_init();
        return QSPI_OP_FAILED;
    }
    qspi_hw_init();
    return QSPI_OP_SUCCESS;
}
#endif

int qspi_w25q256fv_erase(cyg_uint8 cmd, cyg_uint32 addr)
{
	int ret=0;
    cyg_uint8 status;

    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
        return ret;
    ret = qspi_fl_wait_wel();
    if(ret<0)
        return ret;

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
    ret = qspi_dma_write_page(qspi_dma_tx_chan, cmd, addr, NULL, 0, 8);
#else
    cyg_uint8 wbuf[4];
    //wbuf[0] = SPINOR_OP_BE_64K;
    wbuf[0] = cmd;
    wbuf[1] = (addr >> 16) & 0xff;
    wbuf[2] = (addr >> 8) & 0xff;
    wbuf[3] = addr & 0xff;
    ret = qspi_send_xx_data(wbuf, 4);
#endif
	if(ret<0)
	{
		return QSPI_OP_FAILED;
	}

	wait_cmd = cmd;
    status = wait_flash_idle_extend(0, 1);

    if (status & SR_WIP) {
        qspi_printf("erase addr 0x%x failed, status :0x%x\n", addr, status);
        qspi_wren(SPINOR_OP_WRDI);
        qspi_printf("wren disable, status 	       :0x%x\n",
               qspi_fl_read_status(SPINOR_OP_RDSR1)); // st = 0x1c
        return QSPI_OP_FAILED;
    }
    return QSPI_OP_SUCCESS;
}

int qspi_w25q256fv_erase_all(void)
{
	int ret=0;
    cyg_uint8 status;
    cyg_uint8 wbuf[4];

    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
        return ret;
    ret = qspi_fl_wait_wel();
    if(ret < 0)
        return ret;

    wbuf[0] = SPINOR_OP_CHIP_ERASE;
    ret = qspi_send_xx_data(wbuf, 1);
	if(ret<0)
	{
		return QSPI_OP_FAILED;
	}

	wait_cmd = SPINOR_OP_CHIP_ERASE;
    status = wait_flash_idle_extend(0, 1);

    if (status & SR_WIP) {
        qspi_printf("chip erase failed, status :0x%x\n", status);
        qspi_wren(SPINOR_OP_WRDI);
        qspi_printf("wren disable, status	  :0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR1));
        return QSPI_OP_FAILED;
    }
    return QSPI_OP_SUCCESS;
}

int qspi_w25q256fv_4byte_extend(cyg_uint32 addr, int en)
{
    if (en > 0)
        return qspi_w25q256fv_4byte_extend_enable(addr);
    else
        return qspi_w25q256fv_4byte_extend_disable(addr);
}

int qspi_w25q256fv_quad(int en)
{
    if (en > 0)
        return qspi_w25q256fv_quad_enable();
    else
        return qspi_w25q256fv_quad_disable();
}

int qspi_w25q256fv_wps_op(int en)
{
	u8 rdsr3, st;
    rdsr3 = qspi_fl_read_status(SPINOR_OP_RDSR3);

	if(0 == en)
	{
		if ((rdsr3 & RDSR3_WPS) > 0) {
			qspi_printf("(wps =1), need disable it!");
			rdsr3 &= ~RDSR3_WPS;
		}else{
			return QSPI_OP_SUCCESS;
		}
	}else{
		if ((rdsr3 & RDSR3_WPS) ==  0) {
			qspi_printf("(wps =0), need enable it!");
			rdsr3 |= RDSR3_WPS;
		}else{
		return QSPI_OP_SUCCESS;
		}
	}

	st = qspi_w25q256fv_write_status(SPINOR_OP_WRSR3, rdsr3);
	if ((st & SR_WIP) == SR_WIP) {
		qspi_printf("write WPS failed!");
		return QSPI_OP_FAILED;
	}

    if(st < 0)
		return QSPI_OP_FAILED;

	return QSPI_OP_SUCCESS;
}

int qspi_w25q256fv_prot_region(int region, int en)
{
	if(qspi_w25q256fv_wps_op(0) < 0)
        return QSPI_OP_FAILED;

    if (en > 0)
        return qspi_w25q256fv_prot_region_enable(region);
    else
        return qspi_w25q256fv_prot_region_disable(region);
}

cyg_uint8 qspi_w25q256fv_read_status(void)
{
    qspi_printf("status-1		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR1));
    qspi_printf("status-2		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR2));
    qspi_printf("status-3		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR3));
    qspi_printf("extend addr		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RD_EXTADDR));
    return 0;
}

int qspi_w25q256fv_reset(void)
{
    int ret=QSPI_OP_SUCCESS;
    cyg_uint8 cmd = SPINOR_OP_ENABLE_RESET;
    ret = qspi_send_xx_data(&cmd, 1);
    if(ret<0)
        return ret;

    cmd = SPINOR_OP_RESET;
    ret = qspi_send_xx_data(&cmd, 1);
    HAL_DELAY_US(10000); // wait for device Reset sequence complete

    if(ret<0)
        return ret;

    return QSPI_OP_SUCCESS;
}

#define BLOCK_PROTECTED 	(1)
#define BLOCK_UNPROTECTED	(0)

//only one block status
int qspi_w25q256fv_read_block_lock_status(cyg_uint32 offs)
{
	cyg_uint8 rdsr3 = 0;
	cyg_uint8 lock_status=0;
	int ret=0;

    rdsr3 = qspi_fl_read_status(SPINOR_OP_RDSR3);

    if ((rdsr3 & RDSR3_WPS) == 0) {
        qspi_printf("(wps = 0), not in block protection mode!\n");
		return -EINVAL;
	}

	ret = qspi_read_xx_data(WPS_BLOCK_READ_LOCK, offs, 1, &lock_status, 1);
	if(ret != 1)
	{
		qspi_printf("read block lock status error!");
		return -EINVAL;
	}

	if(lock_status&1)
	{
		return BLOCK_PROTECTED;
	}else{
		return BLOCK_UNPROTECTED;
	}
}

static int inline qspi_w25q256fv_send_lock_cmd(cyg_uint8 cmd, cyg_uint32 addr, cyg_uint32 vld_addr)
{
	cyg_uint8 wbuf[6];
    int ret=QSPI_OP_SUCCESS;

	ret = qspi_wren(SPINOR_OP_WREN);
    if(ret<0)
        return ret;
    ret = qspi_fl_wait_wel();
    if(ret<0)
        return ret;

	wbuf[0] = cmd;

	if(vld_addr)
	{
		wbuf[1] = (addr >> 16) & 0xff;
		wbuf[2] = (addr >> 8) & 0xff;
		wbuf[3] = addr & 0xff;
		ret = qspi_send_xx_data(wbuf, 4);
	}else{
		ret = qspi_send_xx_data(wbuf, 1);
	}

    if(ret<0)
        return QSPI_OP_FAILED;

    return QSPI_OP_SUCCESS;
}

int qspi_w25q256fv_block_lock(cyg_uint32 offs)
{
	int ret=0;

	ret = qspi_w25q256fv_wps_op(1);
	if(0 != ret)
	{
		qspi_printf("enable wps failed!");
		return ret;
	}

	ret = qspi_w25q256fv_read_block_lock_status(offs);

	if(BLOCK_PROTECTED == ret)
	{
		return QSPI_OP_SUCCESS;
	}

	ret = qspi_w25q256fv_send_lock_cmd(WPS_BLOCK_LOCK, offs, 1);

    if(ret < 0)
        return QSPI_OP_FAILED;

	return QSPI_OP_SUCCESS;
}

int qspi_w25q256fv_block_unlock(cyg_uint32 offs)
{
	int ret=0;

	ret = qspi_w25q256fv_wps_op(1);
	if(0 != ret)
	{
		qspi_printf("enable wps failed!");
		return ret;
	}

	ret = qspi_w25q256fv_read_block_lock_status(offs);

	if(BLOCK_UNPROTECTED == ret)
	{
		return QSPI_OP_SUCCESS;
	}

	ret = qspi_w25q256fv_send_lock_cmd(WPS_BLOCK_UNLOCK, offs, 1);

    if(ret < 0)
        return QSPI_OP_FAILED;

	return QSPI_OP_SUCCESS;
}


int qspi_w25q256fv_global_lock(void)
{
	return qspi_w25q256fv_send_lock_cmd(WPS_GLOBAL_BLOCK_LOCK, 0, 0);
}

int qspi_w25q256fv_global_unlock(void)
{
	return qspi_w25q256fv_send_lock_cmd(WPS_GLOBAL_BLOCK_UNLOCK, 0, 0);
}
