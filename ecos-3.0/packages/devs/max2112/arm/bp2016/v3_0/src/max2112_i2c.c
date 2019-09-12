//==========================================================================
//
//      max2112_i2c.c
//
//      I2C support for MAX2112
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2008 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later
// version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with eCos; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// As a special exception, if other files instantiate templates or use
// macros or inline functions from this file, or you compile this file
// and link it with other works to produce a work based on this file,
// this file does not by itself cause the resulting work to be covered by
// the GNU General Public License. However the source code for this file
// must still be made available in accordance with section (3) of the GNU
// General Public License v2.
//
// This exception does not invalidate any other reasons why a work based
// on this file might be covered by the GNU General Public License.
// -------------------------------------------
// ####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    wu jinying <wujinying@haigebeidou.cn>
// Contributors:
// Date:         2018-01-23
// Purpose:
// Description:
//
//####DESCRIPTIONEND####
//
//==========================================================================

//=============================================================================
//                               INCLUDES
//=============================================================================

#include <cyg/kernel/kapi.h>
#include <cyg/io/devtab.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/cyg_ass.h>
#include <errno.h>
#include <string.h>

#include <cyg/io/i2c.h>
#include <cyg/io/i2c/i2c_dw_bus.h>
#include <cyg/io/max2112/max2112_bp2016.h>
#include <pkgconf/devs_bp2016_i2c_dw.h>
#include <cyg/hal/gpio/gpio_if.h>
#include <cyg/hal/hal_diag.h>

#ifdef CYGPKG_BP2016_I2C_DEV_MAX2112
#include <pkgconf/bp2016_i2c_dev_max2112.h>

// this is for debug trace,
#if (CYGPKG_DEVS_MAX2112_ARM_BP2016_DEBUG_LEVEL > 0)
#define max2112_printf(fmt, args...)                                                               \
    diag_printf("[max2112 api] %s:%d " fmt "\n", __func__, __LINE__, ##args);
#else
#define max2112_printf(fmt, ...)
#endif

#define MAX2112_I2C_SLV_ADDR (0x60)

typedef struct max2112_priv_chan_info {
    cyg_drv_mutex_t lock;
    // cyg_drv_cond_t      signal;

    cyg_uint8 i2c_mode;
} max2112_priv_chan_info;

//-----------------------------------------------------------------------------
// 256-Kb I2C CMOS Serial max2112
//
CYG_I2C_DEVICE(bp2016_max2112_dev, NULL, MAX2112_I2C_SLV_ADDR, 0, CYG_I2C_DEFAULT_DELAY);

max2112_priv_chan_info max2112_info;
max2112_gpio_t max2112_gpio;

//=============================================================================
// Setup I2C bus 0
//=============================================================================
static bool max2112_init(struct cyg_devtab_entry *tab)
{
    max2112_priv_chan_info *chan = (max2112_priv_chan_info *) tab->priv;

    // cyg_dw_i2c_bus *extra = (cyg_dw_i2c_bus *) bp2016_max2112_dev.i2c_bus->i2c_extra;

    memset(&max2112_info, 0, sizeof(max2112_priv_chan_info));
    memset(&max2112_gpio, 0, sizeof(max2112_gpio_t));
    cyg_drv_mutex_init(&chan->lock);

    max2112_info.i2c_mode = DW_IC_CON_SPEED_FAST;
    // extra->i2c_mode = max2112_info.i2c_mode;
	return 0;
}

static inline void max2112_set_gpio(void)
{
    gpio_set_output_direction(max2112_gpio.gpio0);
    gpio_set_output_high(max2112_gpio.gpio0);

    gpio_set_output_direction(max2112_gpio.gpio1);
    gpio_set_output_high(max2112_gpio.gpio1);

    gpio_set_output_direction(max2112_gpio.gpio_sel);
    gpio_set_output_low(max2112_gpio.gpio_sel);
    HAL_DELAY_US(30000);
}

//===========================================================================
// Write data to max2112
// If the call is blocking (blocking == true) then the function will poll
// the device if it is busy writing. If it is non blocking then the function
// immediately return 0 if the device is busy
//===========================================================================
static Cyg_ErrNo max2112_write(cyg_io_handle_t handle, const void *buffer, cyg_uint32 *len)
{
    cyg_uint32 result;
    cyg_dw_i2c_bus *extra; //= (cyg_dw_i2c_bus *) bp2016_max2112_dev.i2c_bus->i2c_extra;
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *) handle;
    max2112_priv_chan_info *chan = (max2112_priv_chan_info *) t->priv;
    cyg_i2c_bus *i2c_bus;

    i2c_bus = bp2016_max2112_dev.i2c_bus;
    // i2c_bus is NULL
    if (!i2c_bus) {
        diag_printf("%s: i2c_bus is NULL!\n", __func__);
        return 0;
    }

    extra = (cyg_dw_i2c_bus *) i2c_bus->i2c_extra;

    CYG_ASSERT(buffer, "Invalid data pointer");
    CYG_ASSERT((*len > 0), "Invalid data size");

    max2112_set_gpio();
    cyg_drv_mutex_lock(&chan->lock);

    cyg_i2c_transaction_begin(&bp2016_max2112_dev);
    // addr = CYG_CPU_TO_BE16(addr);
    extra->i2c_mode = max2112_info.i2c_mode;
    result = cyg_i2c_transaction_tx(&bp2016_max2112_dev, true, (cyg_uint8 *) buffer, *len, true);

    cyg_i2c_transaction_end(&bp2016_max2112_dev);
    cyg_drv_mutex_unlock(&chan->lock);

    return result;
}

