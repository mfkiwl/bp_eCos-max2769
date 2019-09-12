#include <cyg/infra/diag.h>
#include <cyg/hal/hal_diag.h>

#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/flash/qspi_dev.h>
#include <cyg/hal/regs/qspi.h>
#include <string.h>
#include "qspi_dbg.h"
#include "fl_common_cmd.h"

#define SPINOR_OP_CLRR 0x30
#define SPINOR_OP_SW_RESET 0xf0 /* Software Reset */

extern inline void qspi_wren(cyg_uint8 cmd);
extern void qspi_ahb_read_enable(cyg_uint8 rd_cmd, cyg_uint8 dfs);
extern void qspi_ahb_write_enable(cyg_uint8 wren_cmd);
extern int qspi_send_xx_data(cyg_uint8 *tx_dbuf, int tx_len);
extern int qspi_read_xx_data(cyg_uint8 cmd, cyg_uint32 addr, int addr_vld, cyg_uint8 *rx_dbuf,
                             int rx_len);
extern int qspi_read_spansion_data(cyg_uint8 cmd, cyg_uint32 addr, int addr_vld, 
		cyg_uint8 *rx_dbuf, int rx_len);
extern int qspi_read_page_data(cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *rx_dbuf, int rx_len,
                               int dfs);

extern int qspi_dma_send_cmd_data(cyg_uint8 cmd, cyg_uint8 data);
static cyg_uint8 wbuf_s[280];
static cyg_uint8 wait_cmd=0; 

extern cyg_uint8 qspi_fl_read_status(cyg_uint8 cmd);

/*not require WEN command to precede its*/
static void qspi_s25fl512s_write_bar(cyg_uint8 cmd, cyg_uint8 data)
{
#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
    qspi_dma_send_cmd_data(cmd, data);
#else
    cyg_uint8 wbuf[2];
    wbuf[0] = cmd;
    wbuf[1] = data;
    qspi_send_xx_data(wbuf, 2);
#endif
}

/*not necessary to set WEL bit, before the clear SR command is executed*/
/* for clear erase Fail Flag and Program Fail Flag*/
static int qspi_f25fl512s_clear_status(cyg_uint8 clrr_cmd)
{
    qspi_send_xx_data(&clrr_cmd, 1);
    return 0;
}

static cyg_uint8 wait_flash_idle_extend(cyg_uint8 rdsr_cmd, cyg_uint32 wip, cyg_uint32 wip_mask,
                                        cyg_uint32 e_err, cyg_uint32 e_err_mask, cyg_uint32 p_err,
                                        cyg_uint32 p_err_mask)
{
	cyg_uint32 timeout;

	switch(wait_cmd)
	{
		case SPINOR_OP_BP:
			timeout = tm_array[0].prg_page_tm; 
			break;
		case SPINOR_OP_WRSR:
			timeout = tm_array[0].wr_status_tm;
			break;
		case SPINOR_OP_CHIP_ERASE:
			timeout = tm_array[0].ers_chip_tm;
			break;
		case SPINOR_OP_SE:
			timeout = tm_array[0].ers_blk64k_tm;
			break;
		case SPINOR_OP_BE_32K:
			timeout = tm_array[0].ers_blk32k_tm;
			break;
		case SPINOR_OP_BE_4K:
			timeout = tm_array[0].ers_sec_tm;
			break;
		default:
			timeout = tm_array[0].wr_status_tm;
			break;
	}

    cyg_uint8 st = 0;
    while (timeout) {
        st = qspi_fl_read_status(rdsr_cmd);
        if ((st & wip_mask) == wip)
            break;
        if ((st & e_err_mask) == e_err)
            break;
        if ((st & p_err_mask) == p_err)
            break;

		HAL_DELAY_US(50);
		timeout -= 50;
    }
    return st;
}

