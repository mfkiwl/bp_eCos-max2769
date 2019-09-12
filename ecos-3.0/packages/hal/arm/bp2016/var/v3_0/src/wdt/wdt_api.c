#include <cyg/kernel/kapi.h>           // C API
#include <cyg/hal/drv_api.h>
#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h> // base types
#include <cyg/hal/hal_if.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/regs/wdt.h>
#include <cyg/hal/drv_api.h>
#include <cyg/hal/wdt/wdt_if.h>
#include <cyg/hal/api/wdt_api.h>

#define WDT_RESTART         0x76
#define WDT_EN              1
#define RESP_MODE_DIS       0
#define WDT_RTL             6

static cyg_drv_mutex_t  wdt_lock;
static bool wdt_api_init_flg = false;

void wdt_api_init(void)
{
    if(true == wdt_api_init_flg){
        return ;
    }
    cyg_drv_mutex_init(&wdt_lock);
    wdt_api_init_flg = true;
}

bool wdt_api_enable(void)
{
    if(false == wdt_api_init_flg){
        return false;
    }
    cyg_drv_mutex_lock(&wdt_lock);
    wdt_cr(hwp_apWdt0, WDT_EN, RESP_MODE_DIS, WDT_RTL);//wdt enable + disable resp mode
    cyg_drv_mutex_unlock(&wdt_lock);
    return true;
}

bool wdt_api_feed(void)
{
    if(false == wdt_api_init_flg){
        return false;
    }
    cyg_drv_mutex_lock(&wdt_lock);
    wdt_crr(hwp_apWdt0, WDT_RESTART);
    cyg_drv_mutex_unlock(&wdt_lock);
    return true;
}

bool wdt_api_set_pause(WDT_TIME time)
{
    if(false == wdt_api_init_flg){
        return false;
    }
    u32 torr_value = 0;
    u32 torr_index = 0;
    torr_value = get_time2torr((u32)time);
    torr_index = get_torr_index(torr_value);
    cyg_drv_mutex_lock(&wdt_lock);
    wdt_torr(hwp_apWdt0, torr_index, torr_index);
    wdt_crr(hwp_apWdt0, WDT_RESTART);
    cyg_drv_mutex_unlock(&wdt_lock);
    return true;
}

cyg_uint32 wdt_api_get_pause(void)
{
    if(false == wdt_api_init_flg){
        return 0xffffffff;
    }
    cyg_uint32 counter_val = 0;
    cyg_drv_mutex_lock(&wdt_lock);
    counter_val = wdt_ccvr(hwp_apWdt0);
    cyg_drv_mutex_unlock(&wdt_lock);
    return counter_val;
}


