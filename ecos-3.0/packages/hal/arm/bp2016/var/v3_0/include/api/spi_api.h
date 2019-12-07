#ifndef __SPI_API_H__
#define __SPI_API_H__

// INDEX
#define SPI_INDEX_0 0X00
#define SPI_INDEX_1 0X01
#define SPI_INDEX_2 0X02
#define SPI_INDEX_3 0X03
#define SPI_INDEX_4 0X04

typedef union spi_dev_config_t {
    struct {
        cyg_uint32 index;
        cyg_uint32 cs;
        cyg_uint32 cpol;
        cyg_uint32 cpha;
        cyg_uint32 baud;
        cyg_uint32 bus_width;
        cyg_uint32 sample_delay;
        cyg_uint32 gpio_num;    //this is for board cs more than spi2 controler
    } spi_dw_cfg;

    struct {
        cyg_uint32 index;
        cyg_uint32 cs;
        cyg_uint32 baud;
        cyg_uint32 addr_width;
        cyg_uint32 data_width;
    } spi3wire_cfg;
} spi_dev_config_t;

/*
* Function Name: SpiInit
* Input Para:    spi config pointer
* Output Para:   none
* Return value:  handler: spi handler
*                0: error
*/
extern void * SpiInit(spi_dev_config_t *);

/*
* Function Name: SpiWrite
* Condition:     rely on need init spi module
* Input Para:    handler: spi handler
*                *wbuf: point to the data to be written
*                *wlen: length to be written
* Output Para:   *wlen: length of data actually write
* Return value:  ENOERR:  OK
*                -ENODEV: no such device
*                -EINVAL: invalid argument
*/
extern Cyg_ErrNo SpiWrite(void *, const cyg_uint8 *, cyg_uint32 *);

/*
* Function Name: SpiRead
* Condition:     need init spi module
* Input Para:    handler: spi handler
*                *wbuf: point to the data to be written
*                *wlen: length to be written
*                *rbuf: buffer to store data
*                *rlen: length of data to be read
* Output Para:   *rlen: length of data actually read
* Return value:  ENOERR:  OK
*                -ENODEV: no such device
*                -EINVAL: invalid argument
*/
extern Cyg_ErrNo SpiRead(void *, cyg_uint8 *, cyg_uint32 *);

/*
* Function Name: SpiWritethenRead
* Condition:     need init spi module
* Input Para:    handler: spi handler
*                *rbuf: buffer to store data
*                *rlen: length of data to be read
* Output Para:   *rlen: length of data actually read
* Return value:  ENOERR:  OK
*                -ENODEV: no such device
*                -EINVAL: invalid argument
*/
extern Cyg_ErrNo SpiWritethenRead(void *handler, const cyg_uint8 *wbuf, cyg_uint32 *wlen, cyg_uint8 *rbuf, cyg_uint32 *rlen);

/*
* Function Name: SpiDone
* Condition:
* Input Para:    handler: spi handler
*                cs:    cs index
* Output Para:   none
* Return value:  ENOERR:  OK
*                -ENODEV: no such device
*                -EINVAL: invalid argument
*/
extern Cyg_ErrNo SpiDone(void *handler);

/*
* Function Name: SpiGetBaud
* Condition:
* Input Para:    index: spi index, valid value is 0~4
*                cs:    cs index
*                *baud: memory to save baud
* Output Para:   baud:  output baud
* Return value:  ENOERR:  OK
*                -ENODEV: no such device
*                -EINVAL: invalid argument
*/
extern Cyg_ErrNo SpiGetBaud(void *handler, cyg_uint32 *baud);

/*
* Function Name: SpiSetBaud
* Condition:
* Input Para:    index: spi index, valid value is 0~4
*                cs:    cs index
*                *baud: memory to save baud
* Output Para:   none
* Return value:  ENOERR:  OK
*                -ENODEV: no such device
*                -EINVAL: invalid argument
*/
extern Cyg_ErrNo SpiSetBaud(void *handler, cyg_uint32 *baud);

#endif /* __SPI_API_H__ */

