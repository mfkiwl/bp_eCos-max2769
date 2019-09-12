#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <pkgconf/io.h>
#include <pkgconf/io_disk.h>

#include <cyg/io/io.h>
#include <cyg/io/devtab.h>
#include <cyg/io/disk.h>
#include <cyg/infra/cyg_ass.h>      // assertion support
#include <cyg/infra/diag.h>         // diagnostic output
#include <cyg/hal/hal_if.h>

#include <cyg/io/mmc/bitops.h>
#include <cyg/io/mmc/list.h>
#include <cyg/io/mmc/mmc.h>
#include <cyg/io/mmc/card.h>
#include <cyg/io/mmc/host.h>
#include <cyg/io/mmc/mmc_common.h>
#include <cyg/hal/a7/cortex_a7.h>


#include "evatronix_host.h"
#include "evatronix_host_struct.h"
#include "evatronix_ops.h"

/**************************************************************************/
#define DRIVER_NAME     "bp2016-sdmmc"
#define evatronix_sd_vcode "v3.2 2018-Sep-07"


#define TEST_SLOT (-1)

#ifdef DATA_ERROR_DEBUG
#define	GPIONUM		(8+32+22)	//R386, portb[22]
#endif

#define			MEMMAX0_TUNING

#define			WIFI_OOB
#define			REQUEST_STAT

/* specify your SDIO-capable slot */
#define	SDIO_SLOT_ID	(1)
#define 		SUPPORT_MULTI_SLOTS

//#define			PRCM_SUPPORT

/* if single slot, specify your slot id here */
#define 	SLOT_ID		(-101)

#ifdef	PRCM_SUPPORT
#	define	CLK_CONID	"bp2016_sd"
#endif

/**************************************************************************/
static int sdio_trace_param = -1;

volatile unsigned int unused = 0xffffffff;

//static bool s3_test = false;
static bool cd_test = false;
/**************************************************************************/

#define EVA_CD_THREAD_PRI   (4)
#define EVA_FUNC_THREAD_PRI   (6)

static void evatronix_remove_slot(struct evatronix_slot *slot);
static void evatronix_serve_request(struct evatronix_host *host, struct evatronix_slot *slot);
//static void evatronix_tasklet_cd(unsigned long data);

static void s3_show_regs(struct evatronix_host *host, int id);

static u8 check_dma_desc(struct evatronix_slot *slot, struct mmc_data *data, int log);

static void
evatronix_finish_request(struct evatronix_host *host, struct evatronix_slot *slot, struct mmc_request *mrq);
static void
evatronix_start_request(struct evatronix_host *host, struct evatronix_slot *slot, struct mmc_request *req);

int slot_attr_init(struct evatronix_sdio_slot_data *sd,
					struct mmc_host *mmc, unsigned int id);

struct evtx_mmc_platform_data evtx_sdmmc_data = {
	.ref_clk		= 95000000,
	.nr_slots 		= 2,
	.detect_delay_ms	= 2*1000,
	.slot_attr_init		= slot_attr_init,

	.slots[0] = {
		.ctype       	= SD_CARD, //EMMC_CARD,
		//.force_rescan	= true,
		//.caps		= (MMC_CAP_NONREMOVABLE|
		//			MMC_CAP_4_BIT_DATA/*|MMC_CAP_MMC_HIGHSPEED*/),
		.freq 		= 95000000/2,
		.ocr_avail	= 0xff8000,
	},

	.slots[1] = {
		.ctype       	= SD_CARD,
		.freq 		= 25000000/2,
	},
};

static inline void udelay(int delay)
{
    CYGACC_CALL_IF_DELAY_US(delay);
}

static inline void mdelay(int delay)
{
    udelay(1000 * delay);
}

int slot_attr_init(struct evatronix_sdio_slot_data *sd,
					struct mmc_host *mmc, unsigned int id)
{
	if(!sd || id > 3)
		return -EINVAL;

	mmc->caps 	= 0;
	mmc->f_min	= 400000;

	mmc->f_max 	= sd->freq;
	mmc->ocr_avail 	= sd->ocr_avail;
	mmc->caps 	= sd->caps;
	mmc->pm_caps 	= sd->pm_caps;

	printk("slot%u: f_min %d, f_max %d, ocr_avial 0x%x, caps 0x%lx, pm_caps 0x%x",
				id, mmc->f_min, mmc->f_max,
				mmc->ocr_avail, mmc->caps, mmc->pm_caps);
	return 0;
}

void evatronix_host_reg(struct evatronix_host *host)
{
	diag_printf("HSFR0: 0x%08x\n", host_readl(host, HSFR0));
	diag_printf("HSFR1: 0x%08x\n", host_readl(host, HSFR1));
	diag_printf("HSFR2: 0x%08x\n", host_readl(host, HSFR2));
}

/*
 * keep in mind, make this as simple as possible,
 * or eMMC will suffer cuz 5C1 is piece of ...
 */
void evatronix_dbg(struct mmc_host *mmc)
{
	int id;
	struct evatronix_slot  *slot = mmc_priv(mmc);
	struct evatronix_host *host = slot->host;
	struct mmc_data *data = slot->mrq->data;

	id = slot->id;

	printk( "%s: slot%d, CMD%d, flag %d\n", __func__, id,
			slot->mrq->cmd->opcode, host->flag);
	printk( "\t*** irq_status       0x%08x\n", slot->irq_status);
	printk( "\t*** pending_events   0x%08lx\n", slot->pending_events);
	printk( "\t*** completed_events 0x%08lx\n", slot->completed_events);
	printk( "\t*** step		0x%x\n", slot->step);
	printk( "\t*** SFR02            0x%08x\n", slot_readl(host, SFR2, id));
	printk( "\t*** SFR03            0x%08x\n", slot_readl(host, SFR3, id));
	printk( "\t*** SFR10            0x%08x\n", slot_readl(host, SFR10, id));
	printk( "\t*** SFR11            0x%08x\n", slot_readl(host, SFR11, id));
	printk( "\t*** STATE            0x%x\n", host->state);
	//printk( "\t*** tasklet->state   0x%x\n", host->tasklet.state);
	//printk( "\t*** tasklet->count   0x%x\n", atomic_read(&(host->tasklet.count)));

#ifdef DEBUG
	printk( "\t*** prev slot%d CMD%d", host->prev_slot, host->prev_cmd);
#endif
	if(slot->mrq->data) {
		printk( "\t *** blksz = %d, blocks = %d, %s\n",
				data->blksz, data->blocks,
				(data->flags & MMC_DATA_WRITE) ? "write":"read");
		printk( "\t*** SFR01            0x%08x\n", slot_readl(host, SFR1, id));
		printk( "\t*** SFR02            0x%08x\n", slot_readl(host, SFR2, id));
		printk( "\t*** SFR21            0x%08x\n", slot_readl(host, SFR21, id));
		printk( "\t*** SFR22            0x%08x\n", slot_readl(host, SFR22, id));
	}

#if 1
    int idx;
	for(idx = 0; idx < 23; idx++)
		err("\t *** SFR%02u 0x%08x", idx,
				__raw_readl((host)->virt_base + (4*idx) + (id+1)*(0x100)));
#endif
}
//EXPORT_SYMBOL(evatronix_dbg);

/* can be called only when data present */
static void crime_scene(struct evatronix_host *host, int id)
{
	int i, idx;
	BUG_ON(host->cur_slot == 0xff);

	struct mmc_data *data = host->slots[host->cur_slot]->mrq->data;
	//struct scatterlist *sg = data->sg;

	if(!data)
		return;

	err(">>>>>>>>>>>>> slot%u CRIME SCENE <<<<<<<<<<<<<", id);
	err("\t*** blksz = %d, blocks = %d, %s\n",
			data->blksz, data->blocks,
			(data->flags & MMC_DATA_WRITE) ? "write":"read");
	for(idx = 0; idx < 23; idx++)
		err("\t *** SFR%02u 0x%08x", idx,
				__raw_readl((host)->virt_base + (4*idx) + (id+1)*(0x100)));

	for(i = 0; i < host->blocks; i++) { 
		err("\t *** 0x%08x 0x%08x\n", host->desc[i].desc_1, host->desc[i].desc_2);
	}

	check_dma_desc(host->slots[host->cur_slot], data, 1);
}

#ifdef	ERROR_RECOVERY

static void evatronix_send_stop_cmd(struct evatronix_host *host, struct evatronix_slot *slot)
{
	ENTER();

	err("\t ### SEND ABORT CMD12");
	slot_writel(host, SFR2, slot->id, 0);
#if 0
	u32 cnt = 0;
	status = slot_readl(host, SFR9, slot->id);
	while(status & 0x1) {
		udelay(10);
		if(cnt ++ > 100) {
			err("slot%u: send stop cmd TO", slot->id);
		}
	}
#endif
	//slot_writel(host, SFR3, slot->id, 0x0c1b0000);
	slot_writel(host, SFR3, slot->id, 0x0cdb0000);

	LEAVE();
}

#define	SRCMD	1
#define	SRDAT	2
#define	SRFA	4

static void soft_reset(struct evatronix_host *host, struct evatronix_slot *slot, int flag)
{
	u32 srdat = 0x4000000;
	u32 srcmd = 0x2000000;
	u32 srfa  = 0x1000000;
	u32 val = 0;
	u32 cnt = 0;

	if(flag == SRCMD) {
		err("\t ### START SRCMD ");
		val = slot_readl(host, SFR11, slot->id);
		val |= srcmd;
		while( (slot_readl(host, SFR11, slot->id) & srcmd) != 0) {
			udelay(2);
			cnt++;
			if(cnt > 100)
				err("slot%u: SRCMD TO");
		}

		val = slot_readl(host, SFR12, slot->id);
		if(val & 0xF0000) {
			slot_writel(host, SFR12, slot->id, 0xF0000);
		}

	} else if(flag == SRDAT) {
		err("\t ### START SRDAT");
		val = slot_readl(host, SFR11, slot->id);
		val |= srdat;
		while( (slot_readl(host, SFR11, slot->id) & srdat) != 0) {
			udelay(2);
			cnt++;
			if(cnt > 100)
				err("\t ### SRDAT TO");
		}

		val = slot_readl(host, SFR12, slot->id);
		if(val & 0x700000) {
			slot_writel(host, SFR12, slot->id, 0x700000);
		}

	} else if(flag == SRFA) {
		err("\t ### START SRFA");
		val = slot_readl(host, SFR11, slot->id);
		val |= srfa;
		while( (slot_readl(host, SFR11, slot->id) & srfa) != 0) {
			udelay(2);
			cnt++;
			if(cnt > 100)
				err("\t ### SRFA TO");
		}
	} else {
		err("\t ### UNSUPPORTED RESET FLAG %d", slot->id, flag);
	}
}


