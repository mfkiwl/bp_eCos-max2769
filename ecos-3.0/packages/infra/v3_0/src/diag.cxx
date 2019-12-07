/*========================================================================
//
//      diag.c
//
//      Infrastructure diagnostic output code
//
//========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002, 2004 Free Software Foundation, Inc.
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
//========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    nickg,gthomas,jlarmour
// Contributors: 
// Date:         1999-02-22
// Purpose:      Infrastructure diagnostic output
// Description:  Implementations of infrastructure diagnostic routines.
//
//####DESCRIPTIONEND####
//
//======================================================================*/

#include <pkgconf/system.h>
#include <pkgconf/hal.h>
#include <pkgconf/infra.h>

#include <cyg/infra/cyg_type.h>         // base types
  
#include <cyg/infra/diag.h>             // HAL polled output
#include <cyg/hal/hal_arch.h>           // architectural stuff for...
#include <cyg/hal/hal_intr.h>           // interrupt control
#include <cyg/hal/hal_diag.h>           // diagnostic output routines
#include <stdarg.h>
#include <limits.h>
  
#ifdef CYG_HAL_DIAG_LOCK_DATA_DEFN
CYG_HAL_DIAG_LOCK_DATA_DEFN;
#endif

#define is_digit(c) ((c >= '0') && (c <= '9'))

// wwzz add for float and double print
#ifdef CYGSEM_DIAG_PRINTF_FLOATING_POINT

# include <float.h>      // for DBL_DIG etc. below
# include <math.h>       // for modf()
# include <sys/ieeefp.h> // Cyg_libm_ieee_double_shape_type

# define MAXFRACT  DBL_DIG
# define MAXEXP    DBL_MAX_10_EXP

# define BUF             (MAXEXP+MAXFRACT+1)     /* + decimal point */
# define DEFPREC         6

static int
diag_cvt( double, int, int, char *, int, char *, char * );


/*
 * Flags used during conversion.
 */
#define ALT             0x001           /* alternate form */
#define HEXPREFIX       0x002           /* add 0x or 0X prefix */
#define LADJUST         0x004           /* left adjustment */
#define LONGDBL         0x008           /* long double; unimplemented */
#define LONGINT         0x010           /* long integer */
#define QUADINT         0x020           /* quad integer */
#define SHORTINT        0x040           /* short integer */
#define ZEROPAD         0x080           /* zero (as opposed to blank) pad */
#define FPT             0x100           /* Floating point number */
#define SIZET           0x200           /* size_t */

#endif  // wwzz add end
 

// wwzz add func for float support. start
#ifdef CYGSEM_DIAG_PRINTF_FLOATING_POINT
#define to_digit(c)     ((c) - '0')
//#define is_digit(c)     ((unsigned)to_digit(c) <= 9)
#define to_char(n)      ((n) + '0')

static char *
diag_round(double fract, int *exp, char *start, char *end, char ch, char *signp)
{
        double tmp;

        if (fract)
        (void)modf(fract * 10, &tmp);
        else
                tmp = to_digit(ch);
        if (tmp > 4)
                for (;; --end) {
                        if (*end == '.')
                                --end;
                        if (++*end <= '9')
                                break;
                        *end = '0';
                        if (end == start) {
                                if (exp) {      /* e/E; increment exponent */
                                        *end = '1';
                                        ++*exp;
                                }
                                else {          /* f; add extra digit */
                                *--end = '1';
                                --start;
                                }
                                break;
                        }
                }
        /* ``"%.3f", (double)-0.0004'' gives you a negative 0. */
        else if (*signp == '-')
                for (;; --end) {
                        if (*end == '.')
                                --end;
                        if (*end != '0')
                                break;
                        if (end == start)
                                *signp = 0;
                }
        return (start);
} // diag_round()


static char *
diag_exponent(char *p, int exp, int fmtch)
{
        char *t;
        char expbuf[MAXEXP];

        *p++ = fmtch;
        if (exp < 0) {
                exp = -exp;
                *p++ = '-';
        }
        else
                *p++ = '+';
        t = expbuf + MAXEXP;
        if (exp > 9) {
                do {
                        *--t = to_char(exp % 10);
                } while ((exp /= 10) > 9);
                *--t = to_char(exp);
                for (; t < expbuf + MAXEXP; *p++ = *t++);
        }
        else {
                *p++ = '0';
                *p++ = to_char(exp);
        }
        return (p);
} // diag_exponent()


