#include <cyg/kernel/kapi.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/infra/diag.h>

#include <pkgconf/hal.h>
#include <pkgconf/io_flash.h>

#include <cyg/io/io.h>
#include <cyg/io/config_keys.h>
#include <cyg/io/flash.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <cyg/hal/hal_if.h>

#ifdef CYGPKG_DEVS_QSPI_BP2016
#include <pkgconf/devs_qspi_bp2016.h>
#include <cyg/hal/api/qspi_api.h>
#endif

//#define FLASH_TEST_DEVICE "/dev/flash/0/0"

int qspi_flash_init(cyg_io_handle_t *flash_handle, const char *name)
{
    Cyg_ErrNo stat;

	cyg_flash_scan(NULL);
    if ((stat = cyg_io_lookup(name, flash_handle)) == -ENOENT) {
        printf("FLASH: driver init failed: %s\n", strerror(-stat));
        return -EINVAL;
    }

	return 0;
}

int qspi_flash_lock(cyg_io_handle_t handle, cyg_uint32 offs, cyg_uint32 len)
{
    Cyg_ErrNo stat;
    cyg_io_flash_getconfig_unlock_t u;

    u.offset = offs;
    u.len = len;
    len = sizeof(u);
    if ((stat = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_FLASH_LOCK, &u, &len)) != 0 ||
        u.flasherr != 0) {
        printf("FLASH: lock failed: %s %s\n", strerror(stat), cyg_flash_errmsg(u.flasherr));
        return -EINVAL;
    }

    return ENOERR;
}

int qspi_flash_unlock(cyg_io_handle_t handle, cyg_uint32 offs, cyg_uint32 len)
{
    Cyg_ErrNo stat;
    cyg_io_flash_getconfig_unlock_t u;

    u.offset = offs;
    u.len = len;
    len = sizeof(u);
    if ((stat = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_FLASH_UNLOCK, &u, &len)) != 0 ||
        u.flasherr != 0) {
        printf("FLASH: unlock failed: %s %s\n", strerror(stat), cyg_flash_errmsg(u.flasherr));
        return -EINVAL;
    }

    return ENOERR;
}

int qspi_flash_erase(cyg_io_handle_t handle, cyg_uint32 offs, cyg_uint32 len)
{
    Cyg_ErrNo stat;
    cyg_io_flash_getconfig_erase_t e;
    cyg_uint32 elen = 0;

    e.offset = offs;
    e.len = len;
    e.err_address = 0;
    elen = sizeof(e);
    printf("%s: addr 0x%x len 0x%x\n", __func__, e.offset, e.len);
    if ((stat = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_FLASH_ERASE, &e, &elen)) != 0 ||
        e.flasherr != 0) {
        printf("FLASH: erase failed: %s %s\n", cyg_flash_errmsg(stat),
                    cyg_flash_errmsg(e.flasherr));
        return -EINVAL;
    }

    return ENOERR;
}

int qspi_flash_sector_erase(cyg_io_handle_t handle, cyg_uint32 offs, cyg_uint32 len)
{
    Cyg_ErrNo stat;
    cyg_io_flash_getconfig_erase_t e;
    cyg_uint32 elen = 0;

    e.offset = offs;
    e.len = len;
    e.err_address = 0;
    elen = sizeof(e);
    printf("%s: addr 0x%x len 0x%x\n", __func__, e.offset, e.len);
    if ((stat = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_FLASH_SECTOR_ERASE, &e, &elen)) != 0 ||
        e.flasherr != 0) {
        printf("FLASH: erase failed: %s %s\n", cyg_flash_errmsg(stat),
                    cyg_flash_errmsg(e.flasherr));
        return -EINVAL;
    }

    return ENOERR;
}

cyg_uint32 qspi_flash_get_flashsize(cyg_io_handle_t handle)
{
    cyg_io_flash_getconfig_devsize_t d;
    cyg_uint32 len;
    Cyg_ErrNo stat;

    len = sizeof(d);
    stat = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_FLASH_DEVSIZE, &d, &len);

    if (ENOERR != stat) {
        printf("%s: error, ret = %d \n", __func__, stat);
        return 0;
    }

    return d.dev_size;
}

cyg_uint32 qspi_flash_get_flashmem(cyg_io_handle_t handle)
{
    cyg_io_flash_getconfig_devaddr_t r;
    cyg_uint32 len;
    Cyg_ErrNo stat;

    len = sizeof(r);
    stat = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_FLASH_DEVADDR, &r, &len);

    if (ENOERR != stat) {
        printf("%s: error, ret = %d \n", __func__, stat);
        return 0;
    }

    return r.dev_addr;
}

cyg_uint32 qspi_flash_get_blocksize(cyg_io_handle_t handle)
{
    Cyg_ErrNo stat;
    cyg_io_flash_getconfig_blocksize_t b;
    cyg_uint32 len;

    len = sizeof(b);

    // if the block size of the chip is not equal,  this code need modify
    // we can get each pos block by set different offset,
    b.offset = 0;
    stat = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_FLASH_BLOCKSIZE, &b, &len);

    if (ENOERR != stat) {
        printf("%s: error, ret = %d \n", __func__, stat);
        return 0;
    }

    return b.block_size;
}

cyg_uint32 qspi_flash_get_sectorsize(cyg_io_handle_t handle)
{
    Cyg_ErrNo stat;
    cyg_io_flash_getconfig_blocksize_t b;
    cyg_uint32 len;

    len = sizeof(b);

    // if the block size of the chip is not equal,  this code need modify
    // we can get each pos block by set different offset,
    b.offset = 0;
    stat = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_FLASH_SECTOR_SIZE, &b, &len);

    if (ENOERR != stat) {
        printf("%s: error, ret = %d \n", __func__, stat);
        return 0;
    }

    return b.block_size;
}

// wwzz add 20181024
cyg_uint32 qspi_flash_read(cyg_io_handle_t handle, void *buff, cyg_uint32 *len, cyg_uint32 addr)
{
    return cyg_io_bread(handle, buff, len, addr);
}

cyg_uint32 qspi_flash_write(cyg_io_handle_t handle, void *buff, cyg_uint32 *len, cyg_uint32 addr)
{
    return cyg_io_bwrite(handle, buff, len, addr);
}

int qspi_flash_chip_erase(cyg_io_handle_t handle)
{
    Cyg_ErrNo stat;
    cyg_io_flash_getconfig_erase_t e;
    cyg_uint32 elen = 0;

    e.offset = 0;
    e.len = 0;
    e.err_address = 0;
    elen = sizeof(e);
    //printf("%s: addr 0x%x len 0x%x\n", __func__, e.offset, e.len);
    if (((stat = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_FLASH_CHIP_ERASE, &e, &elen)) != 0) ||
        (e.flasherr != 0)) {
        printf("FLASH: chip erase failed: %s %s\n", cyg_flash_errmsg(stat),
                    cyg_flash_errmsg(e.flasherr));
        return -EINVAL;
    }

    return ENOERR;
}

cyg_uint32 qspi_flash_read_id(cyg_io_handle_t handle)
{
    Cyg_ErrNo stat;
    cyg_uint32 id;
    cyg_uint32 len;

    len = sizeof(id);

    // if the block size of the chip is not equal,  this code need modify
    // we can get each pos block by set different offset,
    stat = cyg_io_get_config(handle, CYG_IO_GET_CONFIG_FLASH_READ_ID, &id, &len);

    if (ENOERR != stat) {
        printf("%s: error, ret = %d \n", __func__, stat);
        return 0xffffffff;
    }

    return id;
}




