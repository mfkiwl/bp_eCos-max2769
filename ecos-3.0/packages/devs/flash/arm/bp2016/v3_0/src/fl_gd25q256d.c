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

#define SR2_QUAD_EN 0x02
#define SR3_P_ERR_GD 0x04
#define SR3_E_ERR_GD 0x08

extern inline int qspi_wren(cyg_uint8 cmd);
extern void qspi_ahb_read_enable(cyg_uint8 rd_cmd, cyg_uint8 dfs);
extern void qspi_ahb_write_enable(cyg_uint8 wren_cmd);
extern int qspi_send_xx_data(cyg_uint8 *tx_dbuf, int tx_len);
extern int qspi_read_xx_data(cyg_uint8 cmd, cyg_uint32 addr, int addr_vld, cyg_uint8 *rx_dbuf, int rx_len);
cyg_uint8 qspi_fl_read_status(cyg_uint8 cmd);
cyg_uint8 qspi_gd25q256d_read_status(void);
extern int qspi_dma_send_cmd_data(cyg_uint8 cmd, cyg_uint8 data);

static int wait_cmd = 0;
static cyg_uint8 wbuf_s[280];

extern int qspi_fl_wait_wel(void);

static int qspi_gd25q256d_write_extend_addr(cyg_uint8 cmd, cyg_uint8 data)
{
    int ret;

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
static int qspi_gd25q256d_clear_status(cyg_uint8 clrr_cmd)
{
    int ret;
    ret = qspi_send_xx_data(&clrr_cmd, 1);
    if(ret < 0)
    {
        return QSPI_OP_FAILED;
    }else{
        return QSPI_OP_SUCCESS;
    }
}

/*
 * only for program/erase/write status
 */
static cyg_uint8 wait_flash_idle_extend(cyg_uint8 *r_st3)
{
    cyg_uint8 st1 = 0, st3 = 0;
    cyg_uint32 timeout = 0;

    switch (wait_cmd) {
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

/**/
static int qspi_gd25q256d_clear_status_for_err_bits(void)
{
    int ret = QSPI_OP_SUCCESS;

    // wip = 1, p_err =1
    ret = qspi_gd25q256d_clear_status(SPINOR_OP_CLRR);
    if(ret < 0)
        return QSPI_OP_FAILED;

    qspi_printf("clear and status , st1 0x%x, st3 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR1),
                qspi_fl_read_status(SPINOR_OP_RDSR3));
    // wip = 1, p_err = 0;
    ret = qspi_wren(SPINOR_OP_WRDI);
    // wip = 0, p_err = 0;
    qspi_printf("wren disable, status	  :0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR1));

    return ret;
}

static int qspi_gd25q256d_write_status(cyg_uint8 cmd, cyg_uint8 sr_x)
{
	int ret=0;
    cyg_uint8 status, st3 = 0;

    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
    {
        return QSPI_OP_FAILED;
    }
    ret=qspi_fl_wait_wel();
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
	if(ret < 0)
	{
		return QSPI_OP_FAILED;
	}

    wait_cmd = cmd;
    status = wait_flash_idle_extend(&st3);
    if ((status & SR_WIP) || (st3 & SR3_P_ERR_GD)) { // both these bits all config to 1
        qspi_printf("write status failed, status 0x%x, st3 0x%x\n", status, st3);
        ret = qspi_gd25q256d_clear_status_for_err_bits();
        if(ret < 0)
            return QSPI_OP_FAILED;
        status |= SR_WIP; // maybe not need this code.
    }
    return (int)status;
}

static int qspi_gd25q256d_4byte_extend_enable(cyg_uint32 addr)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 ext_data = 0, ext_rd;
    int ret;

    ext_rd = qspi_fl_read_status(SPINOR_OP_RD_EXTADDR);
    if ((addr & (1 << 24)) > 0)
        ext_data |= SPINOR_OP_EXTR_A24;

    if (ext_rd != ext_data) {
#ifdef QSPI_DEBUG
        s = HAL_GET_COUNTER;
#endif
        ret = qspi_gd25q256d_write_extend_addr(SPINOR_OP_WR_EXTADDR, ext_data);

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

static int qspi_gd25q256d_4byte_extend_disable(cyg_uint32 addr)
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
        ret = qspi_gd25q256d_write_extend_addr(SPINOR_OP_WR_EXTADDR, 0);

#ifdef QSPI_DEBUG
        e = HAL_GET_COUNTER;
        us = archtimer_tick_to_us((unsigned long) (e - s));
#endif

        qspi_debug("extend addr register       : 0x%x\n",
                    qspi_fl_read_status(SPINOR_OP_RD_EXTADDR));

        qspi_debug("address_extend_disable need: %d us (%d ms)\n\n", us, us / 1000);
    }

    return ret;
}

static int qspi_gd25q256d_quad_enable(void)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    int st;
    int ret = QSPI_OP_SUCCESS;

    //qspi_printf("quad enable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif

    st = qspi_fl_read_status(SPINOR_OP_RDSR2);
    if ((st & SR2_QUAD_EN) == SR2_QUAD_EN) {
        //qspi_printf("quad bit had enabled\n");
        return ret;
    }

    st = qspi_gd25q256d_write_status(SPINOR_OP_WRSR2, st | SR2_QUAD_EN);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif

    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }

    if(st < 0)
        ret = QSPI_OP_FAILED;

    //qspi_printf("status 2 register   : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR2));
    qspi_debug("quad_enable need    : %d us (%d ms)\n\n", us, us / 1000);

    return ret;
}

int qspi_gd25q256d_quad_disable(void)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 st;
    int ret = QSPI_OP_SUCCESS;

    //qspi_printf("quad disable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif

    st = qspi_fl_read_status(SPINOR_OP_RDSR2);
    if ((st & SR2_QUAD_EN) == 0) {
        //qspi_printf("quad bit had disabled\n");
        return ret;
    }

    st &= ~(SR2_QUAD_EN);
    st = qspi_gd25q256d_write_status(SPINOR_OP_WRSR2, st);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif

    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }
    if(st < 0)
        ret = QSPI_OP_FAILED;

    //qspi_printf("status 2 register   : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR2));
    qspi_debug("quad_disable need   :%d us (%d ms)\n\n", us, us / 1000);

    return ret;
}

