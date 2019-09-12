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

#include <cyg/io/mmc/core.h>
#include <cyg/io/mmc/fs.h>
#include <cyg/io/mmc/block.h>

#include "evatronix_host.h"
#include "evatronix_host_struct.h"
#include "evatronix_ops.h"

// Communication parameters. First some debug support
#define DEBUG 0 
#if DEBUG > 0
# define DEBUG1(format, ...)    diag_printf(format, ## __VA_ARGS__)
#else
# define DEBUG1(format, ...)
#endif
#if DEBUG > 1
# define DEBUG2(format, ...)    diag_printf(format, ## __VA_ARGS__)
#else
# define DEBUG2(format, ...)
#endif


Cyg_ErrNo
evtx_mmc_disk_read(disk_channel* chan, void* buf_arg, cyg_uint32 blocks, cyg_uint32 first_block)
{
    struct evatronix_slot*    disk    = (struct evatronix_slot*) chan->dev_priv;
    cyg_uint8*                  buf = (cyg_uint8*) buf_arg;
    Cyg_ErrNo                   code = ENOERR;
    struct request      req;
    
    DEBUG1("evtx_mmc_disk_read(): first block %d, buf %p, len %lu blocks (%lu bytes)\n",
           first_block, buf, (unsigned long)blocks, (unsigned long)blocks*512);

    if (! disk->mmc_connected) {
        return -ENODEV;
    }

    if ((first_block + blocks) >= disk->mmc_block_count) {
        return -EINVAL;
    }

    req.cmd_flags = READ;
    req.pos = first_block;
    req.blocks = blocks;
    req.buf = buf;
    req.blksz = 512;

    code = mmc_blk_issue_rq(disk->mmc->card, &req);

    return code;
}

Cyg_ErrNo
evtx_mmc_disk_write(disk_channel* chan, const void* buf_arg, cyg_uint32 blocks, cyg_uint32 first_block)
{
    struct evatronix_slot*    disk    = (struct evatronix_slot*) chan->dev_priv;
    const cyg_uint8*            buf = (cyg_uint8*) buf_arg;
    Cyg_ErrNo                   code = ENOERR;
    struct request      req;

    DEBUG2("evtx_mmc_disk_write(): first block %d, buf %p, len %lu blocks (%lu bytes)\n",
           first_block, buf, (unsigned long)blocks, (unsigned long)blocks*512);

    if (! disk->mmc_connected) {
        return -ENODEV;
    }

    if (disk->read_only) {
        return -EROFS;
    }

    if ((first_block + blocks) >= disk->mmc_block_count) {
        return -EINVAL;
    }

    req.cmd_flags = WRITE;
    req.pos = first_block;
    req.blocks = blocks;
    req.buf =(void *)buf;
    req.blksz = 512;

    code = mmc_blk_issue_rq(disk->mmc->card, &req);
    
    return code;
}

// get_config() and set_config(). There are no supported get_config() operations
// at this time.
Cyg_ErrNo
evtx_mmc_disk_get_config(disk_channel* chan, cyg_uint32 key, const void* buf, cyg_uint32* len)
{
    CYG_UNUSED_PARAM(disk_channel*, chan);
    CYG_UNUSED_PARAM(cyg_uint32, key);
    CYG_UNUSED_PARAM(const void*, buf);
    CYG_UNUSED_PARAM(cyg_uint32*, len);
    
    return -EINVAL;
}

Cyg_ErrNo
evtx_mmc_disk_set_config(disk_channel* chan, cyg_uint32 key, const void* buf, cyg_uint32* len)
{
    Cyg_ErrNo                   result  = ENOERR;
    struct evatronix_slot*    disk    = (struct evatronix_slot*) chan->dev_priv;

    switch(key) {
      case CYG_IO_SET_CONFIG_DISK_MOUNT:
        // There will have been a successful lookup(), so there's
        // little point in checking the disk again.
        break;

      case CYG_IO_SET_CONFIG_DISK_UMOUNT:
        if (0 == chan->info->mounts) {
            // If this is the last unmount of the card, mark it as
            // disconnected. If the user then removes the card and
            // plugs in a new one everything works cleanly. Also
            // reset the SPI device's clockrate.
            disk->mmc_connected = false;
            //mmc_spi_restore_baud(disk);
            result = (chan->callbacks->disk_disconnected)(chan);
        }
        break;
    }

    return result;
}

