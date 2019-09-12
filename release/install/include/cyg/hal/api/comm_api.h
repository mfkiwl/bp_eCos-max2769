#ifndef _COMM_API_H_
#define _COMM_API_H_

#include <cyg/hal/plf/common_def.h>

typedef cyg_uint32 (*callback_func)(cyg_uint32 , cyg_uint32);

typedef enum {
    CH_ST_IDLE = 0,
    CH_ST_SENDING = 1,
    CH_ST_SEND_TIMEOUT_OR_QUIT = 2,
    CH_ST_SEND_RECV_ACK = 3,
    CH_ST_SEND_RECV_ACK_AFTER_TIMEOUT = 4,

    CH_ST_RECVING = 0x11,
    CH_ST_RECV_TIMEOUT = 0x12,
    CH_ST_RECV_DATA = 0x13,
    CH_ST_RECV_SEND_ACK = 0x14,
    CH_ST_RECV_TIMEOUT_SEND_ACK = 0x15,
}CH_ST_S;

typedef enum {
    COMM_ERR_E_OK = 0,
    COMM_ERR_E_NO_RESP = 1,
    COMM_ERR_E_NO_CALLBACK = 2,
    COMM_ERR_E_PARA_INV = 3,
    COMM_ERR_E_SEM_TO = 4,
}COMM_ERR_E;

typedef enum {
    FUN_USER_FUNC0 = 0,
    FUN_USER_FUNC1,
    FUN_USER_FUNC2 = 2,
    FUN_USER_FUNC3,
    FUN_USER_FUNC4 = 4,
    FUN_USER_FUNC5,
    FUN_USER_FUNC6 = 6,
    FUN_USER_FUNC7,
    FUN_USER_FUNC8 = 8,
    FUN_USER_FUNC9,
    FUN_USER_FUNC10 = 10,
    FUN_USER_FUNC11,

    FUN_SYS_FUNC0 = 12,
    FUN_SYS_FUNC1,
    FUN_SYS_FUNC2 = 14,
    FUN_SYS_FUNC3,
    FUN_END,
}FUNC_T;


// function declare
void * cyg_cpu_comm_alloc(FUNC_T fun);
void cyg_cpu_comm_free(void *handle);
bool cyg_cpu_comm_send_msg(void * handle, char * buff, cyg_uint32 len, bool sync, cyg_uint32 timeout);
bool cyg_cpu_comm_wait_ack(void * handle, cyg_uint32 timeout);
void cyg_cpu_comm_recv_callback_register(void * handle, callback_func callback);
bool cyg_cpu_comm_recv(void * handle, cyg_uint32 timeout);
bool cyg_cpu_comm_ack(void * handle, cyg_uint32 ret);
cyg_uint32 cyg_cpu_comm_get_status(void * handle, CH_ST_S *send_st, CH_ST_S *recv_st);

#endif