static cyg_uint8 qspi_s25fl512s_write_status(cyg_uint8 sr, cyg_uint8 cr)
{
    cyg_uint8 status;
    cyg_uint8 wbuf[3];
	int ret = 0;
    qspi_wren(SPINOR_OP_WREN);

    wbuf[0] = SPINOR_OP_WRSR;
    wbuf[1] = sr;
    wbuf[2] = cr;
    ret = qspi_send_xx_data(wbuf, 3);
	if(!ret)
	{
		qspi_printf("error! return ...");
		return 0;
	}

	wait_cmd = SPINOR_OP_WRSR;
    status = wait_flash_idle_extend(SPINOR_OP_RDSR, 0, 1, SR_E_ERR_SPAN, SR_E_ERR_SPAN,
                                    SR_P_ERR_SPAN, SR_P_ERR_SPAN);

    if ((status & SR_P_ERR_SPAN) == SR_P_ERR_SPAN) {
        qspi_printf("write status failed, status 0x:%x\n", status);
        qspi_f25fl512s_clear_status(SPINOR_OP_CLRR);
        qspi_printf("clear and status	               : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
        qspi_wren(SPINOR_OP_WRDI);
    }
    return status;
}

static void qspi_s25fl512s_4byte_extend_enable(cyg_uint32 addr)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 bar_data = 0, bar_d;

    bar_d = qspi_fl_read_status(SPINOR_OP_BRRD);
    if ((addr & (1 << 24)) > 0)
        bar_data |= SPINOR_OP_BR_BA24;
    if ((addr & (1 << 25)) > 0)
        bar_data |= SPINOR_OP_BR_BA25;
    if (bar_d != bar_data) {
#ifdef QSPI_DEBUG
        s = HAL_GET_COUNTER;
#endif
        qspi_s25fl512s_write_bar(SPINOR_OP_BRWR, bar_data);

#ifdef QSPI_DEBUG
        e = HAL_GET_COUNTER;
        us = tick_to_us((unsigned long) (e - s));
#endif

        qspi_debug("bank register             : 0x%x, wr: 0x%x\n", qspi_fl_read_status(SPINOR_OP_BRRD),
               bar_data);
        qspi_debug("address_extend_enable need: %d us (%d ms)\n\n", us, us / 1000);
    }
}

static void qspi_s25fl512s_4byte_extend_disable(cyg_uint32 addr)
{
    unsigned long long s, e;
    cyg_uint32 us;
    cyg_uint8 bar_d = 0;

    bar_d = qspi_fl_read_status(SPINOR_OP_BRRD);

    if (bar_d != 0) {
        s = HAL_GET_COUNTER;
        qspi_s25fl512s_write_bar(SPINOR_OP_BRWR, 0);

        e = HAL_GET_COUNTER;
        us = tick_to_us((unsigned long) (e - s));

        qspi_printf("bank register              : 0x%x, wr: 0\n", qspi_fl_read_status(SPINOR_OP_BRRD));
        qspi_printf("address_extend_disable need: %d us (%d ms)\n\n", us, us / 1000);
    }
}

