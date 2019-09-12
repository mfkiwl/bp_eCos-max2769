//==========================================================================
//
//        mmc_test.c
//
//        mmc performance test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2017-10-18
// Description:   mmc performance test for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <cyg/kernel/kapi.h>
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/infra/diag.h>

#include <pkgconf/hal.h>

#include <cyg/infra/diag.h>

#include <cyg/io/io.h>
#include <cyg/io/config_keys.h>
#include <errno.h>
#include <string.h>
#include <cyg/hal/hal_if.h>

#ifdef CYGPKG_DEVS_DISK_MMC
#include <pkgconf/devs_disk_mmc.h>
#endif

#ifdef CYGVAR_DEVS_BP2016_EVATRONIX_SDMMC_EN
#include <pkgconf/io_disk.h>
#include <cyg/io/disk.h>
#include <cyg/io/diskio.h>

// this is for debug trace,
#define mmc_printf(fmt, args...)                                                               \
    printf("[mmc test] %s:%d " fmt "\n", __func__, __LINE__, ##args);

#define MMC_TEST_OFFSET 0
#define MMC_TEST_LENGTH 0x2000
#define MMC_TEST_DEVICE "/dev/sdmmc0/"

#define CYG_MMC_ERR_OK      0

//===========================================================================
//                               DATA TYPES
//===========================================================================

static void dump_mmc_buffer(void *buffer, cyg_uint32 len)
{
    cyg_uint32 i = 0;
    cyg_uint8 *buf = (cyg_uint8 *) buffer;

    printf("\n");
    for (i = 0; i < len; i++) {
		if(!(i%16))
			printf("\n%08x: ", i);
        printf("%02x ", *(buf + i));
    }
    printf("\n");
}

#define TIME_OUT (100)

static int mmc_init = 0;

static cyg_io_handle_t mmc_handle;
//static cyg_uint8 *test_buf1 = NULL, *test_buf2 = NULL;
static cyg_uint8 test_buf1[MMC_TEST_LENGTH], test_buf2[MMC_TEST_LENGTH];
static CYG_ADDRWORD phys_block_size=0;
static cyg_uint32 block_size, mmc_num_blocks;

int do_mmc_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    cyg_uint32 res = 0;
    cyg_uint32 i = 0, len = 0;
    cyg_disk_info_t info;

    cyg_uint32 addr;
    Cyg_ErrNo stat;

    mmc_printf("\n-------------mmc thread test----------------------\n");

    if (strcmp(argv[1], "init") == 0)
	{
		stat = cyg_io_lookup(MMC_TEST_DEVICE, &mmc_handle);
		if (stat == -ENOENT) {
			diag_printf("MMC: driver init failed: %s\n", strerror(-stat));
			return -EINVAL;
		}
		diag_printf("MMC: driver init ret = %d\n", stat);

		mmc_init = 1;

        len = sizeof(cyg_disk_info_t);
        stat = cyg_io_get_config(mmc_handle, CYG_IO_GET_CONFIG_DISK_INFO, &info, &len);
        if (ENOERR != stat) {
            diag_printf("%s: error, ret = %d \n", __func__, stat);
            return 0;
        } 

        block_size = info.block_size;
		mmc_num_blocks = info.blocks_num;
        phys_block_size = info.phys_block_size;

		diag_printf("MMC: %d blocks of 0x%x bytes each.\n", 
				mmc_num_blocks, block_size);
    }

    if (strcmp(argv[1], "rd") == 0) {

        if (argc < 4) {
            mmc_printf("command error!\r\n	cmd: mmc rd addr len");
            return ENOERR;
        }

        addr = (cyg_uint32) atoi(argv[2]);
        len = (cyg_uint32) atoi(argv[3]);

		if(len>MMC_TEST_LENGTH)
		{
			diag_printf("read len is too long, set to 0x%x\n", MMC_TEST_LENGTH);
			len = MMC_TEST_LENGTH;
		}

		memset(test_buf1, 0, MMC_TEST_LENGTH);
		mmc_printf("read addr 0x%x len %u", addr, len);
        res = len/512;
		if ((stat = cyg_io_bread(mmc_handle,  (void *)test_buf1, &res, addr)) != CYG_MMC_ERR_OK) {
			diag_printf("MMC: read error  %d\n", stat);
			return -EINVAL;
		}    

        dump_mmc_buffer(test_buf1, len);

        return ENOERR;
    }

    if (strcmp(argv[1], "wr") == 0) {

        if (argc < 4) {
            mmc_printf("command error!\r\n	cmd: mmc wr addr len");
            return ENOERR;
        }

        addr = (cyg_uint32) atoi(argv[2]);
        len = (cyg_uint32) atoi(argv[3]);

		if(len>MMC_TEST_LENGTH)
		{
			diag_printf("read len is too long, set to 0x%x\n", MMC_TEST_LENGTH);
			len = MMC_TEST_LENGTH;
		}

		for(i=0; i<MMC_TEST_LENGTH;i++)
		{
			test_buf1[i] = 0xff - (i & 0xff);
		}
        res = len/512;
		if ((stat = cyg_io_bwrite(mmc_handle,  (void *)test_buf1, &res, addr)) != CYG_MMC_ERR_OK) {
			diag_printf("MMC: read/verify after write failed: %s\n", strerror(stat));
			return -EINVAL;
		}    

        return ENOERR;
    }

    if (strcmp(argv[1], "auto") == 0) {

		/* test all chip erase/read/write */	
        unsigned int step = MMC_TEST_LENGTH/block_size;
        int mmc_end = mmc_num_blocks - step;
#define     TEST_START      (30)


		/* select one block do erase/read/write 0x55, verify it corrected */
		mmc_printf("test write 0x55 begin!");
		memset(test_buf1, 0x55, MMC_TEST_LENGTH);
		addr =	155;
		len = MMC_TEST_LENGTH;
        step = len/block_size;

		if ((stat = cyg_io_bwrite(mmc_handle,  (void *)test_buf1, &step, addr)) != CYG_MMC_ERR_OK) {
			diag_printf("MMC %d: write after erase failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		memset(test_buf2, 0, len);
		if ((stat = cyg_io_bread(mmc_handle,  (void *)test_buf2, &step, addr)) != CYG_MMC_ERR_OK) {
			diag_printf("MMC %d: read after write failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		for(res=0; res<len; res++)
		{
			if(test_buf1[res] != test_buf2[res])
			{
				diag_printf("write 0x55 error %d: offset 0x%x -- value 0x%x\n", __LINE__, res, test_buf2[res]);
				return -1;	
			}
		}

		/* select one block do erase/read/write 0x55, verify it corrected end */

		/* select one block do erase/read/write 0xaa, verify it corrected */
		mmc_printf("test write 0xaa begin!");
		memset(test_buf1, 0xaa, MMC_TEST_LENGTH);
		addr =	block_size*6;
		len = MMC_TEST_LENGTH;
        step = len/block_size;

		if ((stat = cyg_io_bwrite(mmc_handle,  (void *)test_buf1, &step, addr)) != CYG_MMC_ERR_OK) {
			diag_printf("MMC %d: write after erase failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		memset(test_buf2, 0, len);
		if ((stat = cyg_io_bread(mmc_handle,  (void *)test_buf2, &step, addr)) != CYG_MMC_ERR_OK) {
			diag_printf("MMC %d: read after write failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		for(res=0; res<len; res++)
		{
			if(test_buf1[res] != test_buf2[res])
			{
				diag_printf("write 0xaa error %d: offset 0x%x -- value 0x%x\n", __LINE__, res, test_buf2[res]);
				return -1;
			}
		}

		/* select one block do erase/read/write 0, verify it corrected */
		mmc_printf("test write 0 begin!");
		memset(test_buf1, 0, MMC_TEST_LENGTH);
		addr =	block_size*7;
		len = MMC_TEST_LENGTH;
        step = len/block_size;

		if ((stat = cyg_io_bwrite(mmc_handle,  (void *)test_buf1, &step, addr)) != CYG_MMC_ERR_OK) {
			diag_printf("MMC %d: write after erase failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		memset(test_buf2, 0, len);
		if ((stat = cyg_io_bread(mmc_handle,  (void *)test_buf2, &step, addr)) != CYG_MMC_ERR_OK) {
			diag_printf("MMC %d: read after write failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		for(res=0; res<len; res++)
		{
			if(0 != test_buf2[res])
			{
				diag_printf("write 0 error %d: offset 0x%x -- value 0x%x\n", __LINE__, res, test_buf2[res]);
				return -1;
			}
		}

		//erase all
		mmc_printf("test all chip read/write, chip size:0x%x, begin ...", mmc_end);

		for(i=0; i<MMC_TEST_LENGTH;i++)
		{
			test_buf1[i] = i&0xff;
		}

        step = MMC_TEST_LENGTH/block_size;

		for(i=TEST_START; i<mmc_end; i+=step)
		{
			len = MMC_TEST_LENGTH;
			if ((stat = cyg_io_bwrite(mmc_handle,  (void *)test_buf1, &step, i)) != CYG_MMC_ERR_OK) {
				diag_printf("MMC %d: write failed: %s\n", __LINE__, strerror(stat));
				return -EINVAL;
			}

			memset(test_buf2, 0, len);
			if ((stat = cyg_io_bread(mmc_handle,  (void *)test_buf2, &step, i)) != CYG_MMC_ERR_OK) {
				diag_printf("MMC %d: read after write failed: %s\n", __LINE__, strerror(stat));
				return -EINVAL;
			}

			for(res=0; res<len; res++)
			{
				if(test_buf1[res] != test_buf2[res])
				{
					diag_printf("write error: addr 0x%x -- raw: %d, rd %d\n", i+res, test_buf1[res], test_buf2[res]);
					dump_mmc_buffer(test_buf1, len);
					dump_mmc_buffer(test_buf2, len);
					return -EINVAL;
					//break;
				}
			}
		}

		mmc_printf("test all chip erase, read/write, end! ...");

        cyg_thread_delay(TIME_OUT);
    }

    if (strcmp(argv[1], "thread") == 0) {

        extern void mmc_thread_test(void);
        mmc_thread_test();
    }

    return 0;
}

#endif /* CYGVAR_DEVS_BP2016_EVATRONIX_SDMMC_EN */
