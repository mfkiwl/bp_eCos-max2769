#ifndef __PWM_API_H__
#define __PWM_API_H__

enum pwm_index_info {
    PWM_INDEX_0 = 0,
    PWM_INDEX_1,
    PWM_TOTAL_NUM
};

enum pwm_freq_info {
    PWM_BAUD_34KHZ = 0,     //14bit
    PWM_BAUD_8KHZ = 1,      //16bit
};

enum pwm_polarity_info {
    PWM_POLARITY_NEGATIVE = 0,      //negative pulse
    PWM_POLARITY_POSITIVE = 1,      //positive pulse
};

enum pwm_error_code {
    PWM_RET_OK = 0,
    PWM_NOT_INIT = -1,
    PWM_INDEX_INVALID = -2,
    PWM_BAUD_INVALID = -3,
    PWM_POLAR_INVALID = -4,
    PWM_DUTY_INVALID = -5
};

#define MAX_DUTY    100
/*
 * init mutex, this must be call before pwm api interface
 *
 * return: 0: ok, others: error
 */
int pwm_api_init(void);

/*
 * enable pwm controller
 * index: pwm index
 *
 * return: 0: ok, others: error
 */
int pwm_api_set_enable(int index);

/*
 * disable pwm controller
 * index: pwm index
 *
 * return: 0: ok, others: error
 */
int pwm_api_set_disable(int index);

/*
 * setting pwm configs
 * index: pwm index, bp2016 is 0 and 1 valid
 * frequency: enum 0 and 1, means 8KHZ and 34KHZ
 * polarity: positive and negative
 * duty: duty cycle, must be less than 100
 *
 * return: 0: ok, others: error
 */
int pwm_api_config(int index, u32 frequency, u32 polarity, u32 duty);

#endif /* __PWM_API_H__ */