static void evatronix_error_recovery(struct evatronix_host *host,
		struct evatronix_slot *slot, struct mmc_request *mrq)
{
	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data	*data = mrq->data;
	int id = slot->id;

	u32 val = 0;

	ENTER();
	//disable interrupts
	slot_writel(host, SFR13, id, 0xFFFFFFFF);
	slot_writel(host, SFR14, id, 0xffffffff);
	err("slot%u: DISABLE INTERRUPTS", id);

	val = slot_readl(host, SFR12, id);
	err("\t ### SFR12 0x%08x", val);

	/* EAC */
	if(val & 0x1000000 ) {
		val = slot_readl(host, SFR15, id);
		err("\t ### SFR15 0x%08x", val);

		/* ACNE */
		if(val & 0x1) {
			err("\t ### ACNE");
			goto reset;
		} else {
			soft_reset(host, slot, SRCMD);
			evatronix_send_stop_cmd(host, slot);

			/* any erro after abort? */
			if(0) {
				err("\t ### UNRECOVERABLE ERROR No.1", id);
				//BUG();
			}

			soft_reset(host, slot, SRDAT);
		}

	} else {
reset:
		if(cmd->error)
			soft_reset(host, slot, SRCMD);
		if(data->error)
			soft_reset(host, slot, SRDAT);

		evatronix_send_stop_cmd(host, slot);
		/* any error after abort? */
		val = slot_readl(host, SFR12, id);
		if(val & 0x8000) {
			err("\t ### UNRECOVERABLE ERROR No.2", id);
			//BUG();
		}
		udelay(40*2);

		val = slot_readl(host, SFR9, id);
		err("\t ### SFR9 0x%08x", val);
		//enable interrupts now
		if( ((val & 0xF00000) >> 20) == 0xF) {
			slot_writel(host, SFR14, id, 0xFFFFFeFF);
		} else {
			err("\t ### UNRECOVERABLE ERROR No.3", id);
			//BUG();
		}

		//OK, enable interrupts now
		slot_writel(host, SFR13, id, 0xFFFFFFFF);
		slot_writel(host, SFR14, id, 0xFFFFFFFF);
	}

	LEAVE();
}


static int sync_abort(struct evatronix_host *host, struct evatronix_slot *slot)
{
	u32 val = 0;

	val = slot_readl(host, SFR10, slot->id);
	/* IBG */
	val |= 0x80000;
	slot_writel(host, SFR10, slot->id, val);

	/* TC: what if we can NOT get this ?*/
	if(slot->irq_status & 0x2) {
		evatronix_send_stop_cmd(host, slot);
		soft_reset(host, slot, SRCMD);
		soft_reset(host, slot, SRDAT);
		return 0;
	}

	err("slot%u: SYNC_ABORT state error", slot->id);
	return -EINPROGRESS;
}

static int async_abort(struct evatronix_host *host, struct evatronix_slot *slot)
{
	u32 cnt = 0;

	err("slot%u; trying ASYNC_ABORT", slot->id);
	while(slot_readl(host, SFR9, slot->id) & 0x1) {
		udelay(2);
		cnt++;
		if(cnt > 100) {
			err("slot%u: Conmmand Inhibit", slot->id);
			return -EBUSY;
		}
	}

	/* Yeah, we do NOT give a f* */
	evatronix_send_stop_cmd(host, slot);
	soft_reset(host, slot, SRCMD);
	soft_reset(host, slot, SRDAT);

	return 0;
}
#endif

static void evatronix_tasklet_func(struct evatronix_host *host)
{
	//struct evatronix_host *host = (struct evatronix_host *)private;

#ifndef	SUPPORT_MULTI_SLOTS
	host->cur_slot = SLOT_ID;
#endif

	struct mmc_data 	*data;
	struct mmc_command	*cmd;
	struct mmc_request	*req;
	ENTER();

	enum evatronix_state	state;
	enum evatronix_state	prev_state;

	cyg_drv_mutex_lock(&host->lock);
	if((host->state == STATE_IDLE) || (host->cur_slot == 0xff) ) {
		/*
		 * just return if one request was broken
		 * see tasklet_action for detail
		 */
		printk( "%s: state %x, cur_slot %x\n" ,
					__func__, host->state, host->cur_slot);

		cyg_drv_mutex_unlock(&host->lock);
		return;
	}
	struct evatronix_slot *slot = host->slots[host->cur_slot];

	state 	= host->state;
	data 	= slot->data;
	cmd 	= slot->cmd;
	req 	= slot->mrq;

	do {
		prev_state = state;

		switch(state) {
			case STATE_IDLE:
				break;

			case STATE_SENDING_CMD:
				VDBG("slot%u: S SENDING_CMD, PEND 0x%016lx", slot->id, slot->pending_events);

				if(!test_and_clear_bit(EVENT_CMD_COMPLETE, &slot->pending_events))
					break;

				//slot->cmd = NULL;
				set_bit(EVENT_CMD_COMPLETE, &slot->completed_events);

				VDBG("slot%u: S SENDING_CMD, COMP 0x%016lx", slot->id, slot->completed_events);
				/* if CMD error, drop all data transfer attempt */
				if(cmd->error) {

					evatronix_finish_request(host, slot, req);
					goto unlock;
				} else if (!data) {
					if(mmc_resp_type(cmd) == MMC_RSP_R1B) {
						prev_state = state;
						state = STATE_BUSY_RSP;
						break;
					}
					else {
						evatronix_finish_request(host, slot, req);
						goto unlock;
					}
				} else
					prev_state = state = STATE_SENDING_DATA;

			case STATE_SENDING_DATA:
				VDBG("slot%u: S SENDING_DATA, PEND 0x%016lx", slot->id, slot->pending_events);

				if(test_and_clear_bit(EVENT_DATA_ERROR, &slot->pending_events)) {

					evatronix_finish_request(host, slot, req);
					goto unlock;
				}

				if(!test_and_clear_bit(EVENT_DMA_FIN, &slot->pending_events)) {
					break;
				}

				set_bit(EVENT_DMA_FIN, &slot->completed_events);
				VDBG("slot%u: S SENDING_DATA, COMP 0x%016lx", slot->id, slot->completed_events);
				prev_state = state = STATE_DATA_BUSY;

			case STATE_DATA_BUSY:

				VDBG("slot%u: S DATA_BUSY, PEND 0x%016lx", slot->id, slot->pending_events);
				if(test_and_clear_bit(EVENT_DATA_ERROR, &slot->pending_events)) {

#if 0
					/* you have to clean the crap */
					if(slot->data->flags & MMC_DATA_WRITE) {
						if(!test_and_clear_bit(EVENT_XFER_COMPLETE, &slot->pending_events))
							break;
						set_bit(EVENT_XFER_COMPLETE, &slot->completed_events);
					}
#endif
					evatronix_finish_request(host, slot, req);
					goto unlock;
				}

				if(!test_and_clear_bit(EVENT_XFER_COMPLETE, &slot->pending_events) )
					break;

				slot->data 	= NULL;
				set_bit(EVENT_XFER_COMPLETE, &slot->completed_events);
				VDBG("slot%u: S DATA_BUSY, COMP 0x%016lx", slot->id, slot->completed_events);

				data->bytes_xfered = data->blocks * data->blksz;
				data->error = 0;
				VDBG("slot%u: %d bytes transfered", slot->id, data->bytes_xfered);

				evatronix_finish_request(host, slot, req);
				goto unlock;
#if 0
			case STATE_SENDING_STOP:
				VDBG("slot%u: S SENDING_STOP, PEND 0x%08lx", slot->id, slot->pending_events);
				if(!test_and_clear_bit(EVENT_CMD_COMPLETE, &slot->pending_events))
					break;

				//slot->cmd = NULL;
				set_bit(EVENT_CMD_COMPLETE, &slot->completed_events);
				VDBG("slot%u: S SENDING_STOP, COMP 0x%08lx", slot->id, slot->completed_events);
				evatronix_finish_request(host, slot, req);
				goto unlock;
#endif
			case STATE_BUSY_RSP:
				VDBG("slot%u: S BUSY_RSP,     PEND 0x%016lx", slot->id, slot->pending_events);

				if(!test_and_clear_bit(EVENT_XFER_COMPLETE, &slot->pending_events))
					break;

				set_bit(EVENT_XFER_COMPLETE, &slot->completed_events);
				VDBG("slot%u: S BUSY_RSP,     COMP 0x%016lx", slot->id, slot->completed_events);

				evatronix_finish_request(host, slot, req);
				goto unlock;

			case STATE_DATA_ERROR:
				state = STATE_DATA_BUSY;
				break;
		}
	} while(state != prev_state);

	host->state = state;
unlock:
	LEAVE();
	cyg_drv_mutex_unlock(&host->lock);
}

