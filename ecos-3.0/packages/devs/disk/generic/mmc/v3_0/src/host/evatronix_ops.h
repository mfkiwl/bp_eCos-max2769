#ifndef __EVATRONIX_OPS_H__
#define __EVATRONIX_OPS_H__

#include <cyg/infra/diag.h>
#include <pkgconf/io.h>
#include <pkgconf/io_disk.h>

 Cyg_ErrNo
evtx_mmc_disk_read(disk_channel* chan, void* buf_arg, cyg_uint32 blocks, cyg_uint32 first_block);

 Cyg_ErrNo
evtx_mmc_disk_write(disk_channel* chan, const void* buf_arg, cyg_uint32 blocks, cyg_uint32 first_block);

 Cyg_ErrNo
evtx_mmc_disk_get_config(disk_channel* chan, cyg_uint32 key, const void* buf, cyg_uint32* len);

 Cyg_ErrNo
evtx_mmc_disk_set_config(disk_channel* chan, cyg_uint32 key, const void* buf, cyg_uint32* len);

 Cyg_ErrNo
evtx_mmc_disk_lookup(struct cyg_devtab_entry** tab, struct cyg_devtab_entry *sub_tab, const char* name);

#endif //__EVATRONIX_OPS_H__
