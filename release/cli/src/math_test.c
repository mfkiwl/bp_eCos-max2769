#include <cyg/infra/cyg_type.h>    // Common type definitions and support
#include <cyg/infra/testcase.h>    // Test infrastructure
#include <math.h>                  // Header for this package
#include <sys/ieeefp.h>            // Cyg_libm_ieee_double_shape_type
#include "vectors/vector_support.h"// extra support for math tests

#include "math_test.h"

//#ifdef HG_TC_MATH_SQRT_ENABLE
#include "vectors/fabs.h"
// FUNCTIONS

static void
fabs_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(fabs_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &fabs, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &fabs_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("fabs() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("fabs() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library fabs() function");
} // test()


void fabs_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "fabs() function");

    START_TEST( fabs_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()

//#endif


#ifdef HG_TC_MATH_SQRT_ENABLE

#include "vectors/acos.h"

// FUNCTIONS

static void
acos_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(acos_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &acos, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &acos_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("acos() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("acos() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library acos() function");
} // test()


void acos_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "acos() function");

    START_TEST( acos_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

}


#endif


#ifdef HG_TC_MATH_SQRT_ENABLE

#include "vectors/asin.h"

// FUNCTIONS

static void
asin_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(asin_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &asin, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &asin_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("asin() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("asin() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library asin() function");
} // test()

void asin_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "asin() function");

    START_TEST( asin_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

}

#endif


#ifdef HG_TC_MATH_SQRT_ENABLE

#include "vectors/atan.h"

// FUNCTIONS

static void
atan_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(atan_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &atan, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &atan_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("atan() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("atan() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library atan() function");
} // test()


void atan_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "atan() function");

    START_TEST( atan_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

}

#endif


#ifdef HG_TC_MATH_SQRT_ENABLE

#include "vectors/atan2.h"

// FUNCTIONS

static void
atan2_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(atan2_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &atan2, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_DOUBLE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &atan2_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("atan2() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("atan2() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library atan2() function");
} // test()


void atan2_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "atan2() function");

    START_TEST( atan2_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()

#endif


#ifdef HG_TC_MATH_SQRT_ENABLE

#include "vectors/ceil.h"

// FUNCTIONS

static void
ceil_vec_test( CYG_ADDRWORD data )
{    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(ceil_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &ceil, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &ceil_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("ceil() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("ceil() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library ceil() function");
} // test()

void ceil_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "ceil() function");

    START_TEST( ceil_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()

#endif


#ifdef HG_TC_MATH_SQRT_ENABLE

#include "vectors/cos.h"

// FUNCTIONS

static void
cos_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(cos_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &cos, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &cos_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("cos() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("cos() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library cos() function");
} // test()


void cos_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "cos() function");

    START_TEST( cos_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()

#endif


#ifdef HG_TC_MATH_SQRT_ENABLE


#include "vectors/cosh.h"

// FUNCTIONS

static void
cosh_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(cosh_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &cosh, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &cosh_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("cosh() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("cosh() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library cosh() function");
} // test()

#endif


#ifdef HG_TC_MATH_SQRT_ENABLE


void cosh_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "cosh() function");

    START_TEST( cosh_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_EXP_ENABLE

#include "vectors/exp.h"

static void
exp_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(exp_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &exp, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &exp_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("exp() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("exp() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library exp() function");
} // test()


void exp_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "exp() function");

    START_TEST( exp_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_FLOOR_ENABLE

#include "vectors/floor.h"

static void
floor_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(floor_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &floor, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &floor_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("floor() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("floor() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library floor() function");
} // test()


void floor_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "floor() function");

    START_TEST( floor_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_FMOD_ENABLE

#include "vectors/fmod.h"

static void
fmod_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(fmod_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &fmod, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_DOUBLE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &fmod_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("fmod() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("fmod() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library fmod() function");
} // test()

void fmod_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "fmod() function");

    START_TEST( fmod_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif


#ifdef HG_TC_MATH_FREXP_ENABLE

#include "vectors/frexp.h"

static void
frexp_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(frexp_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &frexp, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_INT_P, CYG_LIBM_TEST_VEC_DOUBLE,
                     &frexp_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("frexp() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("frexp() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library frexp() function");
} // test()

void frexp_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "frexp() function");

    START_TEST( frexp_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_LDEXP_ENABLE

#include "vectors/ldexp.h"

static void
ldexp_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(ldexp_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &ldexp, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_INT, CYG_LIBM_TEST_VEC_DOUBLE,
                     &ldexp_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("ldexp() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("ldexp() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library ldexp() function");
} // test()

void ldexp_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "ldexp() function");

    START_TEST( ldexp_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_LOG_ENABLE

#include "vectors/log.h"

static void
log_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(log_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &log, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &log_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("log() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("log() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library log() function");
} // test()

void log_test()
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "log() function");

    START_TEST( log_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_LOG10_ENABLE

#include "vectors/log10.h"

static void
log10_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(log10_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &log10, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &log10_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("log10() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("log10() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library log10() function");
} // test()

void log10_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "log10() function");

    START_TEST( log10_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_MODF_ENABLE

#include "vectors/modf.h"

static void
modf_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(modf_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &modf, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_DOUBLE_P, CYG_LIBM_TEST_VEC_DOUBLE,
                     &modf_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("modf() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("modf() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library modf() function");
} // test()

void modf_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "modf() function");

    START_TEST( modf_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_POW_ENABLE

#include "vectors/pow.h"

static void
pow_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(pow_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &pow, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_DOUBLE,
                     &pow_vec[0], vec_size );
    
    if (ret==true) {
        CYG_TEST_PASS("pow() is stable");
    } // if
    else {
        CYG_TEST_FAIL("pow() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library pow() function");
} // test()

void pow_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "pow() function");

    START_TEST( pow_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_SIN_ENABLE

#include "vectors/sin.h"

static void
sin_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(sin_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &sin, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &sin_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("sin() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("sin() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library sin() function");
} // test()

void sin_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "sin() function");

    START_TEST( sin_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_SINH_ENABLE

#include "vectors/sinh.h"

static void
sinh_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(sinh_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &sinh, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &sinh_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("sinh() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("sinh() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library sinh() function");
} // test()

void sinh_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
            "sinh() function");

    START_TEST( sinh_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_SQRT_ENABLE

#include "vectors/sqrt.h"
// FUNCTIONS

static void
sqrt_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(sqrt_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &sqrt, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &sqrt_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("sqrt() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("sqrt() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library sqrt() function");
} // test()

void sqrt_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "sqrt() function");

    START_TEST( sqrt_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_TAN_ENABLE

#include "vectors/tan.h"

// FUNCTIONS

static void
tan_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(tan_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &tan, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &tan_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("tan() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("tan() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library tan() function");
} // test()

void tan_test(void)
{
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "tan() function");

    START_TEST( tan_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

#ifdef HG_TC_MATH_TANH_ENABLE

#include "vectors/tanh.h"

static void
tanh_vec_test( CYG_ADDRWORD data )
{
    cyg_ucount32 vec_size;
    cyg_bool ret;

    vec_size = sizeof(tanh_vec) / sizeof(Cyg_libm_test_double_vec_t);
    ret = doTestVec( (CYG_ADDRWORD) &tanh, CYG_LIBM_TEST_VEC_DOUBLE,
                     CYG_LIBM_TEST_VEC_NONE, CYG_LIBM_TEST_VEC_DOUBLE,
                     &tanh_vec[0], vec_size );

    if (ret==true)
    {
        CYG_TEST_PASS("tanh() is stable");
    } // if
    else
    {
        CYG_TEST_FAIL("tanh() failed tests");
    } // else

    CYG_TEST_FINISH("Finished tests from testcase " __FILE__ " for Math "
                    "library tanh() function");
} // test()


void tanh_test(void){
    CYG_TEST_INIT();

    CYG_TEST_INFO("Starting tests from testcase " __FILE__ " for Math library "
                  "tanh() function");

    START_TEST( tanh_vec_test );

    CYG_TEST_PASS_FINISH("Testing is not applicable to this configuration");

} // main()
#endif