//static irqreturn_t evatronix_irq_handler_new(int irq, void *dev_id)
static cyg_uint32 evatronix_irq_handler_new(cyg_vector_t vector, cyg_addrword_t data)
{
	struct evatronix_host *host = (struct evatronix_host *)data;
	struct evatronix_slot *slot = NULL;
	u32 pending;
	int	i, cnt = 0;
	int ignore_cd = 1;
    
    cyg_drv_interrupt_mask(vector);

	for( i = 0; i < host->nr_slots; i++) {
		/* reset */
		cnt = 0;

		slot = host->slots[i];
		if(!slot)
			continue;
		do {
			pending = slot_readl(host, SFR12, i);

			if(!pending)
				continue;

			/* clear host-side interrupts */
			slot_writel(host, SFR12, i, pending);
			wmb();

			VDBG("slot%u: pending 0x%08x, irq_status 0x%08x", i, pending, slot->irq_status);
			slot->irq_status |= pending;

			if((pending & 0x100) && (slot->id != SDIO_SLOT_ID))
				err("slot%u: Spurious SDIO IRQ", slot->id);

			if((pending & SFR12_CARD_INTERRUPT) && (slot->id == SDIO_SLOT_ID) ) {
				//set_bit(EVENT_CARD_INTR, &slot->pending_events);
				//cyg_thread_resume(&slot->tasklet);
				printk( "%s: SDIO IRQ comming \n", __func__);
				mmc_signal_sdio_irq(slot->mmc);
				//SDIO int bit ROC
			}

			if(pending & SFR12_COMMAND_COMPLETE) {
				set_bit(EVENT_CMD_COMPLETE, &slot->pending_events);
				//cyg_thread_resume(host->tasklet_func);
			}
			if(pending & 0x8000) {

#define	CMD_ERROR	(SFR12_COMMAND_INDEX_ERROR | SFR12_COMMAND_END_BIT_ERROR \
		| SFR12_COMMAND_CRC_ERROR	\
		| SFR12_AUTO_CMD12_ERROR| SFR12_COMMAND_TIMEOUT_ERROR)
				if(pending & CMD_ERROR ) {
					/* CMD error, stop now */
					set_bit(EVENT_CMD_COMPLETE, &slot->pending_events);
					if(pending & SFR12_COMMAND_TIMEOUT_ERROR)
						slot->cmd->error = -ETIMEDOUT;
					else
						slot->cmd->error = -EILSEQ;
					//cyg_thread_resume(host->tasklet_func);
				}

#define	DATA_ERROR	(SFR12_DATA_END_BIT_ERROR | SFR12_DATA_CRC_ERROR\
		| SFR12_DATA_TIMEOUT_ERROR)
				/**
				 * GB9663 received this INTR, and crash, found that
				 * data ptr is NULL. very curious.
				 */
				if((pending & DATA_ERROR)  && slot->data ) {
					set_bit(EVENT_DATA_ERROR, &slot->pending_events);
					slot->data->error = -EINPROGRESS;

#ifdef DATA_ERROR_DEBUG
					err("TRIGER condition ................");
					gpio_direction_output(GPIONUM, 0);
					udelay(10);
					evatronix_dbg(slot->mmc);
					if(slot->mrq->data)
						crime_scene(host, slot->id);
					BUG();
#endif
					//cyg_thread_resume(host->tasklet_func);
				}

				if(pending & SFR12_ADMA_ERROR) {
					set_bit(EVENT_DATA_ERROR, &slot->pending_events);
					slot->data->error = -EILSEQ;
					//BUG();
#ifdef DATA_ERROR_DEBUG
					err("TRIGER condition DMA error ................");
					gpio_direction_output(GPIONUM, 0);
					udelay(10);
					evatronix_dbg(slot->mmc);
					if(slot->mrq->data)
						crime_scene(host, i);
					BUG();
#endif
					//cyg_thread_resume(host->tasklet_func);
				}
			}
			/* DMA engine has finished processing of desc */
			if(pending & SFR12_DMA_INTERRUPT) {
				set_bit(EVENT_DMA_FIN, &slot->pending_events);
				//cyg_thread_resume(host->tasklet_func);
			}

			if(pending & SFR12_TRANSFER_COMPLETE) {
				set_bit(EVENT_XFER_COMPLETE, &slot->pending_events);
				//cyg_thread_resume(host->tasklet_func);
			}

#if 1 
			if(pending & (SFR12_CARD_REMOVAL | SFR12_CARD_INSERTION) ) {
				cd_test = true;
				if(!ignore_cd) {
					set_bit(EVENT_CARD_DETECT, &slot->pending_events);
					//cyg_thread_resume(host->tasklet_cd);
				}
			}
#endif
		} while(cnt++ < 3);
	}

    cyg_drv_interrupt_acknowledge(vector);
    return (CYG_ISR_CALL_DSR);
}

//==========================================================================
// DSR signals data
//==========================================================================
static void evatronix_irq_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
	struct evatronix_host *host = (struct evatronix_host *)data;
	ENTER();

    evatronix_tasklet_func(host);
    cyg_drv_interrupt_unmask(vector);
}

static void evatronix_finish_request(struct evatronix_host *host,
		struct evatronix_slot *slot, struct mmc_request *mrq)
{
	int id = slot->id;
	unsigned int resp[4];
	//unsigned int srdat = 0x4000000;
	//unsigned int cnt = 0;
#ifdef	ERROR_RECOVERY
	unsigned int dataError = 0;
#endif

	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data	  *data	= mrq->data;
	struct mmc_host   *mmc 	= slot->mmc;
	struct evatronix_slot *new_slot = NULL;

	/* put this before evatronix_dbg */
	host->flag = 1;
	ENTER();

#ifdef	ERROR_RECOVERY
	if(cmd->opcode == 12) {
		err("slot%u: RECOVER FROM A DATA ERROR", slot->id);
		return;
	}
#endif

	if(sdio_trace_param > 0) {
#ifdef DEBUG
		slot->tm_end = (jiffies - slot->tm_start) * 1000 / HZ;

		if(slot->tm_end > 20)
			printk( "slot%d req process time %d ms\n", slot->id, slot->tm_end);
#endif
	}

#ifdef DEBUG
	if(cmd->error ) {
#else
	if(cmd->error && (cmd->opcode != 5)
			&& (cmd->opcode != 8)
			&& (cmd->opcode != 55)) {
#endif
		err("slot%u: CMD%02u %d, req[%llu]", slot->id, cmd->opcode,cmd->error, slot->reqid);

#ifdef DEBUG
		err(" SFR02 0x%08x, SFR03 0x%08x",
				slot_readl(host, SFR2, id),
				slot_readl(host, SFR3, id));
		evatronix_dbg(slot->mmc);
#endif
	}

	if ( (!cmd->error)  && (cmd->flags & MMC_RSP_PRESENT) ) {
		if (cmd->flags & MMC_RSP_136) {
			/* response type 2 */
			resp[0] = slot_readl(host, SFR4, id);
			resp[1] = slot_readl(host, SFR5, id);
			resp[2] = slot_readl(host, SFR6, id);
			resp[3] = slot_readl(host, SFR7, id);

			cmd->resp[3] = resp[0] << 8;
			cmd->resp[2] = (resp[1] << 8) | ((resp[0] >> 24) & 0xff);
			cmd->resp[1] = (resp[2] << 8) | ((resp[1] >> 24) & 0xff);
			cmd->resp[0] = (resp[3] << 8) | ((resp[2] >> 24) & 0xff);
			if((slot->id == TEST_SLOT) && cd_test)
            {
				printk( "slot%u: RESP 0x%08x 0x%08x 0x%08x 0x%08x", id,
							cmd->resp[3], cmd->resp[2],
							cmd->resp[1], cmd->resp[0]);
            }
		} else /*if(cmd->flags & MMC_RSP_48)*/ {
			/* response types 1, 1b, 3, 4, 5, 6 */
			/* maybe need shift-right 8bit ***********/
			resp[0] = slot_readl(host, SFR4, id);
			cmd->resp[0] = resp[0];
			cmd->resp[1] = 0;//(resp[0] >> 24) & 0xff;
			cmd->resp[2] = 0;
			cmd->resp[3] = 0;

			if((slot->id == TEST_SLOT) && cd_test)
				printk( "slot%u: RESP 0x%08x", id, cmd->resp[0]);
		}
	}

	if(data) {
		if(data->error) {
#ifdef	ERROR_RECOVERY
			dataError = 1;
#endif
			err("slot%u: CMD%02u DAT %s error %d, irq_status 0x%08x",
					id, cmd->opcode,
					((data->flags & MMC_DATA_WRITE) ? "WR": "RD"),
					data->error, slot->irq_status);
			err("\t *** blocks 0x%x, blksz 0x%x, blocks %d",
					data->blocks, data->blksz, host->blocks);
			crime_scene(host, id);
			//BUG();
		}
	}

#ifdef ERROR_RECOVERY
	if(dataError) {
		err("slot%u: Fatal Attack. soft reset >>>>>>>>", slot->id);
		evatronix_error_recovery(host, slot, host->mrq);
	}
#endif

	host->mrq		= NULL;
#ifdef DEBUG
	host->prev_slot 	= slot->id;
	host->prev_cmd 		= slot->mrq->cmd->opcode;
#endif
	slot->mrq 		= NULL;
	slot->cmd 		= NULL;
	slot->data 		= NULL;

#ifdef DEBUG
	/* crash if did NOT finish the job */
	if(slot->pending_events) {
		err("slot%u: pending 0x%08x", slot->id, slot->pending_events);
		err("slot%u: complet 0x%08x", slot->id, slot->completed_events);
		WARN_ON(slot->pending_events);
		WARN_ON(!slot->completed_events);
	}
#endif

	slot->pending_events 	= 0;
	slot->completed_events 	= 0;

	slot->irq_status	= 0;
	slot->step		= 0;

	host->cur_slot		= 0xff;
	host->blocks		= 0;
	/*
	 * put this before serve_reqeust
	 */
	cyg_drv_mutex_unlock(&host->lock);
	mmc_request_done(mmc, mrq);
	cyg_drv_mutex_lock(&host->lock);

	if(!list_empty(&host->queue)) {
		new_slot = list_entry(host->queue.next,
				struct evatronix_slot, queue_node);
		list_del(&new_slot->queue_node);

		host->state = STATE_SENDING_CMD;
		VDBG("slot%u: still has MRQ", new_slot->id);
		evatronix_serve_request(host, new_slot);
	} else {
		VDBG("host now IDLE");
		host->state = STATE_IDLE;
	}

	LEAVE();
}

static void evatronix_start_request(struct evatronix_host *host,
		struct evatronix_slot *slot, struct mmc_request *req)
{
	ENTER();

