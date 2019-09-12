/*
 * Block driver for media (i.e., flash cards)
 *
 * Copyright 2002 Hewlett-Packard Company
 * Copyright 2005-2008 Pierre Ossman
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Author:  Andrew Christian
 *          28 May 2002
 */
#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <cyg/io/mmc/card.h>
#include <cyg/io/mmc/host.h>
#include <cyg/io/mmc/mmc.h>
#include <cyg/io/mmc/sd.h>
#include <cyg/io/mmc/block.h>
#include <cyg/io/mmc/mmc_common.h>
#include <cyg/io/mmc/list.h>
#include <cyg/io/mmc/fs.h>
#include <cyg/io/mmc/bitops.h>

#define INAND_CMD38_ARG_EXT_CSD  113
#define INAND_CMD38_ARG_ERASE    0x00
#define INAND_CMD38_ARG_TRIM     0x01
#define INAND_CMD38_ARG_SECERASE 0x80
#define INAND_CMD38_ARG_SECTRIM1 0x81
#define INAND_CMD38_ARG_SECTRIM2 0x88

#define MMC_BLK_CMD23	(1 << 0)	/* Can do SET_BLOCK_COUNT for multiblock */
#define MMC_BLK_REL_WR	(1 << 1)	/* MMC Reliable write support */

struct mmc_blk_request {
	struct mmc_request	mrq;
	struct mmc_command	sbc;
	struct mmc_command	cmd;
	struct mmc_command	stop;
	struct mmc_data		data;
};

static char __attribute__((aligned(4)))blocks[4];
static u32 mmc_sd_num_wr_blocks(struct mmc_card *card)
{
	int err;
	u32 result;
	//cyg_uint32 *blocks;

	struct mmc_request mrq = {0};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	unsigned int timeout_us;

	//struct scatterlist sg;

    cyg_mutex_lock(&card->host->lock);

	cmd.opcode = MMC_APP_CMD;
	cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

	err = mmc_wait_for_cmd(card->host, &cmd, 0);
    cyg_mutex_unlock(&card->host->lock);

	if (err)
		return (u32)-1;
	if (!mmc_host_is_spi(card->host) && !(cmd.resp[0] & R1_APP_CMD))
		return (u32)-1;

	memset(&cmd, 0, sizeof(struct mmc_command));

    cyg_mutex_lock(&card->host->lock);

	cmd.opcode = SD_APP_SEND_NUM_WR_BLKS;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	data.timeout_ns = card->csd.tacc_ns * 100;
	data.timeout_clks = card->csd.tacc_clks * 100;

	timeout_us = data.timeout_ns / 1000;
	timeout_us += data.timeout_clks * 1000 /
		(card->host->ios.clock / 1000);

	if (timeout_us > 100000) {
		data.timeout_ns = 100000000;
		data.timeout_clks = 0;
	}

	data.blksz = 4;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	mrq.cmd = &cmd;
	mrq.data = &data;

    //maybe need do cache flush
    data.data_buf = blocks; 

	mmc_wait_for_req(card->host, &mrq);
    cyg_mutex_unlock(&card->host->lock);

	result = *blocks ; // ntohl(*blocks);

	if (cmd.error || data.error)
		result = (u32)-1;

	return result;
}

static u32 get_card_status(struct mmc_card *card, struct request *req)
{
	struct mmc_command cmd = {0};
	int err;

    cyg_mutex_lock(&card->host->lock);

	cmd.opcode = MMC_SEND_STATUS;
	if (!mmc_host_is_spi(card->host))
		cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err)
		printk( "%s: error %d sending status command",
		       req->disk_name, err);

    cyg_mutex_unlock(&card->host->lock);

	return cmd.resp[0];
}