static int
diag_cvt(double number, int prec, int flags, char *signp, int fmtch, char *startp,
    char *endp)
{
    Cyg_libm_ieee_double_shape_type ieeefp;
    char *t = startp;

    ieeefp.value = number;
    *signp = 0;
    if ( ieeefp.number.sign ){  // this checks for <0.0 and -0.0
        number = -number;
        *signp = '-';
    }

    if (finite(number)) {
        char *p;
        double fract;
        int dotrim, expcnt, gformat;
        double integer, tmp;

        dotrim = expcnt = gformat = 0;
        fract = modf(number, &integer);

        /* get an extra slot for rounding. */
        t = ++startp;

        /*
         * get integer portion of number; put into the end of the buffer; the
         * .01 is added for modf(356.0 / 10, &integer) returning .59999999...
         */
        for (p = endp - 1; integer; ++expcnt) {
                tmp = modf(integer / 10, &integer);
                *p-- = to_char((int)((tmp + .01) * 10));
        }
        switch (fmtch) {
        case 'f':
                /* reverse integer into beginning of buffer */
                if (expcnt)
                        for (; ++p < endp; *t++ = *p);
                else
                        *t++ = '0';
                /*
                 * if precision required or alternate flag set, add in a
                 * decimal point.
                 */
                if (prec || flags&ALT)
                        *t++ = '.';
                /* if requires more precision and some fraction left */
                if (fract) {
                        if (prec)
                                do {
                                        fract = modf(fract * 10, &tmp);
                                        *t++ = to_char((int)tmp);
                                } while (--prec && fract);
                        if (fract)
                                startp = diag_round(fract, (int *)NULL, startp,
                                    t - 1, (char)0, signp);
                }
                for (; prec--; *t++ = '0');
                break;
        case 'e':
        case 'E':
eformat:        if (expcnt) {
                        *t++ = *++p;
                        if (prec || flags&ALT)
                                *t++ = '.';
                        /* if requires more precision and some integer left */
                        for (; prec && ++p < endp; --prec)
                                *t++ = *p;
                        /*
                         * if done precision and more of the integer component,
                         * round using it; adjust fract so we don't re-round
                         * later.
                         */
                        if (!prec && ++p < endp) {
                                fract = 0;
                                startp = diag_round((double)0, &expcnt, startp,
                                    t - 1, *p, signp);
                        }
                        /* adjust expcnt for digit in front of decimal */
                        --expcnt;
                }
                /* until first fractional digit, decrement exponent */
                else if (fract) {
                        /* adjust expcnt for digit in front of decimal */
                        for (expcnt = -1;; --expcnt) {
                                fract = modf(fract * 10, &tmp);
                                if (tmp)
                                        break;
                        }
                        *t++ = to_char((int)tmp);
                        if (prec || flags&ALT)
                                *t++ = '.';
                }
                else {
                        *t++ = '0';
                        if (prec || flags&ALT)
                                *t++ = '.';
                }
                /* if requires more precision and some fraction left */
                if (fract) {
                        if (prec)
                                do {
                                        fract = modf(fract * 10, &tmp);
                                        *t++ = to_char((int)tmp);
                                } while (--prec && fract);
                        if (fract)
                                startp = diag_round(fract, &expcnt, startp,
                                    t - 1, (char)0, signp);
                }
                /* if requires more precision */
                for (; prec--; *t++ = '0');

                /* unless alternate flag, trim any g/G format trailing 0's */
                if (gformat && !(flags&ALT)) {
                        while (t > startp && *--t == '0');
                        if (*t == '.')
                                --t;
                        ++t;
                }
                t = diag_exponent(t, expcnt, fmtch);
                break;
        case 'g':
        case 'G':
                /* a precision of 0 is treated as a precision of 1. */
                if (!prec)
                        ++prec;
                /*
                 * ``The style used depends on the value converted; style e
                 * will be used only if the exponent resulting from the
                 * conversion is less than -4 or greater than the precision.''
                 *      -- ANSI X3J11
                 */
                if (expcnt > prec || (!expcnt && fract && fract < .0001)) {
                        /*
                         * g/G format counts "significant digits, not digits of
                         * precision; for the e/E format, this just causes an
                         * off-by-one problem, i.e. g/G considers the digit
                         * before the decimal point significant and e/E doesn't
                         * count it as precision.
                         */
                        --prec;
                        fmtch -= 2;             /* G->E, g->e */
                        gformat = 1;
                        goto eformat;
                }
                /*
                 * reverse integer into beginning of buffer,
                 * note, decrement precision
                 */
                if (expcnt)
                        for (; ++p < endp; *t++ = *p, --prec);
                else
                        *t++ = '0';
                /*
                 * if precision required or alternate flag set, add in a
                 * decimal point.  If no digits yet, add in leading 0.
                 */
                if (prec || flags&ALT) {
                        dotrim = 1;
                        *t++ = '.';
                }
                else
                        dotrim = 0;
                /* if requires more precision and some fraction left */
                if (fract) {
                        if (prec) {
                                do {
                                        fract = modf(fract * 10, &tmp);
                                        *t++ = to_char((int)tmp);
                                } while(!tmp);
                                while (--prec && fract) {
                                        fract = modf(fract * 10, &tmp);
                                        *t++ = to_char((int)tmp);
                                }
                        }
                        if (fract)
                                startp = diag_round(fract, (int *)NULL, startp,
                                    t - 1, (char)0, signp);
                }
                /* alternate format, adds 0's for precision, else trim 0's */
                if (flags&ALT)
                        for (; prec--; *t++ = '0');
                else if (dotrim) {
                        while (t > startp && *--t == '0');
                        if (*t != '.')
                                ++t;
                }
        }
    } else {
	unsigned case_adj;
	switch (fmtch) {
	case 'f':
	case 'g':
	case 'e':
	    case_adj = 'a' - 'A';
	    break;
	default:
	    case_adj = 0;
	}
	if (isnan(number)) {
	    *t++ = 'N' + case_adj;
	    *t++ = 'A' + case_adj;
	    *t++ = 'N' + case_adj;
	} else { // infinite
	    *t++ = 'I' + case_adj;
	    *t++ = 'N' + case_adj;
	    *t++ = 'F' + case_adj;
	}
    }
    return (t - startp);
} // diag_cvt()