	/* gimme that */
	slot->mrq 	= req;
	slot->data 	= req->data;
	slot->cmd 	= req->cmd;

	slot->reqid++;
	slot->step = 2;

	if(host->state == STATE_IDLE) {
		host->state     = STATE_SENDING_CMD;

		VDBG("lucky boy slot%d, req[%llu]", slot->id, slot->reqid);
		evatronix_serve_request(host, slot);
	} else {
		VDBG("slot%u: host BUSY %x",
				slot->id, host->state);
		list_add_tail(&slot->queue_node, &host->queue);
	}

#ifdef DEBUG
	if(sdio_trace_param > 0)
		slot->tm_start = jiffies;
#endif

	//cyg_drv_mutex_unlock_bh(&host->lock);
	cyg_drv_mutex_unlock(&host->lock);
	LEAVE();
}

static u8 check_dma_desc(struct evatronix_slot *slot, struct mmc_data *data, int log)
{
	struct evatronix_host *host = slot->host;
	u32 NumberOfDescriptos, i, length;
	u32 *pDesc, Descriptors;
	u8 ret = 1;
	//struct scatterlist *sg;
	u32 desc_addr = 0;

	NumberOfDescriptos = host->blocks;
	pDesc = (u32 *)host->desc;
	//sg = data->sg;

	for (i = 0; i < NumberOfDescriptos*2; i += 2){
		//length = ADMA2_DESCRIPTOR_LENGHT(sg_dma_len(&sg[i/2]));
		length = ADMA2_DESCRIPTOR_LENGHT(data->blksz);
		wmb();
		Descriptors = (
				ADMA2_DESCRIPTOR_TYPE_TRAN
				| length
				| ADMA2_DESCRIPTOR_VAL);

		if((i + 2) == NumberOfDescriptos * 2)
			Descriptors |= (0x04 | ADMA1_DESCRIPTOR_END);
		desc_addr =((u32)data->data_buf + data->blksz*(i/2)); // ((u32)sg_dma_address(&sg[i/2]));
		wmb();

		if(Descriptors == 0 || pDesc[i] == 0)
			printk( " %s, desc == 0 <<<<<<<<<<<<<<<<<<<<<\n", __func__);
		if(desc_addr == 0 || pDesc[i + 1] == 0)
			printk( "%s, desc_addr = 0 <<<<<<<<<<<<< \n", __func__);

		if(log > 0) {
			printk( "check: 0x%08x[0x%08x] 0x%08x[0x%08x]\n", 
				Descriptors, pDesc[i], desc_addr, pDesc[i + 1]);
		}
		if(Descriptors == pDesc[i] && desc_addr == pDesc[i + 1]) {
			continue;
		}
		pDesc[i] = Descriptors; 
		pDesc[i + 1] = desc_addr;
		wmb();
		ret = 0; 
	}
	wmb();
	if(ret == 0) {
		printk( " %s BUG <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n", __func__);
	}

	return ret;
}

static u8 create_dma_desc(struct evatronix_slot *slot, struct mmc_data *data, u32 *DescriptorsAddress)
{
	struct evatronix_host *host = slot->host;
	u32 NumberOfDescriptos, i;
	u32 *Descriptors;

	//struct scatterlist *sg;
	//enum dma_data_direction dma_data_dir;
	u32 desc_addr = 0;
	u32 length;
	//volatile u32 TMP;
	host->blocks = data->blocks;

	NumberOfDescriptos = host->blocks;
	if(NumberOfDescriptos > 1)
		VDBG("slot%u: %d DMA descriptors\n", slot->id, NumberOfDescriptos);
	Descriptors = (u32 *)host->desc;
	//sg = data->sg;
#if 0
	while((u32)Descriptors % 4){
		err("slot%u: unaligned DMA address %p", slot->id, Descriptors);
		Descriptors = (u32*)(((u8*)Descriptors) + 1);
	}
#endif
	for (i = 0; i < NumberOfDescriptos * 2; i += 2){
		length = ADMA2_DESCRIPTOR_LENGHT(data->blksz);
		wmb();
		Descriptors[i] = (
				ADMA2_DESCRIPTOR_TYPE_TRAN
				| length
				| ADMA2_DESCRIPTOR_VAL);

		desc_addr = ((u32)data->data_buf + data->blksz*(i/2));// ((u32)sg_dma_address(&sg[i/2]));
		Descriptors[i + 1] = desc_addr; // ((u32)sg_dma_address(&sg[i/2]));
	}

	/* XXX generate an INTR when finished desc processing */
	//Descriptors[NumberOfDescriptos * 2 - 2] |= (ADMA1_DESCRIPTOR_END);
	Descriptors[NumberOfDescriptos * 2 - 2] |= (0x4|ADMA1_DESCRIPTOR_END);

    v7_dma_clean_range((u32)Descriptors, (u32)Descriptors + 8*NumberOfDescriptos);

    VDBG("Descriptors[0]=0x%08x Descriptors[1]=0x%08x", Descriptors[0], Descriptors[1]);
    VDBG("host->desc[0]=0x%08x host->desc[1]=0x%08x", host->desc->desc_1, host->desc->desc_2);
	if (data->flags & MMC_DATA_WRITE)
    {
        v7_dma_clean_range((u32)data->data_buf, (u32)data->data_buf + data->blocks*data->blksz);
    }else{
        v7_dma_inv_range((u32)data->data_buf, (u32)data->data_buf + data->blocks*data->blksz);
    }

#ifdef MEMMAX0_TUNING
	if(Descriptors[NumberOfDescriptos * 2 - 2]) {
		unused  = 0;
		if(unused)
			printk( " >>>>>>> SD AMDA BUG <<<<<\n");
	}
#endif
	//check_dma_desc(slot, data, 0);

	desc_addr = (u32)(((u32)Descriptors - (u32)host->desc) + (u32)(host->sg_dma));

	*DescriptorsAddress = desc_addr;

	return SDIO_ERR_NO_ERROR;
}

u8 prepare_dma_transfer(struct evatronix_slot *slot, struct mmc_data *data, u8 DMAMode )
{
	u8 status;
	u32 tmp;
	struct evatronix_host *host = slot->host;

	ENTER();

	switch(DMAMode){
		case ADMA2_MODE:
			status = create_dma_desc(slot, data, &tmp);
			if (status)
				return status;

			rmb();
			wmb();

			// set register with the descriptors table base address
			//slot_writel(host, SFR22, slot->id, tmp);
			slot_writel(host, SFR22, slot->id, host->sg_dma);

			tmp = slot_readl(host, SFR10, slot->id);
			tmp &= ~SFR10_DMA_SELECT_MASK;

			slot_writel(host, SFR10, slot->id, tmp | SFR10_DMA_SELECT_ADMA2);
			break;

		case SDMA_MODE:
		case ADMA1_MODE:
		default:
			return SDIO_ERR_INVALID_PARAMETER;

	}

	LEAVE();
	return SDIO_ERR_NO_ERROR;
}

static void evatronix_serve_request(struct evatronix_host *host,
		struct evatronix_slot *slot)
{
	//struct evatronix_slot 	*slot = host->current_slot;
	struct mmc_request	*mrq = slot->mrq;
	struct mmc_command	*cmd = mrq->cmd;
	struct mmc_data		*data = mrq->data;

	u32 			command_information = 0;
	u32			status = 0, time,tmp;
	u8			busy_check = 0;
	u8			dma_mode;

	ENTER();
	host->mrq 	= mrq;
	host->cur_slot 	= slot->id;
	slot->step 	= 3;

#ifdef DEBUG
//	host->prev_slot = 0xff;
//	host->prev_cmd  = 0xff;
	host->flag	= 0;
#endif

	switch (mmc_resp_type(cmd)) {
		case MMC_RSP_NONE:
			VDBG("slot%u: CMD%d no need resp", slot->id, cmd->opcode);
			command_information |= SFR3_NO_RESPONSE;
			break;

		case MMC_RSP_R2:
			command_information = SFR3_RESP_LENGTH_136 | SFR3_CRC_CHECK_EN;
			break;

		case MMC_RSP_R3:
			//    case MMC_RSP_R4:
			command_information = SFR3_RESP_LENGTH_48;
			break;

		case MMC_RSP_R1:
			//    case MMC_RSP_R5:
			//    case MMC_RSP_R6:
			//    case MMC_RSP_R7:
			command_information = SFR3_RESP_LENGTH_48 |
				SFR3_CRC_CHECK_EN | SFR3_INDEX_CHECK_EN;
			break;

		case MMC_RSP_R1B:
			//    case MMC_RSP_R5B:
			command_information = SFR3_RESP_LENGTH_48B | SFR3_CRC_CHECK_EN |
				SFR3_INDEX_CHECK_EN;

			busy_check = 1;
			break;

		default:
			break;
	}

	// clear all status interrupts except:
	// current limit error, card interrupt, card removal, card insertion
	tmp = ~( SFR12_CURRENT_LIMIT_ERROR
			| SFR12_CARD_INTERRUPT
			| SFR12_CARD_REMOVAL
			| SFR12_CARD_INSERTION );

	slot_writel(host, SFR12, slot->id, tmp);

