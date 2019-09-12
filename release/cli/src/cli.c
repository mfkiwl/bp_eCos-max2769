#include <cyg/kernel/kapi.h>
#include "common.h"
#include <cyg/hal/api/uart_api.h>
#include <cyg/hal/misc/misc.h>

static DYN_SHELL_ITEM_T *p_root_shell;
static DYN_SHELL_ITEM_T *p_common_shell;

#define     DEBUG_UART_PORT     6

/* now declare (and allocate space for) some kernel objects,
   like the two threads we will use */
cyg_thread thread_s[2];		/* space for two thread objects */

char stack[4096*2];		/* space for two 4K stacks */

/* now the handles for the threads */
cyg_handle_t cli_thread;

/* and now variables for the procedure which is the thread */
cyg_thread_entry_t cli_program;

/* we install our own startup routine which sets up threads */
void cyg_user_start(void)
{
    show_release_message();
  cyg_thread_create(0, cli_program, (cyg_addrword_t) 0,
		    "Thread A", (void *) stack, 4096 *2,
		    &cli_thread, &thread_s[0]);

  cyg_thread_resume(cli_thread);
}

#ifdef CYGPKG_IO_SERIAL_ARM_BP2016
char uart_getc(void)
{
    char c;
    unsigned int len = 1;
    UartRead(DEBUG_UART_PORT, &c, &len);
    return c;
}

int uart_putc(char c)
{
    unsigned int len = 1;
    return UartWrite(DEBUG_UART_PORT, &c, &len);
}

int uart_puts(char * info)
{
    unsigned int len = strlen(info);
    return UartWrite(DEBUG_UART_PORT, info, &len);
}
#else

#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/regs/uart.h>
extern HWP_UART_T *serial_hwp_get(U32 port_id);
extern int cyg_hal_plf_serial_getc(HWP_UART_T *hwp_uart);
extern void cyg_hal_plf_serial_putc(HWP_UART_T *hwp_uart, const char c);
extern void cyg_hal_plf_serial_write(HWP_UART_T *hwp_uart, const cyg_uint8* __buf, cyg_uint32 __len);
extern void cyg_hal_plf_serial_init_channel(HWP_UART_T *hwp_uart, int baudrate);

char uart_getc(void)
{
    HWP_UART_T *hw = serial_hwp_get(DEBUG_UART_PORT);
    char c;
    c = cyg_hal_plf_serial_getc(hw);
    return c;
}

int uart_putc(char c)
{
    HWP_UART_T *hw = serial_hwp_get(DEBUG_UART_PORT);
    cyg_hal_plf_serial_putc(hw, (const char)c);
    return 1;
}

int uart_puts(char * info)
{
    HWP_UART_T *hw = serial_hwp_get(DEBUG_UART_PORT);
    unsigned int len = strlen(info);
    cyg_hal_plf_serial_write(hw, (const unsigned char *)info, len);
    return len;
}
#endif

int DVT(void)
{
    p_root_shell = NULL;
    p_common_shell = NULL;

    dyn_shell_init(&p_root_shell, (unsigned char *) "DVT");
    dyn_shell_init(&p_common_shell, (unsigned char *) "");

    printf("shell init end\n");
    register_common_cmd(p_common_shell);

    dyn_shell_load(p_root_shell, p_common_shell);
    printf("DVT test done\r\n");
    return 0;
}


extern void show_all_thread_infor(void);
/* this is a simple program which runs in a thread */
void cli_program(cyg_addrword_t data)
{
#ifdef CYGPKG_IO_SERIAL_ARM_BP2016
    UartOpen(DEBUG_UART_PORT);
#else
    HWP_UART_T *hw = serial_hwp_get(DEBUG_UART_PORT);
    cyg_hal_plf_serial_init_channel(hw, 115200);
#endif
    show_all_thread_infor();
    DVT();
    while(1);
}
