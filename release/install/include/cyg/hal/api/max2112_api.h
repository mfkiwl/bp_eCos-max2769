#ifndef __MAX2112_API_H__
#define __MAX2112_API_H__

/*
* Function Name: Max2112_Init
* Condition:    initialization max2112
* Input Para:	bus_id, i2c bus id which max2112 attached
* Output Para:
* Return value:  ENOERR:  OK
*                -ENOENT: Not found
*/
Cyg_ErrNo Max2112_Init(cyg_uint32 bus_id);

/*
* Function Name: Max2112_read
* Condition:   need init max2112 module
* Input Para:  addr: read begin register of max2112
*              rbuf: read registers value to this buffer
*              rlen: consecutive registers num of read
* Output Para:
* Return value:  ENOERR:  read 0 data out
*                -EDEVNOSUPP: Device does not support this operation
*                -EIO: I/O error
*                -ETIMEDOUT: Operation timed out
*                rlen: read rlen values out, success
*/
Cyg_ErrNo Max2112_read(cyg_uint8 addr, void *rbuf, cyg_uint32 rlen);

/*
* Function Name: Max2112_write
* Condition:   need init max2112 module
* Input Para:  addr: write begin register of max2112
*              wbuf: write this buffer's contents to registers
*              wlen: consecutive registers num of write
* Output Para:
* Return value:  ENOERR:  write 0 data in, failed
*                -EDEVNOSUPP: Device does not support this operation
*                -EIO: I/O error
*                -ETIMEDOUT: Operation timed out
*                wlen: write wlen values to registers, success
*/
Cyg_ErrNo Max2112_write(cyg_uint8 addr, void *wbuf, cyg_uint32 wlen);

/*
* Function Name: Max2112_set_mode
* Condition:     need init max2112 module
* Input Para:    mode: max2112 i2c work mode, 1:standard mode 2:fast speed mode
* 					   3:high speed mode
*
* Output Para:
* Return value:  ENOERR:  OK
*                -EINVAL: invalid argument
*/
Cyg_ErrNo Max2112_set_mode(cyg_uint8 mode);

/*
* Function Name: Max2112_get_mode
* Condition:     need init max2112 module
* Input Para:
*
* Output Para:   max2112 i2c work mode, 1: standard 2:fase 3:high
* Return value:  ENOERR:  OK
*                -EINVAL: invalid argument
*/
Cyg_ErrNo Max2112_get_mode(void);

/*
* Function Name: Max2112_config_gpio
* Condition:     need init max2112 module
* Input Para:  gpio_sel:gpio which we select, set Output to low
* 			   gpio0: gpio in the config group, set Output to high
* 			   gpio1: gpio in the config group, set Output to high
*
* Output Para:
* Return value:  ENOERR:  OK
*                -EINVAL: invalid argument
*/
Cyg_ErrNo Max2112_config_gpio(cyg_uint32 gpio_sel, cyg_uint32 gpio0, cyg_uint32 gpio1);
#endif //__MAX2112_API_H__
