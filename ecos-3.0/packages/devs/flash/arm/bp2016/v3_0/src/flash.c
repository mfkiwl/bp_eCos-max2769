#include <cyg/infra/diag.h>
#include <cyg/hal/hal_diag.h>

#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/flash/qspi_dev.h>
#include <cyg/hal/regs/qspi.h>
#include "qspi_dbg.h"

qspi_timeout_t tm_array[] = {
{300*1000, 520*1000, 1000*1000, 460*1000*1000, 2000, 30*1000 },	//S25FL512S
{300*1000, 1000*1000, 1200*1000, 400*1000*1000, 2400, 30*1000}, //GD25Q512MC
{400*1000, 1600*1000, 2000*1000, 400*1000*1000, 3000, 15*1000}, //w25q256fv
{300*1000, 1000*1000, 1200*1000, 400*1000*1000, 2400, 30*1000}, //GD25Q256D
{0, 0, 0, 0, 0, 0, }
};

extern void qspi_printf_buf_fmt_32bit(cyg_uint32 *buf, cyg_uint32 addr, int len);
extern void qspi_ahb_read_enable(cyg_uint8 rd_cmd, cyg_uint8 dfs);
extern void qspi_ahb_write_enable(cyg_uint8 wren_cmd);

int qspi_fl_read_rdid(cyg_uint8 rdid_cmd, int rdid_len, cyg_uint8 *dbuf);
int qspi_fl_read_devid(cyg_uint8 devid_cmd, int devid_len, cyg_uint8 *dbuf);
int qspi_fl_read_page(cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *rbuf, int len, int dfs);
#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
int qspi_dma_read_page(void *dma_rx_ch, cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *rx_dbuf, int rx_len);
int qspi_s25fl512s_dma_write_page(void *dma_tx_ch, cyg_uint32 addr, cyg_uint8 *dbuf, int len);
int qspi_w25q256fv_dma_write_page(void *dma_tx_ch, cyg_uint32 addr, cyg_uint8 *dbuf, int len);
int qspi_gd25q512mc_dma_write_page(void *dma_tx_ch, cyg_uint32 addr, cyg_uint8 *dbuf, int len);
int qspi_gd25q256d_dma_write_page(void *dma_tx_ch, cyg_uint32 addr, cyg_uint8 *dbuf, int len);
#endif

/******************************************************************************/
/********************* S25FL512S Function begin *******************************/
/******************************************************************************/

int qspi_s25fl512s_write_page(cyg_uint32 addr, cyg_uint8 *dbuf, int len);
cyg_uint8 qspi_s25fl512s_erase(cyg_uint8 cmd, cyg_uint32 addr);
cyg_uint8 qspi_s25fl512s_erase_all(void);
void qspi_s25fl512s_4byte_extend(cyg_uint32 addr, int en);
int qspi_s25fl512s_quad(int en);
int qspi_s25fl512s_prot_region(int region, int en);
cyg_uint8 qspi_s25fl512s_read_status(void);
int qspi_s25fl512s_reset(void);
int qspi_s25fl512s_block_lock(cyg_uint32 addr);
int qspi_s25fl512s_block_unlock(cyg_uint32 addr);
int qspi_s25fl512s_read_block_status(cyg_uint32 addr);

/******************************************************************************/
/********************* S25FL512S Function end *********************************/
/******************************************************************************/

/******************************************************************************/
/********************* W25Q256FV Function begin *******************************/
/******************************************************************************/

int qspi_w25q256fv_write_page(cyg_uint32 addr, cyg_uint8 *dbuf, int len);
cyg_uint8 qspi_w25q256fv_erase(cyg_uint8 cmd, cyg_uint32 addr);
cyg_uint8 qspi_w25q256fv_erase_all(void);
void qspi_w25q256fv_4byte_extend(cyg_uint32 addr, int en);
int qspi_w25q256fv_quad(int en);
int qspi_w25q256fv_prot_region(int region, int en);
cyg_uint8 qspi_w25q256fv_read_status(void);
int qspi_w25q256fv_reset(void);

