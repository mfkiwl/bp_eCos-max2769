#ifndef __SIM_BP2016_H__
#define __SIM_BP2016_H__

#include <cyg/hal/regs/sim.h>

#define     SIM_DEFAULT_CLK     5000

enum sim_set_config_e {
    SIM_SET_REQUEST = 0,
    SIM_SET_ETU,
    SIM_ATR_LOW,
    SIM_ATR_HIGH
};

enum sim_get_config_e {
    SIM_GET_REQUEST = 0,
    SIM_GET_CONFIG
};

enum sim_io_return_e {
    SIM_NO_PROCESS      = 0X00,
    SIM_RX_COMPLETE,
    SIM_TX_COMPLETE,
    SIM_GPCNT_TIMEOUT
};

typedef struct sim_atr_req_t {
    cyg_uint32          etu;
    cyg_uint32          parity;
    cyg_uint32          atr;    //0:rst low, 1: rst high
} sim_atr_req_t;

typedef struct sim_request_t {
    cyg_int8            *wbuf;
    cyg_uint32          wsize;
    cyg_int8            *rbuf;
    cyg_uint32          rsize;
    cyg_uint32          timeout;
    cyg_uint32          op_stat;
} sim_request_t;

#endif /* __SIM_BP2016_H__ */
