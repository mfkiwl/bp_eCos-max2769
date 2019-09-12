#include <pkgconf/hal.h>

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>
#include <cyg/hal/drv_api.h>            // CYG_ISR_HANDLED
#include <cyg/hal/hal_diag.h>

#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // necessary?
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/hal/regs/uart.h>

extern void cyg_hal_plf_serial_init_channel(HWP_UART_T *, int);
extern int cyg_hal_plf_serial_getc(HWP_UART_T *);
extern void cyg_hal_plf_serial_putc(HWP_UART_T *, const char);
extern void cyg_hal_plf_serial_write(HWP_UART_T*, const cyg_uint8*, cyg_uint32); 
extern void cyg_hal_plf_serial_read(HWP_UART_T *,  cyg_uint8*, cyg_uint32);
extern int cyg_hal_plf_serial_isr(void *, int* , CYG_ADDRWORD , CYG_ADDRWORD);
extern cyg_bool cyg_hal_plf_serial_getc_timeout(HWP_UART_T *, cyg_uint8* );
extern int cyg_hal_plf_serial_control(HWP_UART_T *, __comm_control_cmd_t __func, ...);
extern HWP_UART_T *serial_hwp_get(U32 port_id);


#if CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL==0
#define CYG_DEV_CONSOLE_BASE    hwp_apUart0
#elif CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL==1
#define CYG_DEV_CONSOLE_BASE    hwp_apUart1
#elif CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL==2
#define CYG_DEV_CONSOLE_BASE    hwp_apUart2
#elif CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL==3
#define CYG_DEV_CONSOLE_BASE    hwp_apUart3
#elif CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL==4
#define CYG_DEV_CONSOLE_BASE    hwp_apUart4
#elif CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL==5
#define CYG_DEV_CONSOLE_BASE    hwp_apUart5
#elif CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL==6
#define CYG_DEV_CONSOLE_BASE    hwp_apUart6
#elif CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL==7
#define CYG_DEV_CONSOLE_BASE    hwp_apUart7
#else
#define CYG_DEV_CONSOLE_BASE    hwp_apUart0
#endif
 
static void cyg_hal_plf_serial_init(void)
{
    hal_virtual_comm_table_t* comm;
    int cur;

    cur = CYGACC_CALL_IF_SET_CONSOLE_COMM(CYGNUM_CALL_IF_SET_COMM_ID_QUERY_CURRENT);

    // Init channels
    cyg_hal_plf_serial_init_channel(CYG_DEV_CONSOLE_BASE, CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL_BAUD);

    // Setup procs in the vector table
    CYGACC_CALL_IF_SET_CONSOLE_COMM(CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL);
    comm = CYGACC_CALL_IF_CONSOLE_PROCS();
    CYGACC_COMM_IF_CH_DATA_SET(*comm, CYG_DEV_CONSOLE_BASE);
    CYGACC_COMM_IF_WRITE_SET(*comm, cyg_hal_plf_serial_write);
    CYGACC_COMM_IF_READ_SET(*comm, cyg_hal_plf_serial_read);
    CYGACC_COMM_IF_PUTC_SET(*comm, cyg_hal_plf_serial_putc);
    CYGACC_COMM_IF_GETC_SET(*comm, cyg_hal_plf_serial_getc);
    CYGACC_COMM_IF_CONTROL_SET(*comm, cyg_hal_plf_serial_control);
    CYGACC_COMM_IF_DBG_ISR_SET(*comm, cyg_hal_plf_serial_isr);
    CYGACC_COMM_IF_GETC_TIMEOUT_SET(*comm, cyg_hal_plf_serial_getc_timeout);
    // Restore original console
    CYGACC_CALL_IF_SET_CONSOLE_COMM(cur);
}

void
cyg_hal_plf_comms_init(void)
{
    static int initialized = 0;

    if (initialized)
        return;

    initialized = 1;

    cyg_hal_plf_serial_init();
}

/*
 * bp2016 defined some register that will cause a hardware reset;
 * maybe use wdt?
 */
extern void swrst_global_reset(void);
void
cyg_hal_bp2016_soft_reset(void)
{
    swrst_global_reset();
}

void cyg_hal_uart_init(cyg_uint32 id, cyg_uint32 baud)
{
    HWP_UART_T *hw_uart = serial_hwp_get(id);
    if(NULL == hw_uart){
        return;
    }
    cyg_hal_plf_serial_init_channel(hw_uart, baud);
}

void cyg_hal_uart_putc(cyg_uint32 id, cyg_uint8 c)
{
    HWP_UART_T *hw_uart = serial_hwp_get(id);
    if(NULL == hw_uart){
        return;
    }
    cyg_hal_plf_serial_putc(hw_uart, c);
}

void cyg_hal_uart_puts(cyg_uint32 id, cyg_uint8 *s, cyg_uint32 len)
{
    HWP_UART_T *hw_uart = serial_hwp_get(id);
    if(NULL == hw_uart){
        return;
    }
    cyg_hal_plf_serial_write(hw_uart, s, len);
}

int cyg_hal_uart_getc(cyg_uint32 id)
{
    HWP_UART_T *hw_uart = serial_hwp_get(id);
    if(NULL == hw_uart){
        return -1;
    }
    return cyg_hal_plf_serial_getc(hw_uart);
}
