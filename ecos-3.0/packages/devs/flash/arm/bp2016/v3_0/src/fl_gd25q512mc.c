#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/io/flash/qspi_dev.h>
#include <cyg/hal/regs/qspi.h>
#include <cyg/hal/plf/chip_config.h>
#include <errno.h>
#include <string.h>
#include "qspi_dbg.h"
#include "fl_common_cmd.h"

#define SPINOR_OP_CLRR 0x30

#define SR1_BP_BITS_MASK 0x3c
#define SR1_BP_BITS_SHIFT(bp) (bp << 2)
#define SR3_WPS 0x80

#define SR1_QUAD_EN 0x40
#define SR3_P_ERR_GD 0x20
#define SR3_E_ERR_GD 0x40

#define SR_BP_BITS_MASK_GD 0x3C
#define SR_BP_BITS_SHIFT_GD(v) (v << 2)
#define SR_TBPROT_EN_GD 0x40 /* Winbond TBProt I/O: 1=low address */

extern inline int qspi_wren(cyg_uint8 cmd);
extern void qspi_ahb_read_enable(cyg_uint8 rd_cmd, cyg_uint8 dfs);
extern void qspi_ahb_write_enable(cyg_uint8 wren_cmd);
extern int qspi_send_xx_data(cyg_uint8 *tx_dbuf, int tx_len);
extern int qspi_read_xx_data(cyg_uint8 cmd, cyg_uint32 addr, int addr_vld, cyg_uint8 *rx_dbuf,
                             int rx_len);
cyg_uint8 qspi_fl_read_status(cyg_uint8 cmd);
cyg_uint8 qspi_gd25q512mc_read_status(void);
extern int qspi_dma_send_cmd_data(cyg_uint8 cmd, cyg_uint8 data);

static int wait_cmd=0;
static cyg_uint8 wbuf_s[280];
extern int qspi_fl_wait_wel(void);

static int qspi_gd25q512mc_write_extend_addr(cyg_uint8 cmd, cyg_uint8 data)
{
    int ret = QSPI_OP_SUCCESS;

    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
    {
        return QSPI_OP_FAILED;
    }
    ret = qspi_fl_wait_wel();
    if(ret < 0)
    {
        return QSPI_OP_FAILED;
    }

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
    ret = qspi_dma_send_cmd_data(cmd, data);
#else
    cyg_uint8 wbuf[2];
    wbuf[0] = cmd;
    wbuf[1] = data;

    ret = qspi_send_xx_data(wbuf, 2);
#endif
    if(ret < 0)
    {
        return QSPI_OP_FAILED;
    }else{
        return QSPI_OP_SUCCESS;
    }
}

/*not necessary to set WEL bit, before the clear SR command is executed*/
/* for clear erase Fail Flag and Program Fail Flag*/
static int qspi_gd25q512mc_clear_status(cyg_uint8 clrr_cmd)
{
    if(qspi_send_xx_data(&clrr_cmd, 1) < 0)
        return QSPI_OP_FAILED;

    return QSPI_OP_SUCCESS;
}

/*
 * only for program/erase/write status/erase[program] security register
 */
static cyg_uint8 wait_flash_idle_extend(cyg_uint8 *r_st3)
{
    cyg_uint8 st1 = 0, st3 = 0;
	cyg_uint32 timeout = 0;

	switch(wait_cmd)
	{
	case SPINOR_OP_BE_64K:
		timeout = tm_array[1].ers_blk64k_tm;
		break;
	case SPINOR_OP_BE_32K:
		timeout = tm_array[1].ers_blk32k_tm;
		break;
	case SPINOR_OP_BE_SECTOR:
		timeout = tm_array[1].ers_sec_tm;
		break;
	case SPINOR_OP_CHIP_ERASE:
		timeout = tm_array[1].ers_chip_tm;
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
        st1 = qspi_fl_read_status(SPINOR_OP_RDSR1);
        st3 = qspi_fl_read_status(SPINOR_OP_RDSR3);
        if ((st3 & SR3_E_ERR_GD) == SR3_E_ERR_GD)
            break;
        if ((st3 & SR3_P_ERR_GD) == SR3_P_ERR_GD)
            break;
        if ((st1 & SR_WIP) == 0)
            break;

		HAL_DELAY_US(50);
		timeout -= 50;
    }
    *r_st3 = st3;
    return st1;
}

