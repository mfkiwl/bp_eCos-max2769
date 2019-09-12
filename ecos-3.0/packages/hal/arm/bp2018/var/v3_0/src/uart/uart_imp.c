#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_diag.h>

#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // necessary?
#include <cyg/hal/regs/uart.h>

#define LCRVAL LCR_8N1					/* 8 data, 1 stop, no parity */
#define FCRVAL (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)	/* Clear & enable FIFOs */

void cyg_hal_plf_serial_init_channel(HWP_UART_T *hwp_uart, int baudrate)
{
    cyg_uint32 baud_divisor;
	
	baud_divisor = CONFIG_APB_CLOCK / 16 / baudrate;	

	if(CONFIG_APB_CLOCK > (baud_divisor * 16 * baudrate))
		baud_divisor +=((CONFIG_APB_CLOCK/16 - baud_divisor * baudrate)*2 > baudrate) ? 1 : 0;


	hwp_uart->dlh_ier = 0x00;	//disable all interrupts
	hwp_uart->lcr = LCR_BKSE | LCRVAL;
	hwp_uart->rbr_thr_dll = baud_divisor & 0xff;
	hwp_uart->dlh_ier = (baud_divisor >> 8) & 0xff;
	hwp_uart->lcr = LCRVAL;		//data 8, stop 1, parity disable, break disable
	hwp_uart->iir_fcr = FCRVAL;
}

/* 
 * For lsr_dr: Indicate that the rx contains at least one char in RBR or the rx fifo 
 * and when the RBR is read in non-Fifo Mode, or the rx Fifo is empty, lsr:dr is cleared 
*/
int cyg_hal_plf_serial_getc(HWP_UART_T *hwp_uart)
{
	while ((hwp_uart->lsr & LSR_DR) == 0);
	return (hwp_uart->rbr_thr_dll);
}

cyg_bool
cyg_hal_plf_serial_getc_timeout(HWP_UART_T *hwp_uart, cyg_uint8* ch)
{
    int delay_count;
    delay_count = 1000; // delay in .1 ms steps

    for(;;) {
	if (0 == delay_count--) {
	    return false;
	}
	if((hwp_uart->lsr & LSR_DR) == 0) {
        	CYGACC_CALL_IF_DELAY_US(100);
		continue;
	}
	*ch = (hwp_uart->rbr_thr_dll);
        break;
    }

    return true;
}


/*
 * For lsr_thre: If THRE_MODE_USE=Disabled or THRE interrrupt is disabled (IER[7]=0),
 * this bit indicate the THR Or tx fifo is empty 
 * otherwize indicate the THRE interrupt	
 */
void cyg_hal_plf_serial_putc(HWP_UART_T *hwp_uart, const char c)
{
	/*when THR or TX FIFO is non-empty, wait..*/
	while ((hwp_uart->lsr & LSR_THRE) == 0); 
	hwp_uart->rbr_thr_dll = c;
}

void
cyg_hal_plf_serial_write(HWP_UART_T *hwp_uart, const cyg_uint8* __buf, 
                         cyg_uint32 __len)
{
    while(__len-- > 0)
        cyg_hal_plf_serial_putc(hwp_uart, *__buf++);

}

void
cyg_hal_plf_serial_read(HWP_UART_T *hwp_uart,  cyg_uint8* __buf, cyg_uint32 __len)
{
    while(__len-- > 0)
        *__buf++ = cyg_hal_plf_serial_getc(hwp_uart);
}

int
cyg_hal_plf_serial_control(HWP_UART_T *hwp_uart, __comm_control_cmd_t __func, ...)
{
	return 0;	
}

int
cyg_hal_plf_serial_isr(void *__ch_data, int* __ctrlc, 
                       CYG_ADDRWORD __vector, CYG_ADDRWORD __data)
{
	return 0;
}
#if 0
void _serial_putc(const char c)
{
	cyg_hal_plf_serial_putc(hwp_apUart, c);
}

void _serial_puts(const char *s)
{
	while (*s) 
		_serial_putc(*s++);
}


static char *flex_i2a(unsigned num, char *pstr, int radix, int len, int digits)
{
	char ch;
	char *p;
	int sign = 0;

	if (radix < 0) {
		radix = -radix;
		sign = 1;
	}

	// limit the string and number of displayed digits
	if (digits > len-1) digits = len-1;
	p = pstr + len;
	*--p = 0;

	// print the number including sign
	while (pstr != p)
	{
		unsigned int n = num/radix;
		ch = num - n*radix;
		num = n;
		ch += (ch < 10)? '0': 'A'-10;
		*--p = ch;
		if (num == 0) break;
	}

	// output the sign
	if (sign && pstr != p) {
		*--p = '-';
	}

	if (digits == 0)
		return p;

	ch = (radix == 16)? '0': ' ';
	len = digits - (pstr+len-p-1); // target_digits - current_chars
	for (; len > 0; len--)
		*--p = ch;

	return p;
}


 /*
 rprintf() is a tiny version of printf() for use in the boot_rom.
 The format-string is very limited subset of printf():
 	%<size><format>
 where <size> is a single digit size specifier between 0 and 9. <size> is
 accepted for all <formats> but ignored for strings.

 The following <format> codes is supported:
    s    - string
    d    - decimal number, signed
    u    - decimal number, unsigned
    x    - hexadecimal number, unsigned
    %    - literal '%'

 Examples:
        rprintf("The answer to life, universe and %s is %d!\n", "everything", 42);
        rprintf("%d is %u in unsigned (hex: 0x%8x)\n", -42, -42, -42);
    Output:
        "The answer to life, universe and everything is 42!"
        "-42 is 4294967254 in unsigned (hex: 0xFFFFFFD6)"

*/

void rprintf(const char *fmt, ...)
{
	va_list ap;

	char str[16];
	int iarg;

	va_start(ap, fmt);

	const char *s = fmt;
	while(*s) {
		if (*s == '%')
		{
			int size = 0;
			int radix = 10;
			const char *p = 0;

			++s;
			if (*s == '0') s++;
			if (*s >= '0' && *s <= '9') { size = *s++ - '0'; };

			switch(*s) {
			case 's':
				p = va_arg(ap, const char *);
				break;
			case 'x':
				radix = 16;
			case 'd':
			case 'u':
				iarg = va_arg(ap, int);
				if (*s == 'd' && iarg < 0) {
					radix = -10;
					iarg = -iarg;
				}
				p = flex_i2a(iarg, str, radix, sizeof(str), size);
				break;
			case '%':
				_serial_putc('%');
				break;
			case 'c':
				iarg = va_arg(ap, int);
				str[0] = iarg;
				str[1] = 0;
				p = str;
			default: ;
			}

			if (p) _serial_puts(p);
		}
		else {
			_serial_putc(*s);
		}
		++s;
	}

	va_end(ap);
}
#endif 

HWP_UART_T *serial_hwp_get(U32 port_id)
{
    HWP_UART_T *hw_uart = NULL;

    switch (port_id) {
    case 0:
        hw_uart = hwp_apUart0; break;
    case 1:
        hw_uart = hwp_apUart1; break;
    case 2:
        hw_uart = hwp_apUart2; break;
    case 3:
        hw_uart = hwp_apUart3; break;
    case 4:
         break;
    case 5:
         break;
    case 6:
        hw_uart = hwp_apUart6; break;
    case 7:
        hw_uart = hwp_apUart7; break;
    default:
        break;
    }
    return hw_uart;
}