int qspi_w25q256fv_block_lock(cyg_uint32 offs);
int qspi_w25q256fv_block_unlock(cyg_uint32 offs);
int qspi_w25q256fv_read_block_lock_status(cyg_uint32 offs);
void qspi_w25q256fv_global_lock(void);
void qspi_w25q256fv_global_unlock(void);
/******************************************************************************/
/********************* W25Q256FV Function end *********************************/
/******************************************************************************/

/******************************************************************************/
/********************* GD25Q512MC Function begin ******************************/
/******************************************************************************/

int qspi_gd25q512mc_write_page(cyg_uint32 addr, cyg_uint8 *dbuf, int len);
cyg_uint8 qspi_gd25q512mc_erase(cyg_uint8 cmd, cyg_uint32 addr);
cyg_uint8 qspi_gd25q512mc_erase_all(void);
void qspi_gd25q512mc_4byte_extend(cyg_uint32 addr, int en);
int qspi_gd25q512mc_quad(int en);
int qspi_gd25q512mc_prot_region(int region, int en);
cyg_uint8 qspi_gd25q512mc_read_status(void);
int qspi_gd25q512mc_reset(void);

#if 0
int qspi_gd25q512mc_block_lock(cyg_uint32 offs);
int qspi_gd25q512mc_block_unlock(cyg_uint32 offs);
int qspi_gd25q512mc_read_block_lock_status(cyg_uint32 offs);
void qspi_gd25q512mc_global_lock(void);
void qspi_gd25q512mc_global_unlock(void);
#endif
/******************************************************************************/
/********************* GD25Q512MC Function end ********************************/
/******************************************************************************/

/******************************************************************************/
/********************* GD25Q256D Function begin ******************************/
/******************************************************************************/

int qspi_gd25q256d_write_page(cyg_uint32 addr, cyg_uint8 *dbuf, int len);
cyg_uint8 qspi_gd25q256d_erase(cyg_uint8 cmd, cyg_uint32 addr);
cyg_uint8 qspi_gd25q256d_erase_all(void);
void qspi_gd25q256d_4byte_extend(cyg_uint32 addr, int en);
int qspi_gd25q256d_quad(int en);
int qspi_gd25q256d_prot_region(int region, int en);
cyg_uint8 qspi_gd25q256d_read_status(void);
int qspi_gd25q256d_reset(void);

/******************************************************************************/
/********************* GD25Q256D Function end ********************************/
/******************************************************************************/


