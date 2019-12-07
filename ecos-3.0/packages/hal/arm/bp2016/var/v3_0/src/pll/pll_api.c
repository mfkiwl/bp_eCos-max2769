#include <stdio.h>
#include <cyg/hal/drv_api.h>
#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h> // base types
#include <cyg/hal/hal_if.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/pll/pll.h>
#include <cyg/hal/drv_api.h>
#include <cyg/hal/pll/pll.h>
#include <cyg/hal/api/pll_api.h>

static cyg_drv_mutex_t  pll_lock;
static bool pll_api_init_flg = false;

void pll_api_init(void)
{
    if(true == pll_api_init_flg){
        return ;
    }
    cyg_drv_mutex_init(&pll_lock);
    pll_api_init_flg = true;
}

int pll_api_get_rate_kHz(unsigned int pll_num)
{
    int ret = 0;
    int (*pll_get_rate_kHz)(void);
	
    if(false == pll_api_init_flg){
        return 0xffffffff;
    }
    if(pll_num > 3){
        diag_printf("The pll num is 0 to 3, the pll num is %d\n", pll_num);
        return false;
    }
   	if(0 == pll_num){
        pll_get_rate_kHz = pll0_get_rate_kHz;
	} else if(1 == pll_num){
        pll_get_rate_kHz = pll1_get_rate_kHz;
	} else if(2 == pll_num){
        pll_get_rate_kHz = pll2_get_rate_kHz;
	} else if(3 == pll_num){
        pll_get_rate_kHz = pll3_get_rate_kHz;
	} else {
        return false;
    }
    cyg_drv_mutex_lock(&pll_lock);
    ret = pll_get_rate_kHz();
    cyg_drv_mutex_unlock(&pll_lock);
    return ret;
}

bool pll_api_set_rate_kHz(unsigned int pll_num, unsigned int rate_kHz)
{
    int ret = 0;
    int (*pll_set_rate_kHz)(unsigned int rate_kHz);
    if(false == pll_api_init_flg){
        return false;
    }
   	if(pll_num > 3){
        diag_printf("The pll num is 0 to 3, the pll num is %d\n", pll_num);
        return false;
    }
    if(0 == pll_num){
        pll_set_rate_kHz = pll0_set_rate_kHz;
	} else if(1 == pll_num){
        pll_set_rate_kHz = pll1_set_rate_kHz;
	} else if(2 == pll_num){
        pll_set_rate_kHz = pll2_set_rate_kHz;
	} else if(3 == pll_num){
        pll_set_rate_kHz = pll3_set_rate_kHz;
	} else {
        return false;
    }
    cyg_drv_mutex_lock(&pll_lock);
    ret = pll_set_rate_kHz(rate_kHz);
    cyg_drv_mutex_unlock(&pll_lock);
    if(ret != RET_OK){
        return false;
    }
    return true;
}
 
bool pll_api_force_set_pll3_ref_KHz(unsigned int rate_kHz)
{
    if(false == pll_api_init_flg){
        return false;
    }

    cyg_drv_mutex_lock(&pll_lock);
    pll3_ref_clk_khz = rate_kHz;
    cyg_drv_mutex_unlock(&pll_lock);
    // diag_printf("set pll3 refclk to %u KHz\n", pll3_ref_clk_khz);

    return true;
} 

bool pll_api_set_pll3_ref_KHz(unsigned int rate_kHz)
{
    bool ret = true;
    if(false == pll_api_init_flg){
        return false;
    }

    cyg_drv_mutex_lock(&pll_lock);
    switch(rate_kHz)
    {
    case 40000:
    case 50000:
    case 62000:
    case 64000:
            pll3_ref_clk_khz = rate_kHz;
            break;
    default:
            pll3_ref_clk_khz = 64000;
            ret = false;
            break;
    }
    cyg_drv_mutex_unlock(&pll_lock);
    // diag_printf("set pll3 refclk to %u KHz\n", pll3_ref_clk_khz);

    return ret;
}

unsigned int pll_api_get_pll3_ref_KHz(void)
{
    return pll3_ref_clk_khz;
}
