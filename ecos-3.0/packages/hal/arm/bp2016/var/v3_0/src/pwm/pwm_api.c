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
#include <cyg/hal/regs/pwm.h>
#include <cyg/hal/api/pwm_api.h>

static cyg_drv_mutex_t     pwm_lock[PWM_TOTAL_NUM];
static int pwm_init_flag = 0;

extern int pwm_set_enable(HWP_PWM_T *hwp_pwm, u32 en);
extern int pwm_config(HWP_PWM_T *hwp_pwm, u32 frequency, u32 polarity, u32 duty);

int pwm_api_init(void)
{
    if (pwm_init_flag == 0) {
        cyg_drv_mutex_init(&pwm_lock[PWM_INDEX_0]);
        cyg_drv_mutex_init(&pwm_lock[PWM_INDEX_1]);
        pwm_init_flag = 1;
    }

    return PWM_RET_OK;
}

int pwm_api_set_enable(int index)
{
    HWP_PWM_T *hwp_pwm;

    switch (index) {
    case 0:
        hwp_pwm = hwp_pwm0;
        break;
    case 1:
        hwp_pwm = hwp_pwm1;
        break;
    default:
        return PWM_INDEX_INVALID;
        break;
    }

    if (pwm_init_flag == 0) {
        return PWM_NOT_INIT;
    }

    cyg_drv_mutex_lock(&pwm_lock[index]);
    pwm_set_enable(hwp_pwm, 1);
    cyg_drv_mutex_unlock(&pwm_lock[index]);

    return PWM_RET_OK;
}

int pwm_api_set_disable(int index)
{
    HWP_PWM_T *hwp_pwm;

    switch (index) {
    case 0:
        hwp_pwm = hwp_pwm0;
        break;
    case 1:
        hwp_pwm = hwp_pwm1;
        break;
    default:
        return PWM_INDEX_INVALID;
        break;
    }

    if (pwm_init_flag == 0) {
        return PWM_NOT_INIT;
    }

    cyg_drv_mutex_lock(&pwm_lock[index]);
    pwm_set_enable(hwp_pwm, 0);
    cyg_drv_mutex_unlock(&pwm_lock[index]);

    return PWM_RET_OK;
}

int pwm_api_config(int index, u32 frequency, u32 polarity, u32 duty)
{
    HWP_PWM_T *hwp_pwm;

    switch (index) {
    case 0:
        hwp_pwm = hwp_pwm0;
        break;
    case 1:
        hwp_pwm = hwp_pwm1;
        break;
    default:
        return PWM_INDEX_INVALID;
        break;
    }

    if (pwm_init_flag == 0) {
        return PWM_NOT_INIT;
    }

    if (frequency > PWM_BAUD_8KHZ) {
        return PWM_BAUD_INVALID;
    }

    if (polarity > PWM_POLARITY_POSITIVE) {
        return PWM_POLAR_INVALID;
    }

    if (duty > MAX_DUTY) {
        return PWM_DUTY_INVALID;
    }

    cyg_drv_mutex_lock(&pwm_lock[index]);
    pwm_config(hwp_pwm, frequency, polarity, duty);
    cyg_drv_mutex_unlock(&pwm_lock[index]);

    return PWM_RET_OK;
}

