//==========================================================================
//
//      i2c_dw_bus.c
//
//      I2C driver for DesignWare
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2008 Free Software Foundation, Inc.                        
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    jinyingwu 
// Date:         2018-01-12
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
#include <cyg/io/i2c.h>

#include <pkgconf/devs_bp2016_i2c_dw.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/kernel/kapi.h>
#include <errno.h>
#include <string.h>
#ifdef CYGHWR_HAL_ASIC_CLK
#include <cyg/hal/clk/clk.h>
#endif

#ifdef CYGPKG_DEVS_BP2016_I2C_DW

#include <cyg/io/i2c/i2c_dw_bus.h>

#define i2c_printf(fmt, args...)	\
	diag_printf("[I2C bus]: %s %d " fmt "\n", __func__, __LINE__, ##args);

//#define I2C_DEBUG

#ifdef I2C_DEBUG
#define i2c_debug(fmt, args...)		\
	diag_printf("[I2C dbg] %s %d: " fmt "\n", __func__, __LINE__,  ##args);
#else
#define i2c_debug(fmt, args...)
#endif

static char *abort_sources[] = {
	[ABRT_7B_ADDR_NOACK] =
		"slave address not acknowledged (7bit mode)",
	[ABRT_10ADDR1_NOACK] =
		"first address byte not acknowledged (10bit mode)",
	[ABRT_10ADDR2_NOACK] =
		"second address byte not acknowledged (10bit mode)",
	[ABRT_TXDATA_NOACK] =
		"data not acknowledged",
	[ABRT_GCALL_NOACK] =
		"no acknowledgement for a general call",
	[ABRT_GCALL_READ] =
		"read after general call",
	[ABRT_SBYTE_ACKDET] =
		"start byte acknowledged",
	[ABRT_SBYTE_NORSTRT] =
		"trying to send start byte when restart is disabled",
	[ABRT_10B_RD_NORSTRT] =
		"trying to read when restart is disabled (10bit mode)",
	[ABRT_MASTER_DIS] =
		"trying to use disabled adapter",
	[ARB_LOST] =
		"lost arbitration",
};

void print_i2c_regs(HWP_I2C_AP_T *hwp_i2c)
{
    diag_printf("\nDW_IC_CON\t= 0x%x\n", (hwp_i2c->dw_ic_con));
    diag_printf("DW_IC_TAR\t= 0x%x\n", (hwp_i2c->dw_ic_tar));
    diag_printf("DW_IC_INTR_MASK\t= 0x%x\n", (hwp_i2c->dw_ic_intr_mask));
    diag_printf("DW_IC_RAW_INTR_STAT = 0x%x\n", (hwp_i2c->dw_ic_raw_intr_stat));
    diag_printf("DW_IC_INTR_STAT\t= 0x%x\n", (hwp_i2c->dw_ic_intr_stat));
    diag_printf("DW_IC_ENABLE\t= 0x%x\n", (hwp_i2c->dw_ic_enable));
    diag_printf("DW_IC_STATUS\t= 0x%x\n", (hwp_i2c->dw_ic_status));
    diag_printf("DW_IC_TX_ABRT_SOURCE = 0x%x\n", (hwp_i2c->dw_ic_tx_abrt_source));
}

static inline cyg_uint32 i2c_dw_read_clear_intrbits(cyg_dw_i2c_bus* extra)
{
	HWP_I2C_AP_T *hwp_i2c;
    cyg_uint32 stat;

	hwp_i2c = extra->hwp_i2c;
    /*
     * The IC_INTR_STAT register just indicates "enabled" interrupts.
     * Ths unmasked raw version of interrupt status bits are available
     * in the IC_RAW_INTR_STAT register.
     *
     * That is,
     *   stat = dw_readl(IC_INTR_STAT);
     * equals to,
     *   stat = dw_readl(IC_RAW_INTR_STAT) & dw_readl(IC_INTR_MASK);
     *
     * The raw version might be useful for debugging purposes.
     */
    stat = dw_readl(hwp_i2c->dw_ic_intr_stat);

    /*
     * Do not use the IC_CLR_INTR register to clear interrupts, or
     * you'll miss some interrupts, triggered during the period from
     * dw_readl(IC_INTR_STAT) to dw_readl(IC_CLR_INTR).
     *
     * Instead, use the separately-prepared IC_CLR_* registers.
     */
    if (stat & DW_IC_INTR_RX_UNDER)
        dw_readl(hwp_i2c->dw_ic_clr_rx_under);
    if (stat & DW_IC_INTR_RX_OVER)
        dw_readl(hwp_i2c->dw_ic_clr_rx_over);
    if (stat & DW_IC_INTR_TX_OVER)
        dw_readl(hwp_i2c->dw_ic_clr_tx_over);
    if (stat & DW_IC_INTR_RD_REQ)
        dw_readl(hwp_i2c->dw_ic_clr_rd_req);
    if (stat & DW_IC_INTR_TX_ABRT) {
        /*
         * The IC_TX_ABRT_SOURCE register is cleared whenever
         * the IC_CLR_TX_ABRT is read.  Preserve it beforehand.
         */
        extra->abort_source = dw_readl(hwp_i2c->dw_ic_tx_abrt_source);
        dw_readl(hwp_i2c->dw_ic_clr_tx_abrt);
    }
    if (stat & DW_IC_INTR_RX_DONE)
        dw_readl(hwp_i2c->dw_ic_clr_rx_done);
    if (stat & DW_IC_INTR_ACTIVITY)
        dw_readl(hwp_i2c->dw_ic_clr_activity);
    if (stat & DW_IC_INTR_STOP_DET)
        dw_readl(hwp_i2c->dw_ic_clr_stop_det);
    if (stat & DW_IC_INTR_START_DET)
        dw_readl(hwp_i2c->dw_ic_clr_start_det);
    if (stat & DW_IC_INTR_GEN_CALL)
        dw_readl(hwp_i2c->dw_ic_clr_gen_call);

    return stat;
}

static void i2c_dw_read(struct cyg_dw_i2c_bus *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	int rx_valid;
	HWP_I2C_AP_T *hwp_i2c;
	hwp_i2c=dev->hwp_i2c;

	for (; dev->msg_read_idx < dev->msgs_num; dev->msg_read_idx++) {
		cyg_uint32 len;
		cyg_uint8 *buf;

		if (!(msgs[dev->msg_read_idx].flags & I2C_M_RD))
			continue;

		if (!(dev->status & STATUS_READ_IN_PROGRESS)) {
			len = msgs[dev->msg_read_idx].len;
			buf = msgs[dev->msg_read_idx].buf;
		} else {
			len = dev->rx_buf_len;
			buf = dev->rx_buf;
		}

		rx_valid = dw_readl(hwp_i2c->dw_ic_rxflr);

		for (; len > 0 && rx_valid > 0; len--, rx_valid--) {
			*buf++ = dw_readl(hwp_i2c->dw_ic_data_cmd);
			dev->rx_outstanding--;
		}

		if (len > 0) {
			dev->status |= STATUS_READ_IN_PROGRESS;
			dev->rx_buf_len = len;
			dev->rx_buf = buf;
			return;
		} else
			dev->status &= ~STATUS_READ_IN_PROGRESS;
	}
}

/*
 * Initiate (and continue) low level master read/write transaction.
 * This function is only called from i2c_dw_isr, and pumping i2c_msg
 * messages into the tx buffer.  Even if the size of i2c_msg data is
 * longer than the size of the tx buffer, it handles everything.
 */
static void i2c_dw_xfer_msg(struct cyg_dw_i2c_bus *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	cyg_uint32 intr_mask;
	int tx_limit, rx_limit;
	cyg_uint32 addr = msgs[dev->msg_write_idx].addr;
	cyg_uint32 buf_len = dev->tx_buf_len;
	cyg_uint8 *buf = dev->tx_buf;
	HWP_I2C_AP_T *hwp_i2c;

	hwp_i2c=dev->hwp_i2c;
	bool need_restart = false;

	intr_mask = DW_IC_INTR_DEFAULT_MASK;

	for (; dev->msg_write_idx < dev->msgs_num; dev->msg_write_idx++) {
		/*
		 * if target address has changed, we need to
		 * reprogram the target address in the i2c
		 * adapter when we are done with this transfer
		 */
		if (msgs[dev->msg_write_idx].addr != addr) {
			i2c_printf("%s: invalid target address\n", __func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (msgs[dev->msg_write_idx].len == 0) {
			i2c_printf("%s: invalid message length\n", __func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (!(dev->status & STATUS_WRITE_IN_PROGRESS)) {
			/* new i2c_msg */
			buf = msgs[dev->msg_write_idx].buf;
			buf_len = msgs[dev->msg_write_idx].len;

			/* If both IC_EMPTYFIFO_HOLD_MASTER_EN and
			 * IC_RESTART_EN are set, we must manually
			 * set restart bit between messages.
			 */
			if ((dev->master_cfg & DW_IC_CON_RESTART_EN) &&
					(dev->msg_write_idx > 0))
				need_restart = true;
		}

		tx_limit = dev->tx_fifo_depth - dw_readl(hwp_i2c->dw_ic_txflr);
		rx_limit = dev->rx_fifo_depth - dw_readl(hwp_i2c->dw_ic_rxflr);

		while (buf_len > 0 && tx_limit > 0 && rx_limit > 0) {
			cyg_uint32 cmd = 0;

			/*
			 * If IC_EMPTYFIFO_HOLD_MASTER_EN is set we must
			 * manually set the stop bit. However, it cannot be
			 * detected from the registers so we set it always
			 * when writing/reading the last byte.
			 */
			//IC_EMPTYFIFO_HOLD_MASTER_EN = 1 available
			if (dev->msg_write_idx == dev->msgs_num - 1 &&
			    buf_len == 1)
				cmd |= BIT(9); //stop

			if (need_restart) {
				cmd |= BIT(10); //restart
				need_restart = false;
			}

			if (msgs[dev->msg_write_idx].flags & I2C_M_RD) {

				/* avoid rx buffer overrun */
				if (rx_limit - dev->rx_outstanding <= 0)
					break;

				dw_writel(cmd | 0x100, hwp_i2c->dw_ic_data_cmd);
				rx_limit--;
				dev->rx_outstanding++;
			} else
				dw_writel(cmd | *buf++, hwp_i2c->dw_ic_data_cmd);
			tx_limit--; buf_len--;
		}

		dev->tx_buf = buf;
		dev->tx_buf_len = buf_len;

		if (buf_len > 0) {
			/* more bytes to be written */
			dev->status |= STATUS_WRITE_IN_PROGRESS;
			break;
		} else
			dev->status &= ~STATUS_WRITE_IN_PROGRESS;
	}

	/*
	 * If i2c_msg index search is completed, we don't need TX_EMPTY
	 * interrupt any more.
	 */
	if (dev->msg_write_idx == dev->msgs_num)
		intr_mask &= ~DW_IC_INTR_TX_EMPTY;

	if (dev->msg_err)
		intr_mask = 0;

	dw_writel(intr_mask,  hwp_i2c->dw_ic_intr_mask);
}

//==========================================================================
// The ISR does the actual work. It is not that much work to justify
// putting it in the DSR, and it is also not clear whether this would
// even work.  If an error occurs we try to leave the bus in the same
// state as we would if there was no error.
//==========================================================================
static cyg_uint32 dw_i2c_isr(cyg_vector_t vector, cyg_addrword_t data)
{
	cyg_uint32 stat, enabled;
    cyg_dw_i2c_bus* dev = (cyg_dw_i2c_bus*)data;

	HWP_I2C_AP_T *hwp_i2c;
	hwp_i2c = dev->hwp_i2c;

	enabled = dw_readl(hwp_i2c->dw_ic_enable);
	stat = dw_readl(hwp_i2c->dw_ic_raw_intr_stat);
	
	i2c_debug("enabled= 0x%x stat=0x%x\n", enabled, stat);

	if (!enabled || !(stat & ~DW_IC_INTR_ACTIVITY))
		return CYG_ISR_HANDLED;

	stat = i2c_dw_read_clear_intrbits(dev);

	if (stat & DW_IC_INTR_TX_ABRT) {
		i2c_printf("command error, TX ABRT");
		dev->cmd_err |= DW_IC_ERR_TX_ABRT;
		dev->status = STATUS_IDLE;

		/*
		 * Anytime TX_ABRT is set, the contents of the tx/rx
		 * buffers are flushed.  Make sure to skip them.
		 */
		dw_writel(0, hwp_i2c->dw_ic_intr_mask);
		goto tx_aborted;
	}

	if (stat & DW_IC_INTR_RX_FULL)
		i2c_dw_read(dev);

	if (stat & DW_IC_INTR_TX_EMPTY)
		i2c_dw_xfer_msg(dev);

	/*
	 * No need to modify or disable the interrupt mask here.
	 * i2c_dw_xfer_msg() will take care of it according to
	 * the current transmit status.
	 */

tx_aborted:

	if ((stat & (DW_IC_INTR_TX_ABRT | DW_IC_INTR_STOP_DET)) || dev->msg_err)
	{
		//cyg_drv_interrupt_mask(vector);
		cyg_drv_interrupt_mask_intunsafe(vector);
		cyg_drv_interrupt_acknowledge(vector);                                                                                                   
		return (CYG_ISR_HANDLED|CYG_ISR_CALL_DSR);
	}else{
		cyg_drv_interrupt_acknowledge(vector);
		return CYG_ISR_HANDLED;
	}
}

//==========================================================================
// DSR signals data
//==========================================================================
static void
dw_i2c_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    cyg_dw_i2c_bus* extra = (cyg_dw_i2c_bus*)data;
	cyg_drv_cond_signal(&extra->i2c_wait);
}

static inline void dw_i2c_clr_all_intr(HWP_I2C_AP_T *hwp_i2c)
{
    dw_readl(hwp_i2c->dw_ic_clr_intr);
    dw_readl(hwp_i2c->dw_ic_clr_rx_under);
    dw_readl(hwp_i2c->dw_ic_clr_rx_over);
    dw_readl(hwp_i2c->dw_ic_clr_tx_over);
    dw_readl(hwp_i2c->dw_ic_clr_rd_req);
    dw_readl(hwp_i2c->dw_ic_clr_tx_abrt);
    dw_readl(hwp_i2c->dw_ic_clr_rx_done);
    dw_readl(hwp_i2c->dw_ic_clr_activity);
    dw_readl(hwp_i2c->dw_ic_clr_stop_det);
    dw_readl(hwp_i2c->dw_ic_clr_start_det);
    dw_readl(hwp_i2c->dw_ic_clr_gen_call);

    dw_readl(hwp_i2c->dw_ic_clr_restart_det);
    dw_readl(hwp_i2c->dw_ic_clr_scl_stuck_det);
    dw_readl(hwp_i2c->dw_ic_clr_smbus_intr);
}

static void __i2c_dw_enable(HWP_I2C_AP_T *hwp_i2c, bool enable)
{
	int timeout = 100;

	do {
		dw_writel(enable, hwp_i2c->dw_ic_enable);
		if ((dw_readl(hwp_i2c->dw_ic_enable_status) & 1) == enable)
			return;

		/*
		 * Wait 10 times the signaling period of the highest I2C
		 * transfer supported by the driver (for 400KHz this is
		 * 25us) as described in the DesignWare I2C databook.
		 */
		HAL_DELAY_US(25);
	} while (timeout--);
}

// this is configs for 95MHz clk input i2c, api set scm dividor to 95MHz
#define     STANDARD_HIGH_SPEED     0x1a0
#define     STANDARD_LOW_SPEED      0x210
#define     FAST_HIGH_SPEED         0x5e
#define     FAST_LOW_SPEED          0x82
/**
 * i2c_dw_init() - initialize the designware i2c master hardware
 * @dev: device private data
 *
 * This functions configures and enables the I2C master.
 * This function is called during I2C init function, and in case of timeout at
 * run time.
 */
int i2c_dw_init(HWP_I2C_AP_T *hwp_i2c, const unsigned int mode)
{
    unsigned int ic_con = mode;

	/* Disable the adapter */
	__i2c_dw_enable(hwp_i2c, false);

	/* Configure Tx/Rx FIFO threshold levels */
	dw_writel(6, hwp_i2c->dw_ic_tx_tl);
	dw_writel(0, hwp_i2c->dw_ic_rx_tl);

    dw_writel(STANDARD_HIGH_SPEED, hwp_i2c->dw_ic_ss_scl_hcnt);
    dw_writel(STANDARD_LOW_SPEED, hwp_i2c->dw_ic_ss_scl_lcnt);
    dw_writel(FAST_HIGH_SPEED, hwp_i2c->dw_ic_fs_scl_hcnt);
    dw_writel(FAST_LOW_SPEED, hwp_i2c->dw_ic_fs_scl_lcnt);

	/* configure the i2c master */
	//dw_writel(dev, dev->master_cfg , DW_IC_CON);
    ic_con |= DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE | DW_IC_CON_RESTART_EN;
    dw_writel(ic_con, hwp_i2c->dw_ic_con);
    
	// print_i2c_regs(hwp_i2c);
	return 0;
}

static void i2c_dw_set_speed(HWP_I2C_AP_T *hwp_i2c, cyg_uint8 mode)
{
    unsigned int ic_con = mode;

	ic_con = dw_readl(hwp_i2c->dw_ic_con);
	ic_con &= ~(6);		//bit 1,2 
	ic_con |= mode;
	dw_writel(ic_con, hwp_i2c->dw_ic_con);
}

void i2c_dw_xfer_init(HWP_I2C_AP_T *hwp_i2c, unsigned char slv_addr, cyg_uint8 i2c_mode)
{

	/* Disable the adapter */
	__i2c_dw_enable(hwp_i2c, false);

    dw_writel(slv_addr, hwp_i2c->dw_ic_tar);

	i2c_dw_set_speed(hwp_i2c, i2c_mode);
	/* enforce disabled interrupts (due to HW issues) */
    dw_writel(0, hwp_i2c->dw_ic_intr_mask);

	/* Enable the adapter */
	__i2c_dw_enable(hwp_i2c, true);
    //dw_writel(1, hwp_i2c->dw_ic_enable);

	/* Clear and enable interrupts */
	dw_i2c_clr_all_intr(hwp_i2c);
	dw_writel(DW_IC_INTR_DEFAULT_MASK, hwp_i2c->dw_ic_intr_mask);
}

//==========================================================================
// Initialize driver & hardware state
//==========================================================================
void cyg_dw_i2c_init(struct cyg_i2c_bus *bus)
{
    cyg_dw_i2c_bus* extra = (cyg_dw_i2c_bus*)bus->i2c_extra;
  
    cyg_drv_mutex_init(&extra->i2c_lock);
    cyg_drv_cond_init(&extra->i2c_wait, &extra->i2c_lock);
    cyg_drv_interrupt_create(extra->i2c_isrvec,
                             extra->i2c_isrpri,
                             (cyg_addrword_t) extra,
                             &dw_i2c_isr,
                             &dw_i2c_dsr,
                             &(extra->i2c_interrupt_handle),
                             &(extra->i2c_interrupt_data));
    cyg_drv_interrupt_attach(extra->i2c_interrupt_handle);
  
    //
    // Init I2C bus
    //
	i2c_dw_init(extra->hwp_i2c, extra->i2c_mode);
}

/*
 * Waiting for bus not busy
 */
#define TIMEOUT			(50)
static int i2c_dw_wait_bus_not_busy(HWP_I2C_AP_T *hwp_i2c)
{
	int timeout = TIMEOUT;

	while (dw_readl(hwp_i2c->dw_ic_status) & DW_IC_STATUS_ACTIVITY) {
		if (timeout <= 0) {
			i2c_printf("timeout waiting for bus ready\n");
			return -ETIMEDOUT;
		}
		timeout--;
		HAL_DELAY_US(100);
	}

	return 0;
}

static int i2c_dw_handle_tx_abort(cyg_dw_i2c_bus *dev)
{
	unsigned long abort_source = dev->abort_source;
	int i;

    i2c_debug("abort source 0x%x", dev->abort_source);
	if (abort_source & DW_IC_TX_ABRT_NOACK) {
		//for_each_set_bit(i, &abort_source, ARRAY_SIZE(abort_sources))
		for(i=0; i<13; i++)
		{
			if(abort_source &(1<<i))
				i2c_printf("%s\n", abort_sources[i]);

		}
		return -EREMOTEIO;
	}

	//for_each_set_bit(i, &abort_source, ARRAY_SIZE(abort_sources))
	for(i=0; i<13; i++)
	{
		if(abort_source &(1<<i))
			i2c_printf("%s: %s\n", __func__, abort_sources[i]);
	}

	if (abort_source & DW_IC_TX_ARB_LOST)
		return -EAGAIN;
	else if (abort_source & DW_IC_TX_ABRT_GCALL_READ)
		return -EINVAL; /* wrong msgs[] data */
	else
		return -EIO;
}


/*
 * Prepare controller for a transaction and call i2c_dw_xfer_msg
 */
//int i2c_dw_xfer(cyg_dw_i2c_bus *dev, struct i2c_msg msgs[], int num)
int i2c_dw_xfer(cyg_dw_i2c_bus *dev, cyg_uint32 count)
{
	int ret;
	struct i2c_msg *msgs;
	//i2c_debug("msgs: %d", num);
#ifdef CYGHWR_HAL_ASIC_DRV_LOW_POWER    
    CLK_ID_TYPE_T id_type = CLK_I2C0;
#ifdef CYGVAR_DEVS_BP2016_I2C_BUS0_EN
    if((dev->hwp_i2c) == hwp_apI2c0){
        id_type = CLK_I2C0;
    }
#endif
#ifdef CYGVAR_DEVS_BP2016_I2C_BUS1_EN
    if((dev->hwp_i2c) == hwp_apI2c1){
        id_type = CLK_I2C1;
    }
#endif
    hal_clk_enable(id_type);
#endif

    cyg_drv_mutex_lock(&dev->i2c_lock);

	//dev->msgs = msgs;
	msgs = dev->msgs;
	//dev->msgs_num = num;
	dev->cmd_err = 0;
	dev->msg_write_idx = 0;
	dev->msg_read_idx = 0;
	dev->msg_err = 0;
	dev->status = STATUS_IDLE;
	dev->abort_source = 0;
	dev->rx_outstanding = 0;

	ret = i2c_dw_wait_bus_not_busy(dev->hwp_i2c);
	if (ret < 0)
		goto done;

	/* start the transfers */
	i2c_dw_xfer_init(dev->hwp_i2c, msgs[0].addr, dev->i2c_mode);
    //print_i2c_regs(dev->hwp_i2c);

    cyg_drv_dsr_lock();
    cyg_drv_interrupt_unmask(dev->i2c_isrvec);

	/* wait for tx to complete */
	cyg_drv_cond_wait(&dev->i2c_wait);

    cyg_drv_interrupt_mask(dev->i2c_isrvec);
    cyg_drv_dsr_unlock();
	/*
	 * We must disable the adapter before unlocking the &dev->lock mutex
	 * below. Otherwise the hardware might continue generating interrupts
	 * which in turn causes a race condition with the following transfer.
	 * Needs some more investigation if the additional interrupts are
	 * a hardware bug or this driver doesn't handle them correctly yet.
	 */
	__i2c_dw_enable(dev->hwp_i2c, false);

	if (dev->msg_err) {
		ret = dev->msg_err;
		goto done;
	}

	/* no error */
	if (!dev->cmd_err) {
		ret = count;
		goto done;
	}

	/* We have an error */
	if (dev->cmd_err == DW_IC_ERR_TX_ABRT) {
		ret = i2c_dw_handle_tx_abort(dev);
		goto done;
	}
	ret = -EIO;

done:
    cyg_drv_mutex_unlock(&dev->i2c_lock);
	i2c_debug("done!");

#ifdef CYGHWR_HAL_ASIC_DRV_LOW_POWER    
#ifdef CYGVAR_DEVS_BP2016_I2C_BUS0_EN
    if((dev->hwp_i2c) == hwp_apI2c0){
        id_type = CLK_I2C0;
    }
#endif
#ifdef CYGVAR_DEVS_BP2016_I2C_BUS1_EN
    if((dev->hwp_i2c) == hwp_apI2c1){
        id_type = CLK_I2C1;
    }
#endif
    hal_clk_disable(id_type);
#endif

	return ret;
}

//==========================================================================
// transmit a buffer to a device
//==========================================================================
cyg_uint32 cyg_dw_i2c_tx(const cyg_i2c_device *dev, 
                              cyg_bool              send_start, 
                              const cyg_uint8      *tx_data, 
                              cyg_uint32            count, 
                              cyg_bool              send_stop)
{
	int ret;
    cyg_dw_i2c_bus* extra = 
                           (cyg_dw_i2c_bus*)dev->i2c_bus->i2c_extra;
	struct i2c_msg *msg;

	msg = extra->msgs;

	memset(msg, 0, sizeof(msg));

	msg->addr = dev->i2c_address;
	//msg.flags = I2C_M_RD;
	msg->len = count;
	msg->buf = (cyg_uint8 *)tx_data;
	extra->msgs_num = 1;

	if(!send_stop)
	{
		extra->repeat = 1;
		i2c_debug("need repeat! addr 0x%x len %d", msg->addr, msg->len);
		return count;
	}else{
		extra->repeat = 0;
	}

    ret = i2c_dw_xfer(extra, count);

	i2c_debug("done!");
  
	return (ret == 1) ? count : ret;
}

//==========================================================================
// receive into a buffer from a device
//==========================================================================
cyg_uint32 cyg_dw_i2c_rx(const cyg_i2c_device *dev,
                              cyg_bool              send_start,
                              cyg_uint8            *rx_data,
                              cyg_uint32            count,
                              cyg_bool              send_nak,
                              cyg_bool              send_stop)
{
    cyg_dw_i2c_bus* extra = 
                           (cyg_dw_i2c_bus*)dev->i2c_bus->i2c_extra;
	struct i2c_msg *msg;
	int ret=0;
  
	msg = extra->msgs;
	if(1 == extra->repeat)
	{
		msg++;
		extra->msgs_num++;
	}else{
		extra->msgs_num = 1;
	}

	memset(msg, 0, sizeof(msg));
	msg->addr = dev->i2c_address;
	msg->flags = I2C_M_RD;
	msg->len = count;
	msg->buf = rx_data;

	i2c_debug("addr 0x%x len %d, msgs_num %d", msg->addr, msg->len, extra->msgs_num);

	if(!send_stop)
	{
		extra->repeat = 1;
	}else{
		extra->repeat = 0;
	}

    ret = i2c_dw_xfer(extra,  count);

	i2c_debug("done! ret = %d", ret);

	return (ret == 1) ? count : ret;
}


//==========================================================================
//  generate a STOP
//==========================================================================
void cyg_dw_i2c_stop(const cyg_i2c_device *dev)
{
    cyg_dw_i2c_bus* extra = 
                           (cyg_dw_i2c_bus*)dev->i2c_bus->i2c_extra;
    //extra = extra; // avoid compiler warning in case of singleton

	extra->cmd_err = 0;
	extra->msg_write_idx = 0;
	extra->msg_read_idx = 0;
	extra->msg_err = 0;
	extra->status = STATUS_IDLE;
	extra->abort_source = 0;
	extra->rx_outstanding = 0;
	extra->repeat = 0;
}

#ifdef CYGVAR_DEVS_BP2016_I2C_BUS0_EN

cyg_dw_i2c_bus	bp2016_i2c_bus0_dev = {
  hwp_i2c	   : hwp_apI2c0,									
  i2c_isrvec   : SYS_IRQ_ID_I2C0,
  i2c_isrpri   : INT_PRI_DEFAULT,
  i2c_mode	   : DW_IC_CON_SPEED_FAST,
  cmd_err	   : 0,
  msgs_num     : 0,
  msg_err      : 0,
  status	   : 0,
  tx_fifo_depth : 8,
  rx_fifo_depth : 8,
  tx_buf    :  NULL,
  rx_buf    :  NULL, 
  }; 

CYG_I2C_BUS(bp2016_i2c_bus0,
		    cyg_dw_i2c_init,
		    cyg_dw_i2c_tx,
			cyg_dw_i2c_rx,
			cyg_dw_i2c_stop,
			(void*) &(bp2016_i2c_bus0_dev));

#endif	//CYGVAR_DEVS_BP2016_I2C_BUS0_EN

#ifdef CYGVAR_DEVS_BP2016_I2C_BUS1_EN

cyg_dw_i2c_bus	bp2016_i2c_bus1_dev = {
  hwp_i2c	   :  hwp_apI2c1,									
  i2c_isrvec   : SYS_IRQ_ID_I2C1,
  i2c_isrpri   : INT_PRI_DEFAULT,
  i2c_mode	   : DW_IC_CON_SPEED_FAST,
  cmd_err	   : 0,
  msgs_num     : 0,
  msg_err      : 0,
  status	   : 0,
  tx_fifo_depth : 8,
  rx_fifo_depth : 8,
  tx_buf    :  NULL,
  rx_buf    :  NULL, 
  }; 

CYG_I2C_BUS(bp2016_i2c_bus1,
		    cyg_dw_i2c_init,
		    cyg_dw_i2c_tx,
			cyg_dw_i2c_rx,
			cyg_dw_i2c_stop,
			(void*) &(bp2016_i2c_bus1_dev));

#endif //CYGVAR_DEVS_BP2016_I2C_BUS1_EN

#endif //CYGPKG_DEVS_BP2016_I2C_DW
//---------------------------------------------------------------------------