//===========================================================================
// Read data from max2112 (selective, sequential read)
// If the call is blocking (blocking == true) then the function will poll
// the device if it is busy writing. If it is non blocking then the function
// immediately return 0 if the device is busy
//===========================================================================
static Cyg_ErrNo max2112_read(cyg_io_handle_t handle, void *buffer, cyg_uint32 *len)
{
    cyg_uint32 result;
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *) handle;
    max2112_priv_chan_info *chan = (max2112_priv_chan_info *) t->priv;
    max2112_req_t req;
    cyg_dw_i2c_bus *extra; // = (cyg_dw_i2c_bus *) bp2016_max2112_dev.i2c_bus->i2c_extra;
    cyg_i2c_bus *i2c_bus;

    i2c_bus = bp2016_max2112_dev.i2c_bus;
    // i2c_bus is NULL
    if (!i2c_bus) {
        diag_printf("%s: i2c_bus is NULL!\n", __func__);
        return 0;
    }

    extra = (cyg_dw_i2c_bus *) i2c_bus->i2c_extra;

    CYG_ASSERT(buffer, "Invalid data pointer");
    CYG_ASSERT((*len > 0), "Invalid data size");

    max2112_set_gpio();
    cyg_drv_mutex_lock(&chan->lock);

    memcpy(&req, buffer, sizeof(max2112_req_t));

    cyg_i2c_transaction_begin(&bp2016_max2112_dev);
    extra->i2c_mode = max2112_info.i2c_mode;
    result = cyg_i2c_transaction_tx(&bp2016_max2112_dev, true, (cyg_uint8 *) &(req.addr), 1, false);
    if (1 == result) {
        result = cyg_i2c_transaction_rx(&bp2016_max2112_dev, true, (cyg_uint8 *) req.buf,
                                        (cyg_uint32) req.len, false, true);
    } else {
        diag_printf("%s: error! \n", __func__);
    }
    cyg_i2c_transaction_end(&bp2016_max2112_dev);

    cyg_drv_mutex_unlock(&chan->lock);

    return result;
}

static cyg_bool max2112_select(cyg_io_handle_t handle, cyg_uint32 which, cyg_addrword_t info)
{
    return -EINVAL;
}

static Cyg_ErrNo max2112_lookup(struct cyg_devtab_entry **tab, struct cyg_devtab_entry *st,
                                const char *name)
{
    return ENOERR;
}