static int mmc_blk_issue_discard_rq(struct mmc_card *card, struct request *req)
{
	unsigned int from, nr, arg;
	int err = 0;

	if (!mmc_can_erase(card)) {
		err = -EOPNOTSUPP;
		goto out;
	}

	//from = blk_rq_pos(req);
	//nr = blk_rq_sectors(req);
    from = req->pos;
    nr = req->blocks; 

	if (mmc_can_trim(card))
		arg = MMC_TRIM_ARG;
	else
		arg = MMC_ERASE_ARG;

	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 INAND_CMD38_ARG_EXT_CSD,
				 arg == MMC_TRIM_ARG ?
				 INAND_CMD38_ARG_TRIM :
				 INAND_CMD38_ARG_ERASE,
				 0);
		if (err)
			goto out;
	}
	err = mmc_erase(card, from, nr, arg);
out:
	return err ? 0 : 1;
}

static int mmc_blk_issue_secdiscard_rq(struct mmc_card *card,
				       struct request *req)
{
	unsigned int from, nr, arg;
	int err = 0;

	if (!mmc_can_secure_erase_trim(card)) {
		err = -EOPNOTSUPP;
		goto out;
	}

	//from = blk_rq_pos(req);
	//nr = blk_rq_sectors(req);
    from = req->pos;
    nr = req->blocks; 

	if (mmc_can_trim(card) && !mmc_erase_group_aligned(card, from, nr))
		arg = MMC_SECURE_TRIM1_ARG;
	else
		arg = MMC_SECURE_ERASE_ARG;

	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 INAND_CMD38_ARG_EXT_CSD,
				 arg == MMC_SECURE_TRIM1_ARG ?
				 INAND_CMD38_ARG_SECTRIM1 :
				 INAND_CMD38_ARG_SECERASE,
				 0);
		if (err)
			goto out;
	}
	err = mmc_erase(card, from, nr, arg);
	if (!err && arg == MMC_SECURE_TRIM1_ARG) {
		if (card->quirks & MMC_QUIRK_INAND_CMD38) {
			err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					 INAND_CMD38_ARG_EXT_CSD,
					 INAND_CMD38_ARG_SECTRIM2,
					 0);
			if (err)
				goto out;
		}
		err = mmc_erase(card, from, nr, MMC_SECURE_TRIM2_ARG);
	}
out:
	return err ? 0 : 1;
}

/*
 * Reformat current write as a reliable write, supporting
 * both legacy and the enhanced reliable write MMC cards.
 * In each transfer we'll handle only as much as a single
 * reliable write can handle, thus finish the request in
 * partial completions.
 */
static inline void mmc_apply_rel_rw(struct mmc_blk_request *brq,
				    struct mmc_card *card,
				    struct request *req)
{
	if (!(card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN)) {
		/* Legacy mode imposes restrictions on transfers. */
		if (!IS_ALIGNED(brq->cmd.arg, card->ext_csd.rel_sectors))
			brq->data.blocks = 1;

		if (brq->data.blocks > card->ext_csd.rel_sectors)
			brq->data.blocks = card->ext_csd.rel_sectors;
		else if (brq->data.blocks < card->ext_csd.rel_sectors)
			brq->data.blocks = 1;
	}
}

