//==========================================================================
//
//        qspi_test.c
//
//        qspi performance test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2017-10-18
// Description:   qspi performance test for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <cyg/kernel/kapi.h>
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/infra/diag.h>

#include <pkgconf/hal.h>

#include <cyg/io/io.h>
#include <cyg/io/config_keys.h>
#include <errno.h>
#include <string.h>
#include <cyg/hal/hal_if.h>

#ifdef CYGPKG_DEVS_QSPI_BP2016
#include <pkgconf/io_flash.h>
#include <cyg/io/flash.h>
#include <pkgconf/devs_qspi_bp2016.h>
#include <cyg/hal/api/qspi_api.h>

// this is for debug trace,
#define qspi_printf(fmt, args...)                                                               \
    printf("[qspi test] %s:%d " fmt "\n", __func__, __LINE__, ##args);

#define FLASH_TEST_OFFSET 0
#define FLASH_TEST_LENGTH 0x2000
//#define FLASH_TEST_DEVICE "/dev/flash/0/0x100000"
#define FLASH_TEST_DEVICE "/dev/flash/0/0"

#define qspi_tick_to_us(tick)   (tick/archtimer_perus_to_tick)
//===========================================================================
//                               DATA TYPES
//===========================================================================

static void dump_qspi_buffer(void *buffer, cyg_uint32 len)
{
    cyg_uint32 i = 0;
    cyg_uint8 *buf = (cyg_uint8 *) buffer;

    printf("\n");
    for (i = 0; i < len; i++) {
		if(!(i%16))
			printf("\n0x%x:", i);
        printf("%02x ", *(buf + i));
    }
    printf("\n");
}

#define TIME_OUT (100)

static int qspi_init = 0;

static cyg_io_handle_t flash_handle;
//static cyg_uint8 *test_buf1 = NULL, *test_buf2 = NULL;
static cyg_uint8 __attribute__((aligned(4096)))test_buf1[FLASH_TEST_LENGTH], __attribute__((aligned(4096)))test_buf2[FLASH_TEST_LENGTH];
//static cyg_uint8 test_buf1[FLASH_TEST_LENGTH], test_buf2[FLASH_TEST_LENGTH];
static CYG_ADDRWORD flash_start=0, flash_end=0;
static cyg_uint32 block_size, flash_num_blocks;

int do_qspi_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int res = 0;
    cyg_uint32 i = 0, len = 0;
    u64 s, e;
    unsigned int us=0;

    cyg_uint32 addr;
    Cyg_ErrNo stat;

    qspi_printf("\n-------------qspi thread test----------------------\n");

	if(!qspi_init)
	{
		stat = qspi_flash_init(&flash_handle, FLASH_TEST_DEVICE);
		if (stat != 0) {
			printf("FLASH: driver init failed: %s\n", strerror(-stat));
			return -EINVAL;
		}
#if 0 
		test_buf1 = malloc(2*FLASH_TEST_LENGTH); 

		if(!test_buf1)
		{
			printf("MALLOC buffer failed!");
			return -ENOMEM;
		}

		test_buf2 = test_buf1 + FLASH_TEST_LENGTH;
#endif
		qspi_init = 1;

		flash_end = qspi_flash_get_flashsize(flash_handle) - 1;
		printf("Flash chip size: 0x%x B\r\n", flash_end);

		len = qspi_flash_get_flashmem(flash_handle);
		printf("Flash chip MEM addr: 0x%x B\r\n", len);

		block_size = qspi_flash_get_blocksize(flash_handle);
		flash_num_blocks = flash_end/block_size;

		printf("FLASH: 0x%x - 0x%x, %d blocks of 0x%x bytes each.\n", 
				flash_start, flash_end + 1, flash_num_blocks, block_size);
    }

    if (strcmp(argv[1], "lock") == 0) {

        if (argc < 4) {
            qspi_printf("command error!\r\n	cmd: qspi lock addr len");
            return ENOERR;
        }
        addr= simple_strtoul(argv[2], NULL, 16);
        len = (cyg_uint32) atoi(argv[3]);

		qspi_flash_lock(flash_handle, addr, len);
        return ENOERR;
    }

    if (strcmp(argv[1], "unlock") == 0) {

        if (argc < 4) {
            qspi_printf("command error!\r\n	cmd: qspi unlock addr len");
            return ENOERR;
        }
        addr= simple_strtoul(argv[2], NULL, 16);
        len = (cyg_uint32) atoi(argv[3]);

		qspi_flash_unlock(flash_handle, addr, len);
        return ENOERR;
    }

    if (strcmp(argv[1], "rglock") == 0) {

        if (argc < 3) {
            qspi_printf("command error!\r\n	cmd: qspi rglock number");
            return ENOERR;
        }
        addr = (cyg_uint32) atoi(argv[2]);

		flashiodev_region_lock(flash_handle, addr);
        return ENOERR;
    }

    if (strcmp(argv[1], "rgulock") == 0) {

        if (argc < 3) {
            qspi_printf("command error!\r\n	cmd: qspi rgulock number");
            return ENOERR;
        }
        addr = (cyg_uint32) atoi(argv[2]);

		flashiodev_region_unlock(flash_handle, addr);
        return ENOERR;
    }

    if (strcmp(argv[1], "globallock") == 0) {

		flashiodev_global_lock(flash_handle);
        return ENOERR;
    }

    if (strcmp(argv[1], "globalunlock") == 0) {

		flashiodev_global_unlock(flash_handle);
        return ENOERR;
    }

    if (strcmp(argv[1], "erase") == 0) {

        if (argc < 4) {
            qspi_printf("command error!\r\n	cmd: qspi erase addr len");
            return ENOERR;
        }
        addr= simple_strtoul(argv[2], NULL, 16);
        len = (cyg_uint32) atoi(argv[3]);

		qspi_flash_erase(flash_handle, addr, len);
        return ENOERR;
    }

    if (strcmp(argv[1], "serase") == 0) {

        if (argc < 4) {
            qspi_printf("command error!\r\n	cmd: qspi erase addr len");
            return ENOERR;
        }
        addr= simple_strtoul(argv[2], NULL, 16);
        len = (cyg_uint32) atoi(argv[3]);

        qspi_printf("sector erase, sector size is 0x%x!\r\n", qspi_flash_get_sectorsize(flash_handle));
		qspi_flash_sector_erase(flash_handle, addr, len);
        return ENOERR;
    }

    if (strcmp(argv[1], "rd") == 0) {

        if (argc < 4) {
            qspi_printf("command error!\r\n	cmd: qspi rd addr len");
            return ENOERR;
        }

        addr= simple_strtoul(argv[2], NULL, 16);
        len = (cyg_uint32) atoi(argv[3]);

		if(len>FLASH_TEST_LENGTH)
		{
			printf("read len is too long, set to 0x%x\n", FLASH_TEST_LENGTH);
			len = FLASH_TEST_LENGTH;
		}

		memset(test_buf1, 0, FLASH_TEST_LENGTH);
		qspi_printf("read addr 0x%x len %u", addr, len);
		//if ((stat = cyg_io_bread(flash_handle,  (void *)test_buf1, &len, addr)) != CYG_FLASH_ERR_OK) {
		if ((stat = qspi_flash_read(flash_handle,  (void *)test_buf1, &len, addr)) != CYG_FLASH_ERR_OK) {
			printf("FLASH: read error  %d\n", stat);
			return -EINVAL;
		}    

        dump_qspi_buffer(test_buf1, len);

        return ENOERR;
    }

    if (strcmp(argv[1], "wr") == 0) {

        if (argc < 4) {
            qspi_printf("command error!\r\n	cmd: qspi wr addr len");
            return ENOERR;
        }

        //addr = (cyg_uint32) atoi(argv[2]);
        addr= simple_strtoul(argv[2], NULL, 16);
        len = (cyg_uint32) atoi(argv[3]);

		if(len>FLASH_TEST_LENGTH)
		{
			printf("read len is too long, set to 0x%x\n", FLASH_TEST_LENGTH);
			len = FLASH_TEST_LENGTH;
		}

		for(i=0; i<FLASH_TEST_LENGTH;i++)
		{
			test_buf1[i] = 0xff - (i & 0xff);
		}

		qspi_printf("write addr 0x%x len %u", addr, len);
		//if ((stat = cyg_io_bwrite(flash_handle,  (void *)test_buf1, &len, addr)) != CYG_FLASH_ERR_OK) {
		if ((stat = qspi_flash_write(flash_handle,  (void *)test_buf1, &len, addr)) != CYG_FLASH_ERR_OK) {
			printf("FLASH: read/verify after write failed: %s\n", strerror(stat));
			return -EINVAL;
		}    

        return ENOERR;
    }

    if (strcmp(argv[1], "auto") == 0) {

		/* test all chip erase/read/write */	

		//erase all
		qspi_printf("test all chip erase, read/write, chip size:0x%x, begin ...", flash_end);

        s = HAL_GET_COUNTER;
		qspi_flash_erase(flash_handle, 0, flash_end);
        e = HAL_GET_COUNTER;
        us = (unsigned int)qspi_tick_to_us((e - s));
        printf("###### erase all chip need %d us (%d ms)\n", us, us / 1000);

        us = 0;

		for(i=0; i<flash_end; i+=FLASH_TEST_LENGTH)
		{
			len = FLASH_TEST_LENGTH;

			/* check if erase ok */
			memset(test_buf2, 0, len);

            s = HAL_GET_COUNTER;
			if ((stat = cyg_io_bread(flash_handle,  (void *)test_buf2, &len, i)) != CYG_FLASH_ERR_OK) {
				printf("FLASH %d: read after erase failed: %s\n", __LINE__, strerror(stat));
				return -EINVAL;
			}    

            e = HAL_GET_COUNTER;
            us += qspi_tick_to_us((unsigned long) (e - s));

			for(res=0; res<len; res++)
			{
				if( 0xff != test_buf2[res])
				{
					printf("erase error: addr 0x%x, offset 0x%x value %d\n",i, res, test_buf2[res]);
                    printf("buffer addr: 1 -- %p 2 -- %p\n", test_buf1, test_buf2);
					dump_qspi_buffer(test_buf2, len);
					return -EINVAL;
				}
			}
		}
        printf("###### read all chip need %d us (%d ms)\n", us, us / 1000);

		for(i=0; i<FLASH_TEST_LENGTH;i++)
		{
			test_buf1[i] = i&0xff;
		}

        us = 0;
		for(i=0; i<flash_end; i+=FLASH_TEST_LENGTH)
		{
			len = FLASH_TEST_LENGTH;
            s = HAL_GET_COUNTER;
			if ((stat = cyg_io_bwrite(flash_handle,  (void *)test_buf1, &len, i)) != CYG_FLASH_ERR_OK) {
				printf("FLASH %d: write failed: %s\n", __LINE__, strerror(stat));
				return -EINVAL;
			}    
            e = HAL_GET_COUNTER;
            us += qspi_tick_to_us((unsigned long) (e - s));

			memset(test_buf2, 0, len);
			if ((stat = cyg_io_bread(flash_handle,  (void *)test_buf2, &len, i)) != CYG_FLASH_ERR_OK) {
				printf("FLASH %d: read after write failed: %s\n", __LINE__, strerror(stat));
				return -EINVAL;
			}    

			for(res=0; res<len; res++)
			{
				if(test_buf1[res] != test_buf2[res])
				{
					printf("write error: addr 0x%x -- raw: %d, rd %d\n", i+res, test_buf1[res], test_buf2[res]);
                    printf("buffer addr: 1 -- %p 2 -- %p\n", test_buf1, test_buf2);
					//dump_qspi_buffer(test_buf1, len);
					dump_qspi_buffer(test_buf2, len);
					return -EINVAL;
					//break;
				}
			}

		}
        printf("###### write all chip need %d us (%d ms)\n", us, us / 1000);
		qspi_printf("test all chip erase, read/write, end! ...");
		/* test all chip erase/read/write end */

		/* select one block do erase/read/write 0x55, verify it corrected */
		qspi_printf("test write 0x55 begin!");
		memset(test_buf1, 0x55, FLASH_TEST_LENGTH);
		addr =	block_size*5;
		len = FLASH_TEST_LENGTH;

		qspi_flash_erase(flash_handle, addr, len);

		if ((stat = cyg_io_bwrite(flash_handle,  (void *)test_buf1, &len, addr)) != CYG_FLASH_ERR_OK) {
			printf("FLASH %d: write after erase failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		memset(test_buf2, 0, len);
		if ((stat = cyg_io_bread(flash_handle,  (void *)test_buf2, &len, addr)) != CYG_FLASH_ERR_OK) {
			printf("FLASH %d: read after write failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		for(res=0; res<len; res++)
		{
			if(test_buf1[res] != test_buf2[res])
			{
				printf("write 0x55 error %d: offset 0x%x -- value 0x%x\n", __LINE__, res, test_buf2[res]);
				return -1;	
			}
		}

		/* select one block do erase/read/write 0x55, verify it corrected end */

		/* select one block do erase/read/write 0xaa, verify it corrected */
		qspi_printf("test write 0xaa begin!");
		memset(test_buf1, 0xaa, FLASH_TEST_LENGTH);
		addr =	block_size*6;
		len = FLASH_TEST_LENGTH;

		qspi_flash_erase(flash_handle, addr, len);

		if ((stat = cyg_io_bwrite(flash_handle,  (void *)test_buf1, &len, addr)) != CYG_FLASH_ERR_OK) {
			printf("FLASH %d: write after erase failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		memset(test_buf2, 0, len);
		if ((stat = cyg_io_bread(flash_handle,  (void *)test_buf2, &len, addr)) != CYG_FLASH_ERR_OK) {
			printf("FLASH %d: read after write failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		for(res=0; res<len; res++)
		{
			if(test_buf1[res] != test_buf2[res])
			{
				printf("write 0xaa error %d: offset 0x%x -- value 0x%x\n", __LINE__, res, test_buf2[res]);
				return -1;
			}
		}

		/* select one block do erase/read/write 0xaa, verify it corrected end */

		/* select one block do erase/read/write 0, verify it corrected */
		qspi_printf("test write 0 begin!");
		memset(test_buf1, 0, FLASH_TEST_LENGTH);
		addr =	block_size*7;
		len = FLASH_TEST_LENGTH;

		qspi_flash_erase(flash_handle, addr, len);

		if ((stat = cyg_io_bwrite(flash_handle,  (void *)test_buf1, &len, addr)) != CYG_FLASH_ERR_OK) {
			printf("FLASH %d: write after erase failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		memset(test_buf2, 0, len);
		if ((stat = cyg_io_bread(flash_handle,  (void *)test_buf2, &len, addr)) != CYG_FLASH_ERR_OK) {
			printf("FLASH %d: read after write failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		for(res=0; res<len; res++)
		{
			if(0 != test_buf2[res])
			{
				printf("write 0 error %d: offset 0x%x -- value 0x%x\n", __LINE__, res, test_buf2[res]);
				return -1;
			}
		}

		/* select one block do erase/read/write 0x0, verify it corrected end */

		/************* unalign read/write/erase test ****************/

		qspi_printf("test unalign address read/write begin!");
#define	UNALIGN_TEST_ADDR	(137)

		for(i = 0; i < FLASH_TEST_LENGTH; i++)
		{
			test_buf1[i] = i&0xff;
		}

		addr =	block_size*9+UNALIGN_TEST_ADDR;

		len = UNALIGN_TEST_ADDR;
		qspi_flash_erase(flash_handle, addr, len);

		if ((stat = cyg_io_bwrite(flash_handle,  (void *)test_buf1, &len, addr)) != CYG_FLASH_ERR_OK) {
			printf("FLASH %d: write after erase failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		memset(test_buf2, 0, len);
		if ((stat = cyg_io_bread(flash_handle,  (void *)test_buf2, &len, addr)) != CYG_FLASH_ERR_OK) {
			printf("FLASH %d: read after write failed: %s\n", __LINE__, strerror(stat));
			return -EINVAL;
		}    

		for(res=0; res<len; res++)
		{
			if(test_buf1[res] != test_buf2[res])
			{
				printf("compare error %d: %d -- raw: %d, rd %d\n",__LINE__, res, test_buf1[res], test_buf2[res]);
				break;
			}
		}

		/************* unalign read/write/erase test end ****************/

        cyg_thread_delay(TIME_OUT);
    }

    if (strcmp(argv[1], "thread") == 0) {

        extern void qspi_thread_test(void);
        qspi_thread_test();
    }

    if (strcmp(argv[1], "readid") == 0) {
        addr = qspi_flash_read_id(flash_handle);
        printf("read id 0x%x\n",addr);
    }

    if (strcmp(argv[1], "chiperase") == 0) {
        stat = qspi_flash_chip_erase(flash_handle);
        if(ENOERR == stat)
            printf("chip erase done!\n");
        else
            printf("chip erase err!\n");
    }

    return ENOERR;
}

#endif /* CYGPKG_DEVS_QSPI_BP2016 */
