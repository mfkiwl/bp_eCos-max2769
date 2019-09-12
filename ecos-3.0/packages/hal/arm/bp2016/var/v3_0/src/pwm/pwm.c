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

/*
 *  en: 1: enable, 0: disable
 */
int pwm_set_enable(HWP_PWM_T *hwp_pwm, u32 en)
{
    u32 tmp = hwp_pwm->pwm_ctrl;

    if (en > 0) {
        tmp &= ~(PWM_ENABLE_MASK << PWM_ENABLE);
        tmp |= (PWM_ENABLE_MASK << PWM_ENABLE);
        hwp_pwm->pwm_ctrl = tmp;
    } else {
        tmp &= ~(PWM_ENABLE_MASK << PWM_ENABLE);
        hwp_pwm->pwm_ctrl = tmp;
    }

    return 0;
}

int pwm_config(HWP_PWM_T *hwp_pwm, u32 frequency, u32 polarity, u32 duty)
{
    u32 tmp = hwp_pwm->pwm_ctrl;
    u32 div = 0;

    if (polarity > 0) {
        tmp &= ~(PWM_POLARITY_MASK << PWM_POLARITY);
        tmp |= (PWM_POLARITY_MASK << PWM_POLARITY);
    } else {
        tmp &= ~(PWM_POLARITY_MASK << PWM_POLARITY);
    }

    if (frequency > 0) {    // 16bit
        tmp &= ~(PWM_FREQ_MASK << PWM_FREQ);
        tmp |= (PWM_FREQ_MASK << PWM_FREQ);
    } else {
        tmp &= ~(PWM_FREQ_MASK << PWM_FREQ);
    }

    switch (frequency) {
    case 0: //14bit
        div = PWM_14BIT_MASK * duty / 100;
        tmp &= ~(PWM_DUTY_MASK);
        tmp |= div;
        break;
    case 1: //16bit
        div = PWM_16BIT_MASK * duty / 100;
        tmp &= ~(PWM_DUTY_MASK);
        tmp |= div;
        break;
    default:
        return -1;
    }

    hwp_pwm->pwm_ctrl = tmp;

    return 0;
}

