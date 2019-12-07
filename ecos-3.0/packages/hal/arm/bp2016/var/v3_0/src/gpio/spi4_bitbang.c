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
#include <cyg/hal/api/spi4_bitbang_api.h>

// #define SPI4_BITBANG_DEBUG_LEVEL 
#ifdef SPI4_BITBANG_DEBUG_LEVEL 
#define spi_printf diag_printf
#else
#define spi_printf(fmt, ...)
#endif

static cyg_drv_mutex_t     spi4_bitbang_lock;
static int init_flag = 0;

static int get_bit(unsigned int d, int bit)
{
    return (d >> bit) & 0x1;
}

static void get_val(unsigned int *rdata, unsigned int rbit, unsigned int i)
{
    unsigned int tmp = 0;

    if (rbit > 0) {
        *rdata |= (1 << i);
    } else {
        tmp = ~(1 << i);
        *rdata &= tmp;
    }
}

int spi4_bitbang_api_init(void)
{
    if (init_flag == 0) {
        cyg_drv_mutex_init(&spi4_bitbang_lock);
        init_flag = 1;
    }

    return SPI4_BITBANG_RET_OK;
}

static int assert_handler(spi4_gpio_dev_info *handler)
{
    if (handler == NULL) {
        return NULL_POINTER;
    }
    if (handler->cs >= 128) {
        return CS_GPIO_ID_INVALID;
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
    if (handler->addr_width >= 31) {
        return WIDTH_INVALID;
    }
    if (handler->data_width >= 31) {
        return WIDTH_INVALID;
    }
    if ((handler->addr_width + handler->data_width) > 31) {
        return WIDTH_INVALID;
    }

    return SPI4_BITBANG_RET_OK;
}

/*
 * *handler: gpio number and spi config pointer
 * addr: rx3902 reg addr
 * value: write to address
 */
int spi4_bitbang_api_tx(spi4_gpio_dev_info *handler, u32 addr, unsigned int value)
{
    int i;
    int t;
    int res = 0;
    u32 width = 0;
    u32 bits = 0;
    u32 w_flag = 0;
    u32 d = 0;
    u32 spi_delay = 0;
    u32 tmp = 0;

    res = assert_handler(handler);
    if (res != 0) {
        return res;
    }
    if (init_flag == 0) {
        return NOT_INIT;
    }

    tmp = (1 << handler->addr_width) - 1;
    if (addr > tmp) {
        spi_printf("\nerror: addr overflow");
        return ADDR_INVALID;
    }

    tmp = (1 << handler->data_width) - 1;
    if (value > tmp) {
        spi_printf("\nerror: value overflow");
        return VALUE_INVALID;
    }

    cyg_drv_mutex_lock(&spi4_bitbang_lock);

    width = handler->addr_width + handler->data_width;
    bits = handler->addr_width + handler->data_width;
    w_flag = (0 << width);
    spi_delay = (1000 * 1000 / handler->baud / 2);
    d = w_flag | (addr << handler->data_width) | value;

    // set output
    gpio_api_set_output(handler->cs);
    gpio_api_set_output(handler->sdata);
    gpio_api_set_output(handler->sclk);

    //prepare
    gpio_api_set_low(handler->cs);
    gpio_api_set_low(handler->sdata);
    gpio_api_set_low(handler->sclk);
    HAL_DELAY_US(100);

    gpio_api_set_low(handler->sclk);
    HAL_DELAY_US(spi_delay);
    gpio_api_set_high(handler->sclk);
    HAL_DELAY_US(spi_delay);
    gpio_api_set_low(handler->sclk);

    gpio_api_set_high(handler->cs);

    for(i = 0; i < (width + 1); i++) {
        t = get_bit(d, bits);
        bits --;
        if(t > 0)
            gpio_api_set_high(handler->sdata);
        else
            gpio_api_set_low(handler->sdata);

        HAL_DELAY_US(spi_delay);
        gpio_api_set_high(handler->sclk);
        HAL_DELAY_US(spi_delay);
        gpio_api_set_low(handler->sclk);
    }

    gpio_api_set_low(handler->cs);
    gpio_api_set_low(handler->sclk);
    gpio_api_set_low(handler->sdata);

    cyg_drv_mutex_unlock(&spi4_bitbang_lock);

    return SPI4_BITBANG_RET_OK;
}

/*
 * *handler: gpio number and spi config pointer
 * addr: rx3902 reg addr
 * return: value of read address
 */
int spi4_bitbang_api_rx(spi4_gpio_dev_info *handler, u32 addr, u32 *value)
{
    int i;
    int bits = 0;
    int t;
    int res = 0;
    u32 r_width = 0;
    u32 r_flag = 0;
    u32 d = 0;
    u32 rdata = 0;
    u32 rbit = 0;
    u32 spi_delay = 0;
    u32 tmp = 0;

    res = assert_handler(handler);
    if (res != 0) {
        return res;
    }
    if (init_flag == 0) {
        return NOT_INIT;
    }
    if (value == NULL) {
        return NULL_POINTER;
    }

    tmp = (1 << handler->addr_width) - 1;
    if (addr > tmp) {
        return ADDR_INVALID;
    }

    cyg_drv_mutex_lock(&spi4_bitbang_lock);

    bits = handler->addr_width;
    r_width = handler->data_width + 1;   // dummy 2 cycle
    r_flag = (1 << handler->addr_width);
    spi_delay = (1000 * 1000 / handler->baud / 2);
    d = r_flag | addr;

    // set output
    gpio_api_set_output(handler->cs);
    gpio_api_set_output(handler->sdata);
    gpio_api_set_output(handler->sclk);

    // prepare for communication
    gpio_api_set_low(handler->cs);
    gpio_api_set_low(handler->sdata);
    gpio_api_set_low(handler->sclk);
    HAL_DELAY_US(100);

    gpio_api_set_low(handler->sclk);
    HAL_DELAY_US(spi_delay);
    gpio_api_set_high(handler->sclk);
    HAL_DELAY_US(spi_delay);
    gpio_api_set_low(handler->sclk);

    // tx addr
    gpio_api_set_high(handler->cs);

    for(i = 0; i <= (handler->addr_width); i++) {
        gpio_api_set_low(handler->sclk);
        t = get_bit(d, bits);
        bits --;
        if(t > 0)
            gpio_api_set_high(handler->sdata);
        else
            gpio_api_set_low(handler->sdata);

        HAL_DELAY_US(spi_delay);
        gpio_api_set_high(handler->sclk);
        HAL_DELAY_US(spi_delay);
    }
    gpio_api_set_low(handler->sclk);
    gpio_api_set_low(handler->cs);

    gpio_api_set_input(handler->sdata);

    //rx data, bit_max + 2 cycle
    for(i = r_width; i >= 0; i--) {
        HAL_DELAY_US(spi_delay);
        gpio_api_set_high(handler->sclk);
        HAL_DELAY_US(spi_delay);
        gpio_api_get_data(handler->sdata, &rbit);
        get_val(&rdata, rbit, i);
        gpio_api_set_low(handler->sclk);
    }

    *value = rdata & ((1 << handler->data_width) - 1);

    cyg_drv_mutex_unlock(&spi4_bitbang_lock);

    return SPI4_BITBANG_RET_OK;
}