static Cyg_ErrNo max2112_set_config(cyg_io_handle_t handle, cyg_uint32 key, const void *buffer,
                                    cyg_uint32 *len)
{
    // Cyg_ErrNo res = 0;
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *) handle;
    max2112_priv_chan_info *chan = (max2112_priv_chan_info *) t->priv;
    cyg_uint32 *bus_id = NULL;

    if ((len == NULL) || (*len == 0)) {
        diag_printf("\n%s len is null\n", __func__);
        return -EINVAL;
    }

    cyg_drv_mutex_lock(&chan->lock);

    switch (key) {
    case MAX2112_ATTACH_I2C:
        if (buffer == NULL) {
            diag_printf("\n%s buffer is null\n", __func__);
            cyg_drv_mutex_unlock(&chan->lock);
            return -EINVAL;
        }

        if ((len == NULL) || (*len == 0)) {
            diag_printf("\n%s len is null\n", __func__);
            cyg_drv_mutex_unlock(&chan->lock);
            return -EINVAL;
        }

        bus_id = (cyg_uint32 *) buffer;
        if (0 == *bus_id) {
#ifdef CYGVAR_DEVS_BP2016_I2C_BUS0_EN
            bp2016_max2112_dev.i2c_bus = &bp2016_i2c_bus0;
            ((cyg_dw_i2c_bus *) bp2016_i2c_bus0.i2c_extra)->i2c_mode = max2112_info.i2c_mode;
#else
			diag_printf("%s: Attach to I2C0 but I2C0 not init, exit ...\n", __func__);
            cyg_drv_mutex_unlock(&chan->lock);
            return -EINVAL;
#endif // CYGVAR_DEVS_BP2016_I2C_BUS0_EN
        } else if (1 == *bus_id) {
#ifdef CYGVAR_DEVS_BP2016_I2C_BUS1_EN
            bp2016_max2112_dev.i2c_bus = &bp2016_i2c_bus1;
            ((cyg_dw_i2c_bus *) bp2016_i2c_bus1.i2c_extra)->i2c_mode = max2112_info.i2c_mode;
#else
			diag_printf("%s: Attach to I2C1 but I2C1 not init, exit ...\n", __func__);
            cyg_drv_mutex_unlock(&chan->lock);
            return -EINVAL;
#endif // CYGVAR_DEVS_BP2016_I2C_BUS1_EN
        } else {
            diag_printf("%s bus id %d error!\r\n", __func__, *bus_id);
            cyg_drv_mutex_unlock(&chan->lock);
            return -EINVAL;
        }
        break;

    case MAX2112_CONFIG_GPIO:
        if (buffer == NULL) {
            diag_printf("\n%s buffer is null\n", __func__);
            cyg_drv_mutex_unlock(&chan->lock);
            return -EINVAL;
        }

        if ((len == NULL) || (*len == 0)) {
            diag_printf("\n%s len is null\n", __func__);
            cyg_drv_mutex_unlock(&chan->lock);
            return -EINVAL;
        }

        memcpy(&max2112_gpio, buffer, sizeof(max2112_gpio_t));
        if ((!max2112_gpio.gpio0) && (!max2112_gpio.gpio1) && (!max2112_gpio.gpio_sel)) {
            diag_printf("set config gpio number error!, all are 0");
            cyg_drv_mutex_unlock(&chan->lock);
            return -EINVAL;
        }
        max2112_printf("gpio num: %d %d %d", max2112_gpio.gpio_sel, max2112_gpio.gpio0,
                       max2112_gpio.gpio1);
        break;
    case I2C_MODE_STD:
        chan->i2c_mode = DW_IC_CON_SPEED_STD;
        break;
    case I2C_MODE_FAST:
        chan->i2c_mode = DW_IC_CON_SPEED_FAST;
        break;

    case I2C_MODE_HIGH: // etu
        chan->i2c_mode = DW_IC_CON_SPEED_HIGH;
        break;

    default:
        diag_printf("key error %d , do nothing!\n", key);
        cyg_drv_mutex_unlock(&chan->lock);
        return -EINVAL;
    }

    cyg_drv_mutex_unlock(&chan->lock);
    return ENOERR;
}

static Cyg_ErrNo max2112_get_config(cyg_io_handle_t handle, cyg_uint32 key, void *buffer,
                                    cyg_uint32 *len)
{
    Cyg_ErrNo res = 0;
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *) handle;
    max2112_priv_chan_info *chan = (max2112_priv_chan_info *) t->priv;
    cyg_uint8 mode = 0;

    if (buffer == NULL) {
        diag_printf("\n%s buffer is null\n", __func__);
        return -EINVAL;
    }

    if ((len == NULL) || (*len == 0)) {
        diag_printf("\n%s len is null\n", __func__);
        return -EINVAL;
    }

    cyg_drv_mutex_lock(&chan->lock);

    if (MAX2112_GET_CONFIG == key) {
        switch (chan->i2c_mode) {
        case DW_IC_CON_SPEED_STD:
            mode = I2C_MODE_STD;
            max2112_printf("i2c mode: standard");
            break;
        case DW_IC_CON_SPEED_FAST:
            mode = I2C_MODE_FAST;
            max2112_printf("i2c mode: fast");
            break;
        case DW_IC_CON_SPEED_HIGH:
            mode = I2C_MODE_HIGH;
            max2112_printf("i2c mode: high");
            break;
        default:
            max2112_printf("i2c mode: UNKNOWN!");
            res = -EINVAL;
            break;
        }
    }

    memcpy((cyg_uint8 *) buffer, &mode, sizeof(cyg_uint8));

    cyg_drv_mutex_unlock(&chan->lock);
    return res;
}

CHAR_DEVIO_TABLE(max2112_handler, max2112_write, max2112_read, max2112_select, max2112_get_config,
                 max2112_set_config);

#define CYGDAT_DEVS_MAX2112_BP2016_NAME ("/dev/max2112")
CHAR_DEVTAB_ENTRY(max2112_device, CYGDAT_DEVS_MAX2112_BP2016_NAME, NULL, &max2112_handler,
                  max2112_init, max2112_lookup,
                  (void *) &max2112_info); // Private data pointer

//-----------------------------------------------------------------------------
// EOF platform_i2c.c

#endif // CYGPKG_BP2016_I2C_DEV_MAX2112