struct qspi_fl_info fl_info[] = {
    {
	 "s25fl512s", 
	 0x010220, 
	 256*1024, 
	 256, 
     0,
	 qspi_fl_read_rdid, 
	 qspi_fl_read_devid, 
#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
	 qspi_dma_read_page,
     qspi_s25fl512s_dma_write_page,
#else
	 qspi_fl_read_page,
     qspi_s25fl512s_write_page,
#endif
     qspi_s25fl512s_erase, 
	 qspi_s25fl512s_erase_all, 
	 qspi_s25fl512s_4byte_extend,
     qspi_s25fl512s_quad, 
	 qspi_s25fl512s_prot_region, 
	 qspi_s25fl512s_read_status,
     qspi_s25fl512s_reset, 
	 qspi_s25fl512s_block_lock, 
	 qspi_s25fl512s_block_unlock,
	 qspi_s25fl512s_read_block_status, 
	 NULL, 
	 NULL
	 },
    {
	  "w25q256fv", 
	  0xef4019, // 0x186019 
	  64 * 1024, 
	  512, 
      4096,
	 qspi_fl_read_rdid, 
	 qspi_fl_read_devid, 
#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
	 qspi_dma_read_page,
     qspi_w25q256fv_dma_write_page,
#else
	 qspi_fl_read_page, 
	 qspi_w25q256fv_write_page,
#endif
     qspi_w25q256fv_erase, 
	 qspi_w25q256fv_erase_all, 
	 qspi_w25q256fv_4byte_extend,
     qspi_w25q256fv_quad, 
	 qspi_w25q256fv_prot_region, 
	 qspi_w25q256fv_read_status,
     qspi_w25q256fv_reset, 
	 qspi_w25q256fv_block_lock, 
	 qspi_w25q256fv_block_unlock,
	 qspi_w25q256fv_read_block_lock_status,
	 qspi_w25q256fv_global_lock,
	 qspi_w25q256fv_global_unlock
	 },
    {
	 "gd25q512mc",
     0xc84020, // 0xc84019
     64 * 1024, 
	 1024, 
     4096,
	 qspi_fl_read_rdid, 
	 qspi_fl_read_devid, 
#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
	 qspi_dma_read_page,
     qspi_gd25q512mc_dma_write_page,
#else
	 qspi_fl_read_page,
     qspi_gd25q512mc_write_page,
#endif
     qspi_gd25q512mc_erase, 
	 qspi_gd25q512mc_erase_all, 
	 qspi_gd25q512mc_4byte_extend,
     qspi_gd25q512mc_quad, 
	 qspi_gd25q512mc_prot_region, 
	 qspi_gd25q512mc_read_status,
     qspi_gd25q512mc_reset, 
	  NULL,
	  NULL,
	  NULL,
	  NULL,
	  NULL
	 },
    { 
      "gd25q256d",
      0xc84019, 
      64*1024,
	  512,
      4096,
      qspi_fl_read_rdid,
      qspi_fl_read_devid,
#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
	  qspi_dma_read_page,
      qspi_gd25q256d_dma_write_page,
#else
      qspi_fl_read_page,
      qspi_gd25q256d_write_page,
#endif
      qspi_gd25q256d_erase,
      qspi_gd25q256d_erase_all,
      qspi_gd25q256d_4byte_extend,
      qspi_gd25q256d_quad,
      qspi_gd25q256d_prot_region,
      qspi_gd25q256d_read_status,
      qspi_gd25q256d_reset,
	  NULL,
	  NULL,
	  NULL,
	  NULL,
	  NULL
    },
    {"",0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,NULL, NULL, NULL},
};

// struct qspi_fl_info *fl_dev = NULL; //&fl_info[0];

#define BUF_NUM (2048)
static cyg_uint8 dbuf[BUF_NUM];

extern struct qspi_fl_info * fl_rescan(void)
{
    cyg_uint32 jedid = 0;
    struct qspi_fl_info *fl = &fl_info[0];
    int fl_count = sizeof(fl_info) / sizeof(fl_info[0]), i;

    qspi_fl_read_rdid(SPINOR_OP_RDID, 3, dbuf);
    jedid = (dbuf[0] << 16) | (dbuf[1] << 8) | (dbuf[2]);
    qspi_printf("jedid = 0x%x, support device number :%d\n", jedid, fl_count);

	if(!jedid)
	{
		qspi_w25q256fv_reset();	

		qspi_fl_read_rdid(SPINOR_OP_RDID, 3, dbuf);
		jedid = (dbuf[0] << 16) | (dbuf[1] << 8) | (dbuf[2]);

		if(!jedid)
		{
			qspi_s25fl512s_reset();
			qspi_fl_read_rdid(SPINOR_OP_RDID, 3, dbuf);
			jedid = (dbuf[0] << 16) | (dbuf[1] << 8) | (dbuf[2]);
		}

	}

    for (i = 0; i < fl_count; i++) {
        if (fl->blk_size == 0)
            continue;

        if (fl->jedec == jedid) {
            qspi_printf("fl_rescan device match ok, and name: %s\n", fl->name);
			qspi_printf("blk_size %u, blk_num %u", fl->blk_size, fl->blk_num);
            return fl;
        }
        fl++;
    }
    qspi_printf("fl_rescan device match failed\n");
    return NULL;
}

#if 0
/*
 *  0x66 + 0x99 or 0xf0
 */
int fl_reset(void)
{
    fl_dev->reset();
    qspi_printf("reset ...\n");
    return ENOERR;
}

int fl_read_id(void)
{
    int i;
    cyg_uint32  jeded;
    fl_dev->read_devid(SPINOR_OP_RD_DEVID, 2, dbuf);

    qspi_printf("read id             : %02x %02x\n", dbuf[0], dbuf[1]);

    fl_dev->read_rdid(SPINOR_OP_RDID, 3, dbuf);
    qspi_printf("rd id(JEDED)        : ");
    for (i = 0; i < 3; i++)
        qspi_printf("%02x", dbuf[i]);
    qspi_printf("\n");
    jeded = dbuf[2] + (dbuf[1] << 8) + (dbuf[0] << 16);
    fl_dev->read_status();
    if(fl_dev->jedec == jeded)
        return ENOERR;

     return DVT_CASE_RESULT_FAIL;
}