static int qspi_gd25q512mc_clear_status_for_err_bits(void)
{
    int ret;
    ret = qspi_gd25q512mc_clear_status(SPINOR_OP_CLRR);
    if(ret < 0)
        return QSPI_OP_FAILED;
    qspi_debug("clear and status , st1 0x%x, st3 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR1),
           qspi_fl_read_status(SPINOR_OP_RDSR3));
    ret = qspi_wren(SPINOR_OP_WRDI);
    qspi_debug("wren disable, status	  :0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR1));
    return ret;
}

static int qspi_gd25q512mc_write_status(cyg_uint8 cmd, cyg_uint8 sr_x)
{
	int ret=QSPI_OP_SUCCESS;
    cyg_uint8 status, st3 = 0;

    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
    {
        return QSPI_OP_FAILED;
    }
    ret = qspi_fl_wait_wel();
    if(ret < 0)
    {
        return QSPI_OP_FAILED;
    }

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
    status = wait_flash_idle_extend(&st3);
    if ((status & SR_WIP) || (st3 & SR3_P_ERR_GD)) {
        qspi_printf("write status failed, status 0x%x, st3 0x%x\n", status, st3);
        ret = qspi_gd25q512mc_clear_status_for_err_bits();
        if(ret<0)
        {
            return QSPI_OP_FAILED;
        }
    }
    return (int)status;
}

static int qspi_gd25q512mc_4byte_extend_enable(cyg_uint32 addr)
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
    if ((addr & (1 << 25)) > 0)
        ext_data |= SPINOR_OP_EXTR_A25;
    if ((addr & (1 << 26)) > 0)
        ext_data |= SPINOR_OP_EXTR_A26;

    if (ext_rd != ext_data) {
#ifdef QSPI_DEBUG
        s = HAL_GET_COUNTER;
#endif
        ret = qspi_gd25q512mc_write_extend_addr(SPINOR_OP_WR_EXTADDR, ext_data);

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

static int qspi_gd25q512mc_4byte_extend_disable(cyg_uint32 addr)
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
        ret = qspi_gd25q512mc_write_extend_addr(SPINOR_OP_WR_EXTADDR, 0);

#ifdef QSPI_DEBUG
        e = HAL_GET_COUNTER;
        us = archtimer_tick_to_us((unsigned long) (e - s));
#endif

        qspi_debug("extend addr register       : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RD_EXTADDR));

        qspi_debug("address_extend_disable need: %d us (%d ms)\n\n", us, us / 1000);
    }

    return ret;
}

static int qspi_gd25q512mc_quad_enable(void)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    int st;
    int ret = QSPI_OP_SUCCESS;

    qspi_debug("quad enable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif

    st = qspi_fl_read_status(SPINOR_OP_RDSR1);
    if ((st & SR1_QUAD_EN) == SR1_QUAD_EN) {
        //qspi_printf("quad bit had enabled");
        return ret;
    }

    st = qspi_gd25q512mc_write_status(SPINOR_OP_WRSR1, st | SR1_QUAD_EN);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif

    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }
    if(st < 0)
        ret = QSPI_OP_FAILED;

    qspi_debug("status 1 register   : 0x%x", qspi_fl_read_status(SPINOR_OP_RDSR1));
    qspi_debug("quad_enable need    : %d us (%d ms)\n", us, us / 1000);

    return ret;
}