	if(data) {
		VDBG("slot%u: CMD%d with data 0x%p", slot->id, cmd->opcode, data);
		tmp = slot_readl(host, SFR16, slot->id);

		dma_mode = NONEDMA_MODE;

		if( tmp & SFR16_ADMA2_SUPPORT)
			dma_mode = ADMA2_MODE;

		if(dma_mode != NONEDMA_MODE) {
			status = prepare_dma_transfer(slot, data, dma_mode);

			if (status)
				goto _mmc_request_fail;
			// set DMA enable flag and DMA boundary at 512kB
			command_information |= SFR3_DMA_ENABLE;
		}


		// set block size and block
		tmp = data->blksz | (data->blocks << 16);
		if( (busy_check && (cmd->opcode != 12) && (cmd->opcode != 52)) ) {
			time = 0;
			while((slot_readl(host, SFR9, slot->id) & 3) != 0) {
				udelay(10);
				if(time++ >= 100) {
					status = SDIO_ERR_DAT_LINE_BUSY;
					err("slot%u: DAT line hold too long", slot->id);
				}
				goto _mmc_request_fail;
			}
		}
		slot_writel(host, SFR1, slot->id, tmp);
		VDBG("slot%u: CMD%u SFR1 0x%08x", slot->id, cmd->opcode, tmp);

		//slot_writel(host, SFR1, slot->id, tmp|SFR1_DMA_BUFF_SIZE_512KB);
		command_information |= SFR3_DATA_PRESENT;

		VDBG("slot%u: DMA SFR10 = 0x%x, SFR22 = 0x%x, SFR1 = 0x%x, blocks = %d",
				slot->id,
				slot_readl(host,SFR10,slot->id),
				slot_readl(host,SFR22,slot->id),
				slot_readl(host,SFR1,slot->id),
				data->blocks);

		if (data->flags & MMC_DATA_WRITE)
			;//VDBG("mmc write data to card");
		else if (data->flags & MMC_DATA_READ)
			command_information |= SFR3_TRANS_DIRECT_READ;

		if ( data->blocks > 1 )
			command_information |= SFR3_MULTI_BLOCK_SEL | SFR3_BLOCK_COUNT_ENABLE;

	} else {
		slot_writel(host, SFR1, slot->id, 0);
	}

	wmb();
	rmb();
	slot_writel(host, SFR2, slot->id, cmd->arg);

	if(mrq->stop)
		command_information |= 0x4;

	// check if command line is not busy
	time = 0;
	while((slot_readl(host, SFR9, slot->id) & 1) != 0) {
		udelay(10);
		if(time++ >= 100) {
			status = SDIO_ERR_CMD_LINE_BUSY;
			err("slot%u: CMD line hold too long", slot->id);
			goto _mmc_request_fail;
		}
	}

	slot_writel(host, SFR3, slot->id, (command_information | (cmd->opcode) << 24) );
	wmb();
	rmb();

	/* CMD has been written to regs */
	slot->step = 4;

	if((slot->id == TEST_SLOT) && cd_test)
		printk( "slot%u: CMD%02d SFR2 0x%08x, SFR3 0x%08x",
				slot->id, cmd->opcode,
				slot_readl(host,SFR2,slot->id),
				slot_readl(host,SFR3,slot->id));
	LEAVE();
	return;

_mmc_request_fail:
	cmd->error = -ECANCELED;
	mmc_request_done(slot->mmc, mrq);
	return;
}

static void evatronix_request(struct mmc_host *mmc, struct mmc_request *req)
{
	struct evatronix_slot *slot = mmc_priv(mmc);
	struct evatronix_host *host = slot->host;

	//cyg_drv_mutex_lock_bh(&host->lock);
	cyg_drv_mutex_lock(&host->lock);

	if(!test_bit(EVA_MMC_CARD_PRESENT, &slot->cd_flags)) {
		//cyg_drv_mutex_unlock_bh(&host->lock);
		cyg_drv_mutex_unlock(&host->lock);
		req->cmd->error = -ENOMEDIUM;
		mmc_request_done(slot->mmc, req);

		return;
	}

	slot->step = 1;

	ENTER();
	if(slot->mrq) {
		err("slot%u: JAM at req[%llu] mrq %p", slot->id, slot->reqid, slot->mrq);
	}
	WARN_ON(slot->mrq);

	evatronix_start_request(host, slot, req);
	LEAVE();
}

static void set_host_drv_type(struct evatronix_host *host, struct mmc_ios *ios, int id)
{
	u32 tmp = slot_readl(host, SFR15, id);
	//default drv_type is B
	switch(ios->drv_type) {
	case MMC_CAP_DRIVER_TYPE_A:
		tmp |= 0x1 << 20;
	case MMC_CAP_DRIVER_TYPE_C:
		tmp |= 0x2 << 20;
	case MMC_CAP_DRIVER_TYPE_D:
		tmp |= 0x3 << 20;
	}
	err("slot%u: SFR15 0x%08x host_drv_type", id, tmp);
	slot_writel(host, SFR15, id, tmp);
}

static int set_uhs_mode(struct evatronix_host *host,
		struct mmc_host *mmc, int id, u32 cap)
{
	u32 mode = -EINVAL;
	u32 val = 0;

	//if( (mmc->caps & (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
	//		MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 | MMC_CAP_UHS_DDR50)) == 0) {
	if((mmc->caps & cap) == 0) {
			err("  NOT supported cap. caps is 0x%08lx", mmc->caps);
			return mode;
	}

	if(cap == (mmc->caps & MMC_CAP_UHS_DDR50))
		mode = 0x4 << 16;
	else if(cap == (mmc->caps & MMC_CAP_UHS_SDR104))
		mode = 0x3 << 16;
	else if(cap == (mmc->caps & MMC_CAP_UHS_SDR50))
		mode = 0x2 << 16;
	else if(cap == (mmc->caps & MMC_CAP_UHS_SDR25))
		mode = 0x1 << 16;
	else if(cap == (mmc->caps & MMC_CAP_UHS_SDR12))
		mode = 0x0 << 16;

	val = slot_readl(host, SFR15, id);
	val |= mode;
	slot_writel(host, SFR15, id, val);

	err("%s: slot%u setting UHS mode to 0x%x", __func__, id, val);
	return mode;
}

static void evatronix_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct evatronix_slot *slot = mmc_priv(mmc);
	struct evatronix_host *host = slot->host;
	u32  tmp;

	tmp = slot_readl(host, SFR9, slot->id);
	VDBG("slot%u: SFR9 0x%08x", slot->id, tmp);
	tmp = slot_readl(host, SFR10, slot->id);
	VDBG("slot%u: SFR10 0x%08x", slot->id, tmp);

	switch(ios->power_mode) {
		case MMC_POWER_OFF:
			tmp &= ~SFR10_SD_BUS_POWER;
			slot_writel(host, SFR10, slot->id, tmp);
			VDBG("  power off SFR10 0x%08x", slot_readl(host, SFR10, slot->id));

		case MMC_POWER_UP:
			break;
		case MMC_POWER_ON:
			VDBG("  before POWER ON SFR10 0x%08x, voltage 0x%08x",
					slot_readl(host, SFR10, slot->id), slot->voltage);
			switch(slot->voltage) {
				case SFR10_SET_3_3V_BUS_VOLTAGE:
					tmp |= SFR10_SET_3_3V_BUS_VOLTAGE | SFR10_SD_BUS_POWER;
					break;
				case SFR10_SET_3_0V_BUS_VOLTAGE:
					tmp |= SFR10_SET_3_0V_BUS_VOLTAGE | SFR10_SD_BUS_POWER;
					break;
				case SFR10_SET_1_8V_BUS_VOLTAGE:
					tmp |= SFR10_SET_1_8V_BUS_VOLTAGE | SFR10_SD_BUS_POWER;
					break;
				case 0:
					tmp &= ~SFR10_SD_BUS_POWER;
					break;
				default:
					err("slot%d:  Invalid power mode", slot->id);
					break;
			}

			slot_writel(host, SFR10, slot->id, tmp);
			//slot_writel(host, SFR10, slot->id, SFR10_SET_3_3V_BUS_VOLTAGE | SFR10_SD_BUS_POWER);
			VDBG("  after POWER ON SFR10  0x%08x",
					slot_readl(host, SFR10, slot->id));
			break;
	}

	switch(ios->bus_width) {

		case MMC_BUS_WIDTH_1:

			//Disable mmc8 bit
			tmp = host_readl(host, HSFR0);
			tmp &= ~(0x1000000<<slot->id);
			host_writel(host, HSFR0, tmp);

			//enable data 1-bit
			tmp = slot_readl(host, SFR10, slot->id);
			slot_writel(host, SFR10, slot->id, tmp&(~2));

			VDBG("slot%u: set 1-bit mode, SFR10=0x%08x", slot->id,
					slot_readl(host, SFR10, slot->id));
			break;

		case MMC_BUS_WIDTH_4:
			//Disable mmc8 bit
			tmp = host_readl(host, HSFR0);
			tmp &= ~(0x1000000<<slot->id);
			host_writel(host, HSFR0, tmp);

			//enable data 4-bit
			tmp = slot_readl(host, SFR10, slot->id);
			tmp |= 2;
			slot_writel(host, SFR10, slot->id, tmp);


			VDBG("slot%u: set 4-bit mode, SFR10=0x%08x", slot->id,
					slot_readl(host, SFR10, slot->id));
			break;

		case MMC_BUS_WIDTH_8:
			//Disable mmc8 bit
			tmp = host_readl(host, HSFR0);
			tmp &= ~(0x1000000<<slot->id);
			host_writel(host, HSFR0, tmp);

			//set data 8-bit
			tmp = slot_readl(host, SFR10, slot->id);
			tmp |= 0x20;
			slot_writel(host, SFR10, slot->id, tmp);
			printk( "slot%u: set 8-bit mode, SFR10=0x%08x", slot->id,
					slot_readl(host, SFR10, slot->id));
			break;

		default:
			break;
	}

	if(ios->clock) {

#if 1 //def	PRCM_SUPPORT
		if(slot->cur_clk == ios->clock)
			return;
		slot->cur_clk = ios->clock;

		unsigned int div = 0;
		int source_rate = host->clk_rate;
		div = (source_rate) / (2*ios->clock);
		if(div >= 0xFF)
			err("slot%u: clock divider value error", slot->id);
		VDBG("slot%d: source rate %d, divider %d, ios->clock %d",
					slot->id, source_rate, div, ios->clock);

		if(ios->clock != 400000) {
			printk( "slot%u: div %d, FIN clock rate %dMHz",
					slot->id, div, ios->clock/1000/1000);
		}
#endif
		tmp = slot_readl(host, SFR11, slot->id);
		tmp = tmp & ( ~SFR11_SD_CLOCK_ENABLE );
		slot_writel(host, SFR11, slot->id, tmp);

		tmp = slot_readl(host, SFR11, slot->id);
		tmp &= ~SFR11_SEL_FREQ_BASE_MASK;
		slot_writel(host, SFR11, slot->id, tmp);

		slot_writel(host, SFR11, slot->id, (((div)<<8))|tmp|SFR11_INT_CLOCK_ENABLE);
		while((2&slot_readl(host, SFR11, slot->id)) != 2)
			udelay(100);

		tmp = slot_readl(host, SFR11, slot->id);
		slot_writel(host, SFR11, slot->id, tmp|SFR11_SD_CLOCK_ENABLE);
		VDBG("SFR11=0x%08x", slot_readl(host, SFR11, slot->id));
	} else {
        VDBG("ios->Clock is 0");
	}