#endif // ifdef CYGSEM_DIAG_PRINTF_FLOATING_POINT
// wwzz add func for float support. end


/*----------------------------------------------------------------------*/

externC void diag_write_num(
    cyg_uint32  n,              /* number to write              */
    cyg_ucount8 base,           /* radix to write to            */
    cyg_ucount8 sign,           /* sign, '-' if -ve, '+' if +ve */
    cyg_bool    pfzero,         /* prefix with zero ?           */
    cyg_ucount8 width           /* min width of number          */
    );

class Cyg_dummy_diag_init_class {
public:
    Cyg_dummy_diag_init_class() {
        HAL_DIAG_INIT();
    }
};

// Initialize after HAL.
static Cyg_dummy_diag_init_class cyg_dummy_diag_init_obj 
                                      CYGBLD_ATTRIB_INIT_AFTER(CYG_INIT_HAL);

/*----------------------------------------------------------------------*/
/* Write single char to output                                          */

externC void diag_write_char(char c)
{    
    /* Translate LF into CRLF */
    
    if( c == '\n' )
    {
        HAL_DIAG_WRITE_CHAR('\r');        
    }

    HAL_DIAG_WRITE_CHAR(c);
}

// Default wrapper function used by diag_printf
static void
_diag_write_char(char c, void **param)
{
    diag_write_char(c);
}

/*----------------------------------------------------------------------*/
/* Initialize. Call to pull in diag initializing constructor            */

externC void diag_init(void)
{
}

//
// This routine is used to send characters during 'printf()' functions.
// It can be replaced by providing a replacement via diag_init_putc().
//
static void (*_putc)(char c, void **param) = _diag_write_char;

void
diag_init_putc(void (*putc)(char c, void **param))
{
    _putc = putc;
}

/*----------------------------------------------------------------------*/
/* Write zero terminated string                                         */
  
externC void diag_write_string(const char *psz)
{
    while( *psz ) diag_write_char( *psz++ );
}

/*----------------------------------------------------------------------*/
/* Write decimal value                                                  */

externC void diag_write_dec( cyg_int32 n)
{
    cyg_ucount8 sign;

    if( n < 0 ) n = -n, sign = '-';
    else sign = '+';
    
    diag_write_num( n, 10, sign, false, 0);
}

/*----------------------------------------------------------------------*/
/* Write hexadecimal value                                              */