int fl_read(int rd_cmd, cyg_uint32 addr, cyg_uint8 *buf, cyg_uint32 len, cyg_uint32 dump_regs, int dfs)
{
    unsigned long long s, e;
    unsigned int us;
    int rx_count;
#ifdef QSPI_HW_STAT
    unsigned int us_spi;
    unsigned long long e_spi;
    extern u64 qspi_get_timer_tick_save(void);
#endif

    qspi_printf("cmd %02x \n", rd_cmd);
    s = HAL_GET_COUNTER;
    rx_count = fl_dev->read_page(rd_cmd, addr, buf, len, dfs);
    if (dump_regs > 0)
        qspi_printf_buf_fmt_32bit((cyg_uint32 *) hwp_apQspi, (cyg_uint32) hwp_apQspi, 24);
    e = HAL_GET_COUNTER;
    qspi_printf_buf_fmt_32bit((cyg_uint32 *) buf, (cyg_uint32) addr, rx_count / 4);
#ifdef QSPI_HW_STAT
    e_spi = qspi_get_timer_tick_save();
    us_spi = tick_to_us((unsigned long) (e_spi - s));
    qspi_printf("spi need %d\n", us_spi);
#endif
    us = tick_to_us((unsigned long) (e - s));
    qspi_printf("Test need %d us (%d ms)\n", us, us / 1000);
    return 0;
}

int fl_ahbread(int rd_cmd, cyg_uint32 fl_addr, cyg_uint8 *buf, cyg_uint32 len, cyg_uint32 dump_regs, cyg_uint32 dfs)
{
    unsigned long long s, e;
    unsigned int us;

    qspi_printf("cmd %02x , %d\n", rd_cmd, dfs);
    switch (dfs) {
    case 8:
    case 16:
    case 32:
        break;
    default:
        qspi_printf("dfs %d invalid\n", dfs);
        return DVT_CASE_RESULT_FAIL;
    }

    s = HAL_GET_COUNTER;
    qspi_ahb_read_enable(rd_cmd, dfs);
    if (dump_regs > 0)
        qspi_printf_buf_fmt_32bit((cyg_uint32 *) hwp_apQspi, (cyg_uint32) hwp_apQspi, 24);

    memcpy(buf, (void *) SPIFLASH_D_BASE + fl_addr, len);
    e = HAL_GET_COUNTER;

    qspi_printf_buf_fmt_32bit((cyg_uint32 *) buf, (cyg_uint32) SPIFLASH_D_BASE + fl_addr, len / 4);

    us = tick_to_us((unsigned long) (e - s));
    qspi_printf("Test need %d us (%d ms)\n", us, us / 1000);
    return ENOERR;
}

static cyg_uint8 upgrade_version_to_flash(cyg_uint32 flash_addr, cyg_uint8 *buff, cyg_uint32 len)
{
    cyg_uint32 i, wloops, eloops;
    int length = len + 0xff;
    cyg_uint8 status;

    fl_dev->config_prot_region(0, 0);

    length &= ~(0xff);
    wloops = length >> 8;
    eloops = (len + fl_dev->blk_size - 1) / fl_dev->blk_size; // 256k bytes block size aligned
    qspi_printf("upgrade flash: page num = %d, erase blk num = %d\n", wloops, eloops);
    for (i = 0; i < eloops; i++) {
        status = fl_dev->erase(flash_addr + i * fl_dev->blk_size);
        if (status != QSPI_OP_SUCCESS)
            return -1;
    }

    for (i = 0; i < wloops; i++) {
        status = fl_dev->write_page(flash_addr + i * 256, (cyg_uint8 *) (buff + i * 256), 256);
        if (status != QSPI_OP_SUCCESS)
            return -1;
    }

    return 0;
}

#endif
