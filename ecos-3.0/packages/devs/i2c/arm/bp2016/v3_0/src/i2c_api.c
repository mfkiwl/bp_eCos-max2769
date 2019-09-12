#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/io/devtab.h>
#include <string.h>

#ifdef CYGPKG_DEVS_BP2016_I2C_DW
#include <cyg/io/i2c.h>
#include <cyg/io/i2c/i2c_dw_bus.h>

#ifdef CYGHWR_HAL_ASIC_CLK
#include <cyg/hal/clk/clk.h>
#endif

// wwzz added 20181018
#ifndef CYGNUM_I2C_DEV_MAX
  #define CYGNUM_I2C_DEV_MAX  (16)
#endif

#define INV_I2C_DEV_ID  (0xFFFFFFFF)

static cyg_mutex_t    i2c_dev_get_lock;

typedef struct local_i2c_dev {
    cyg_i2c_device     i2c_dev;
    cyg_mutex_t    lock;
    cyg_uint8          i2c_mode;
    bool               init;
} local_i2c_dev;


local_i2c_dev i2c_dev_arr[CYGNUM_I2C_DEV_MAX];

// called in platform_init when startup
void  i2c_api_init(void)
{
    static bool i2c_api_init_flag = false;
    if(true == i2c_api_init_flag) return;
    cyg_mutex_init(&i2c_dev_get_lock);
    i2c_api_init_flag = true;
}

static cyg_uint32 get_i2c_dev(void)
{
    cyg_uint32 i, ret = INV_I2C_DEV_ID;

    cyg_mutex_lock(&i2c_dev_get_lock);
    for(i = 0; i < CYGNUM_I2C_DEV_MAX; i++){
        if(i2c_dev_arr[i].init == false){
            ret = i;
        }
    }
    cyg_mutex_unlock(&i2c_dev_get_lock);
    return ret;
}

static void free_i2c_dev(cyg_uint32 id)
{
    cyg_mutex_lock(&i2c_dev_get_lock);
    if(i2c_dev_arr[id].init == true){
        i2c_dev_arr[id].init = false;
    }
    cyg_mutex_unlock(&i2c_dev_get_lock);
}

void I2C_dev_deinit(void *handle)
{
    if(NULL == handle) return;

    local_i2c_dev *op_i2c_dev = (local_i2c_dev *)handle;
    cyg_drv_mutex_lock(&op_i2c_dev->lock);
    op_i2c_dev->init = false;
    handle = NULL;
    cyg_drv_mutex_unlock(&op_i2c_dev->lock);
}

// this is for asic set input to 95MHz
#define     I2C_DEFAULT_CLK_IN      95000
#ifdef CYGHWR_HAL_ASIC_CLK
static int clk_set_i2c(cyg_uint32 index, int rate_kHz)
{
    switch (index) {
    case 0:
        hal_clk_set_rate_kHz(CLK_I2C0, rate_kHz);
        break;
    case 1:
        hal_clk_set_rate_kHz(CLK_I2C1, rate_kHz);
        break;
    }
    return 0;
}
#else
static int clk_set_i2c(cyg_uint32 index, int rate_kHz)
{
    return 0;
}
#endif

