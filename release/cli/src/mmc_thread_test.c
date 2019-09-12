#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>

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

static void dump_mmc_buffer(void *buffer, cyg_uint32 len)
{
    cyg_uint32 i = 0;
    cyg_uint8 *buf = (cyg_uint8 *) buffer;

    diag_printf("\n");
    for (i = 0; i < len; i++) {
		if(!(i%16))
			diag_printf("\n%08x: ", i);
        diag_printf("%02x ", *(buf + i));
    }
    diag_printf("\n");
}

// this is for debug trace,
#define mmc_printf(fmt, args...)                                                               \
    diag_printf("[mmc test] %s:%d " fmt "\n", __func__, __LINE__, ##args);

#define  DEF_THREAD_SIZE        (0x3000)
static cyg_thread thread_s[2];		/* space for two thread objects */
/* now the handles for the threads */
static cyg_handle_t mmc_thread_0, mmc_thread_1;
static char stack[2][DEF_THREAD_SIZE];		/* space for two 4K stacks */
cyg_thread_entry_t mmc_entry;

#define MMC_PART0		"/dev/sdmmc0/"
#define THREAD_TEST_LENGHT		0x800
#define THREAD_TEST_STEP        (THREAD_TEST_LENGHT/512)
#define PART_SIZE0      8000
#define PART_SIZE1      (PART_SIZE0 + PART_SIZE0/4) 
#define PART_BASE       1000

static cyg_io_handle_t fhandle;

void mmc_entry(cyg_addrword_t data)
{
    Cyg_ErrNo stat;
	CYG_ADDRWORD part_size=0;
	cyg_uint32 bsize=512, len=0, len_t=0, addr, base_addr=0;
	int id=-1, res=0;
	int i=0;
	cyg_uint8 tmp_buf1[THREAD_TEST_LENGHT], tmp_buf2[THREAD_TEST_LENGHT];

	id = (int) data;

	mmc_printf("##################  %d enter! #####################", id);

	if(0 == id)
	{
        part_size = PART_SIZE0;
        base_addr = PART_BASE;
	}else{
        part_size = PART_SIZE1;
        base_addr = PART_BASE + PART_SIZE0 + 1000;
	}

    diag_printf("part_size is 0x%x  handles %d \n", part_size, (u32)fhandle);

    cyg_thread_delay(10);

	for(i=0; i<part_size; i+=THREAD_TEST_STEP)
	{
		len = THREAD_TEST_LENGHT;
        len_t = len/512;

		/* check if erase ok */
		memset(tmp_buf2, 0, len);
		if ((stat = cyg_io_bread(fhandle,  (void *)tmp_buf2, &len_t, base_addr + i)) != ENOERR) {
			diag_printf("%d: MMC %d: read after erase failed: %s\n", id, __LINE__, strerror(stat));
			return;
			//return -EINVAL;
		}    
        mmc_printf("%d: check erase, read addr %d done", id, base_addr + i);
        cyg_thread_delay(10);
#if 0
		for(res=0; res<len; res++)
		{
			if( 0 != tmp_buf2[res])
			{
				diag_printf("%d: erase error: addr 0x%x value %d\n", id, i+res, tmp_buf2[res]);
				dump_mmc_buffer(tmp_buf2, len);
				return;
				//return -EINVAL;
			}
		}
        mmc_printf("%d: check erase,  compare addr %d done", id, base_addr + i);
#endif
	}

	for(i=0; i<THREAD_TEST_LENGHT;i++)
	{
		tmp_buf1[i] = i&0xff;
	}

    mmc_printf("%d: read/write partition test begin!", id);
	for(i=0; i<part_size; i+=THREAD_TEST_STEP)
	{
		len = THREAD_TEST_LENGHT;
        len_t = len/512;
		if ((stat = cyg_io_bwrite(fhandle,  (void *)tmp_buf1, &len_t, base_addr + i)) != ENOERR) {
			diag_printf("%d: MMC %d: write failed: %s\n", id, __LINE__, strerror(stat));
			return;
		}

        mmc_printf("%d: write addr %d done", id, base_addr + i);
        cyg_thread_delay(10);

		memset(tmp_buf2, 0, len);
		if ((stat = cyg_io_bread(fhandle,  (void *)tmp_buf2, &len_t, base_addr + i)) != ENOERR) {
			diag_printf("%d: MMC %d: read after write failed: %s\n", id, __LINE__, strerror(stat));
			return;
		}    

		for(res=0; res<len; res++)
		{
			if(tmp_buf1[res] != tmp_buf2[res])
			{
				diag_printf("%d: write error: addr 0x%x -- raw: %d, rd %d\n",
						id, i+res, tmp_buf1[res], tmp_buf2[res]);
				dump_mmc_buffer(tmp_buf1, len);
				dump_mmc_buffer(tmp_buf2, len);
				return;
				//break;
			}
		}

        cyg_thread_delay(10);
        mmc_printf("%d: addr: %d compare 0x%x done", id, base_addr + i, THREAD_TEST_LENGHT);
	}
	mmc_printf("test part%d erase, read/write, end! ...", id);
	/* test all chip erase/read/write end */

	/* select one block do erase/read/write 0x55, verify it corrected */
	mmc_printf("%d: test write 0x55 begin!", id);
	memset(tmp_buf1, 0x55, THREAD_TEST_LENGHT);
	addr =	bsize*5;
	len = THREAD_TEST_LENGHT;
    len_t = len/512;

	if ((stat = cyg_io_bwrite(fhandle,  (void *)tmp_buf1, &len_t, base_addr + addr)) != ENOERR) {
		diag_printf("%d: MMC %d: write after erase failed: %s\n", id, __LINE__, strerror(stat));
		return;
	}    
    mmc_printf("%d: write 0x55  addr %d done", id, base_addr + addr);

	memset(tmp_buf2, 0, len);
	if ((stat = cyg_io_bread(fhandle,  (void *)tmp_buf2, &len_t, base_addr + addr)) != ENOERR) {
		diag_printf("%d: MMC %d: read after write failed: %s\n", id, __LINE__, strerror(stat));
		return;
	}    
    mmc_printf("%d: read 0x55  addr %d done", id, base_addr + addr);
    cyg_thread_delay(10);

	for(res=0; res<len; res++)
	{
		if(tmp_buf1[res] != tmp_buf2[res])
		{
			diag_printf("%d: write 0x55 error %d: offset 0x%x -- value 0x%x\n",
					id, __LINE__, res, tmp_buf2[res]);
			return;	
		}
	}

	mmc_printf("##################  %d done! #####################", id);
}

extern void show_all_thread_infor(void);
//void mmc_thread_rw_test(void)
void mmc_thread_test(void)
{
  Cyg_ErrNo stat;

  mmc_printf("mmc init!");
  stat = cyg_io_lookup(MMC_PART0, &fhandle);
  if (stat == -ENOENT) {
      diag_printf("MMC: driver init failed: %s\n", strerror(-stat));
      return;
  }

  cyg_thread_create(4, mmc_entry, (cyg_addrword_t)0,
		    "MMC Thread rw1", (void *) stack[0], DEF_THREAD_SIZE,
		    &mmc_thread_0, &thread_s[0]);

  cyg_thread_resume(mmc_thread_0);

#if 1 
  cyg_thread_create(4, mmc_entry, (cyg_addrword_t)1,
		    "MMC Thread rw2", (void *) stack[1], DEF_THREAD_SIZE,
		    &mmc_thread_1, &thread_s[1]);

  cyg_thread_resume(mmc_thread_1);
#endif
  mmc_printf("end!");
  
  show_all_thread_infor();
}
#endif /* CYGPKG_DEVS_MMC_BP2016 */
