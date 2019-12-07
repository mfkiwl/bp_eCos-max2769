#ifndef __UART_API_H__
#define __UART_API_H__

// INDEX
#define HWP_UART_PORT_NUM        (0x8)
#define UART_ID_0                  0x00
#define UART_ID_1                  0x01
#define UART_ID_2                  0x02
#define UART_ID_3                  0x03
#define UART_ID_4                  0x04
#define UART_ID_5                  0x05
#define UART_ID_6                  0x06
#define UART_ID_7                  0x07

// Supported baud rates
typedef enum {
    CYGNUM_UART_BAUD_50 = 1,
    CYGNUM_UART_BAUD_75,
    CYGNUM_UART_BAUD_110,
    CYGNUM_UART_BAUD_134_5,
    CYGNUM_UART_BAUD_150 = 5,
    CYGNUM_UART_BAUD_200,
    CYGNUM_UART_BAUD_300,
    CYGNUM_UART_BAUD_600,
    CYGNUM_UART_BAUD_1200,
    CYGNUM_UART_BAUD_1800 = 10,
    CYGNUM_UART_BAUD_2400,
    CYGNUM_UART_BAUD_3600,
    CYGNUM_UART_BAUD_4800,
    CYGNUM_UART_BAUD_7200,
    CYGNUM_UART_BAUD_9600 = 15,
    CYGNUM_UART_BAUD_14400,
    CYGNUM_UART_BAUD_19200,
    CYGNUM_UART_BAUD_38400,
    CYGNUM_UART_BAUD_57600,
    CYGNUM_UART_BAUD_115200 = 20,
    CYGNUM_UART_BAUD_230400,
    CYGNUM_UART_BAUD_460800,
    CYGNUM_UART_BAUD_921600,
    CYGNUM_UART_BAUD_1875000,
    CYGNUM_UART_BAUD_2850000 = 25
} cyg_uart_baud_rate_t;

#define CYGNUM_UART_BAUD_MIN CYGNUM_UART_BAUD_50
#define CYGNUM_UART_BAUD_MAX CYGNUM_UART_BAUD_2850000

// Note: two levels of macro are required to get proper expansion.
#define _CYG_UART_BAUD_RATE(n) CYGNUM_UART_BAUD_##n
#define CYG_UART_BAUD_RATE(n) _CYG_UART_BAUD_RATE(n)

// Stop bit selections
typedef enum {
    CYGNUM_UART_STOP_1 = 1,
    CYGNUM_UART_STOP_1_5,
    CYGNUM_UART_STOP_2
} cyg_uart_stop_bits_t;

// Parity modes
typedef enum {
    CYGNUM_UART_PARITY_NONE = 0,
    CYGNUM_UART_PARITY_EVEN,
    CYGNUM_UART_PARITY_ODD,
    CYGNUM_UART_PARITY_MARK,
    CYGNUM_UART_PARITY_SPACE
} cyg_uart_parity_t;

// Word length
typedef enum {
    CYGNUM_UART_WORD_LENGTH_5 = 5,
    CYGNUM_UART_WORD_LENGTH_6,
    CYGNUM_UART_WORD_LENGTH_7,
    CYGNUM_UART_WORD_LENGTH_8
} cyg_uart_word_length_t;

// Init flag
typedef enum {
    CYGNUM_UART_INIT_TYPE_NO = 0,
    CYGNUM_UART_INIT_TYPE_SYS,
    CYGNUM_UART_INIT_TYPE_POLLING
} cyg_uart_init_type_t;

typedef struct {
    cyg_uint32   baud;
    cyg_uart_stop_bits_t   stop;
    cyg_uart_parity_t      parity;
    cyg_uart_word_length_t word_length;
    cyg_uint32               flags;
} cyg_uart_info_t;

typedef struct {  
    cyg_int32 rx_bufsize;  // rx buffer total size
    cyg_int32 rx_data_count;  // current total bytes in rx buffer(wait for reading)
    cyg_int32 tx_bufsize;  // tx buffer total size
    cyg_int32 tx_data_count;  // currend total bytes in tx buffer (to be send)
} cyg_uart_buf_info_t;

// global functions
/*
* Function Name: UartOpen
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
* Return value:  cyg_int32 res: ENOERR:OK
*                               -ENODEV:Input para invalid
*/
cyg_int32 UartOpen(cyg_uint32 id);

/*
* Function Name: UartWrite
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
*                void    *wbuf--point to the data to be written
*                cyg_uint32 *wlen--size to be written
* Output Para:   cyg_uint32 *wlen--size left to be written
* Return value:  cyg_int32 res: ENOERR:OK
*                               -EINVAL:Input para invalid
*                               -EPERM:Operation is not permitted
*                               -EINTR:Operation is interrupted
*                               -EDEVNOSUPP:device does not support this operation
*/
cyg_int32 UartWrite(cyg_uint32 id, void *wbuf, cyg_uint32 *wlen);