void * I2C_dev_init(cyg_uint8 bus_id, cyg_uint8 slv_addr, cyg_uint8 mode)
{
    cyg_uint32 id;
    struct cyg_i2c_bus* bus = NULL;
    if((bus_id > 1) ||((mode != DW_IC_CON_SPEED_STD) 
                        && (mode != DW_IC_CON_SPEED_FAST)
                        && (mode != DW_IC_CON_SPEED_HIGH))){
        return NULL;
    }

    id = get_i2c_dev();
    if(INV_I2C_DEV_ID == id){
        return NULL;
    }

    if (0 == bus_id) {
#ifdef CYGVAR_DEVS_BP2016_I2C_BUS0_EN
        bus = &bp2016_i2c_bus0;
#else
        diag_printf("%s: Attach to I2C0 but I2C0 not init, exit ...\n", __func__);
        free_i2c_dev(id);
        return NULL;
#endif // CYGVAR_DEVS_BP2016_I2C_BUS0_EN
    } else if (1 == bus_id) {
#ifdef CYGVAR_DEVS_BP2016_I2C_BUS1_EN
        bus = &bp2016_i2c_bus1;
#else
        diag_printf("%s: Attach to I2C1 but I2C1 not init, exit ...\n", __func__);
        free_i2c_dev(id);
        return NULL; 
#endif
    }

    clk_set_i2c(bus_id, I2C_DEFAULT_CLK_IN);

    cyg_mutex_init(&i2c_dev_arr[id].lock);
    i2c_dev_arr[id].i2c_dev.i2c_bus = bus;
    i2c_dev_arr[id].i2c_dev.i2c_address = slv_addr;
    i2c_dev_arr[id].i2c_dev.i2c_flags = 0;
    i2c_dev_arr[id].i2c_dev.i2c_delay = CYG_I2C_DEFAULT_DELAY;
    i2c_dev_arr[id].i2c_mode = mode;
    i2c_dev_arr[id].init = true;

    return &i2c_dev_arr[id];
}

Cyg_ErrNo I2C_dev_read(void * handle, cyg_uint8* wbuf, cyg_uint32 wlen, void * rbuf, cyg_uint32 rlen)
{
    cyg_uint32 result;
    cyg_dw_i2c_bus *extra; //= (cyg_dw_i2c_bus *) bp2016_max2112_dev.i2c_bus->i2c_extra;
    if(NULL == handle) return -EINVAL;

    local_i2c_dev *op_i2c_dev = (local_i2c_dev *)handle;
    cyg_i2c_device *dev = &(op_i2c_dev->i2c_dev);

    cyg_mutex_lock(&op_i2c_dev->lock);

    extra = (cyg_dw_i2c_bus *)(dev->i2c_bus->i2c_extra);

    cyg_i2c_transaction_begin(dev);
    extra->i2c_mode = op_i2c_dev->i2c_mode;
    if(wbuf && wlen){
        result = cyg_i2c_transaction_tx(dev, true, wbuf, wlen, false);
        if(wlen == result){
            result = cyg_i2c_transaction_rx(dev, true, rbuf, (cyg_uint32) rlen, false, true);
        }else{
            diag_printf("%s: error! \n", __func__);
        }
    }else{
        diag_printf("%s: read only! \n", __func__);
        result = cyg_i2c_transaction_rx(dev, true, (cyg_uint8 *) rbuf, (cyg_uint32) rlen, false, true);
    }
    cyg_i2c_transaction_end(dev);

    cyg_mutex_unlock(&op_i2c_dev->lock);

    return result;
}

Cyg_ErrNo I2C_dev_write(void * handle, void * wbuf, cyg_uint32 wlen)
{
    cyg_uint32 result;
    cyg_dw_i2c_bus *extra; //= (cyg_dw_i2c_bus *) bp2016_max2112_dev.i2c_bus->i2c_extra;
    if(NULL == handle) return -EINVAL;

    local_i2c_dev *op_i2c_dev = (local_i2c_dev *)handle;
    cyg_i2c_device *dev = &(op_i2c_dev->i2c_dev);

    cyg_mutex_lock(&op_i2c_dev->lock);
    extra = (cyg_dw_i2c_bus *)(dev->i2c_bus->i2c_extra);
    cyg_i2c_transaction_begin(dev);
    extra->i2c_mode = op_i2c_dev->i2c_mode;
    result = cyg_i2c_transaction_tx(dev, true, (cyg_uint8 *) wbuf, wlen, true);

    cyg_i2c_transaction_end(dev);
    cyg_drv_mutex_unlock(&op_i2c_dev->lock);

    return result;
}

#endif //CYGPKG_DEVS_BP2016_I2C_DW