static int qspi_s25fl512s_quad_enable(void)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 rdcr, rdsr;
    int ret = 0;

    qspi_debug("quad enable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif

    rdsr = qspi_fl_read_status(SPINOR_OP_RDSR);
    rdcr = qspi_fl_read_status(SPINOR_OP_RDCR);
    if ((rdcr & CR_QUAD_EN_SPAN) == CR_QUAD_EN_SPAN) {
        qspi_printf("quad bit had enabled\n");
        return ret;
    }
    if ((rdcr & CR_BPNV_EN_SPAN) == 0) // protect disable
        rdsr = 0;

    rdsr = qspi_s25fl512s_write_status(rdsr, CR_QUAD_EN_SPAN | rdcr);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = tick_to_us((unsigned long) (e - s));
#endif

    if ((rdsr & SR_P_ERR_SPAN) == SR_P_ERR_SPAN) {
        ret = -1;
    }

    qspi_debug("status register     : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
    qspi_debug("configuration status: 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDCR));
    qspi_debug("quad_enable need    : %d us (%d ms)\n\n", us, us / 1000);

    return ret;
}

static int qspi_s25fl512s_quad_disable(void)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 rdsr, rdcr;
    int ret = 0;

    qspi_debug("quad disable\n");
#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif

    rdsr = qspi_fl_read_status(SPINOR_OP_RDSR);
    rdcr = qspi_fl_read_status(SPINOR_OP_RDCR);
    if ((rdcr & CR_QUAD_EN_SPAN) == 0)
        return ret;

    if ((rdcr & CR_BPNV_EN_SPAN) == 0) // protect disable
        rdsr = 0;

    rdsr = qspi_s25fl512s_write_status(rdsr, rdcr & (~CR_QUAD_EN_SPAN));

#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = tick_to_us((unsigned long) (e - s));
#endif

    if ((rdsr & SR_P_ERR_SPAN) == SR_P_ERR_SPAN) {
        ret = -1;
    }

    qspi_debug("status register     : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
    qspi_debug("configuration status: 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDCR));
    qspi_debug("quad_disable need   :%d us (%d ms)\n\n", us, us / 1000);

    return ret;
}

static int qspi_s25fl512s_prot_region_enable(int region)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 rdcr = 0, rdsr = 0;
    int ret = 0;

    region &= 0x7;
    if (region >= 8)
        return -1;

    qspi_debug("\nprot enable \n");

#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif
    rdsr = qspi_fl_read_status(SPINOR_OP_RDSR);
    rdcr = qspi_fl_read_status(SPINOR_OP_RDCR);

    if (((rdcr & CR_BPNV_EN_SPAN) == CR_BPNV_EN_SPAN) &&
        (rdsr & SR_BP_BITS_MASK) == SR_BP_BITS_SHIFT(region)) {
        qspi_printf("region had enabled, rdsr = 0x%x, rdcr = 0x%x\n", rdsr, rdcr);
        return 0;
    }

    rdsr = qspi_s25fl512s_write_status(SR_BP_BITS_SHIFT(region),
                                       rdcr | CR_BPNV_EN_SPAN | CR_TBPROT_EN_SPAN);
#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = tick_to_us((unsigned long) (e - s));
#endif
    if ((rdsr & SR_P_ERR_SPAN) == SR_P_ERR_SPAN) {
        ret = -1;
    }

    qspi_debug("status		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
    qspi_debug("configuration status: 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDCR));
    qspi_debug("prot_enable need    : %d us (%d ms)\n", us, us / 1000);
    return ret;
}

static int qspi_s25fl512s_prot_region_disable(int region)
{
#ifdef QSPI_DEBUG
    unsigned long long s, e;
    cyg_uint32 us;
#endif
    cyg_uint8 rdsr = 0, rdcr;

    region &= 0x7;
    if (region >= 8)
        return -1;

    qspi_debug("\nprot disable\n");
#ifdef QSPI_DEBUG
    s = HAL_GET_COUNTER;
#endif
    rdsr = qspi_fl_read_status(SPINOR_OP_RDSR);
    rdcr = qspi_fl_read_status(SPINOR_OP_RDCR);

    if ((rdsr & SR_BP_BITS_MASK) == 0)
        return 0;

    rdsr = qspi_s25fl512s_write_status(0, rdcr | CR_BPNV_EN_SPAN | CR_TBPROT_EN_SPAN);
    if ((rdsr & SR_P_ERR_SPAN) == SR_P_ERR_SPAN) {
        return -1;
    }

#ifdef QSPI_DEBUG
    e = HAL_GET_COUNTER;
    us = tick_to_us((unsigned long) (e - s));
#endif

    qspi_debug("status		    : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
    qspi_debug("configuration status: 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDCR));
    qspi_debug("prot_disable need   :%d us (%d ms)\n", us, us / 1000);
    return 0;
}

int qspi_s25fl512s_write_page(cyg_uint32 addr, cyg_uint8 *dbuf, int len)
{
    cyg_uint8 status;
	int ret=0;

    qspi_wren(SPINOR_OP_WREN);

    wbuf_s[0] = SPINOR_OP_BP;
    wbuf_s[1] = (addr >> 16) & 0xff;
    wbuf_s[2] = (addr >> 8) & 0xff;
    wbuf_s[3] = addr & 0xff;

    if (len <= 256)
        memcpy(wbuf_s + 4, dbuf, len);
    ret = qspi_send_xx_data(wbuf_s, len + 4);
	if(!ret)
	{
		qspi_printf("error! return ...");
		return 0;
	}

	wait_cmd = SPINOR_OP_BP;
    status = wait_flash_idle_extend(SPINOR_OP_RDSR, 0, 1, SR_E_ERR_SPAN, SR_E_ERR_SPAN,
                                    SR_P_ERR_SPAN, SR_P_ERR_SPAN);
    if ((status & SR_P_ERR_SPAN) == SR_P_ERR_SPAN)
        qspi_printf("program addr 0x%x failed, status 0x:%x\n", addr, status);

    if (status & SR_WIP) {
        qspi_f25fl512s_clear_status(SPINOR_OP_CLRR);
        qspi_printf("clear and status	               : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
        qspi_wren(SPINOR_OP_WRDI);
        return QSPI_OP_FAILED;
    }

    return QSPI_OP_SUCCESS;
}

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
extern int qspi_dma_write_page(void *dma_tx_ch, cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *tx_dbuf, int tx_len, int dfs);
extern inline void qspi_hw_init(void);
int qspi_s25fl512s_dma_write_page(void *dma_tx_ch, cyg_uint32 addr, cyg_uint8 *dbuf, int len)
{
    cyg_uint8 status;

    qspi_wren(SPINOR_OP_WREN);
    qspi_dma_write_page(dma_tx_ch, SPINOR_OP_BP, addr, dbuf, len, 8);

	wait_cmd = SPINOR_OP_BP;
    status = wait_flash_idle_extend(SPINOR_OP_RDSR, 0, 1, SR_E_ERR_SPAN, SR_E_ERR_SPAN,
                                    SR_P_ERR_SPAN, SR_P_ERR_SPAN);
    if ((status & SR_P_ERR_SPAN) == SR_P_ERR_SPAN)
        qspi_printf("%s:program addr 0x%x failed, status 0x:%x\n", __FUNCTION__, addr, status);

    if (status & SR_WIP) {
        qspi_f25fl512s_clear_status(SPINOR_OP_CLRR);
        qspi_printf("%s:clear and status	               : 0x%x\n", __FUNCTION__,
               qspi_fl_read_status(SPINOR_OP_RDSR));
        qspi_wren(SPINOR_OP_WRDI);
        qspi_hw_init();
        return QSPI_OP_FAILED;
    }

    qspi_hw_init();
    return QSPI_OP_SUCCESS;
}
#endif

cyg_uint8 qspi_s25fl512s_erase(cyg_uint8 cmd, cyg_uint32 addr)
{
    cyg_uint8 status;
	int ret = 0;

	qspi_debug("cmd 0x%x addr 0x%x", cmd, addr);
    qspi_wren(SPINOR_OP_WREN);

#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
    ret = qspi_dma_write_page(qspi_dma_tx_chan, cmd, addr, NULL, 0, 8);
	if(ret < 0)
	{
		return QSPI_OP_FAILED;
	}
#else
    cyg_uint8 wbuf[4];
    //wbuf[0] = SPINOR_OP_SE;
    wbuf[0] = cmd;
    wbuf[1] = (addr >> 16) & 0xff;
    wbuf[2] = (addr >> 8) & 0xff;
    wbuf[3] = addr & 0xff;
    ret = qspi_send_xx_data(wbuf, 4);

	if(!ret)
	{
		return QSPI_OP_FAILED;
	}
#endif

	wait_cmd = cmd;
    status = wait_flash_idle_extend(SPINOR_OP_RDSR, 0, 1, SR_E_ERR_SPAN, SR_E_ERR_SPAN,
                                    SR_P_ERR_SPAN, SR_P_ERR_SPAN);
    if ((status & SR_E_ERR_SPAN) == SR_E_ERR_SPAN) // st = 0x3f
        qspi_printf("erase addr 0x%x failed, status 0x:%x\n", addr, status);

    if (status & SR_WIP) {
        qspi_f25fl512s_clear_status(SPINOR_OP_CLRR);
        qspi_printf("clear and status	               : 0x%x\n",
               qspi_fl_read_status(SPINOR_OP_RDSR)); // st = 0x1e
        qspi_wren(SPINOR_OP_WRDI);
        qspi_printf("wren disable     	               : 0x%x\n",
               qspi_fl_read_status(SPINOR_OP_RDSR)); // st = 0x1c
		qspi_printf("done!");
        return QSPI_OP_FAILED;
    }

    return QSPI_OP_SUCCESS;
}

cyg_uint8 qspi_s25fl512s_erase_all(void)
{
    cyg_uint8 status;
    cyg_uint8 wbuf[4];
	int ret = 0;

    qspi_wren(SPINOR_OP_WREN);
    wbuf[0] = SPINOR_OP_CHIP_ERASE;
    ret = qspi_send_xx_data(wbuf, 1);
	if(!ret)
	{
		return QSPI_OP_FAILED;
	}

	wait_cmd = SPINOR_OP_CHIP_ERASE;
    status = wait_flash_idle_extend(SPINOR_OP_RDSR, 0, 1, SR_E_ERR_SPAN, SR_E_ERR_SPAN,
                                    SR_P_ERR_SPAN, SR_P_ERR_SPAN);

    if ((status & SR_E_ERR_SPAN) == SR_E_ERR_SPAN)
        qspi_printf("chip erase failed, status 0x:%x\n", status);

    if (status & SR_WIP) {
        qspi_f25fl512s_clear_status(SPINOR_OP_CLRR);
        qspi_printf("clear and status	               : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
        return QSPI_OP_FAILED;
    }
    return QSPI_OP_SUCCESS;
}

void qspi_s25fl512s_4byte_extend(cyg_uint32 addr, int en)
{
    if (en > 0)
        qspi_s25fl512s_4byte_extend_enable(addr);
    else
        qspi_s25fl512s_4byte_extend_disable(addr);
}

int qspi_s25fl512s_quad(int en)
{
    if (en > 0)
        return qspi_s25fl512s_quad_enable();
    else
        return qspi_s25fl512s_quad_disable();
}

int qspi_s25fl512s_prot_region(int region, int en)
{
    if (en > 0)
        return qspi_s25fl512s_prot_region_enable(region);
    else
        return qspi_s25fl512s_prot_region_disable(region);
}

cyg_uint8 qspi_s25fl512s_read_status(void)
{
    qspi_printf("status		        : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
    qspi_printf("configuration status: 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDCR));
    qspi_printf("bank register       : 0x%x\n", qspi_fl_read_status(SPINOR_OP_BRRD));
    return 0;
}

int qspi_s25fl512s_reset(void)
{
	int ret = 0;
    cyg_uint8 cmd = SPINOR_OP_SW_RESET;
    ret = qspi_send_xx_data(&cmd, 1);
	if(!ret)
	{
		return QSPI_OP_FAILED;
	}
    HAL_DELAY_US(10000); // wait for device Reset sequence complete
    return 0;
}

/******************************************************************************/
/********************** for asp relative command ******************************/
/******************************************************************************/

static cyg_uint8 qspi_s25fl512s_write_registers(cyg_uint8 cmd, cyg_uint16 val)
{
    cyg_uint8 status;
    cyg_uint8 wbuf[3];
	int ret=0;
    qspi_wren(SPINOR_OP_WREN);

    wbuf[0] = cmd;
    wbuf[1] = val&0xff;
    wbuf[2] = (val>>8)&0xff;
    ret = qspi_send_xx_data(wbuf, 3);
	if(!ret)
	{
		return QSPI_OP_FAILED;
	}

    status = wait_flash_idle_extend(SPINOR_OP_RDSR, 0, 1, SR_E_ERR_SPAN, SR_E_ERR_SPAN,
                                    SR_P_ERR_SPAN, SR_P_ERR_SPAN);

    if (((status & SR_P_ERR_SPAN) == SR_P_ERR_SPAN)
			|| (status & SR_WIP)) {
        qspi_printf("write status failed, status 0x:%x\n", status);
        qspi_f25fl512s_clear_status(SPINOR_OP_CLRR);
        qspi_printf("clear and status	               : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
        qspi_wren(SPINOR_OP_WRDI);
		return QSPI_OP_FAILED;
    }
    
	return QSPI_OP_SUCCESS;
}

cyg_uint16 qspi_fl_read_registers(cyg_uint8 cmd)
{
    cyg_uint8 data[2];
	cyg_uint16 val=0;
    qspi_read_xx_data(cmd, 0, 0, data, 2);

	val = (data[0] | (data[1]<<8));
    return val;
}

/* DYB Read */
#define SPANSION_DYBRD	(0xE0)
/* DYB Write */
#define SPANSION_DYBWR	(0xE1)
/* PPB Read */
#define SPANSION_PPBRD	(0xE2)
/* PPB Program */
#define SPANSION_PPBP	(0xE3)
/* PPB Erase */
#define SPANSION_PPBE	(0xE4)
/* ASP Read */
#define SPANSION_ASPRD	(0x2B)
/* ASP Program */
#define SPANSION_ASPP	(0x2F)
/* PPB Lock Bit Read */
#define SPANSION_PLBRD	(0xA7)
/* PPB Lock Bit Write */
#define SPANSION_PLBWR	(0xA6)

#define	PASSWORD_MODE	(2)
#define	PERSISTENT_MODE	(4)
#define	MODE_MASK		((0x11)<1)

int qspi_s25fl512s_asp_mode_set(cyg_uint8 mode)
{
	int ret;
	cyg_uint16 st;

	if((mode != PASSWORD_MODE) && (mode != PERSISTENT_MODE))
	{
		qspi_printf("mode %d error!", mode);
		return QSPI_OP_FAILED;
	}

	st = qspi_fl_read_registers(SPANSION_ASPRD);
	qspi_printf("ASP Register value: 0x%x", st);
	if((st&0x6) == 0)
	{
		qspi_printf("Password mode and Persistent mode are all enabled!, error ...");
		return QSPI_OP_FAILED;
	}
		
	if((st&mode) == mode)
	{
		qspi_printf("mode is already enabled! val 0x%x", st);
		return 0;
	}

	if((st&0x6) != 6)
	{
		qspi_printf("It's already set to mode val 0x%x, don't to set to 0x%x, it will cause error!", st&0x6, mode);
		return 0;
	}

	st = (st & ~(MODE_MASK)) | mode;

	qspi_printf("write ASP Register to 0x%x", st);

	ret = qspi_s25fl512s_write_registers(SPANSION_ASPP, st);

	qspi_printf("done!");
    return ret;
}

int qspi_s25fl512s_asp_ppb_read(cyg_uint32 offset)
{
	cyg_uint8 data[2];
	int ret=0;

	qspi_printf("offset 0x%x\n", offset);
	ret = qspi_read_spansion_data(SPANSION_PPBRD, offset, 1, data, 2);

	if(!ret)
	{
		qspi_printf("read ppb failed!");
		return 0;
	}
	qspi_printf("offset 0x%x PPB %u %u", offset, (cyg_uint32)data[0], (cyg_uint32)data[1]);

	return data[0];
}

int qspi_s25fl512s_asp_ppb_program(cyg_uint32 addr)
{
    cyg_uint8 status;
    cyg_uint8 wbuf[5];
	int ret = QSPI_OP_SUCCESS;

	qspi_printf("enter!");
    qspi_wren(SPINOR_OP_WREN);

    wbuf[0] = SPANSION_PPBP;
    wbuf[1] = (addr >> 24) & 0xff;
    wbuf[2] = (addr >> 16) & 0xff;
    wbuf[3] = (addr >> 8) & 0xff;
    wbuf[4] = addr & 0xff;
    ret = qspi_send_xx_data(wbuf, 5);
	if(!ret)
	{
		return QSPI_OP_FAILED;
	}

	wait_cmd = SPANSION_PPBP;

    status = wait_flash_idle_extend(SPINOR_OP_RDSR, 0, 1, SR_E_ERR_SPAN, SR_E_ERR_SPAN,
                                    SR_P_ERR_SPAN, SR_P_ERR_SPAN);

    if ((status & SR_P_ERR_SPAN) == SR_P_ERR_SPAN)
	{
        qspi_printf("PPB program addr 0x%x failed, status 0x:%x\n", addr, status);
		ret = QSPI_OP_FAILED;
	}

    if (status & SR_WIP) {
        qspi_f25fl512s_clear_status(SPINOR_OP_CLRR);
        qspi_printf("clear and status	               : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
        qspi_wren(SPINOR_OP_WRDI);
        ret = QSPI_OP_FAILED;
    }

	qspi_printf("done!");
	return ret;
}

int qspi_s25fl512s_asp_ppb_erase(void)
{
	int ret = QSPI_OP_SUCCESS;
    cyg_uint8 status;
    cyg_uint8 wbuf[2];

    qspi_wren(SPINOR_OP_WREN);

    wbuf[0] = SPANSION_PPBE;
    ret = qspi_send_xx_data(wbuf, 1);
	if(!ret)
	{
		return QSPI_OP_FAILED;
	}

	wait_cmd = SPANSION_PPBE;
    status = wait_flash_idle_extend(SPINOR_OP_RDSR, 0, 1, SR_E_ERR_SPAN, SR_E_ERR_SPAN,
                                    SR_P_ERR_SPAN, SR_P_ERR_SPAN);

    if ((status & SR_P_ERR_SPAN) == SR_P_ERR_SPAN)
	{
        qspi_printf("failed! status 0x:%x\n", status);
		ret = QSPI_OP_FAILED;
	}

    if (status & SR_WIP) {
        qspi_f25fl512s_clear_status(SPINOR_OP_CLRR);
        qspi_printf("clear and status	               : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
        qspi_wren(SPINOR_OP_WRDI);
        ret = QSPI_OP_FAILED;
    }

    return ret;
}

int qspi_s25fl512s_asp_ppb_lockbit_read(void)
{
	cyg_uint8 data[2];

    qspi_read_xx_data(SPANSION_PLBRD, 0, 0, data, 2);

	qspi_printf("PPB lock bit status %u %u", (cyg_uint32)data[0], (cyg_uint32)data[1]);
	return 0;
}

cyg_uint8 qspi_s25fl512s_asp_dyb_read(cyg_uint32 offset)
{
	cyg_uint8 data[2];
	int ret=0;

	qspi_printf("offset 0x%x\n", offset);
	ret = qspi_read_spansion_data(SPANSION_DYBRD, offset, 1, data, 2);

	if(!ret)
	{
		qspi_printf("read dyb failed!");
		return 0;
	}
	qspi_printf("offset 0x%x DYB status %u %u", offset, (cyg_uint32)data[0], (cyg_uint32)data[1]);
	return data[0];
}

int qspi_s25fl512s_asp_dyb_program(cyg_uint32 addr, cyg_uint8 val)
{
	int ret = QSPI_OP_SUCCESS;
    cyg_uint8 status;
    cyg_uint8 wbuf[6];

	qspi_printf("enter!");
    qspi_wren(SPINOR_OP_WREN);

    wbuf[0] = SPANSION_DYBWR;
    wbuf[1] = (addr >> 24) & 0xff;
    wbuf[2] = (addr >> 16) & 0xff;
    wbuf[3] = (addr >> 8) & 0xff;
    wbuf[4] = addr & 0xff;
    wbuf[5] = val;
    ret = qspi_send_xx_data(wbuf, 6);
	if(!ret)
	{
		return QSPI_OP_FAILED;
	}

	wait_cmd = SPANSION_DYBWR;
    status = wait_flash_idle_extend(SPINOR_OP_RDSR, 0, 1, SR_E_ERR_SPAN, SR_E_ERR_SPAN,
                                    SR_P_ERR_SPAN, SR_P_ERR_SPAN);

    if ((status & SR_P_ERR_SPAN) == SR_P_ERR_SPAN)
	{
        qspi_printf("DYB program addr 0x%x failed, status 0x:%x\n", addr, status);
		ret = QSPI_OP_FAILED;
	}

    if (status & SR_WIP) {
        qspi_f25fl512s_clear_status(SPINOR_OP_CLRR);
        qspi_printf("clear and status	               : 0x%x\n", qspi_fl_read_status(SPINOR_OP_RDSR));
        qspi_wren(SPINOR_OP_WRDI);
        ret = QSPI_OP_FAILED;
    }

	qspi_printf("done!");
    return ret;
}

int qspi_s25fl512s_block_lock(cyg_uint32 addr)
{
	//qspi_s25fl512s_asp_ppb_read(addr);
	//qspi_s25fl512s_asp_ppb_program(addr);
	//qspi_s25fl512s_asp_ppb_read(addr);
	qspi_s25fl512s_asp_dyb_program(addr, 0);
	qspi_s25fl512s_asp_dyb_read(addr);
	return 0;
}

int qspi_s25fl512s_block_unlock(cyg_uint32 addr)
{
	qspi_s25fl512s_asp_ppb_erase();
	qspi_s25fl512s_asp_ppb_read(addr);
	qspi_s25fl512s_asp_dyb_program(addr, 0xff);
	qspi_s25fl512s_asp_dyb_read(addr);
	return 0;
}

int qspi_s25fl512s_read_block_status(cyg_uint32 addr)
{
	qspi_s25fl512s_asp_mode_set(PERSISTENT_MODE);
	qspi_s25fl512s_asp_ppb_lockbit_read();
	qspi_s25fl512s_asp_ppb_read(addr);
	return qspi_s25fl512s_asp_dyb_read(addr);
}