static int mmc_blk_issue_rw_rq(struct mmc_card *card, struct request *req)
{
	//struct mmc_blk_data *md = container_of(card, struct mmc_blk_data, card);
	//struct mmc_card *card = md->queue.card;
	struct mmc_blk_request brq;
	int ret = 1, disable_multi = 0;

	/*
	 * Reliable writes are used to implement Forced Unit Access and
	 * REQ_META accesses, and are supported only on MMCs.
	 */
	bool do_rel_wr = (rq_data_dir(req) == WRITE) &&
		(card->flags & MMC_BLK_REL_WR);

	do {
		struct mmc_command cmd = {0};
		u32 readcmd, writecmd, status = 0;

		memset(&brq, 0, sizeof(struct mmc_blk_request));
        cyg_mutex_lock(&card->host->lock);
		
        brq.mrq.cmd = &brq.cmd;
		brq.mrq.data = &brq.data;

		//brq.cmd.arg = blk_rq_pos(req);
		brq.cmd.arg = req->pos;
		if (!mmc_card_blockaddr(card))
			brq.cmd.arg <<= 9;
		brq.cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
		brq.data.blksz = 512;
		brq.stop.opcode = MMC_STOP_TRANSMISSION;
		brq.stop.arg = 0;
		brq.stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
		brq.data.blocks = req->blocks; //blk_rq_sectors(req);

		/*
		 * The block layer doesn't support all sector count
		 * restrictions, so we need to be prepared for too big
		 * requests.
		 */
		if (brq.data.blocks > card->host->max_blk_count)
			brq.data.blocks = card->host->max_blk_count;

		/*
		 * After a read error, we redo the request one sector at a time
		 * in order to accurately determine which sectors can be read
		 * successfully.
		 */
		if (disable_multi && brq.data.blocks > 1)
			brq.data.blocks = 1;

		if (brq.data.blocks > 1 || do_rel_wr) {
			/* SPI multiblock writes terminate using a special
			 * token, not a STOP_TRANSMISSION request.
			 */
			if (!mmc_host_is_spi(card->host) ||
			    rq_data_dir(req) == READ)
				brq.mrq.stop = &brq.stop;
			readcmd = MMC_READ_MULTIPLE_BLOCK;
			writecmd = MMC_WRITE_MULTIPLE_BLOCK;
		} else {
			brq.mrq.stop = NULL;
			readcmd = MMC_READ_SINGLE_BLOCK;
			writecmd = MMC_WRITE_BLOCK;
		}
		if (rq_data_dir(req) == READ) {
			brq.cmd.opcode = readcmd;
			brq.data.flags |= MMC_DATA_READ;
		} else {
			brq.cmd.opcode = writecmd;
			brq.data.flags |= MMC_DATA_WRITE;
		}

		if (do_rel_wr)
			mmc_apply_rel_rw(&brq, card, req);

		/*
		 * Pre-defined multi-block transfers are preferable to
		 * open ended-ones (and necessary for reliable writes).
		 * However, it is not sufficient to just send CMD23,
		 * and avoid the final CMD12, as on an error condition
		 * CMD12 (stop) needs to be sent anyway. This, coupled
		 * with Auto-CMD23 enhancements provided by some
		 * hosts, means that the complexity of dealing
		 * with this is best left to the host. If CMD23 is
		 * supported by card and host, we'll fill sbc in and let
		 * the host deal with handling it correctly. This means
		 * that for hosts that don't expose MMC_CAP_CMD23, no
		 * change of behavior will be observed.
		 *
		 * N.B: Some MMC cards experience perf degradation.
		 * We'll avoid using CMD23-bounded multiblock writes for
		 * these, while retaining features like reliable writes.
		 */

		if ((card->flags & MMC_BLK_CMD23) &&
		    mmc_op_multi(brq.cmd.opcode) &&
		    (do_rel_wr || !(card->quirks & MMC_QUIRK_BLK_NO_CMD23))) {
			brq.sbc.opcode = MMC_SET_BLOCK_COUNT;
			brq.sbc.arg = brq.data.blocks |
				(do_rel_wr ? (1 << 31) : 0);
			brq.sbc.flags = MMC_RSP_R1 | MMC_CMD_AC;
			brq.mrq.sbc = &brq.sbc;
		}

		mmc_set_data_timeout(&brq.data, card);

		brq.data.data_buf = req->buf;

		//mmc_queue_bounce_pre(mq);

		mmc_wait_for_req(card->host, &brq.mrq);
        cyg_mutex_unlock(&card->host->lock);

		/*
		 * Check for errors here, but don't jump to cmd_err
		 * until later as we need to wait for the card to leave
		 * programming mode even when things go wrong.
		 */
		if (brq.sbc.error || brq.cmd.error ||
		    brq.data.error || brq.stop.error) {
			if (brq.data.blocks > 1 && rq_data_dir(req) == READ) {
				/* Redo read one sector at a time */
				printk( "%s: retrying using single "
				       "block read\n", req->disk_name);
				disable_multi = 1;
				continue;
			}
			status = get_card_status(card, req);
		} else if (disable_multi == 1) {
			disable_multi = 0;
		}

		if (brq.sbc.error) {
			printk( "%s: error %d sending SET_BLOCK_COUNT "
			       "command, response %#x, card status %#x\n",
			       req->disk_name, brq.sbc.error,
			       brq.sbc.resp[0], status);
		}

		if (brq.cmd.error) {
			printk( "%s: error %d sending read/write "
			       "command, response %#x, card status %#x\n",
			       req->disk_name, brq.cmd.error,
			       brq.cmd.resp[0], status);
		}

		if (brq.data.error) {
			if (brq.data.error == -ETIMEDOUT && brq.mrq.stop)
				/* 'Stop' response contains card status */
				status = brq.mrq.stop->resp[0];
			printk( "%s: error %d transferring data,"
			       " sector %u, nr %u, card status %#x\n",
			       req->disk_name, brq.data.error,
			       (unsigned)(req->pos),
			       (unsigned)(req->blocks), status);
		}

		if (brq.stop.error) {
			printk( "%s: error %d sending stop command, "
			       "response %#x, card status %#x\n",
			       req->disk_name, brq.stop.error,
			       brq.stop.resp[0], status);
		}

		if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ) {
			do {
				int err;

                cyg_mutex_lock(&card->host->lock);
				
                cmd.opcode = MMC_SEND_STATUS;
				cmd.arg = card->rca << 16;
				cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
				err = mmc_wait_for_cmd(card->host, &cmd, 5);

                cyg_mutex_unlock(&card->host->lock);
				if (err) {
					printk( "%s: error %d requesting status\n",
					       req->disk_name, err);
					goto cmd_err;
				}
				/*
				 * Some cards mishandle the status bits,
				 * so make sure to check both the busy
				 * indication and the card state.
				 */
			} while (!(cmd.resp[0] & R1_READY_FOR_DATA) ||
				(R1_CURRENT_STATE(cmd.resp[0]) == 7));

		}

		if (brq.cmd.error || brq.stop.error || brq.data.error) {
			if (rq_data_dir(req) == READ) {
				/*
				 * After an error, we redo I/O one sector at a
				 * time, so we only reach here after trying to
				 * read a single sector.
				 */
                ret = -EIO;
				continue;
			}
			goto cmd_err;
		}

		/*
		 * A block was successfully transferred.
		 */
        ret = 0;
	} while (ret);

	return ret;

 cmd_err:
 	/*
 	 * If this is an SD card and we're writing, we can first
 	 * mark the known good sectors as ok.
 	 *
	 * If the card is not SD, we can still ok written sectors
	 * as reported by the controller (which might be less than
	 * the real number of written sectors, but never more).
	 */
	if (mmc_card_sd(card)) {
		u32 blocks;

		blocks = mmc_sd_num_wr_blocks(card);
		if (blocks != (u32)-1) {
            ret = -EIO;
		}else{
            ret = 0;
        }
	}

	return ret;
}

