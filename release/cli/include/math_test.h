#ifndef __MATH_TEST_H__
#define __MATH_TEST_H__

/* The size of every test iterm is almost 108KB,
 * But no ddr BP2016_PSV board the max size of fw supported is 440KB,
 * So we need macro the test iterm that not used.
 *
 */


#define HG_TC_MATH_FABS_ENABLE

//#ifdef HG_TC_MATH_FABS_ENABLE
void fabs_test(void);
//#endif

#ifdef HG_TC_MATH_ACOS_ENABLE
void acos_test(void);
#endif

#ifdef HG_TC_MATH_ASIN_ENABLE
void asin_test(void);
#endif

#ifdef HG_TC_MATH_ATAN_ENABLE
void atan_test(void);
#endif

#ifdef HG_TC_MATH_ATAN2_ENABLE
void atan2_test(void);
#endif

#ifdef HG_TC_MATH_CEIL_ENABLE
void ceil_test(void);
#endif

#ifdef HG_TC_MATH_COS_ENABLE
void cos_test(void);
#endif

#ifdef HG_TC_MATH_COSH_ENABLE
void cosh_test(void);
#endif

#ifdef HG_TC_MATH_EXP_ENABLE
void exp_test(void);
#endif

#ifdef HG_TC_MATH_FLOOR_ENABLE
void floor_test(void);
#endif

#ifdef HG_TC_MATH_FMOD_ENABLE
void fmod_test(void);
#endif

#ifdef HG_TC_MATH_FREXP_ENABLE
void frexp_test(void);
#endif

#ifdef HG_TC_MATH_LDEXP_ENABLE
void ldexp_test(void);
#endif

#ifdef HG_TC_MATH_LOG_ENABLE
void log_test(void);
#endif

#ifdef HG_TC_MATH_LOG10_ENABLE
void log10_test(void);
#endif

#ifdef HG_TC_MATH_MODF_ENABLE
void modf_test(void);
#endif

#ifdef HG_TC_MATH_POW_ENABLE
void pow_test(void);
#endif

#ifdef HG_TC_MATH_SIN_ENABLE
void sin_test(void);
#endif

#ifdef HG_TC_MATH_SINH_ENABLE
void sinh_test(void);
#endif

#ifdef HG_TC_MATH_SQRT_ENABLE
void sqrt_test(void);
#endif

#ifdef HG_TC_MATH_TAN_ENABLE
void tan_test(void);
#endif

#ifdef HG_TC_MATH_TANH_ENABLE
void tanh_test(void);
#endif

#endif