static int qspi_gd25q256d_prot_region_enable(int region)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 rdsr1 = 0, st;
    int ret = QSPI_OP_SUCCESS;

    if (region > 0xf)
        return QSPI_OP_FAILED;

    //qspi_printf("\nprot enable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif
    rdsr1 = qspi_fl_read_status(SPINOR_OP_RDSR1);

    if ((rdsr1 & SR1_BP_BITS_MASK) == SR1_BP_BITS_SHIFT(region)) {
        //qspi_printf("region had enabled, rdsr1 = 0x%x\n", rdsr1);
        return QSPI_OP_SUCCESS;
    }

    rdsr1 |= SR1_BP_BITS_SHIFT(region);
    st = qspi_gd25q256d_write_status(SPINOR_OP_WRSR1, rdsr1);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif
    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }
    if(st < 0)
        ret = QSPI_OP_FAILED;

    qspi_gd25q256d_read_status();
    qspi_debug("prot_enable need    : %d us (%d ms), w:0x%x\n", us, us / 1000, rdsr1);

    return ret;
}

static int qspi_gd25q256d_prot_region_disable(int region)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 rdsr1 = 0, st;
    int ret = QSPI_OP_SUCCESS;

    //qspi_printf("\nprot disable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif
    rdsr1 = qspi_fl_read_status(SPINOR_OP_RDSR1);

    if ((rdsr1 & SR1_BP_BITS_MASK) == 0) {
        //qspi_printf("region had disabled, rdsr1 = 0x%x\n", rdsr1);
        return QSPI_OP_SUCCESS;
    }

    rdsr1 &= ~SR1_BP_BITS_MASK;
    st = qspi_gd25q256d_write_status(SPINOR_OP_WRSR1, rdsr1);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = archtimer_tick_to_us((unsigned long) (e - s));
#endif
    if ((st & SR_WIP) == SR_WIP) {
        ret = QSPI_OP_FAILED;
    }
    if(st < 0)
        ret = QSPI_OP_FAILED;

    qspi_gd25q256d_read_status();
    qspi_debug("prot_disable need    : %d us (%d ms), w:0x%x\n", us, us / 1000, rdsr1);

    return ret;
}