void evtx_mmc_get_disk_info(struct mmc_card *card, cyg_disk_info_t *info)
{
	sector_t size;

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

    printk("blocks_num 0x%lx ", size);
    info->block_size = 512;
    info->phys_block_size = 512;
    info->blocks_num = (u32)size;
}

int evtx_mmc_parse_mbr(disk_channel* chan,  cyg_disk_identify_t  *ident)
{
    static cyg_uint8 __attribute__((aligned(4)))data[512];
    cyg_uint8*  partition;
    cyg_uint32  lba_first, lba_size, lba_end, head, cylinder, sector;
    int code=0;

    code = evtx_mmc_disk_read(chan, data, 1, 0);
    if (code) {
        return code;
    }
#if DEBUG > 1
    {
        cyg_uint8 *ptr_data;
        int i=0;

        DEBUG2("MBR dump\n");
        for (i = 0; i < 512; i += 16) {
            ptr_data = &data[i];
            DEBUG2(" %04x: %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
                    i,
                    ptr_data[ 0], ptr_data[ 1], ptr_data[ 2], ptr_data[ 3],
                    ptr_data[ 4], ptr_data[ 5], ptr_data[ 6], ptr_data[ 7],
                    ptr_data[ 8], ptr_data[ 9], ptr_data[10], ptr_data[11],
                    ptr_data[12], ptr_data[13], ptr_data[14], ptr_data[15]);
        }
    }
#endif
#if DEBUG > 0
    DEBUG1("Read block 0 (partition table)\n");
    DEBUG1("Signature 0x%02x 0x%02x, should be 0x55 0xaa\n", data[0x1fe], data[0x1ff]);
    // There should be four 16-byte partition table entries at offsets
    // 0x1be, 0x1ce, 0x1de and 0x1ee. The numbers are stored little-endian
    for (i = 0; i < 4; i++) {
        partition = &(data[0x1be + (0x10 * i)]);
        DEBUG1("Partition %d: boot %02x, first sector %02x %02x %02x, file system %02x, last sector %02x %02x %02x\n", i,   \
                partition[0], partition[1], partition[2], partition[3], partition[4], \
                partition[5], partition[6], partition[7]);
        DEBUG1("           : first sector (linear) %02x %02x %02x %02x, sector count %02x %02x %02x %02x\n", \
                partition[11], partition[10], partition[9], partition[8], \
                partition[15], partition[14], partition[13], partition[12]);
    }
#endif        
    if ((0x0055 != data[0x1fe]) || (0x00aa != data[0x1ff])) {
        return -ENOTDIR;
    }
    partition   = &(data[0x1be]);
    lba_first   = (partition[11] << 24) | (partition[10] << 16) | (partition[9] << 8) | partition[8];
    lba_size    = (partition[15] << 24) | (partition[14] << 16) | (partition[13] << 8) | partition[12];
    lba_end     = lba_first + lba_size - 1;

    // First sector in c/h/s format
    cylinder    = ((partition[2] & 0xC0) << 2) | partition[3];
    head        = partition[1];
    sector      = partition[2] & 0x3F;

    // lba_start == (((cylinder * Nh) + head) * Ns) + sector - 1, where (Nh == heads/cylinder) and (Ns == sectors/head)
    // Strictly speaking we should be solving some simultaneous
    // equations here for lba_start/lba_end, but that gets messy.
    // The first partition is at the start of the card so cylinder will be 0,
    // and we can ignore Nh.
    //CYG_ASSERT(0 != cylinder, "Driver assumption - partition 0 is at start of card\n");
    CYG_ASSERT(0 == head,     "Driver assumption - partition table is sensible\n");
    ident->sectors_num  = ((lba_first + 1) - sector) / head;

    // Now for lba_end.
    cylinder    = ((partition[6] & 0xC0) << 2) | partition[7];
    head        = partition[5];
    sector      = partition[6] & 0x3F;
    ident->heads_num  = ((((lba_end + 1) - sector) / ident->sectors_num) - head) / cylinder;

    return 0;
}

static inline int mmc_blk_readonly(struct mmc_card *card)
{
	return mmc_card_readonly(card) ||
	       !(card->csd.cmdclass & CCC_BLOCK_WRITE);
}

static char* itoa(char* dest, unsigned int v)
{
    char b[16];
    char* p = &b[16];

    *--p = 0;
    if (v) {
        while (v){
            *--p = (v % 10) + '0';
            v = v / 10;
        }
    } else
        *--p = '0';

    return strcpy(dest, p);
}

