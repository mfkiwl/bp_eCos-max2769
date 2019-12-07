#include <cyg/infra/diag.h>
#include <cyg/hal/hal_diag.h>
#include "fl_common_cmd.h"
#include <cyg/io/flash/qspi_dev.h>

extern int qspi_read_xx_data(cyg_uint8 cmd, cyg_uint32 addr, int addr_vld, cyg_uint8 *rx_dbuf,
                             int rx_len);
extern int qspi_read_page_data(cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *rx_dbuf, int rx_len,
                               int dfs);

int qspi_fl_read_rdid(cyg_uint8 rdid_cmd, int rdid_len, cyg_uint8 *dbuf)
{
    return qspi_read_xx_data(rdid_cmd, 0, 0, dbuf, rdid_len);
}

int qspi_fl_read_devid(cyg_uint8 devid_cmd, int devid_len, cyg_uint8 *dbuf)
{
    return qspi_read_xx_data(devid_cmd, 0, 1, dbuf, devid_len);
}

int qspi_fl_read_page(cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *rbuf, int len, int dfs)
{
    return qspi_read_page_data(cmd, addr, rbuf, len, dfs);
}

cyg_uint8 qspi_fl_read_status(cyg_uint8 cmd)
{
    cyg_uint8 data = 0;
    qspi_read_xx_data(cmd, 0, 0, &data, 1);
    return data;
}

int qspi_fl_wait_wel(void)
{
    volatile cyg_uint8 st = 0;
    int count=10000;  //100ms

    while (count--) {
        st = qspi_fl_read_status(SPINOR_OP_RDSR1);
        if (st & SR_WEL)
            return QSPI_OP_SUCCESS;

		HAL_DELAY_US(1);
    }

    return QSPI_OP_FAILED;
}