/*
* Function Name: UartRead
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
*                void    *rbuf--the buff to store data
*                cyg_uint32 *rlen--size to be written
* Output Para:   cyg_uint32 *rlen--character actually read
* Return value:  cyg_int32 res: ENOERR:OK
*                               -EINVAL:Input para invalid
*                               -EPERM:Operation is not permitted
*                               -EINTR:Operation is interrupted
*                               -EDEVNOSUPP:device does not support this operation
*/
cyg_int32 UartRead(cyg_uint32 id, void *rbuf, cyg_uint32 *rlen);

/*
* Function Name: UartReadTimeOut
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
*                void    *rbuf--the buff to store data
*                cyg_uint32 *rlen--size to be written
*                cyg_uint32 timeout--ticks(10ms) of timeout
* Output Para:   cyg_uint32 *rlen--character actually read
* Return value:  cyg_int32 res: ENOERR:OK
*                               -EINVAL:Input para invalid
*                               -EPERM:Operation is not permitted
*                               -EINTR:Operation is interrupted
*                               -EDEVNOSUPP:device does not support this operation
*/
cyg_int32 UartReadTimeOut(cyg_uint32 id, void *rbuf, cyg_uint32 *rlen, cyg_uint32 timeout);

/*
* Function Name: UartSetConfig
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
*                cyg_uart_info_t *config--config structure, include key para for this dev
* Return value:  cyg_int32 res: ENOERR:OK
*                               -EINVAL:Input para invalid
*                               -EPERM:Operation is not permitted
*                               -EINVAL:Invalid config para
*/
cyg_int32 UartSetConfig(cyg_uint32 id, cyg_uart_info_t *config);

/*
* Function Name: UartGetConfig
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
* OutPut para:   cyg_uart_info_t *config--store the config para of this dev
* Return value:  cyg_int32 res: ENOERR:OK
*                               -EINVAL:Input para invalid
*                               -EPERM:Operation is not permitted
*/
cyg_int32 UartGetConfig(cyg_uint32 id, cyg_uart_info_t *config);

/*
* Function Name: UartGetBufInfo
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
* OutPut para:   cyg_uart_buf_info_t *info--store the buf info of this dev
* Return value:  cyg_int32 res: ENOERR:OK
*                               -EINVAL:Input para invalid
*                               -EPERM:Operation is not permitted
*/
cyg_int32 UartGetBufInfo(cyg_uint32 id, cyg_uart_buf_info_t *info);


/*
* Function Name: UartAbort
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
* Return value:  cyg_int32 res: ENOERR:OK
*                               -EINVAL:Input para invalid
*                               -EPERM:Operation is not permitted
*/
cyg_int32 UartAbort(cyg_uint32 id);

//########################################################################
// polling interfaces
/*
* Function Name: UartPollingSetBaud
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
*                cyg_uint32 baudrate--baudrate for this port
* Return value:  cyg_int32 res: ENOERR:OK
*                               -EPERM:Operation is not permitted
*/
cyg_int32 UartPollingSetBaud(cyg_uint32 id, cyg_uint32 baudrate);

/*
* Function Name: UartPollingInit
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
*                cyg_uint32 baud--baudrate for this port
* Return value:  cyg_int32 res: ENOERR:OK
*                               -EPERM:Operation is not permitted
*/
cyg_int32 UartPollingInit(cyg_uint32 id, cyg_uint32 baud);

/*
* Function Name: UartPollingDeInit
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
* Return value:  cyg_int32 res: ENOERR:OK
*                               -EPERM:Operation is not permitted
*/
cyg_int32 UartPollingDeInit(cyg_uint32 id);

/*
* Function Name: UartPollingPutS
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
*                char       *s--string to be written
* Return value:  NONE
*/
void UartPollingPutS(cyg_uint32 id, char *s);

/*
* Function Name: UartPollingPutC
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
*                char       c--character to be written
* Return value:  NONE
*/
void UartPollingPutC(cyg_uint32 id, char c);

/*
* Function Name: UartPollingGetC
* Input Para:    cyg_uint32 id--uart port id, valid value is 0~7
* OutPut para:   char *c--store the character read out
* Return value:  cyg_int32 res: ENOERR:OK
*                               -EINVAL:Invalid id
*                               -EPERM:Operation is not permitted
*/
cyg_int32 UartPollingGetC(cyg_uint32 id, char *c);


#endif /* __UART_API_H__ */