int qspi_gd25q512mc_quad_disable(void)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    int st;
    int ret = QSPI_OP_SUCCESS;

    qspi_debug("quad disable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif

    st = qspi_fl_read_status(SPINOR_OP_RDSR1);
    if ((st & SR1_QUAD_EN) == 0) {
        //qspi_printf("quad bit had disabled!\n");
        return ret;
    }

    st &= ~(SR1_QUAD_EN);
    st = qspi_gd25q512mc_write_status(SPINOR_OP_WRSR1, st);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif

    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }
    if(st < 0)
        ret = QSPI_OP_FAILED;

    qspi_debug("status 1 register   : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR1));
    qspi_debug("quad_disable need   :%d us (%d ms)\n", us, us / 1000);

    return ret;
}

static int qspi_gd25q512mc_prot_region_enable(int region)
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
    if ((rdsr3 & SR3_WPS) > 0) {
        qspi_printf("failed! need utilize individual block lock;(wps =1)\n");
        return QSPI_OP_FAILED;
    }

    if ((rdsr1 & SR1_BP_BITS_MASK) == SR1_BP_BITS_SHIFT(region)) {
        //qspi_printf("region had enabled, rdsr1 = 0x%x, rdsr3 = 0x%x\n", rdsr1, rdsr3);
        return QSPI_OP_SUCCESS;
    }

    rdsr1 |= SR1_BP_BITS_SHIFT(region);
    st = qspi_gd25q512mc_write_status(SPINOR_OP_WRSR1, rdsr1);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif
    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }
    if(st < 0)
        ret = QSPI_OP_FAILED;

    qspi_gd25q512mc_read_status();
    qspi_debug("prot_enable need    : %d us (%d ms), w:0x%x\n", us, us / 1000, rdsr1);

    return ret;
}

static int qspi_gd25q512mc_prot_region_disable(int region)
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

    if ((rdsr3 & SR3_WPS) > 0) {
        qspi_printf("failed! need utilize individual block lock;(wps =1)\n");
        return QSPI_OP_FAILED;
    }

    if ((rdsr1 & SR1_BP_BITS_MASK) == 0) {
        //qspi_printf("region had disabled, rdsr1 = 0x%x, rdsr3 = 0x%x\n", rdsr1, rdsr3);
        return QSPI_OP_SUCCESS;
    }

    rdsr1 &= ~SR1_BP_BITS_MASK;
    st = qspi_gd25q512mc_write_status(SPINOR_OP_WRSR1, rdsr1);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif
    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }
    if(st < 0)
        ret = QSPI_OP_FAILED;

    qspi_gd25q512mc_read_status();
    qspi_debug("prot_disable need    : %d us (%d ms), w:0x%x\n", us, us / 1000, rdsr1);

    return ret;
}

int qspi_gd25q512mc_write_page(cyg_uint32 addr, cyg_uint8 *dbuf, int len)
{
	int ret=QSPI_OP_SUCCESS;
    cyg_uint8 status, st3;

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

    status = wait_flash_idle_extend(&st3);
    if ((status & SR_WIP) || (st3 & SR3_P_ERR_GD)) {
        qspi_printf("program addr 0x%x failed, status 0x%x, st3 0x%x\n", addr, status, st3);
        qspi_gd25q512mc_clear_status_for_err_bits();
        return QSPI_OP_FAILED;
    }
    return QSPI_OP_SUCCESS;
}

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
extern int qspi_dma_write_page(void *dma_tx_ch, cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *tx_dbuf, int tx_len, int dfs);
extern inline void qspi_hw_init(void);
int qspi_gd25q512mc_dma_write_page(void *dma_tx_ch, cyg_uint32 addr, cyg_uint8 *dbuf, int len)
{
    cyg_uint8 status, st3;
    int ret=QSPI_OP_SUCCESS;

    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
        return ret;
    ret = qspi_fl_wait_wel();
    if(ret < 0)
        return ret;

    ret = qspi_dma_write_page(dma_tx_ch, SPINOR_OP_BP, addr, dbuf, len, 8);
    if(ret < 0)
        return ret;

	wait_cmd = SPINOR_OP_BP;
    status = wait_flash_idle_extend(&st3);
    if ((status & SR_WIP) || (st3 & SR3_P_ERR_GD)) {
        qspi_printf("%s:program addr 0x%x failed, status 0x%x, st3 0x%x\n", __FUNCTION__, addr, status,
               st3);
        qspi_gd25q512mc_clear_status_for_err_bits();
        qspi_hw_init();
        return QSPI_OP_FAILED;
    }
    qspi_hw_init();
    return QSPI_OP_SUCCESS;
}
#endif