int qspi_gd25q256d_write_page(cyg_uint32 addr, cyg_uint8 *dbuf, int len)
{
	int ret=0;
    cyg_uint8 status, st3;

    // qspi_gd25q256d_4byte_extend(addr, addr&0x3000000);

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

    wbuf_s[0] = SPINOR_OP_BP;
    wbuf_s[1] = (addr >> 16) & 0xff;
    wbuf_s[2] = (addr >> 8) & 0xff;
    wbuf_s[3] = addr & 0xff;

    if (len <= 256)
        memcpy(wbuf_s + 4, dbuf, len);
    ret = qspi_send_xx_data(wbuf_s, len + 4);
	if(ret < 0)
	{
		return QSPI_OP_FAILED;
	}
    wait_cmd = SPINOR_OP_BP;

    status = wait_flash_idle_extend(&st3); // wip = 1, p_err = 1
    if ((status & SR_WIP) || (st3 & SR3_P_ERR_GD)) {
        qspi_printf("program addr 0x%x failed, status 0x%x, st3 0x%x\n", addr, status, st3);
        qspi_gd25q256d_clear_status_for_err_bits();
        return QSPI_OP_FAILED;
    }
    return QSPI_OP_SUCCESS;
}

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
extern int qspi_dma_write_page(void *dma_tx_ch, cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *tx_dbuf, int tx_len, int dfs);
extern inline void qspi_hw_init(void);
int qspi_gd25q256d_dma_write_page(void *dma_tx_ch, cyg_uint32 addr, cyg_uint8 *dbuf, int len)
{
    cyg_uint8 status, st3;
    int ret;

    qspi_debug("ENTER");

    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
    {
        return QSPI_OP_FAILED;
    }
    ret = qspi_fl_wait_wel();
    if(ret < 0)
        return ret;

    ret = qspi_dma_write_page(dma_tx_ch, SPINOR_OP_BP, addr, dbuf, len, 8);
    if(ret < 0)
        return ret;

    wait_cmd = SPINOR_OP_BP;
    status = wait_flash_idle_extend(&st3);
    if ((status & SR_WIP) || (st3 & SR3_P_ERR_GD)) {
        qspi_printf("%s:program addr 0x%x failed, status 0x%x, st3 0x%x\n", __FUNCTION__, addr,
                    status, st3);
        qspi_gd25q256d_clear_status_for_err_bits();
        qspi_hw_init();
        return QSPI_OP_FAILED;
    }
    qspi_hw_init();
    return QSPI_OP_SUCCESS;
}
#endif

int qspi_gd25q256d_erase(cyg_uint8 cmd, cyg_uint32 addr)
{
	int ret=0;
    cyg_uint8 status, st3;

    qspi_debug("cmd 0x%x addr 0x%x", cmd, addr);
    // qspi_gd25q256d_4byte_extend(addr, addr&0x3000000);
    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
    {
        return QSPI_OP_FAILED;
    }
    
    ret = qspi_fl_wait_wel();
	if(ret<0)
	{
		return QSPI_OP_FAILED;
	}

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
    ret = qspi_dma_write_page(qspi_dma_tx_chan, cmd, addr, NULL, 0, 8);
#else
    cyg_uint8 wbuf[4];
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

    wait_cmd = SPINOR_OP_BE_64K;
    status = wait_flash_idle_extend(&st3);
    if ((status & SR_WIP) || (st3 & SR3_E_ERR_GD)) {
        qspi_printf("erase addr 0x%x failed, status 0x%x, st3 0x%x\n", addr, status, st3);
        qspi_gd25q256d_clear_status_for_err_bits();
        return QSPI_OP_FAILED;
    }
    return QSPI_OP_SUCCESS;
}

int qspi_gd25q256d_erase_all(void)
{
	int ret=0;
    cyg_uint8 status, st3;
    cyg_uint8 wbuf[4];

    ret = qspi_wren(SPINOR_OP_WREN);
    if(ret < 0)
    {
        return QSPI_OP_FAILED;
    }
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

    wait_cmd = SPINOR_OP_CHIP_ERASE;
    status = wait_flash_idle_extend(&st3);
    if ((status & SR_WIP) || (st3 & SR3_E_ERR_GD)) {
        qspi_printf("chip erase failed, status 0x%x, st3 0x%x\n", status, st3);
        qspi_gd25q256d_clear_status_for_err_bits();
        return QSPI_OP_FAILED;
    }
    return QSPI_OP_SUCCESS;
}

int qspi_gd25q256d_4byte_extend(cyg_uint32 addr, int en)
{
    if (en > 0)
        return qspi_gd25q256d_4byte_extend_enable(addr);
    else
        return qspi_gd25q256d_4byte_extend_disable(addr);
}

int qspi_gd25q256d_quad(int en)
{
    if (en > 0)
        return qspi_gd25q256d_quad_enable();
    else
        return qspi_gd25q256d_quad_disable();
}

int qspi_gd25q256d_prot_region(int region, int en)
{
    if (en > 0)
        return qspi_gd25q256d_prot_region_enable(region);
    else
        return qspi_gd25q256d_prot_region_disable(region);
}

cyg_uint8 qspi_gd25q256d_read_status(void)
{
    qspi_printf("status-1		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR1));
    qspi_printf("status-2		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR2));
    qspi_printf("status-3		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR3));
    qspi_printf("extend addr		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RD_EXTADDR));
    return 0;
}

int qspi_gd25q256d_reset(void)
{
    cyg_uint8 cmd = SPINOR_OP_ENABLE_RESET;
    int ret;

    ret = qspi_send_xx_data(&cmd, 1);
    if(ret < 0)
        return ret;

    cmd = SPINOR_OP_RESET;
    ret = qspi_send_xx_data(&cmd, 1);
    if(ret < 0)
        return ret;

    HAL_DELAY_US(10000); // wait for device Reset sequence complete
    return QSPI_OP_SUCCESS;
}