externC void diag_write_hex( cyg_uint32 n)
{
    diag_write_num( n, 16, '+', false, 0);
}    

/*----------------------------------------------------------------------*/
/* Generic number writing function                                      */
/* The parameters determine what radix is used, the signed-ness of the  */
/* number, its minimum width and whether it is zero or space filled on  */
/* the left.                                                            */

externC void diag_write_long_num(
    cyg_uint64  n,              /* number to write              */
    cyg_ucount8 base,           /* radix to write to            */
    cyg_ucount8 sign,           /* sign, '-' if -ve, '+' if +ve */
    cyg_bool    pfzero,         /* prefix with zero ?           */
    cyg_ucount8 width           /* min width of number          */
    )
{
    char buf[32];
    cyg_count8 bpos;
    char bufinit = pfzero?'0':' ';
    char *digits = (char *)"0123456789ABCDEF";

    /* init buffer to padding char: space or zero */
    for( bpos = 0; bpos < (cyg_count8)sizeof(buf); bpos++ ) buf[bpos] = bufinit;

    /* Set pos to start */
    bpos = 0;

    /* construct digits into buffer in reverse order */
    if( n == 0 ) buf[bpos++] = '0';
    else while( n != 0 )
    {
        cyg_ucount8 d = n % base;
        buf[bpos++] = digits[d];
        n /= base;
    }

    /* set pos to width if less. */
    if( (cyg_count8)width > bpos ) bpos = width;

    /* set sign if negative. */
    if( sign == '-' )
    {
        if( buf[bpos-1] == bufinit ) bpos--;
        buf[bpos] = sign;
    }
    else bpos--;

    /* Now write it out in correct order. */
    while( bpos >= 0 )
        diag_write_char(buf[bpos--]);
}

externC void diag_write_num(
    cyg_uint32  n,              /* number to write              */
    cyg_ucount8 base,           /* radix to write to            */
    cyg_ucount8 sign,           /* sign, '-' if -ve, '+' if +ve */
    cyg_bool    pfzero,         /* prefix with zero ?           */
    cyg_ucount8 width           /* min width of number          */
    )
{
    diag_write_long_num((long long)n, base, sign, pfzero, width);
}

/*----------------------------------------------------------------------*/
/* perform some simple sanity checks on a string to ensure that it      */
/* consists of printable characters and is of reasonable length.        */

static cyg_bool diag_check_string( const char *str )
{
    cyg_bool result = true;
    const char *s;

    if( str == NULL ) return false;
    
    for( s = str ; result && *s ; s++ )
    {
        char c = *s;

        /* Check for a reasonable length string. */
        
        if( s-str > 2048 ){
            diag_write_string("GT 2048\r\n");
            result = false;
        }

        /* We only really support CR, NL, tab and backspace at present.
	 * If we want to use other special chars, this test will
         * have to be expanded.  */

        if( c == '\n' || c == '\r' || c == '\b' || c == '\t' || c == '\033' )
            continue;

        /* Check for printable chars. This assumes ASCII */
        
        if( c < ' ' || c > '~' ){
            diag_write_string(">~ or <  \r\n");
            result = false;
        }

    }

    return result;
}

/*----------------------------------------------------------------------*/

static int
_cvt(unsigned long long val, char *buf, long radix, char *digits)
{
    char temp[80];
    char *cp = temp;
    int length = 0;

    if (val == 0) {
        /* Special case */
        *cp++ = '0';
    } else {
        while (val) {
            *cp++ = digits[val % radix];
            val /= radix;
        }
    }
    while (cp != temp) {
        *buf++ = *--cp;
        length++;
    }
    *buf = '\0';
    return (length);
}

//#define is_digit(c) ((c >= '0') && (c <= '9'))