Cyg_ErrNo
evtx_mmc_disk_lookup(struct cyg_devtab_entry** tab, struct cyg_devtab_entry *sub_tab, const char* name)
{
    disk_channel*               chan    = (disk_channel*) (*tab)->priv;
    struct evatronix_slot*     disk    = (struct evatronix_slot*) chan->dev_priv;
    Cyg_ErrNo                   result;

    DEBUG2("evtx_mmc_disk_lookup(): target name=%s\n", name );
    DEBUG2("                     : device name=%s dep_name=%s\n", tab[0]->name, tab[0]->dep_name );
    DEBUG2("                     : sub    name=%s dep_name=%s\n", sub_tab->name, sub_tab->dep_name );

    if (disk->mmc_connected) {
        // There was a card plugged in last time we looked. Is it still there?
        if (!evatronix_get_cd(disk->mmc, NULL)) {
            // The old card is gone. Either there is no card plugged in, or
            // it has been replaced with a different one. If the latter the
            // existing mounts must be removed before anything sensible
            // can be done.
            disk->mmc_connected = false;
            (*chan->callbacks->disk_disconnected)(chan);
            if (0 != chan->info->mounts) {
                return -ENODEV;
            }
        }
    }

    if ((0 != chan->info->mounts) && !disk->mmc_connected) {
        // There are still mount points to an old card. We cannot accept
        // new mount requests until those have been cleaned out.
        return -ENODEV;
    }

    if (!disk->mmc_connected) {
        //struct mmc_card *card;
        cyg_disk_identify_t ident;
        cyg_disk_info_t         *info;        // disk info 

        //card = disk->mmc->card; 
        
        cyg_drv_mutex_lock(&chan->controller->lock);
        result = evatronix_slot_init(disk);

        if(result)
        {
            err("slot%d: evatronix_slot_init failed", disk->id);
            cyg_drv_mutex_unlock(&chan->controller->lock);
            return result;
        }

        mmc_rescan_new(disk->mmc);
        result = mmc_blk_probe(disk->mmc->card);

        if(result)
        {
            err("slot%d: mmc_blk_probe failed, ret = %d", disk->id, result);
            cyg_drv_mutex_unlock(&chan->controller->lock);
            return result;
        }

        info = chan->info;
        evtx_mmc_get_disk_info(disk->mmc->card, info);

        disk->read_only = mmc_blk_readonly(disk->mmc->card);
        disk->mmc_block_count = info->blocks_num;

        VDBG("slot%d: has been initialized, ret = %d, blocks_num %d", disk->id, result, info->blocks_num);
        // A card has been found. Tell the higher-level code about it.
        // This requires an identify structure, although it is not
        // entirely clear what purpose that serves.
        disk->mmc_connected = true;
        // Serial number, up to 20 characters; The CID register contains
        // various fields which can be used for this.
        memcpy(ident.model_num, disk->mmc->card->cid.prod_name, 8); 
        ident.model_num[9] = '\0';

        itoa(ident.serial, disk->mmc->card->cid.serial);
        itoa(ident.firmware_rev, disk->mmc->card->cid.manfid);

        // We don't have no cylinders, heads, or sectors, but
        // higher-level code may interpret partition data using C/H/S
        // addressing rather than LBA. Hence values for some of these
        // settings were calculated above.
        //evtx_mmc_parse_mbr(chan,  &ident);

        ident.cylinders_num     = 1;
        //ident.heads_num         = disk->mmc_heads_per_cylinder;
        //ident.sectors_num       = disk->mmc_sectors_per_head;
        ident.lba_sectors_num   = info->blocks_num;
        ident.phys_block_size   = info->block_size;// disk->mmc_write_block_length/512;
        ident.max_transfer      = disk->mmc->max_blk_count;

        cyg_drv_mutex_unlock(&chan->controller->lock);

        printk("Calling disk_connected(): serial %s, firmware %s, model %s, heads %d, sectors %d, lba_sectors_num %d, phys_block_size %d\n, max_transfer %d", \
               ident.serial, ident.firmware_rev, ident.model_num, ident.heads_num, ident.sectors_num,
               ident.lba_sectors_num, ident.phys_block_size, ident.max_transfer);
        (*chan->callbacks->disk_connected)(*tab, &ident);

        // We now have a valid card and higher-level code knows about it. Fall through.
    }

    // And leave it to higher-level code to finish the lookup, taking
    // into accounts partitions etc.
    return (*chan->callbacks->disk_lookup)(tab, sub_tab, name);
}
