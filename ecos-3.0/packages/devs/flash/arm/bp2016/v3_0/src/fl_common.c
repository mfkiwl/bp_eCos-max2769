#include <cyg/infra/diag.h>
#include <cyg/hal/hal_diag.h>

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
    cyg_uint8 data;
    qspi_read_xx_data(cmd, 0, 0, &data, 1);
    return data;
}
