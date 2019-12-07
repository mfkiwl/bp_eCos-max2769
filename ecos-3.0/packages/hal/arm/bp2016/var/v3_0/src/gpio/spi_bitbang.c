#include <pkgconf/io.h>
#include <cyg/io/io.h>
#include <cyg/kernel/kapi.h>           // C API
#include <cyg/hal/drv_api.h>
#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h> // base types
#include <cyg/hal/hal_if.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/api/gpio_api.h>
#include <cyg/hal/api/spi_bitbang_api.h>

// #define SPI_BITBANG_DEBUG_LEVEL 
#ifdef SPI_BITBANG_DEBUG_LEVEL 
#define spi_printf diag_printf
#else
#define spi_printf(fmt, ...)
#endif

static cyg_drv_mutex_t     spi_bitbang_lock;
static int init_flag = 0;

static int get_bit(unsigned int d, int bit)
{
    return (d >> bit) & 0x1;
}

int spi_bitbang_api_init(void)
{
    if (init_flag == 0) {
        cyg_drv_mutex_init(&spi_bitbang_lock);
        init_flag = 1;
    }

    return SPI_BITBANG_RET_OK;
}

static int assert_handler(spi_gpio_dev_info *handler)
{
    if (handler == NULL) {
        return NULL_POINTER;
    }
    if (handler->sync >= 128) {
        return SYNC_GPIO_ID_INVALID;
    }
    if (handler->sdata >= 128) {
        return SDATA_GPIO_ID_INVALID;
    }
    if (handler->sclk >= 128) {
        return SCLK_GPIO_ID_INVALID;
    }
    if (handler->baud > 500000) {
        return BAUD_INVALID;
    }
    if (handler->data_width >= 31) {
        return WIDTH_INVALID;
    }

    return SPI_BITBANG_RET_OK;
}

/*
 * *handler: gpio number and spi config pointer
 * value: write data to address
 */
int spi_bitbang_api_tx(spi_gpio_dev_info *handler, unsigned int value)
{
    int i;
    int t;
    int res = 0;
    u32 width = 0;
    u32 bits = 0;
    u32 d = 0;
    u32 spi_delay = 0;

    res = assert_handler(handler);
    if (res != 0) {
        return res;
    }
    if (init_flag == 0) {
        return NOT_INIT;
    }

    cyg_drv_mutex_lock(&spi_bitbang_lock);

    width = handler->data_width;
    bits = handler->data_width - 1;
    spi_delay = (1000 * 1000 / handler->baud / 2);
    d = value;

    // set output
    gpio_api_set_output(handler->sync);
    gpio_api_set_output(handler->sdata);
    gpio_api_set_output(handler->sclk);

    //prepare
    gpio_api_set_high(handler->sync);
    gpio_api_set_high(handler->sdata);
    gpio_api_set_low(handler->sclk);
    HAL_DELAY_US(100);

    gpio_api_set_low(handler->sync);
    for(i = 0; i < width; i++) {
        gpio_api_set_high(handler->sclk);
        t = get_bit(d, bits);
        bits --;
        if(t > 0)
            gpio_api_set_high(handler->sdata);
        else
            gpio_api_set_low(handler->sdata);

        HAL_DELAY_US(spi_delay);
        gpio_api_set_low(handler->sclk);
        HAL_DELAY_US(spi_delay);
    }
    gpio_api_set_high(handler->sync);

    cyg_drv_mutex_unlock(&spi_bitbang_lock);

    return SPI_BITBANG_RET_OK;
}