int mmc_blk_issue_rq(struct mmc_card *card, struct request *req)
{
	int ret;

#if 0
	ret = mmc_set_blocklen(card, 512);

	if (ret) {
		printk( "unable to set block size to 512: %d\n",
			 ret);
		return -EINVAL;
	}

	ret = mmc_blk_part_switch(card, md);
	if (ret) {
		ret = 0;
		goto out;
	}
#endif

	if (req->cmd_flags & REQ_DISCARD) {
		if (req->cmd_flags & REQ_SECURE)
			ret = mmc_blk_issue_secdiscard_rq(card, req);
		else
			ret = mmc_blk_issue_discard_rq(card, req);
	} else {
		ret = mmc_blk_issue_rw_rq(card, req);
	}

	return ret;
}

static void card_flags_get(struct mmc_card *card)
{
	if (mmc_host_cmd23(card->host)) {
		if (mmc_card_mmc(card) ||
		    (mmc_card_sd(card) &&
		     card->scr.cmds & SD_SCR_CMD23_SUPPORT))
			card->flags |= MMC_BLK_CMD23;
	}

	if (mmc_card_mmc(card) &&
	    card->flags & MMC_BLK_CMD23 &&
	    ((card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN) ||
	     card->ext_csd.rel_sectors)) {
		card->flags |= MMC_BLK_REL_WR;
	}
}