int qspi_gd25q512mc_erase(cyg_uint8 cmd, cyg_uint32 addr)
{
	int ret=0;
    cyg_uint8 status, st3;

    ret= qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
        return ret;
    ret=qspi_fl_wait_wel();
    if(ret < 0)
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
    status = wait_flash_idle_extend(&st3);
    if ((status & SR_WIP) || (st3 & SR3_E_ERR_GD)) {
        qspi_printf("erase addr 0x%x failed, status 0x%x, st3 0x%x\n", addr, status, st3);
        qspi_gd25q512mc_clear_status_for_err_bits();
        return QSPI_OP_FAILED;
    }
    return QSPI_OP_SUCCESS;
}

int qspi_gd25q512mc_erase_all(void)
{
	int ret=0;
    cyg_uint8 status, st3;
    cyg_uint8 wbuf[4];

    ret= qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
        return ret;
    ret = qspi_fl_wait_wel();
	if(ret<0)
	{
		return QSPI_OP_FAILED;
	}

    wbuf[0] = SPINOR_OP_CHIP_ERASE;
    ret = qspi_send_xx_data(wbuf, 1);
	if(ret < 0)
	{
		return QSPI_OP_FAILED;
	}

	wait_cmd=SPINOR_OP_CHIP_ERASE;
    status = wait_flash_idle_extend(&st3);
    if ((status & SR_WIP) || (st3 & SR3_E_ERR_GD)) {
        qspi_printf("chip erase failed, status 0x%x, st3 0x%x\n", status, st3);
        qspi_gd25q512mc_clear_status_for_err_bits();
        return QSPI_OP_FAILED;
    }
    return QSPI_OP_SUCCESS;
}

int qspi_gd25q512mc_4byte_extend(cyg_uint32 addr, int en)
{
    if (en > 0)
        return qspi_gd25q512mc_4byte_extend_enable(addr);
    else
        return qspi_gd25q512mc_4byte_extend_disable(addr);
}

int qspi_gd25q512mc_quad(int en)
{
    if (en > 0)
        return qspi_gd25q512mc_quad_enable();
    else
        return qspi_gd25q512mc_quad_disable();
}

int qspi_gd25q512mc_wps_op(int en)
{
	u8 rdsr3, st;
    rdsr3 = qspi_fl_read_status(SPINOR_OP_RDSR3);

	if(0 == en)
	{
		if ((rdsr3 & SR3_WPS) > 0) {
			qspi_printf("(wps =1), need disable it!");
			rdsr3 &= ~SR3_WPS;
		}else{
			return QSPI_OP_SUCCESS;
		}
	}else{
		if ((rdsr3 & SR3_WPS) ==  0) {
			qspi_printf("(wps =0), need enable it!");
			rdsr3 |= SR3_WPS;
		}else{
			return QSPI_OP_SUCCESS;
		}
	}

	st = qspi_gd25q512mc_write_status(SPINOR_OP_WRSR3, rdsr3);
	if ((st & SR_WIP) == SR_WIP) {
		qspi_printf("write WPS failed!");
		return QSPI_OP_FAILED;
	}

	return QSPI_OP_SUCCESS;
}