	if(ios->timing) {
		eng("slot%u: timing 0x%x", slot->id, ios->timing);
		tmp = slot_readl(host, SFR10, slot->id);
		tmp |= 0x4;
		slot_writel(host, SFR10, slot->id, tmp);
	}

	if(ios->drv_type)
		set_host_drv_type(host, ios, slot->id);

    LEAVE();
}

static int evatronix_slot_get_cd(struct evatronix_host *host, struct evatronix_slot *slot)
{
	unsigned int reg;
	ENTER();
#if 0
	if(slot->pdata->force_rescan) {
		LEAVE();
		return 0xF;
	}
#endif
	reg = slot_readl(host, SFR9, slot->id);
	eng("slot%u: SFR9 0x%08x", slot->id, reg);

	if(reg & (1 << 16)) {
		VDBG("slot%u: card present", slot->id);
		LEAVE();
		return 1;
	} else {
		eng("slot%u: card NOT present", slot->id);
		LEAVE();
		return 0;
	}
}

int evatronix_get_cd(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct evatronix_slot *slot = mmc_priv(mmc);
	struct evatronix_host *host = slot->host;
	unsigned int reg;
#if 0
	if(slot->pdata->force_rescan)
		return 1;
#endif
	{
		reg = slot_readl(host, SFR9, slot->id);
		VDBG("slot%u: SFR9 0x%08x", slot->id, reg);

		/* no CD pin implemented for SDIO & eMMC card,
		 * so just return 1, do force scan
		 */
		if(reg&(1<<16/*card inserted*/)) {
			//printk( "slot%u: ==== card present ====", slot->id);
			return 1;
		} else {
			printk( "slot%u: ==== card NOT present ====", slot->id);
			return 0;
		}
	}
}

static void evatronix_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct evatronix_slot *slot = mmc_priv(mmc);
	struct evatronix_host *host = slot->host;
	u32 val, mask = 0x100;

	val = slot_readl(host, SFR13, slot->id);
	val &= ~mask;

	if(enable) {
		//printk( "slot%u: enable SDIO irq", slot->id);
		val |= mask;
		slot_writel(host, SFR13, slot->id, val);
	} else {
		//printk( "slot%u: disable SDIO irq", slot->id);
		slot_writel(host, SFR13, slot->id, val);
	}
}


static int voltage_switch(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct evatronix_slot *slot = mmc_priv(mmc);
	struct evatronix_host *host = slot->host;
	int id = slot->id;
	int err = 0, mode;
	u32 val = 0;

	printk( "Enter %s\n", __func__);

	if(ios->signal_voltage == MMC_SIGNAL_VOLTAGE_330) {
		printk( "Leave %s: do nothing\n", __func__);
		return 0;
	}

	//disable sdclk
	val = slot_readl(host, SFR11, id);
	val &= ~0x4;
	slot_writel(host, SFR11, id, val);

	//check data lines
	val = slot_readl(host, SFR9, id);
	err("%s: slot%u SFR09 0x%08x", __func__, id, val);
	if((val & 0xF00000) == 0) {
		if(slot->pdata->voltage_switch) {
			err = slot->pdata->voltage_switch();
			if(err)
				goto fail;
		}

		//enable 1.8V
		val = slot_readl(host, SFR15, id);
		val |= 0x80000;
		slot_writel(host, SFR15, id, val);
		mdelay(5*2);

#if 0
		val = slot_readl(host, SFR15, id);
		err("%s: slot%u SFR15 0x%08x", __func__, id, val);
		//if( (val & 0x80000) == 0) {
#endif
			if((mode = set_uhs_mode(host, mmc, id, MMC_CAP_UHS_SDR12)) < 0) {
				err = mode;
				goto fail;
			}

			//enable sdclk
			val = slot_readl(host, SFR11, id);
			val |= 0x4;
			slot_writel(host, SFR11, id, val);
			mdelay(1*2);

			//check data lines
			val = slot_readl(host, SFR9, id);
			err("%s: slot%u SFR09 0x%08x x2", __func__, id, val);
			if((val & 0xF0000) == 0xF0000) {
				printk( "Leave %s\n", __func__);
				return 0;
			} else {
				err = -1;
				goto fail;
			}
#if 0
		} else {
			err = -2;
			goto fail;
		}
#endif
	} else {
		err = -3;
		goto fail;
	}
fail:
	//printk( "%s: fail with error %d\n", __func__, err);
	return err;
}

static const struct mmc_host_ops evatronix_mmc_ops = {
	.request 	= evatronix_request,
	.set_ios	= evatronix_set_ios,
	.get_cd		= evatronix_get_cd,
	.enable_sdio_irq	= evatronix_sdio_irq,
	.start_signal_voltage_switch	= voltage_switch,
};

void evatronix_init_slot(struct evatronix_host *host, int id);

struct mmc_host evtx_mmc_host[MAX_SLOTS];

static int evatronix_create_slot(struct evatronix_host *host, struct evatronix_slot *slot, int id)
{
	struct mmc_host *mmc = NULL;
	struct evatronix_sdio_slot_data	*pdata;
	int ret;

	ENTER();

    mmc = &evtx_mmc_host[id];
    memset(mmc, 0, sizeof(struct mmc_host));
	mmc_alloc_host(mmc, id);

	//slot 		= mmc_priv(mmc);
	slot->mmc 	= mmc;
	slot->id 	= id;
	slot->host 	= host;
	host->slots[id] = slot;

	pdata = &host->pdata->slots[id];
	slot->pdata = pdata;
	if(!pdata) {
		err("slot%u: slot data NULL", id);
		ret = -EINVAL;
		goto out;
	}

	/* now, give them to slot */
	slot->ctype	= pdata->ctype;
	slot->freq	= pdata->freq;

	INIT_LIST_HEAD(&slot->queue_node);

	slot->mrq 		= NULL;
	slot->cmd		= NULL;
	slot->data		= NULL;

	slot->reqid             = 0;
	slot->cur_clk		= 0;

	slot->pending_events 	= 0;
	slot->completed_events 	= 0;
	slot->irq_status	= 0;
	slot->last_detect_state = evatronix_slot_get_cd(host, slot);
	slot->step		= 0;

	if(host->pdata->slot_attr_init)
		host->pdata->slot_attr_init(pdata, mmc, id);
	else {
		err("slot%u: need slot_attr_init", id);
		//return -EINVAL;
	}

	/* core layer setup */
	mmc->ops 	= &evatronix_mmc_ops;
    mmc->private = slot; 

	/**
	 * max_blk_size <= FIFO buffer size
	 * FIFO buffer size = 2 ^ FIFO_DEPTH
	 * FIFO_DEPTH is a generic parameter of SDIO host core,
	 * which is XXX for BP2016
	 */
	if(slot->ctype == SDIO_CARD) {
		mmc->max_blk_size = PAGE_SIZE * 4;
	} else
		mmc->max_blk_size       = 65536;
	eng("slot%u: max_blk_size 0x%x", id, mmc->max_blk_size);

	//XXX
	mmc->max_blk_count 	= PAGE_SIZE / sizeof(DmaDesc);
	mmc->max_seg_size	= PAGE_SIZE;
	mmc->max_segs		= PAGE_SIZE / sizeof(DmaDesc);
	mmc->max_req_size       = mmc->max_seg_size * mmc->max_blk_count;


	/* Use 3.3V as preffered */
	if(mmc->ocr_avail & (MMC_VDD_32_33|MMC_VDD_33_34))
		slot->voltage = SFR10_SET_3_3V_BUS_VOLTAGE;

	/* 1.8V voltage support */
	else if(mmc->ocr_avail & MMC_VDD_165_195) {
		slot->voltage = SFR10_SET_1_8V_BUS_VOLTAGE;
	} else {
		err("slot%u: === voltage range error ===", slot->id);
		ret = -EINVAL;
		goto out;
	}

    slot->initialized = 1;
	LEAVE();
	return 0;
out:
	if(mmc) {
		mmc_remove_host(mmc);
        //mmc_blk_remove(mmc->card);
	}

	LEAVE();
	return ret;
}

void evatronix_init_slot(struct evatronix_host *host, int id)
{
	u32 tmp;
	ENTER();
	struct evatronix_slot *slot = host->slots[id];

	if(slot->ctype == SDIO_CARD) {
#ifdef	WIFI_OOB
		slot_writel(host, SFR13, id, 0xFFFFFeFF);
		slot_writel(host, SFR14, id, 0xFFFFFeFF);
#else
		slot_writel(host, SFR13, id, 0xFFFFFFFF);
		slot_writel(host, SFR14, id, 0xFFFFFFFF);
		eng("slot%u: sdio intr enabled", id);
#endif

	} else {
		slot_writel(host, SFR13, id, 0xFFFFFeFF);
		slot_writel(host, SFR14, id, 0xFFFFFeFF);
	}

	VDBG("slot%d: IRQ enable SFR9 0x%08x", id, slot_readl(host, SFR9, id));
	VDBG("slot%d: IRQ enable SFR13 0x%08x", id, slot_readl(host, SFR13, id));
	VDBG("slot%u: IRQ enable SFR14 0x%08x", id, slot_readl(host, SFR14, id));
	//set timeout
	//SFR11_TIMEOUT_TMCLK_X_2_POWER_24
	tmp = slot_readl(host, SFR11, id);
	tmp &= SFR11_TIMEOUT_MASK;
	/*
	 * Data time out counter value
	 * 2 ^ (tmp + 13) sdmclk cycles,
	 * which is 100MHz for BP2016
	 */
	tmp |= 0xc0000;
	slot_writel(host, SFR11, id, tmp);
	VDBG("slot%d:  SFR11 0x%08x", id, slot_readl(host, SFR11, id));

	/*input timing tunning May-12-2012 */
	//slot_writel(host, SFR28, id, 0x1);

	if (evatronix_get_cd(slot->mmc, NULL))
		set_bit(EVA_MMC_CARD_PRESENT, &slot->cd_flags);
	else
		clear_bit(EVA_MMC_CARD_PRESENT, &slot->cd_flags);


	LEAVE();
	return;
}

