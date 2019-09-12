#ifndef __I2C_API__
#define __I2C_API__

void  i2c_api_init(void);
void I2C_dev_deinit(void *handle);
void * I2C_dev_init(cyg_uint8 bus_id, cyg_uint8 slv_addr, cyg_uint8 mode);
Cyg_ErrNo I2C_dev_read(void * handle, cyg_uint8* wbuf, cyg_uint32 wlen, void * rbuf, cyg_uint32 rlen);
Cyg_ErrNo I2C_dev_write(void * handle, void * wbuf, cyg_uint32 wlen);

#endif


