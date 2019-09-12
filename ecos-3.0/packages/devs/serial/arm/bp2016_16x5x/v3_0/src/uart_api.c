#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/io/config_keys.h>

#include <cyg/hal/regs/uart.h>
#include <cyg/hal/regs/scm.h>
#include <cyg/hal/api/uart_api.h>
#include <cyg/hal/reset/reset.h>

#include <string.h>

#define HWP_SERIAL_PORT_NUM        (0x8)

// this is for debug trace,
#define CYGPKG_DEVS_UART_ARM_BP2016_DEBUG_LEVEL 0
#if (CYGPKG_DEVS_UART_ARM_BP2016_DEBUG_LEVEL > 0)
#define uart_printf diag_printf
#else
#define uart_printf(fmt, ...)
#endif

typedef struct uart_api_manager{
    cyg_uart_init_type_t  init_type;
    cyg_mutex_t  tx_mx;
    cyg_mutex_t  rx_mx;
    cyg_io_handle_t  handle;
}UART_API_MAN_ST;

UART_API_MAN_ST  uart_api_manager_st[HWP_SERIAL_PORT_NUM] = {{0}};

cyg_int32 UartOpen(cyg_uint32 id)
{
    Cyg_ErrNo res;
    cyg_io_handle_t  handle;
    if(id >= HWP_SERIAL_PORT_NUM){
        uart_printf("%s id(%d) error \n", __FUNCTION__, id);
        return -EINVAL;
	}

    if(CYGNUM_UART_INIT_TYPE_SYS == uart_api_manager_st[id].init_type){
        return ENOERR;
    }

    if(CYGNUM_UART_INIT_TYPE_NO != uart_api_manager_st[id].init_type){
        return -EPERM;
    }

    switch (id) {
    case UART_ID_0:
        res = cyg_io_lookup("/dev/ser0", &handle);
        break;
    case UART_ID_1:
        res = cyg_io_lookup("/dev/ser1", &handle);
        break;
    case UART_ID_2:
        res = cyg_io_lookup("/dev/ser2", &handle);
        break;
    case UART_ID_3:
        res = cyg_io_lookup("/dev/ser3", &handle);
        break;
    case UART_ID_4:
        res = cyg_io_lookup("/dev/ser4", &handle);
        break;
    case UART_ID_5:
        res = cyg_io_lookup("/dev/ser5", &handle);
        break;
    case UART_ID_6:
        res = cyg_io_lookup("/dev/ser6", &handle);
        break;
    case UART_ID_7:
        res = cyg_io_lookup("/dev/ser7", &handle);
        break;
    default:
        uart_printf("\n%s para error(input id=%d)\n", __FUNCTION__, id);
        return (-ENODEV);
        break;
    }
    uart_printf("\nopen handle = %x\n", (cyg_int32)(handle));
    if(ENOERR == res){
        cyg_mutex_init(&uart_api_manager_st[id].tx_mx);
        cyg_mutex_init(&uart_api_manager_st[id].rx_mx);
        uart_api_manager_st[id].init_type = CYGNUM_UART_INIT_TYPE_SYS;
        uart_api_manager_st[id].handle = handle;
    }
    return res;
}

Cyg_ErrNo UartWrite(cyg_uint32 id, void *wbuf, cyg_uint32 *wlen)
{
    Cyg_ErrNo res = 0;

	if((NULL == wbuf) || (id >= HWP_SERIAL_PORT_NUM)){
        uart_printf("%s para error \n", __FUNCTION__);
        return -EINVAL;
	}
    if(CYGNUM_UART_INIT_TYPE_SYS != uart_api_manager_st[id].init_type){
        uart_printf("%s error, \n", __FUNCTION__);
        return -EPERM;
    }

    uart_printf("%s: wbuf = 0x%x, wlen = %d\n", __FUNCTION__, (cyg_uint32)wbuf, *wlen);

    cyg_mutex_lock(&uart_api_manager_st[id].tx_mx);

    // dump_debug_buffer(rq, sizeof(struct request_w_r));
    res = cyg_io_write(uart_api_manager_st[id].handle, wbuf, wlen);

    cyg_mutex_unlock(&uart_api_manager_st[id].tx_mx);

    return res;
}

