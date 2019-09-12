#ifndef __REG_PWM_H__
#define __REG_PWM_H__

#define REG_PWM_BASE (APB2_BASE + 0x80000)

typedef volatile struct {
    REG32 pwm_ctrl;         /* 0x00000000*/
} HWP_PWM_T;

#define hwp_pwm0 ((HWP_PWM_T *) (REG_PWM_BASE))
#define hwp_pwm1 ((HWP_PWM_T *) (REG_PWM_BASE + 4))

// ctrl
#define PWM_ENABLE              28   /*PWM ENABLE*/
#define PWM_ENABLE_MASK         1    /*PWM ENABLE MASK*/

#define PWM_FREQ                24   /*PWM BIT WIDTH*/
#define PWM_FREQ_MASK           1    /*PWM FREQENCY MASK*/

#define PWM_POLARITY            20   /*PWM polarity*/
#define PWM_POLARITY_MASK       1    /*PWM polarity*/

#define PWM_DUTY_MASK           ((1 << 20) - 1)
#define PWM_16BIT_MASK          ((1 << 16) - 1)
#define PWM_14BIT_MASK          ((1 << 14) - 1)

#endif /* __REG_PWM_H__ */

