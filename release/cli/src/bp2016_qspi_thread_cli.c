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

#ifdef CYGPKG_DEVS_QSPI_BP2016
#include <pkgconf/io_flash.h>
#include <cyg/io/flash.h>
#include <pkgconf/devs_qspi_bp2016.h>
#include <cyg/hal/api/qspi_api.h>

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

// this is for debug trace,
#define qspi_printf(fmt, args...)                                                               \
    printf("[qspi test] %s:%d " fmt "\n", __func__, __LINE__, ##args);

#define  DEF_THREAD_SIZE        (0x3000)
static cyg_thread thread_s[2];		/* space for two thread objects */
/* now the handles for the threads */
static cyg_handle_t qspi_thread_0, qspi_thread_1;
static char stack[2][DEF_THREAD_SIZE];		/* space for two 4K stacks */
cyg_thread_entry_t qspi_entry;

#define FLASH_PART0		"/dev/flash/0/0x100000,0x800000"
#define FLASH_PART1		"/dev/flash/0/0x900000"
#define THREAD_TEST_LENGHT		0x800

void qspi_entry(cyg_addrword_t data)
{
	cyg_io_handle_t fhandle;
    Cyg_ErrNo stat;
	CYG_ADDRWORD flash_start=0, part_size=0;
	cyg_uint32 bsize=0, part_blocks=0 ,len=0, addr;
	int id=-1, res=0;
	int i=0;
	cyg_uint8 tmp_buf1[THREAD_TEST_LENGHT], tmp_buf2[THREAD_TEST_LENGHT];

	id = (int) data;

	qspi_printf("##################  %d enter! #####################", id);

	if(0 == id)
	{
		qspi_printf("init part 0");
		stat = qspi_flash_init(&fhandle, FLASH_PART0);
	}else{
		qspi_printf("init part 1");
		stat = qspi_flash_init(&fhandle, FLASH_PART1);
	}

	if (stat != 0) {
		diag_printf("%d: FLASH: driver init failed: %s\n", id, strerror(-stat));
		//return -EINVAL;
		return;
	}

    cyg_thread_delay(10);
	part_size = qspi_flash_get_flashsize(fhandle) - 1;
	diag_printf("%d: Flash chip size: 0x%x B\r\n", id, part_size);

	len = qspi_flash_get_flashmem(fhandle);
	diag_printf("%d: Flash chip MEM start addr: 0x%x B\r\n", id, len);

	bsize = qspi_flash_get_blocksize(fhandle);
	part_blocks = part_size/bsize;

	diag_printf("%d: FLASH: 0x%x - 0x%x, %d blocks of 0x%x bytes each.\n", 
			id, flash_start, part_size + 1, part_blocks, bsize);

	//erase all
	qspi_printf("test part%d erase, read/write, chip size:0x%x, begin ...", id, part_size);
	qspi_flash_erase(fhandle, 0, part_size);
    cyg_thread_delay(10);
    qspi_printf("%d: erase all partition done!", id);
#if 1
	for(i=0; i<part_size; i+=THREAD_TEST_LENGHT)
	{
		len = THREAD_TEST_LENGHT;

		/* check if erase ok */
		memset(tmp_buf2, 0, len);
		if ((stat = cyg_io_bread(fhandle,  (void *)tmp_buf2, &len, i)) != CYG_FLASH_ERR_OK) {
			diag_printf("%d: FLASH %d: read after erase failed: %s\n", id, __LINE__, strerror(stat));
			return;
			//return -EINVAL;
		}    
        cyg_thread_delay(10);
        //qspi_printf("%d: check erase, read 0x%x done", id, THREAD_TEST_LENGHT);

		for(res=0; res<len; res++)
		{
			if( 0xff != tmp_buf2[res])
			{
				diag_printf("%d: erase error: addr 0x%x value %d\n", id, i+res, tmp_buf2[res]);
				dump_qspi_buffer(tmp_buf2, len);
				return;
				//return -EINVAL;
			}
		}
        qspi_printf("%d: check erase, addr:0x%x compare 0x%x done", id, i, THREAD_TEST_LENGHT);
	}

	for(i=0; i<THREAD_TEST_LENGHT;i++)
	{
		tmp_buf1[i] = i&0xff;
	}

    qspi_printf("%d: read/write partition test begin!", id);
	for(i=0; i<part_size; i+=THREAD_TEST_LENGHT)
	{
		len = THREAD_TEST_LENGHT;
		if ((stat = cyg_io_bwrite(fhandle,  (void *)tmp_buf1, &len, i)) != CYG_FLASH_ERR_OK) {
			diag_printf("%d: FLASH %d: write failed: %s\n", id, __LINE__, strerror(stat));
			return;
		}    

        cyg_thread_delay(10);
        //qspi_printf("%d: write 0x%x done", id, THREAD_TEST_LENGHT);

		memset(tmp_buf2, 0, len);
		if ((stat = cyg_io_bread(fhandle,  (void *)tmp_buf2, &len, i)) != CYG_FLASH_ERR_OK) {
			diag_printf("%d: FLASH %d: read after write failed: %s\n", id, __LINE__, strerror(stat));
			return;
		}    

        //qspi_printf("%d: read 0x%x done", id, THREAD_TEST_LENGHT);
        //cyg_thread_delay(10);

		for(res=0; res<len; res++)
		{
			if(tmp_buf1[res] != tmp_buf2[res])
			{
				diag_printf("%d: write error: addr 0x%x -- raw: %d, rd %d\n",
						id, i+res, tmp_buf1[res], tmp_buf2[res]);
				dump_qspi_buffer(tmp_buf1, len);
				dump_qspi_buffer(tmp_buf2, len);
				return;
				//break;
			}
		}

        cyg_thread_delay(10);
        qspi_printf("%d: addr:0x%x compare 0x%x done", id, i, THREAD_TEST_LENGHT);
	}
	qspi_printf("test part%d erase, read/write, end! ...", id);
	/* test all chip erase/read/write end */

	/* select one block do erase/read/write 0x55, verify it corrected */
	qspi_printf("%d: test write 0x55 begin!", id);
	memset(tmp_buf1, 0x55, THREAD_TEST_LENGHT);
	addr =	bsize*5;
	len = THREAD_TEST_LENGHT;

	qspi_flash_erase(fhandle, addr, len);
    qspi_printf("%d: erase done!", id);
    cyg_thread_delay(10);

	if ((stat = cyg_io_bwrite(fhandle,  (void *)tmp_buf1, &len, addr)) != CYG_FLASH_ERR_OK) {
		diag_printf("%d: FLASH %d: write after erase failed: %s\n", id, __LINE__, strerror(stat));
		return;
	}    
    qspi_printf("%d: write 0x55  0x%x done", id, THREAD_TEST_LENGHT);

	memset(tmp_buf2, 0, len);
	if ((stat = cyg_io_bread(fhandle,  (void *)tmp_buf2, &len, addr)) != CYG_FLASH_ERR_OK) {
		diag_printf("%d: FLASH %d: read after write failed: %s\n", id, __LINE__, strerror(stat));
		return;
	}    
    qspi_printf("%d: read 0x55  0x%x done", id, THREAD_TEST_LENGHT);
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

	qspi_printf("##################  %d done! #####################", id);
#endif
}

extern void show_all_thread_infor(void);
//void qspi_thread_rw_test(void)
void qspi_thread_test(void)
{
  qspi_printf("enter!");
  cyg_thread_create(4, qspi_entry, (cyg_addrword_t)0,
		    "Thread rw1", (void *) stack[0], DEF_THREAD_SIZE,
		    &qspi_thread_0, &thread_s[0]);

  cyg_thread_resume(qspi_thread_0);

#if 1 
  cyg_thread_create(4, qspi_entry, (cyg_addrword_t)1,
		    "Thread rw2", (void *) stack[1], DEF_THREAD_SIZE,
		    &qspi_thread_1, &thread_s[1]);

  cyg_thread_resume(qspi_thread_1);
#endif
  qspi_printf("end!");
  
  show_all_thread_infor();
}
#endif /* CYGPKG_DEVS_QSPI_BP2016 */