int qspi_gd25q512mc_prot_region(int region, int en)
{
	if(qspi_gd25q512mc_wps_op(0) < 0)
	{
		qspi_printf("WPS=1 ,failed!\n");
		return QSPI_OP_FAILED;
	}

    if (en > 0)
        return qspi_gd25q512mc_prot_region_enable(region);
    else
        return qspi_gd25q512mc_prot_region_disable(region);
}

cyg_uint8 qspi_gd25q512mc_read_status(void)
{
    qspi_printf("status-1		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR1));
    qspi_printf("status-2		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR2));
    qspi_printf("status-3		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR3));
    qspi_printf("extend addr		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RD_EXTADDR));
    return 0;
}

int qspi_gd25q512mc_reset(void)
{
    int ret=QSPI_OP_SUCCESS;

    cyg_uint8 cmd = SPINOR_OP_ENABLE_RESET;
    ret = qspi_send_xx_data(&cmd, 1);
    if(ret < 0)
        return QSPI_OP_FAILED;

    cmd = SPINOR_OP_RESET;
    ret = qspi_send_xx_data(&cmd, 1);
    if(ret < 0)
        return QSPI_OP_FAILED;
    HAL_DELAY_US(10000); // wait for device Reset sequence complete
    return 0;
}

#if 0
//only one block status
int qspi_gd25q512mc_read_block_lock_status(cyg_uint32 offs)
{
	cyg_uint8 rdsr3 = 0;
	cyg_uint8 lock_status=0;
	int ret=0;

    rdsr3 = qspi_fl_read_status(SPINOR_OP_RDSR3);

    if ((rdsr3 & SR3_WPS) == 0) {
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

static void inline qspi_gd25q512mc_send_lock_cmd(cyg_uint8 cmd, cyg_uint32 addr, cyg_uint32 vld_addr)
{
	cyg_uint8 status;
	cyg_uint8 wbuf[6];

	qspi_wren(SPINOR_OP_WREN);
    qspi_fl_wait_wel();

	wbuf[0] = cmd;

	if(vld_addr)
	{
		wbuf[1] = (addr >> 16) & 0xff;
		wbuf[2] = (addr >> 8) & 0xff;
		wbuf[3] = addr & 0xff;
		qspi_send_xx_data(wbuf, 4);
	}else{
		qspi_send_xx_data(wbuf, 1);
	}
}

int qspi_gd25q512mc_block_lock(cyg_uint32 offs)
{
	int ret=0;

	ret = qspi_gd25q512mc_wps_op(1);
	if(0 != ret)
	{
		qspi_printf("enable wps failed!\n");
		return ret;
	}

	ret = qspi_gd25q512mc_read_block_lock_status(offs);

	if(BLOCK_PROTECTED == ret)
	{
		return ENOERR; 
	}

	qspi_gd25q512mc_send_lock_cmd(WPS_BLOCK_LOCK, offs, 1);

	return ENOERR;
}

int qspi_gd25q512mc_block_unlock(cyg_uint32 offs)
{
	int ret=0;

	ret = qspi_gd25q512mc_wps_op(1);
	if(0 != ret)
	{
		qspi_printf("enable wps failed!\n");
		return ret;
	}

	ret = qspi_gd25q512mc_read_block_lock_status(offs);

	if(BLOCK_UNPROTECTED == ret)
	{
		return ENOERR;
	}

	qspi_gd25q512mc_send_lock_cmd(WPS_BLOCK_UNLOCK, offs, 1);

	return ENOERR;
}


void qspi_gd25q512mc_global_lock(void)
{
	qspi_gd25q512mc_send_lock_cmd(WPS_GLOBAL_BLOCK_LOCK, 0, 0);
}

void qspi_gd25q512mc_global_unlock(void)
{
	qspi_gd25q512mc_send_lock_cmd(WPS_GLOBAL_BLOCK_UNLOCK, 0, 0);
}
#endif
