#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/io/devtab.h>
#include <string.h>

#ifdef CYGPKG_BP2016_I2C_DEV_MAX2112
#include <cyg/io/max2112/max2112_bp2016.h>
#include <cyg/hal/api/max2112_api.h>
#include <pkgconf/bp2016_i2c_dev_max2112.h>

// this is for debug trace,
#if (CYGPKG_DEVS_MAX2112_ARM_BP2016_DEBUG_LEVEL > 0)
#define max2112_printf(fmt, args...)                                                               \
    diag_printf("[max2112 api] %s:%d " fmt "\n", __func__, __LINE__, ##args);
#else
#define max2112_printf(fmt, ...)
#endif

typedef struct max2112_api_t {
    cyg_io_handle_t max2112_handler;
    cyg_drv_mutex_t lock;
    max2112_req_t req;
} max2112_api_t;

static max2112_api_t max2112_dev;

// static cyg_uint32 max2112_flag=0;

static Cyg_ErrNo Max2112_Open(void)
{
    Cyg_ErrNo res = 0;

    res = cyg_io_lookup("/dev/max2112", &max2112_dev.max2112_handler);

    return res;
}

Cyg_ErrNo Max2112_Init(cyg_uint32 bus_id)
{
    Cyg_ErrNo res = 0;
    cyg_io_handle_t max2112_hdl;
    cyg_uint32 req_len = 0;

    // memset(&max2112_dev, 0 , sizeof(max2112_api_t));

    res = Max2112_Open();

    if (res != ENOERR) {
        max2112_printf("error, ret = %d", res);
        return res;
    }
    cyg_drv_mutex_init(&max2112_dev.lock);

    cyg_drv_mutex_lock(&max2112_dev.lock);
    max2112_hdl = max2112_dev.max2112_handler;
    req_len = sizeof(cyg_uint32);

    res = cyg_io_set_config(max2112_hdl, MAX2112_ATTACH_I2C, &bus_id, &req_len);
    if (res != ENOERR) {
        max2112_printf("attach to i2c bus %d error, ret = %d", bus_id, res);
    }
    cyg_drv_mutex_unlock(&max2112_dev.lock);

    return res;
}

Cyg_ErrNo Max2112_config_gpio(cyg_uint32 gpio_sel, cyg_uint32 gpio0, cyg_uint32 gpio1)
{
    cyg_io_handle_t max2112_hdl;
    Cyg_ErrNo res = 0;
    cyg_uint32 req_len = 0;
    max2112_gpio_t gpio_p;

    cyg_drv_mutex_lock(&max2112_dev.lock);

    max2112_hdl = max2112_dev.max2112_handler;

    gpio_p.gpio_sel = gpio_sel;
    gpio_p.gpio0 = gpio0;
    gpio_p.gpio1 = gpio1;
    req_len = sizeof(max2112_gpio_t);

    res = cyg_io_set_config(max2112_hdl, MAX2112_CONFIG_GPIO, (void *) &gpio_p, &req_len);
    if (res != ENOERR) {
        max2112_printf("config gpio error, ret %d", res);
    }

    cyg_drv_mutex_unlock(&max2112_dev.lock);
    return res;
}

Cyg_ErrNo Max2112_read(cyg_uint8 addr, void *rbuf, cyg_uint32 rlen)
{
    max2112_req_t *req;
    cyg_uint32 req_len;

    cyg_io_handle_t max2112_hdl;
    Cyg_ErrNo res = 0;

    if (NULL == rbuf) {
        max2112_printf("rbuf is NULL!");
        return ENOERR;
    }

    if ((addr > 13) || (rlen > 14)) {
        max2112_printf("addr or len error! addr=%u len %u\n", addr, rlen);
        return ENOERR;
    }

    cyg_drv_mutex_lock(&max2112_dev.lock);

    max2112_hdl = max2112_dev.max2112_handler;
    req = &max2112_dev.req;
    req->buf = rbuf;
    req->addr = addr;
    req->len = rlen;

    req_len = sizeof(max2112_req_t);
    res = cyg_io_read(max2112_hdl, (void *) req, &req_len);

    if (res != rlen) {
        max2112_printf("error, ret %d", res);
    }

    cyg_drv_mutex_unlock(&max2112_dev.lock);

    return res;
}

// there is only 14 registers of max2112,
// so set write buffer len to 16 is enough
static cyg_uint8 cwbuf[16] = {
    0,
};

Cyg_ErrNo Max2112_write(cyg_uint8 addr, void *wbuf, cyg_uint32 wlen)
{
    cyg_uint32 req_len = 0;
    cyg_io_handle_t max2112_hdl;
    Cyg_ErrNo res = 0;

    if (NULL == wbuf) {
        max2112_printf("wbuf is NULL!");
        return ENOERR;
    }

    if ((addr > 13) || (wlen > 14)) {
        max2112_printf("addr or len error! addr=%u len %u\n", addr, wlen);
        return ENOERR;
    }

    cyg_drv_mutex_lock(&max2112_dev.lock);
    max2112_hdl = max2112_dev.max2112_handler;

    cwbuf[0] = addr;
    memcpy(&cwbuf[1], (cyg_uint8 *) wbuf, wlen);
    req_len = wlen + 1;

    res = cyg_io_write(max2112_hdl, (void *) cwbuf, &req_len);

    if (res != (wlen + 1)) {
        max2112_printf("error, ret %d", res);
    } else
        res--;

    cyg_drv_mutex_unlock(&max2112_dev.lock);

    return res;
}

Cyg_ErrNo Max2112_set_mode(cyg_uint8 mode)
{
    cyg_io_handle_t max2112_hdl;
    Cyg_ErrNo res = 0;
    cyg_uint32 req_len = 1;

    cyg_drv_mutex_lock(&max2112_dev.lock);

    max2112_hdl = max2112_dev.max2112_handler;

    res = cyg_io_set_config(max2112_hdl, mode, NULL, &req_len);
    if (res != ENOERR) {
        max2112_printf("error, ret %d", res);
    }

    cyg_drv_mutex_unlock(&max2112_dev.lock);
    return res;
}

static cyg_uint8 mode = 0;
Cyg_ErrNo Max2112_get_mode(void)
{
    cyg_io_handle_t max2112_hdl;
    Cyg_ErrNo res = 0;
    cyg_uint32 req_len = 1;

    cyg_drv_mutex_lock(&max2112_dev.lock);
    max2112_hdl = max2112_dev.max2112_handler;

    res = cyg_io_get_config(max2112_hdl, MAX2112_GET_CONFIG, (void *) &mode, &req_len);
    if (res != ENOERR) {
        max2112_printf("error, ret %d", res);
    } else {
        switch (mode) {
        case I2C_MODE_STD:
            diag_printf("max2112 i2c mode: standard\r\n");
            res = (cyg_int32) mode;
            break;
        case I2C_MODE_FAST:
            diag_printf("max2112 i2c mode: fast\r\n");
            res = (cyg_int32) mode;
            break;
        case I2C_MODE_HIGH:
            diag_printf("max2112 i2c mode: high speed\r\n");
            res = (cyg_int32) mode;
            break;
        default:
            diag_printf("max2112 i2c mode: UNKNOWN! mode value %d", mode);
            break;
        }
    }

    cyg_drv_mutex_unlock(&max2112_dev.lock);
    return res;
}

#endif // CYGPKG_BP2016_I2C_DEV_MAX2112