static const struct mmc_fixup blk_fixups[] =
{
	MMC_FIXUP("SEM02G", 0x2, 0x100, add_quirk, MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM04G", 0x2, 0x100, add_quirk, MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM08G", 0x2, 0x100, add_quirk, MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM16G", 0x2, 0x100, add_quirk, MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM32G", 0x2, 0x100, add_quirk, MMC_QUIRK_INAND_CMD38),

	/*
	 * Some MMC cards experience performance degradation with CMD23
	 * instead of CMD12-bounded multiblock transfers. For now we'll
	 * black list what's bad...
	 * - Certain Toshiba cards.
	 *
	 * N.B. This doesn't affect SD cards.
	 */
	MMC_FIXUP("MMC08G", 0x11, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC16G", 0x11, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC32G", 0x11, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	END_FIXUP
};


int mmc_blk_probe(struct mmc_card *card)
{
	int err;
	sector_t size;

	/*
	 * Check that the card supports the command class(es) we need.
	 */
    printk("cmdclass 0x%x %d", card->csd.cmdclass, card->csd.cmdclass);
	if (!(card->csd.cmdclass & CCC_BLOCK_READ))
		return -ENODEV;

	if (!mmc_card_sd(card) && mmc_card_blockaddr(card)) {
		/*
		 * The EXT_CSD sector count is in number or 512 byte
		 * sectors.
		 */
		size = card->ext_csd.sectors;
	} else {
		/*
		 * The CSD capacity field is in units of read_blkbits.
		 * set_capacity takes units of 512 bytes.
		 */
		size = card->csd.capacity << (card->csd.read_blkbits - 9);
	}

    printk("card size is 0x%lx", size);
    card_flags_get(card);

	err = mmc_set_blocklen(card, 512);

	if (err) {
		printk( "unable to set block size to 512: %d\n",
			 err);
		return -EINVAL;
	}

	mmc_fixup_device(card, blk_fixups);

    return 0;
}

#if 0
/*
 * There is one mmc_blk_data per slot.
 */
struct mmc_blk_data {
	cyg_drv_mutex_t     lock;
    char    disk_name[12];
	struct mmc_card     *card;
	struct list_head part;

	unsigned int	flags;

	unsigned int	usage;
	unsigned int	read_only;
	unsigned int	part_type;
	unsigned int	name_idx;

	/*
	 * Only set in main mmc_blk_data associated
	 * with mmc_card with mmc_set_drvdata, and keeps
	 * track of the current selected device partition.
	 */
	unsigned int	part_curr;
	//struct device_attribute force_ro;
};

static int mmc_blk_alloc_part(struct mmc_card *card,
			      struct mmc_blk_data *md,
			      unsigned int part_type,
			      sector_t size,
			      bool default_ro,
			      const char *subname)
{
	char cap_str[10];
	struct mmc_blk_data *part_md;

	//part_md = mmc_blk_alloc_req(card, disk_to_dev(md->disk), size, default_ro,
	part_md = mmc_blk_alloc_req(card, size, default_ro,subname);
	if (IS_ERR(part_md))
		return PTR_ERR(part_md);
	part_md->part_type = part_type;
	list_add(&part_md->part, &md->part);

#if 0
	string_get_size((u64)get_capacity(part_md->disk) << 9, STRING_UNITS_2,
			cap_str, sizeof(cap_str));
#endif
	printk( "%s partition %u %s\n",
	       mmc_card_name(card), part_md->part_type, cap_str);
	return 0;
}

static int mmc_blk_alloc_parts(struct mmc_card *card, struct mmc_blk_data *md)
{
	int ret = 0;

	if (!mmc_card_mmc(card))
		return 0;

	if (card->ext_csd.boot_size) {
		ret = mmc_blk_alloc_part(card, md, EXT_CSD_PART_CONFIG_ACC_BOOT0,
					 card->ext_csd.boot_size >> 9,
					 true,
					 "boot0");
		if (ret)
			return ret;
		ret = mmc_blk_alloc_part(card, md, EXT_CSD_PART_CONFIG_ACC_BOOT1,
					 card->ext_csd.boot_size >> 9,
					 true,
					 "boot1");
		if (ret)
			return ret;
	}

	return ret;
}

static inline int mmc_blk_part_switch(struct mmc_card *card,
				      struct mmc_blk_data *md)
{
	int ret;
	struct mmc_blk_data *main_md = container_of(card, struct mmc_blk_data, card);  //mmc_get_drvdata(card);
	if (main_md->part_curr == md->part_type)
		return 0;

	if (mmc_card_mmc(card)) {
		card->ext_csd.part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
		card->ext_csd.part_config |= md->part_type;

		ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_PART_CONFIG, card->ext_csd.part_config,
				 card->ext_csd.part_time);
		if (ret)
			return ret;
    }

	main_md->part_curr = md->part_type;
	return 0;
}

