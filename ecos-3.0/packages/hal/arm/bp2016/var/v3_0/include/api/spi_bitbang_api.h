#ifndef __SPI_BITBANG_API_H__
#define __SPI_BITBANG_API_H__

typedef struct spi_gpio_dev_info {
    // gpio id
    u32 sync;
    u32 sdata;
    u32 sclk;
    // data width
    u32 baud;
    u32 data_width;
} spi_gpio_dev_info;

enum {
    SPI_BITBANG_RET_OK = 0,
    NOT_INIT = -1,
    WIDTH_INVALID = -2,
    VALUE_INVALID = -4,
    BAUD_INVALID = -5,
    NULL_POINTER = -6,
    SYNC_GPIO_ID_INVALID = -7,
    SDATA_GPIO_ID_INVALID = -8,
    SCLK_GPIO_ID_INVALID = -9,
};

/*
 * return: 0: pass, others: error
 */
int spi_bitbang_api_init(void);

/*
 * *handler: gpio number and spi config pointer
 * value: write value to address
 * return: 0: success, others: error
 */
int spi_bitbang_api_tx(spi_gpio_dev_info *handler, unsigned int value);

#endif /* __SPI_BITBANG_API_H__ */
