#ifndef __SPI4_BITBANG_API_H__
#define __SPI4_BITBANG_API_H__

typedef struct spi4_gpio_dev_info {
    // gpio id
    u32 cs;
    u32 sdata;
    u32 sclk;
    // addr and data width
    u32 baud;
    u32 addr_width;
    u32 data_width;
} spi4_gpio_dev_info;

enum {
    SPI4_BITBANG_RET_OK = 0,
    NOT_INIT = -1,
    WIDTH_INVALID = -2,
    ADDR_INVALID = -3,
    VALUE_INVALID = -4,
    BAUD_INVALID = -5,
    NULL_POINTER = -6,
    CS_GPIO_ID_INVALID = -7,
    SDATA_GPIO_ID_INVALID = -8,
    SCLK_GPIO_ID_INVALID = -9,
};

/*
 * return: 0: pass, others: error
 */
int spi4_bitbang_api_init(void);

/*
 * *handler: gpio number and spi config pointer
 * addr: rx3902 reg addr
 * value: write to address
 * return: 0: success, others: error
 */
int spi4_bitbang_api_tx(spi4_gpio_dev_info *handler, u32 addr, unsigned int value);

/*
 * *handler: gpio number and spi config pointer
 * addr: rx3902 reg addr
 * *value: value of read address
 * return: 0: success, others: error
 */
int spi4_bitbang_api_rx(spi4_gpio_dev_info *handler, u32 addr, u32 *value);

#endif /* __SPI4_BITBANG_API_H__ */