static struct mmc_blk_data *mmc_blk_alloc_req(struct mmc_card *card,
					      //struct device *parent,
					      sector_t size,
					      bool default_ro,
					      const char *subname)
{
	struct mmc_blk_data *md;
	int devidx, ret;

	md = malloc(sizeof(struct mmc_blk_data));
	if (!md) {
		ret = -ENOMEM;
		goto out;
	}

	/*
	 * !subname implies we are creating main mmc_blk_data that will be
	 * associated with mmc_card with mmc_set_drvdata. Due to device
	 * partitions, devidx will not coincide with a per-physical card
	 * index anymore so we keep track of a name index.
	 */
	if (!subname) {
		//md->name_idx = find_first_zero_bit(name_use, max_devices);
		md->name_idx = card->host->index;
		//__set_bit(md->name_idx, name_use);
	}
#if 0
	else
		md->name_idx = ((struct mmc_blk_data *)
				dev_to_disk(parent)->private_data)->name_idx;
#endif
	/*
	 * Set the read-only status based on the supported commands
	 * and the write protect switch.
	 */
	md->read_only = mmc_blk_readonly(card);
#if 0
	md->disk = alloc_disk(perdev_minors);
	if (md->disk == NULL) {
		ret = -ENOMEM;
		goto err_kfree;
	}
#endif

	cyg_drv_mutex_init(&md->lock);
	INIT_LIST_HEAD(&md->part);
	md->usage = 1;
#if 0
	set_capacity(md->disk, size);
#endif

	if (mmc_host_cmd23(card->host)) {
		if (mmc_card_mmc(card) ||
		    (mmc_card_sd(card) &&
		     card->scr.cmds & SD_SCR_CMD23_SUPPORT))
			md->flags |= MMC_BLK_CMD23;
	}

	if (mmc_card_mmc(card) &&
	    md->flags & MMC_BLK_CMD23 &&
	    ((card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN) ||
	     card->ext_csd.rel_sectors)) {
		md->flags |= MMC_BLK_REL_WR;
	}

	return md;

 out:
	return ERR_PTR(ret);
}

#endif