Cyg_ErrNo UartRead(cyg_uint32 id, void *rbuf, cyg_uint32 *rlen)
{
    Cyg_ErrNo res = 0;

	if((NULL == rbuf) || (id >= HWP_SERIAL_PORT_NUM)){
        uart_printf("%s para error \n", __FUNCTION__);
        return -EINVAL;
	}
    if(CYGNUM_UART_INIT_TYPE_SYS != uart_api_manager_st[id].init_type){
        return -EPERM;
    }

    uart_printf("%s: rbuf = 0x%x, rlen = %d\n", __FUNCTION__, (cyg_uint32)rbuf, *rlen);

    cyg_mutex_lock(&uart_api_manager_st[id].rx_mx);

    res = cyg_io_read(uart_api_manager_st[id].handle, rbuf, rlen);

    cyg_mutex_unlock(&uart_api_manager_st[id].rx_mx);

    return res;
}

Cyg_ErrNo UartSetConfig(cyg_uint32 id, cyg_uart_info_t *config)
{
    Cyg_ErrNo res = 0;
    cyg_uint32 len = sizeof(cyg_uart_info_t);

	if((NULL == config) || (id >= HWP_SERIAL_PORT_NUM)){
        uart_printf("%s para error \n", __FUNCTION__);
        return -EINVAL;
	}
    if(CYGNUM_UART_INIT_TYPE_SYS != uart_api_manager_st[id].init_type){
        return -EPERM;
    }

    uart_printf("%s: config_addr = 0x%x\n", __FUNCTION__, (cyg_uint32)config);

    cyg_mutex_lock(&uart_api_manager_st[id].tx_mx);
    

    res = cyg_io_set_config(uart_api_manager_st[id].handle, CYG_IO_SET_CONFIG_SERIAL_INFO,
                           (void *)config, &len);

    cyg_mutex_unlock(&uart_api_manager_st[id].tx_mx);
    return res;
}

Cyg_ErrNo UartGetConfig(cyg_uint32 id, cyg_uart_info_t *config)
{
    cyg_uint32 len = sizeof(cyg_uart_info_t);

	if((NULL == config) || (id >= HWP_SERIAL_PORT_NUM)){
        uart_printf("%s para error \n", __FUNCTION__);
        return -EINVAL;
	}
    if(CYGNUM_UART_INIT_TYPE_SYS != uart_api_manager_st[id].init_type){
        return -EPERM;
    }

    uart_printf("%s: config_addr = 0x%x\n", __FUNCTION__, (cyg_uint32)config);

    return cyg_io_get_config(uart_api_manager_st[id].handle, CYG_IO_GET_CONFIG_SERIAL_INFO,
                           (void *)config, &len);
}

Cyg_ErrNo UartAbort(cyg_uint32 id)
{
    cyg_uint32 len = 1;

	if(id >= HWP_SERIAL_PORT_NUM){
        uart_printf("%s para error \n", __FUNCTION__);
        return -EINVAL;
	}
    if(CYGNUM_UART_INIT_TYPE_SYS != uart_api_manager_st[id].init_type){
        return -EPERM;
    }

    return cyg_io_get_config(uart_api_manager_st[id].handle, CYG_IO_GET_CONFIG_SERIAL_ABORT,
                           0, &len);
}

Cyg_ErrNo UartClose(cyg_uint32 id)
{
	if(id >= HWP_SERIAL_PORT_NUM){
        uart_printf("%s id(%d) error \n", __FUNCTION__, id);
        return -EINVAL;
	}
    if(CYGNUM_UART_INIT_TYPE_SYS != uart_api_manager_st[id].init_type){
        return ENOERR;
    }

    memset((void *)&uart_api_manager_st[id], 0, sizeof(UART_API_MAN_ST));
    uart_api_manager_st[id].init_type = CYGNUM_UART_INIT_TYPE_NO;
    return ENOERR;
}

//// polling interfaces
extern void cyg_hal_uart_init(cyg_uint32 id, cyg_uint32 baud);
extern void cyg_hal_uart_putc(cyg_uint32 id, cyg_uint8 c);
extern void cyg_hal_uart_puts(cyg_uint32 id, cyg_uint8 *s, cyg_uint32 len);
extern int cyg_hal_uart_getc(cyg_uint32 id);
cyg_int32 UartPollingSetBaud(cyg_uint32 id, cyg_uint32 baudrate)
{
	if(id >= HWP_SERIAL_PORT_NUM){
        uart_printf("%s id(%d) error \n", __FUNCTION__, id);
        return -EINVAL;
	}
    if(CYGNUM_UART_INIT_TYPE_POLLING == uart_api_manager_st[id].init_type){
        cyg_mutex_lock(&uart_api_manager_st[id].tx_mx);
    }else if(CYGNUM_UART_INIT_TYPE_NO != uart_api_manager_st[id].init_type){
        return -EPERM;
    }
    
    cyg_hal_uart_init(id, baudrate);
    if(CYGNUM_UART_INIT_TYPE_POLLING == uart_api_manager_st[id].init_type){
        cyg_mutex_unlock(&uart_api_manager_st[id].tx_mx);
    }
    return ENOERR;
}