/*
 * this will reset entire Host Controller,
 * reset all flip-slops in every slot
 */
static void controller_reset(struct evatronix_host *host)
{
	ENTER();
	unsigned int timeout = 0;
	u32 val = host_readl(host, HSFR0);
	val |= 0x1;
	host_writel(host, HSFR0, val);

    printk("HSFR0 0x%x, val=0x%x", host_readl(host, HSFR0), val);

	while((host_readl(host, HSFR0)&0x1) != 0) {
		udelay(5);
		if(timeout++ > 100)
			err("******* SDIO Host controller reset TO *******");
	}
	/* used for the debouncing circuit */
	host_writel(host, HSFR1, DEBOUNCING_TIME);
	LEAVE();
	return;
}

static void slot_cleanup(struct evatronix_host *host, struct evatronix_slot *slot, int present)
{
	struct mmc_request *mrq 	= slot->mrq;

	if(host->mrq == NULL) {
		printk("\t *** host IDLE");
	}

	VDBG("step 1 -------------------------");
	if(mrq != NULL) {
		VDBG("step 2 -------------------------");
		if(host->mrq == mrq) {

			VDBG("step 3 -------------------------");
			err("\t *** slot%u: CD %s mrq == host->mrq", slot->id, (present ? "insert" : "remove"));
			mrq->cmd->error = -ENOMEDIUM;

			VDBG("step 4 -------------------------");
			evatronix_finish_request(host, slot, mrq);

		} else {
			VDBG("step 6 -------------------------");
			err("\t *** CD, only remove queued req now");
			list_del(&slot->queue_node);

			mrq->cmd->error = -ENOMEDIUM;
			if(mrq->data)
				mrq->data->error = -ENOMEDIUM;
			if(mrq->stop)
				mrq->stop->error = -ENOMEDIUM;

			slot->mrq 	= NULL;
			slot->cmd 	= NULL;
			slot->data 	= NULL;

			VDBG("step 7 -------------------------");
			cyg_drv_mutex_unlock(&host->lock);
			mmc_request_done(slot->mmc, mrq);
			cyg_drv_mutex_lock(&host->lock);
		}

	} else {
		printk("\t *** slot%u: CD %s slot mrq 0x%x", 
			slot->id, (present ? "insert" : "remove"), (u32)slot->mrq);
	}
}

static void evatronix_reset_slot(struct evatronix_host *host, int id)
{
	int	count = 0;
	u32	val;

	val = slot_readl(host, SFR11, id);
	wmb();
	val |= (1 << 24); //reset slot
	slot_writel(host, SFR11, id, val);
	wmb();
	do {
		udelay(2);
		count ++;
		val = slot_readl(host, SFR11, id);
	}while(val & (1 << 24));

	printk("\t *** reset slot%d, wait %d us", id, count * 2);
}

int evatronix_slot_init(struct evatronix_slot *slot)
{
    struct evatronix_host *host = slot->host;
    int cnt = 0;

    ENTER();

    int present, val=0;

    present = evatronix_slot_get_cd(host, slot);
    if(present == 0)
    {
        printk("slot %d have no card!", slot->id);
        return -1;
    }

    evatronix_reset_slot(host, slot->id);
    evatronix_init_slot(host, slot->id);

    val = slot_readl(host, SFR10, slot->id);
    val = slot->voltage | SFR10_SET_3_3V_BUS_VOLTAGE| SFR10_SD_BUS_POWER;
    slot_writel(host, SFR10, slot->id, val);

    slot_cleanup(host, slot, present);

    /* re-arm slot
     * U cann't predict it's an 'insert' or 'remove' action when this tasklet called
     * so this is a safe way for both actions.
     * Without this, slot may won't work cause lose of clk and power due to 'remove'
     */

    val = slot_readl(host, SFR11, slot->id);
    if(!(val & 0x5)) {
        val |= 0x1;
        slot_writel(host, SFR11, slot->id, val);
        if(!(0x2 & slot_readl(host, SFR11, slot->id))) {
            udelay(2);
            if(cnt++ > 100) {
                err("%s: waiting for ICS TO", __func__);
                return -1;
            }
            cnt++;
        }
        //val |= 0x4|(0x7d<<8);
        val |= 0x4|(0x77<<8);
        slot_writel(host, SFR11, slot->id, val);
    }

    printk("\t *** re-arm: SFR10 0x%08x, SFR11 0x%08x",
            slot_readl(host, SFR10, slot->id),
            slot_readl(host, SFR11, slot->id));

    LEAVE();
    return 0;
}

#if 0
static void evatronix_tasklet_cd(unsigned long data)
{
	struct evatronix_host *host = (struct evatronix_host *)data;
	int i, cnt = 0;

	ENTER();

	for(i = 0; i < host->nr_slots; i++) {
		struct evatronix_slot *slot = host->slots[i];
		int present;
		u32 val = 0;
		int change = 0;

		present = evatronix_slot_get_cd(host, slot);
		if(present == 0xf)
			continue;

		while(present != slot->last_detect_state) {
			cyg_drv_mutex_lock(&host->lock);
			if(!test_and_clear_bit(EVENT_CARD_DETECT, &slot->pending_events)) {
				cyg_drv_mutex_unlock(&host->lock);
				break;
			}
			set_bit(EVENT_CARD_DETECT, &slot->completed_events);

			change = 1;
			if(present == 0) {
				//reset slot
				evatronix_reset_slot(host, i);

				//init slot
				evatronix_init_slot(host, i);
			}
			err("\nslot%u: present 0x%x, last_detect_state 0x%x",
				slot->id, present, slot->last_detect_state);

			val = slot_readl(host, SFR9, slot->id);
			err("\t *** get CD SFR9 0x%08x", val);
			err("\t *** card %s", present ? "inserted" : "removed");


			//val = slot_readl(host, SFR10, slot->id);
			if(present) {
				set_bit(EVA_MMC_CARD_PRESENT, &slot->cd_flags);
				//err("\t *** CD power up slot%d", slot->id);
				//val = slot->voltage | SFR10_SET_3_3V_BUS_VOLTAGE| SFR10_SD_BUS_POWER;
			} else {
				//err("\t *** CD power off slot%d", slot->id);
				//val &= ~ SFR10_SD_BUS_POWER;
				clear_bit(EVA_MMC_CARD_PRESENT, &slot->cd_flags);
			}
			//slot_writel(host, SFR10, slot->id, val);

			slot->last_detect_state = present;
			slot_cleanup(host, slot, present);

			cyg_drv_mutex_unlock(&host->lock);
			udelay(100);
			present = evatronix_slot_get_cd(host, slot);
			err("\t *** present 0x%x, last_detect_state 0x%x",
					present, slot->last_detect_state);

		}

		if(change == 0) { //After enter s3 state, uplug tf card, resume back and plug tf card.
			err("\t *** plug state unchanged! present = %d\n", present);
			cyg_drv_mutex_lock(&host->lock);

			evatronix_reset_slot(host, i);
			evatronix_init_slot(host, i);
			if(present) {
				val = slot_readl(host, SFR10, slot->id);
				val = slot->voltage | SFR10_SET_3_3V_BUS_VOLTAGE| SFR10_SD_BUS_POWER;
				slot_writel(host, SFR10, slot->id, val);
			}
			slot_cleanup(host, slot, present);
			cyg_drv_mutex_unlock(&host->lock);
		}

		/* re-arm slot
		 * U cann't predict it's an 'insert' or 'remove' action when this tasklet called
		 * so this is a safe way for both actions.
		 * Without this, slot may won't work cause lose of clk and power due to 'remove'
		 */
		val = slot_readl(host, SFR10, slot->id);
		if(!(val & SFR10_SD_BUS_POWER)) {
			val = slot->voltage | SFR10_SET_3_3V_BUS_VOLTAGE| SFR10_SD_BUS_POWER;
			slot_writel(host, SFR10, slot->id, val);
		}

		val = slot_readl(host, SFR11, slot->id);
		if(!(val & 0x5)) {
			val |= 0x1;
			slot_writel(host, SFR11, slot->id, val);
			if(!(0x2 & slot_readl(host, SFR11, slot->id))) {
				udelay(2);
				if(cnt++ > 100) {
					err("%s: waiting for ICS TO", __func__);
					goto out;
				}
				cnt++;
			}
			val |= 0x4|(0x7d<<8);
			slot_writel(host, SFR11, slot->id, val);
		}

		err("\t *** re-arm: SFR10 0x%08x, SFR11 0x%08x",
						slot_readl(host, SFR10, slot->id),
						slot_readl(host, SFR11, slot->id));
#if 0
		mmc_detect_change(slot->mmc,
				msecs_to_jiffies(host->pdata->detect_delay_ms));
#endif
	}

out:
	LEAVE();
}
#endif

struct evatronix_host   g_host;
#define EVTX_DESC_LEN      PAGE_SIZE
//#pragma pack(64) 
static char __attribute__((aligned(64)))desc_buf[EVTX_DESC_LEN];

static u32 host_init_flag = 0;