static int
_vprintf(void (*putc)(char c, void **param), void **param, const char *fmt, va_list ap)
{
    char buf[sizeof(long long)*8];
    char c, sign, *cp=buf;
    int left_prec, right_prec, zero_fill, pad, pad_on_right, 
        i, islong, islonglong;
    long long val = 0;
    int res = 0, length = 0;

    // wwzz add for float support
        int size;               /* size of converted field or string */
#ifdef CYGSEM_DIAG_PRINTF_FLOATING_POINT
        int flags = 0;
        int prec = -1;
        char softsign;          /* temporary negative sign for floats */
        double _double;         /* double precision arguments %[eEfgG] */
        int fpprec = 0;             /* `extra' floating precision in [eEfgG] */
#endif

    if (!diag_check_string(fmt)) {
        diag_write_string("<Bad format string: ");
        diag_write_hex((cyg_uint32)fmt);
        diag_write_string(" :");
        for( i = 0; i < 8; i++ ) {
            diag_write_char(' ');
            val = va_arg(ap, unsigned long);
            diag_write_hex(val);
        }
        diag_write_string(">\n");
        return 0;
    }
    while ((c = *fmt++) != '\0') {
        if (c == '%') {
            c = *fmt++;
            left_prec = right_prec = pad_on_right = islong = islonglong = 0;
#ifdef CYGSEM_DIAG_PRINTF_FLOATING_POINT
            fpprec = DEFPREC;
#endif
            if (c == '-') {
                c = *fmt++;
                pad_on_right++;
            }
            if (c == '0') {
                zero_fill = true;
                c = *fmt++;
            } else {
                zero_fill = false;
            }
            while (is_digit(c)) {
                left_prec = (left_prec * 10) + (c - '0');
                c = *fmt++;
            }
            if (c == '.') {
                c = *fmt++;
                zero_fill++;
                while (is_digit(c)) {
                    right_prec = (right_prec * 10) + (c - '0');
                    c = *fmt++;
                }
#ifdef CYGSEM_DIAG_PRINTF_FLOATING_POINT
                fpprec = right_prec;
#endif
            } else {
                right_prec = left_prec;
            }
            sign = '\0';
            if (c == 'l') {
                // 'long' qualifier
                c = *fmt++;
		islong = 1;
                if (c == 'l') {
                    // long long qualifier
                    c = *fmt++;
                    islonglong = 1;
                }
            }
            if (c == 'z') {
                c = *fmt++;
		islong = sizeof(size_t) == sizeof(long);
            }
            // Fetch value [numeric descriptors only]
            switch (c) {
            case 'p':
		islong = 1;
            case 'd':
            case 'D':
            case 'x':
            case 'X':
            case 'u':
            case 'U':
            case 'b':
            case 'B':
                if (islonglong) {
                    val = va_arg(ap, long long);
                } else if (islong) {
                    val = (long long)va_arg(ap, long);
                } else{
                    val = (long long)va_arg(ap, int);
                }
                if ((c == 'd') || (c == 'D')) {
                    if (val < 0) {
                        sign = '-';
                        val = -val;
                    }
                } else {
                    // wwzz add for debug llu
                    if(islonglong){
                        // %llu %llx
                        break;
                    }
                    // Mask to unsigned, sized quantity
                    if (islong) {
                        val &= ((long long)1 << (sizeof(long) * 8)) - 1;
                    } else if (!islonglong) { // no need to mask longlong
                        val &= ((long long)1 << (sizeof(int) * 8)) - 1;
                    }
                }
                break;

                // wwzz add float support
#ifdef CYGSEM_DIAG_PRINTF_FLOATING_POINT
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
                //_double = va_arg(arg, double);
                _double = va_arg(ap, double);
                prec = fpprec+1; // wwzz fix precision lose 1 bug
                /*
                 * don't do unrealistic precision; just pad it with
                 * zeroes later, so buffer size stays rational.
                 */
                if (prec > MAXFRACT) {
                    //if ((ch != 'g' && ch != 'G') || (flags&ALT))
                    if ((c != 'g' && c != 'G') || (flags&ALT))
                        fpprec = prec - MAXFRACT;
                    prec = MAXFRACT;
                } else if (prec == -1)
                    prec = DEFPREC;
                /*
                 * diag_cvt may have to round up before the "start" of
                 * its buffer, i.e. ``intf("%.2f", (double)9.999);'';
                 * if the first character is still NUL, it did.
                 * softsign avoids negative 0 if _double < 0 but
                 * no significant digits will be shown.
                 */
                cp = buf;
                *cp = '\0';
                size = diag_cvt(_double, prec, flags, &softsign, c,
                        cp, buf + sizeof(buf));
                if (softsign)
                    sign = '-';
                if (*cp == '\0')
                    cp++;
                length = size;
                fpprec = fpprec;
                break;
#else
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
                // Output nothing at all
                //(void) va_arg(arg, double); // take off arg anyway
                (void) va_arg(ap, double); // take off arg anyway
                cp = "";
                size = 0;
                sign = '\0';
                size = size;
                break;
#endif // ifdef CYGSEM_DIAG_PRINTF_FLOATING_POINT



            default:
                break;
            }
            // Process output
            switch (c) {
            case 'p':  // Pointer
                (*putc)('0', param);
                (*putc)('x', param);
                zero_fill = true;
                left_prec = sizeof(unsigned long)*2;
                res += 2;  // Account for "0x" leadin
            case 'd':
            case 'D':
            case 'u':
            case 'U':
            case 'x':
            case 'X':
                switch (c) {
                case 'd':
                case 'D':
                case 'u':
                case 'U':
                    length = _cvt(val, buf, 10, (char *)"0123456789");
                    break;
                case 'p':
                case 'x':
                    length = _cvt(val, buf, 16, (char *)"0123456789abcdef");
                    break;
                case 'X':
                    length = _cvt(val, buf, 16, (char *)"0123456789ABCDEF");
                    break;
                }
                cp = buf;
                break;
            case 's':
            case 'S':
                cp = va_arg(ap, char *);
                if (cp == NULL) 
                    cp = (char *)"<null>";
                else if (!diag_check_string(cp)) {
                    diag_write_string("<Not a string: 0x");
                    diag_write_hex((cyg_uint32)cp);
                    cp = (char *)">";
                }
                length = 0;
                while (cp[length] != '\0') length++;
                break;
            case 'c':
            case 'C':
                c = va_arg(ap, int /*char*/);
                (*putc)(c, param);
                res++;
                continue;
            case 'b':
            case 'B':
                length = left_prec;
                if (left_prec == 0) {
                    if (islonglong)
                        length = sizeof(long long)*8;
                    else if (islong)
                        length = sizeof(long)*8;
                    else
                        length = sizeof(int)*8;
                }
                for (i = 0;  i < length-1;  i++) {
                    buf[i] = ((val & ((long long)1<<i)) ? '1' : '.');
                }
                cp = buf;
                break;
            case '%':
                (*putc)('%', param);
                res++;
                continue;

            // wwzz add float support
#ifdef CYGSEM_DIAG_PRINTF_FLOATING_POINT
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
                cp = buf;
                break;

#endif

            default:
                (*putc)('%', param);
                (*putc)(c, param);
                res += 2;
                continue;
            }
            pad = left_prec - length;
            if (sign != '\0') {
                pad--;
            }
            if (zero_fill) {
                c = '0';
                if (sign != '\0') {
                    (*putc)(sign, param);
                    res++;
                    sign = '\0';
                }
            } else {
                c = ' ';
            }
            if (!pad_on_right) {
                while (pad-- > 0) {
                    (*putc)(c, param);
                    res++;
                }
            }
            if (sign != '\0') {
                (*putc)(sign, param);
                res++;
            }
            while (length-- > 0) {
                c = *cp++;
                (*putc)(c, param);
                res++;
            }
            if (pad_on_right) {
                while (pad-- > 0) {
                    (*putc)(' ', param);
                    res++;
                }
            }
        } else {
            (*putc)(c, param);
            res++;
        }
    }
    return (res);
}