cyg_int32 UartPollingInit(cyg_uint32 id, cyg_uint32 baud)
{
    cyg_uint32 baudrate = baud;
    //(CYGNUM_IO_SERIAL_ARM_BP2016_16X5X_SERIAL0_BAUD),
    cyg_int32  res;
	if(id >= HWP_SERIAL_PORT_NUM){
        uart_printf("%s id(%d) error \n", __FUNCTION__, id);
        return -EINVAL;
	}

    if(CYGNUM_UART_INIT_TYPE_POLLING == uart_api_manager_st[id].init_type){
        return ENOERR;
    }

    if(CYGNUM_UART_INIT_TYPE_NO != uart_api_manager_st[id].init_type){
        return -EPERM;
    }
    uart_printf("%s id(%d), set default baud %d \n", __FUNCTION__, baudrate);

    // reset uart controller, temp, shoule be replaced by scm reset interface
    hal_sw_reset(id + SWRST_UART0);

    res = UartPollingSetBaud(id, baudrate);
    if(ENOERR == res){
        cyg_mutex_init(&uart_api_manager_st[id].tx_mx);
        cyg_mutex_init(&uart_api_manager_st[id].rx_mx);
        uart_api_manager_st[id].init_type = CYGNUM_UART_INIT_TYPE_POLLING;
    }
    return res;
}


cyg_int32 UartPollingDeInit(cyg_uint32 id)
{
	if(id >= HWP_SERIAL_PORT_NUM){
        uart_printf("%s id(%d) error \n", __FUNCTION__, id);
        return -EINVAL;
	}
    if(CYGNUM_UART_INIT_TYPE_POLLING != uart_api_manager_st[id].init_type){
        return ENOERR;
    }

    memset((void *)&uart_api_manager_st[id], 0, sizeof(UART_API_MAN_ST));
    uart_api_manager_st[id].init_type = CYGNUM_UART_INIT_TYPE_NO;
    return ENOERR;
}

void UartPollingPutC(cyg_uint32 id, char c)
{
	if(id >= HWP_SERIAL_PORT_NUM){
        uart_printf("%s id(%d) error \n", __FUNCTION__, id);
        return;
	}
    if(CYGNUM_UART_INIT_TYPE_POLLING != uart_api_manager_st[id].init_type){
        uart_printf("%s error, uart status invalid(must call UartInit first).\n", __FUNCTION__);
        return;
    }

    cyg_mutex_lock(&uart_api_manager_st[id].tx_mx);
    cyg_hal_uart_putc(id, c);
    cyg_mutex_unlock(&uart_api_manager_st[id].tx_mx);
}

void UartPollingPutS(cyg_uint32 id, char *s)
{
	if(id >= HWP_SERIAL_PORT_NUM){
        uart_printf("%s id(%d) error \n", __FUNCTION__, id);
        return;
	}
    if(CYGNUM_UART_INIT_TYPE_POLLING != uart_api_manager_st[id].init_type){
        uart_printf("%s error, uart status invalid(must call UartInit first).\n", __FUNCTION__);
        return;
    }

    cyg_mutex_lock(&uart_api_manager_st[id].tx_mx);
    cyg_hal_uart_puts(id, (cyg_uint8 *)s, strlen(s));
    cyg_mutex_unlock(&uart_api_manager_st[id].tx_mx);
}

cyg_int32 UartPollingGetC(cyg_uint32 id, char *c)
{
	if(id >= HWP_SERIAL_PORT_NUM){
        uart_printf("%s id(%d) error \n", __FUNCTION__, id);
        return -EINVAL;
	}
    if(CYGNUM_UART_INIT_TYPE_POLLING != uart_api_manager_st[id].init_type){
        uart_printf("%s error, uart status invalid(must call UartInit first).\n", __FUNCTION__);
        return -EPERM;
    }

    cyg_mutex_lock(&uart_api_manager_st[id].rx_mx);
    *c = cyg_hal_uart_getc(id);
    cyg_mutex_unlock(&uart_api_manager_st[id].rx_mx);
    return ENOERR;
}