static cyg_bool evatronix_probe(struct cyg_devtab_entry* tab)
{
    disk_channel*   chan    = (disk_channel*) tab->priv;
    struct evatronix_slot *cur_slot = (struct evatronix_slot *)chan->dev_priv; 
    struct evatronix_host *host;

	int ret = 0;

	ENTER();

	printk("SD/MMC/SDIO code version %s", evatronix_sd_vcode);
    printk("begin create slot %d", cur_slot->id);
    printk("Descriptors buffer address 0x%08x", (u32)desc_buf); 
    if(cur_slot->initialized)
    {
        printk("slot %d is already initialized. return ...", cur_slot->id);
        return 0;
    }

    if(!host_init_flag)
    {
        extern int scm_mmc_en(int);
        scm_mmc_en(0);
        memset(&g_host, 0, sizeof(g_host));
        host = &g_host;

        host->pdata = &evtx_sdmmc_data;
        if(!host->pdata) {
            err("%s: host->pdata is NULL", __func__);
            return -ENXIO;
        }

        host->virt_base = hwsd_addr; 
        host->phys_base = SD_BASE_ADDR;
        host->sdmmc_isrvec = SYS_IRQ_ID_SD;
        host->sdmmc_isrpri = INT_PRI_DEFAULT; 
        host->clk_rate = host->pdata->ref_clk;

        printk("virt_base 0x%x phys_base 0x%x ref_clk %d", (u32)host->virt_base, host->phys_base, host->clk_rate);
        host->suspended		= 0;

        host->mrq		= NULL;
        host->cur_slot		= cur_slot->id;
        host->nr_slots 		= host->pdata->nr_slots;
        host->state		= STATE_IDLE;
        host->blocks		= 0;

#ifdef DEBUG
        host->prev_slot		= 0xff;
        host->prev_cmd		= 0xff;
#endif

        controller_reset(host);
        evatronix_host_reg(host);
        printk("card status reg: 0x%x", slot_readl(host, SFR9, cur_slot->id));

        cyg_drv_mutex_init(&host->lock);

        cyg_drv_interrupt_create(host->sdmmc_isrvec,
                host->sdmmc_isrpri,
                (cyg_addrword_t)host,
                &evatronix_irq_handler_new,
                &evatronix_irq_dsr,
                &(host->sdmmc_interrupt_handle),
                &(host->sdmmc_interrupt_data));
        cyg_drv_interrupt_attach(host->sdmmc_interrupt_handle);
        cyg_drv_interrupt_unmask(host->sdmmc_isrvec);


		s3_show_regs(host, cur_slot->id);
        host->desc = (DmaDesc *)desc_buf;

        host->sg_dma = (u32)host->desc;

        eng("DMA desc paddr 0x%08x, vaddr 0x%08x", host->sg_dma, (u32)host->desc);

        INIT_LIST_HEAD(&host->queue);
    }else{
        printk("host is is already initialized, just init slot %d!", cur_slot->id); 
        host = &g_host;
    }

    (*chan->callbacks->disk_init)(tab);
    if(slot_readl(host, SFR9, cur_slot->id) & 0x10000) {
        VDBG("slot%u: card present", cur_slot->id);
    }

    printk("begin create slot %d", cur_slot->id);

    cyg_drv_mutex_lock(&host->lock);

    ret = evatronix_create_slot(host, cur_slot, cur_slot->id);
    if (ret) {
        err("%s: create slot%d failed", __func__, cur_slot->id);
            evatronix_remove_slot(host->slots[cur_slot->id]);
    }

    cyg_drv_mutex_unlock(&host->lock);

	LEAVE();
	return ret;
}

static void evatronix_remove_slot(struct evatronix_slot *slot)
{
	struct mmc_host *mmc = slot->mmc;
	struct evatronix_host *host = slot->host;

	ENTER();
	/* for safety */
	slot_writel(host, SFR13, slot->id, 0);
	slot_writel(host, SFR14, slot->id, 0);

	slot->host->slots[slot->id] = NULL;

	mmc_remove_host(mmc);
	LEAVE();
}

static void s3_show_regs(struct evatronix_host *host, int id)
{
	printk("SFR10 0x%08x", slot_readl(host, SFR10, id));
	printk("SFR11 0x%08x", slot_readl(host, SFR11, id));
	printk("SFR13 0x%08x", slot_readl(host, SFR13, id));
	printk("SFR14 0x%08x", slot_readl(host, SFR14, id));
}

#ifdef CONFIG_PM
static int evatronix_suspend(struct platform_device *pdev, pm_message_t state)
{
	int i, ret = 0;
	struct evatronix_host *host = platform_get_drvdata(pdev);

	//s3_test = true;
	err("Enter %s", __func__);

	if(!host || host->suspended)
		return 0;


	for(i = 0; i < host->nr_slots; i++) {
		struct evatronix_slot *slot;
		slot = host->slots[i];
		if(!slot)
			continue;

		printk( "======= slot%u before suspend =========", i);
		s3_show_regs(host, i);

#ifdef CONFIG_WIFI_S3_PLAN_B
		if(slot->ctype != SDIO_CARD) {
#endif
			ret = mmc_suspend_host(slot->mmc);
		if(ret < 0) {
			err("slot%u: SUSPEND failed error %d", i, ret);
			while(--i >= 0) {
				/* one fails, all sink */
				slot = host->slots[i];
#ifdef CONFIG_WIFI_S3_PLAN_B
				if(slot && (slot->ctype != SDIO_CARD) )
#else
				if(slot)
#endif
					mmc_resume_host(slot->mmc);
			}

			return ret;
		}
#ifdef CONFIG_WIFI_S3_PLAN_B
		}
#endif
	}

	host->suspended = 1;
	err("Leave %s", __func__); 
	return 0;
}

static int evatronix_resume(struct platform_device *pdev)
{
	int i, ret = 0;
	int cnt = 0;
	u32 val = 0;
	unsigned long flags;

	err("Enter %s", __func__);
	struct	evatronix_host *host = platform_get_drvdata(pdev);

	if(!host || !host->suspended)
		return 0;

	controller_reset(host);
	printk( "host reset FIN");

	for(i = host->nr_slots - 1; i >= 0; i--) {
		struct evatronix_slot *slot;
		slot = host->slots[i];
		if(!slot)
			continue;

		evatronix_init_slot(host, i);

		if(slot->ctype == SDIO_CARD) {
			cyg_drv_mutex_lock(&host->lock, flags);
			printk( "slot2 restore start");

			//power
			val = slot_readl(host, SFR10, i);
			val &= ~(SFR10_SET_3_3V_BUS_VOLTAGE | SFR10_SD_BUS_POWER);
			val |= SFR10_SET_3_3V_BUS_VOLTAGE | SFR10_SD_BUS_POWER;
			slot_writel(host, SFR10, i, val);

			//data width
			val = host_readl(host, HSFR0);
			val &=  ~(0x1000000<<i);
			host_writel(host, HSFR0, val);

			val = slot_readl(host, SFR10, i);
			val |= 2;
			slot_writel(host, SFR10, i, val);

			//clk, 25MHz
			val = SFR11_INT_CLOCK_ENABLE | (0x2<<8);
			slot_writel(host, SFR11, i, val);
			while((2 & slot_readl(host, SFR11, i)) != 2) {
				udelay(2);
				cnt++; 
				if(cnt > 100)
					err("waiting for Internal Clock Stable timeout");
			}
			val |= SFR11_SD_CLOCK_ENABLE;
			slot_writel(host, SFR11, i, val);

			wmb();
			cyg_drv_mutex_unlock(&host->lock, flags);
			printk( "slot2 restore FIN");
#ifndef CONFIG_WIFI_S3_PLAN_B
			ret = mmc_resume_host(slot->mmc);
			if(ret < 0) {
				err("slot%u: RESUME failed %d", i, ret);
				return ret;
			}
#endif

		} else {
			if (evatronix_get_cd(slot->mmc, NULL) == 0) {
				err("----------------maybe plug slot %d, after enter s3\n", slot->id);
				continue;
			}
			ret = mmc_resume_host(slot->mmc);
			if(ret < 0) {
				err("slot%u: RESUME failed", i);
				return ret;
			}
		}

		printk( "======= slot%u after resume =========", i);
		s3_show_regs(host, i);
	}

	host->suspended = 0;
	err("Leave %s", __func__);

	return 0;
}
#else
#define evatronix_suspend	NULL
#define evatronix_resume	NULL
#endif	/* CONFIG_PM */

DISK_FUNS(cyg_evtx_mmc_disk_funs,
          evtx_mmc_disk_read,
          evtx_mmc_disk_write,
          evtx_mmc_disk_get_config,
          evtx_mmc_disk_set_config
          );

static struct evatronix_slot cyg_mmc_evatronix_slot0 = {
        id              :   0,
        initialized     :   0,
};

// No h/w controller structure is needed, but the address of the
// second argument is taken anyway.
DISK_CONTROLLER(cyg_mmc_disk_controller_0, cyg_mmc_evatronix_slot0);

DISK_CHANNEL(cyg_mmc_disk0_channel,
             cyg_evtx_mmc_disk_funs,
             cyg_mmc_evatronix_slot0,
             cyg_mmc_disk_controller_0,
             true,                            /* MBR support */
             1                                /* Number of partitions supported */
             );
             
BLOCK_DEVTAB_ENTRY(cyg_evt_mmc_devtab_entry_0,
                   "/dev/sdmmc0/",
                   0,
                   &cyg_io_disk_devio,
                   &evatronix_probe,
                   &evtx_mmc_disk_lookup,
                   &cyg_mmc_disk0_channel);

#if (SDIO_SLOT_ID == 2)

static struct evatronix_slot cyg_mmc_evatronix_slot1 = {
        initialized     :   0,
        id              :   1,
};

// No h/w controller structure is needed, but the address of the
// second argument is taken anyway.
DISK_CONTROLLER(cyg_mmc_disk_controller_1, cyg_mmc_evatronix_slot1);

DISK_CHANNEL(cyg_mmc_disk1_channel,
             cyg_evtx_mmc_disk_funs,
             cyg_mmc_evatronix_slot1,
             cyg_mmc_disk_controller_1,
             true,                            /* MBR support */
             1                                /* Number of partitions supported */
             );
             
BLOCK_DEVTAB_ENTRY(cyg_evt_mmc_devtab_entry_1,
                   "/dev/sdmmc1/",
                   0,
                   &cyg_io_disk_devio,
                   &evatronix_probe,
                   &evtx_mmc_disk_lookup,
                   &cyg_mmc_disk1_channel);
#endif