struct _sputc_info {
    char *ptr;
    int max, len;
};

static void 
_sputc(char c, void **param)
{
    struct _sputc_info *info = (struct _sputc_info *)param;

    if (info->len < info->max) {
        *(info->ptr)++ = c;
        *(info->ptr) = '\0';
        info->len++;
    }
}

int
diag_sprintf(char *buf, const char *fmt, ...)
{        
    va_list ap;
    struct _sputc_info info;

    va_start(ap, fmt);
    info.ptr = buf;
    info.max = 1024;  // Unlimited
    info.len = 0;
    _vprintf(_sputc, (void **)&info, fmt, ap);
    va_end(ap);
    return (info.len);
}

int
diag_snprintf(char *buf, size_t len, const char *fmt, ...)
{        
    va_list ap;
    struct _sputc_info info;

    va_start(ap, fmt);
    info.ptr = buf;
    info.len = 0;
    info.max = len-1;
    _vprintf(_sputc, (void **)&info, fmt, ap);
    va_end(ap);
    return (info.len);
}

int 
diag_vsprintf(char *buf, const char *fmt, va_list ap)
{
    struct _sputc_info info;

    info.ptr = buf;
    info.max = 1024;  // Unlimited
    info.len = 0;
    _vprintf(_sputc, (void **)&info, fmt, ap);
    return (info.len);
}

