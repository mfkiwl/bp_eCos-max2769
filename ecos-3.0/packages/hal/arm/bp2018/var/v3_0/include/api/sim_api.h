#ifndef __SIM_API_H__
#define __SIM_API_H__

// INDEX
#define SIM_INDEX_0 0X00
#define SIM_INDEX_1 0X01
#define SIM_INDEX_2 0X02

// global functions
/*
* Function Name: SimCardInit
* Input Para:    index: sim card index, valid value is 0~2
* Output Para:   none
* Return value:  ENOERR:  OK
*                -ENODEV: no such device
*/
extern Cyg_ErrNo SimCardInit(cyg_uint32 index);

/*
* Function Name: SimCardAtr
* Condition:     need init sim module
* Input Para:    index: sim card index, valid value is 0~2
*                etu:   sim atr work clock
*                *rbuf: buffer to store data
*                *rlen: length of data to be read
*                timeout: (ms),if *rlen not equal expected length,function returns when timeout
* Output Para:   *rlen: length of data actually read
* Return value:  ENOERR:  OK
*                -EINVAL: invalid argument
*                -ETIMEDOUT: timeout
*/
extern Cyg_ErrNo SimCardAtr(cyg_uint32 index, cyg_uint32 etu, cyg_uint32 parity, cyg_uint8 *rbuf,
                            cyg_uint32 *rlen, cyg_uint32 timeout_ms);

/*
* Function Name: SimCardWrite
* Condition:     need init sim module
* Input Para:    index: sim card index, valid value is 0~2
*                *wbuf: point to the data to be written
*                *wlen: length to be written
* Output Para:   *wlen: length of data actually write
* Return value:  ENOERR:  OK
*                -EINVAL: invalid argument
*/
extern Cyg_ErrNo SimCardWrite(cyg_uint32 index, void *wbuf, cyg_uint32 *wlen);

/*
* Function Name: SimCardRead
* Condition:     need init sim module
* Input Para:    index: sim card index, valid value is 0~2
*                *rbuf: buffer to store data
*                *rlen: length of data to be read
*                timeout: (ms),if *rlen not equal expected length,function returns when timeout
* Output Para:   *rlen: length of data actually read
* Return value:  ENOERR:  OK
*                -EINVAL: invalid argument
*                -ETIMEDOUT: timeout
*/
extern Cyg_ErrNo SimCardRead(cyg_uint32 index, void *rbuf, cyg_uint32 *rlen,
                             cyg_uint32 timeout_ms);

/*
* Function Name: SimCardRequest
* Condition:     need init sim module
* Input Para:    index: sim card index, valid value is 0~2
*                *wbuf: point to the data to be written
*                *wlen: length to be written
*                *rbuf: buffer to store data
*                *rlen: length of data to be read
*                timeout: (ms),if *rlen not equal expected length,function returns when timeout
* Output Para:   *rlen: length of data actually read
* Return value:  ENOERR:  OK
*                -EINVAL: invalid argument
*/
extern Cyg_ErrNo SimCardRequest(cyg_uint32 index, void *wbuf, cyg_uint32 *wlen, void *rbuf,
                                cyg_uint32 *rlen, const cyg_uint32 timeout_ms);

/*
* Function Name: SimCardSetetu
* Condition:     need init sim module
* Input Para:    index: sim card index, valid value is 0~2
*                etu:   sim card work clock
* Output Para:   none
* Return value:  ENOERR:OK
*                -EINVAL:invalid argument
*/
extern Cyg_ErrNo SimCardSetetu(cyg_uint32 index, cyg_uint32 etu);

/*
* Function Name: SimCardGetConfig
* Condition:     need init sim module
* Input Para:    index: sim card index, valid value is 0~2
*                *rbuf: buffer to store data
*                *rlen: length of data to be read
* Output Para:   *rlen: length of data actually read
* Return value:  ENOERR:OK
*                -EINVAL:invalid argument
*/
extern Cyg_ErrNo SimCardGetConfig(cyg_uint32 index, void *rbuf, cyg_uint32 *rlen);

// polling-----------------------

/*
* Function Name: SimCardPollingInit
* Input Para:    index: sim card index, valid value is 0~2
* Output Para:   none
* Return value:  ENOERR:  OK
*                -ENODEV: no such device
*/
extern Cyg_ErrNo SimCardPollingInit(cyg_uint32 index);

/*
* Function Name: SimCardPollingAtr
* Condition:     need init sim module
* Input Para:    index: sim card index, valid value is 0~2
*                etu:   sim atr work clock
*                parity: 0: need parity, 1: not parity
*                *rbuf: buffer to store data
*                *rlen: length of data to be read
*                timeout: (ms),if *rlen not equal expected length,function returns when timeout
* Output Para:   *rlen: length of data actually read
* Return value:  ENOERR:  OK
*                -EINVAL: invalid argument
*                -ETIMEDOUT: timeout
*/
extern Cyg_ErrNo SimCardPollingAtr(cyg_uint32 index, cyg_uint32 etu, cyg_uint32 parity, cyg_uint8 *rbuf,
                                   cyg_uint32 *rlen, cyg_uint32 timeout_ms);

/*
* Function Name: SimCardPollingRead
* Condition:     need init sim module
* Input Para:    index: sim card index, valid value is 0~2
*                *rdata: memory to store data
* Output Para:   *rdata: data read back, only return ENOERR valid
* Return value:  ENOERR:  OK
*                -EINVAL: invalid argument
*/
extern Cyg_ErrNo SimCardPollingRead(cyg_uint32 index, cyg_int8 *rdata);

/*
* Function Name: SimCardPollingWrite
* Condition:     need init sim module
* Input Para:    index: sim card index, valid value is 0~2
*                wdata: data to write, only return ENOERR success, or need rewrite
* Output Para:   none
* Return value:  ENOERR:  OK
*                -EINVAL: invalid argument
*/
extern Cyg_ErrNo SimCardPollingWrite(cyg_uint32 index, cyg_int8 wdata);

/*
* Function Name: SimCardPollingSetetu
* Condition:     need init sim module
* Input Para:    index: sim card index, valid value is 0~2
*                etu:   sim card work clock
* Output Para:   none
* Return value:  ENOERR:OK
*                -EINVAL:invalid argument
*/
extern Cyg_ErrNo SimCardPollingSetetu(cyg_uint32 index, cyg_uint32 etu);

#endif /* __SIM_API_H__ */
