//==========================================================================
//
//        bp2016_pwm_cli.c
//
//        pwm cli test case for bp2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2019-05-10
// Description:   pwm cli test for bp2016 oem
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <stdio.h>
#include "common.h"

#include <cyg/hal/api/pwm_api.h>

#define readl_relaxed(c)    (*(volatile u32 *) (c))

#define REG_PWM_BASE 0xc0180000

int do_pwm_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int res = 0;
    u32 index = 0;
    static u32 freq = 0;    //14bit 8KHz
    static u32 polar = 0;  //negative
    static u32 duty = 0;
    u32 tmp = 0;

    printf("\n-------------pwm testing in --------------------\n");

    if (argc > 2) {
        index = simple_strtoul(argv[2], NULL, 10);
        if (argc > 3) {
            tmp = simple_strtoul(argv[3], NULL, 10);
        }
    }
    printf("\nindex = %d\n", index);

    if (strcmp(argv[1], "init") == 0) {
        res = pwm_api_init();
        if (res != 0) {
            printf("\nerror\n");
            return -1;
        }
        printf("\npwm init\n");
    }

    if (strcmp(argv[1], "enable") == 0) {
        res = pwm_api_set_enable(index);
        if (res != 0) {
            printf("\nenable error = 0x%x\n", res);
            return -1;
        }
        printf("\npwm enable\n");
    }

    if (strcmp(argv[1], "disable") == 0) {
        res = pwm_api_set_disable(index);
        if (res != 0) {
            printf("\ndisable error= 0x%x\n", res);
            return -1;
        }
        printf("\npwm disable\n");
    }

    if (strcmp(argv[1], "config") == 0) {
        polar = PWM_POLARITY_POSITIVE;
        duty = 50;
        freq = PWM_BAUD_8KHZ;
        res = pwm_api_config(index, freq, polar, duty);
        if (res != 0) {
            printf("\nconfig error= 0x%x\n", res);
            return -1;
        }
    }

    if (strcmp(argv[1], "duty") == 0) {
        duty = tmp;
        res = pwm_api_config(index, freq, polar, duty);
        if (res != 0) {
            printf("\nduty error= 0x%x\n", res);
            return -1;
        }
        printf("\npwm set duty = %d\n", duty);
    }

    if (strcmp(argv[1], "freq") == 0) {
        freq = tmp;
        res = pwm_api_config(index, freq, polar, duty);
        if (res != 0) {
            printf("\nfreq error = 0x%x\n", res);
            return -1;
        }
        printf("\npwm set freq = %d\n", freq);
    }

    if (strcmp(argv[1], "polar") == 0) {
        polar = tmp;
        res = pwm_api_config(index, freq, polar, duty);
        if (res != 0) {
            printf("\npolar error = 0x%x\n", res);
            return -1;
        }
    }

    tmp = readl_relaxed(REG_PWM_BASE);
    printf("\nread back ctrl0 = 0x%08x\n", tmp);
    tmp = readl_relaxed(REG_PWM_BASE + 4);
    printf("\nread back ctrl1 = 0x%08x\n", tmp);

    return res;
}