int
diag_printf(const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = _vprintf(_putc, (void **)0, fmt, ap);
    va_end(ap);
    return (ret);
}

int
diag_vprintf(const char *fmt, va_list ap)
{
    int ret;

    ret = _vprintf(_putc, (void **)0, fmt, ap);
    return (ret);
}

void
diag_vdump_buf_with_offset(__printf_fun *pf,
                           cyg_uint8     *p, 
                           CYG_ADDRWORD   s, 
                           cyg_uint8     *base)
{
    int i, c;
    if ((CYG_ADDRWORD)s > (CYG_ADDRWORD)p) {
        s = (CYG_ADDRWORD)s - (CYG_ADDRWORD)p;
    }
    while ((int)s > 0) {
        if (base) {
            (*pf)("%08X: ", (CYG_ADDRWORD)p - (CYG_ADDRWORD)base);
        } else {
            (*pf)("%08X: ", p);
        }
        for (i = 0;  i < 16;  i++) {
            if (i < (int)s) {
                (*pf)("%02X ", p[i] & 0xFF);
            } else {
                (*pf)("   ");
            }
	    if (i == 7) (*pf)(" ");
        }
        (*pf)(" |");
        for (i = 0;  i < 16;  i++) {
            if (i < (int)s) {
                c = p[i] & 0xFF;
                if ((c < 0x20) || (c >= 0x7F)) c = '.';
            } else {
                c = ' ';
            }
            (*pf)("%c", c);
        }
        (*pf)("|\n");
        s -= 16;
        p += 16;
    }
}

void
diag_dump_buf_with_offset(cyg_uint8     *p, 
                          CYG_ADDRWORD   s, 
                          cyg_uint8     *base)
{
    diag_vdump_buf_with_offset(diag_printf, p, s, base);
}

void
diag_dump_buf(void *p, CYG_ADDRWORD s)
{
   diag_dump_buf_with_offset((cyg_uint8 *)p, s, 0);
}

void
diag_dump_buf_with_offset_32bit(cyg_uint32   *p, 
				CYG_ADDRWORD  s, 
				cyg_uint32   *base)
{
    int i;
    if ((CYG_ADDRWORD)s > (CYG_ADDRWORD)p) {
        s = (CYG_ADDRWORD)s - (CYG_ADDRWORD)p;
    }
    while ((int)s > 0) {
        if (base) {
            diag_printf("%08X: ", (CYG_ADDRWORD)p - (CYG_ADDRWORD)base);
        } else {
            diag_printf("%08X: ", (CYG_ADDRWORD)p);
        }
        for (i = 0;  i < 4;  i++) {
            if (i < (int)s/4) {
                diag_printf("%08X ", p[i] );
            } else {
                diag_printf("         ");
            }
        }
        diag_printf("\n");
        s -= 16;
        p += 4;
    }
}

externC void
diag_dump_buf_32bit(void *p, CYG_ADDRWORD s)
{
   diag_dump_buf_with_offset_32bit((cyg_uint32 *)p, s, 0);
}

void
diag_dump_buf_with_offset_16bit(cyg_uint16   *p, 
				CYG_ADDRWORD  s, 
				cyg_uint16   *base)
{
    int i;
    if ((CYG_ADDRWORD)s > (CYG_ADDRWORD)p) {
        s = (CYG_ADDRWORD)s - (CYG_ADDRWORD)p;
    }
    while ((int)s > 0) {
        if (base) {
            diag_printf("%08X: ", (CYG_ADDRWORD)p - (CYG_ADDRWORD)base);
        } else {
            diag_printf("%08X: ", (CYG_ADDRWORD)p);
        }
        for (i = 0;  i < 8;  i++) {
            if (i < (int)s/2) {
	      diag_printf("%04X ", p[i] );
	      if (i == 3) diag_printf(" ");
            } else {
	      diag_printf("     ");
            }
        }
        diag_printf("\n");
        s -= 16;
        p += 8;
    }
}

externC void
diag_dump_buf_16bit(void *p, CYG_ADDRWORD s)
{
   diag_dump_buf_with_offset_16bit((cyg_uint16 *)p, s, 0);
}

/*-----------------------------------------------------------------------*/
/* EOF infra/diag.c */
