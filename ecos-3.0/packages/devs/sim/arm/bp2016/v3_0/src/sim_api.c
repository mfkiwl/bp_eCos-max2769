#include <pkgconf/io_common_sim_arm_bp2016.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/io/devtab.h>

#ifdef CYGPKG_IO_COMMON_SIM_ARM_BP2016
#include <cyg/hal/regs/sim.h>
#include <cyg/io/sim/sim_bp2016.h>
#include <cyg/hal/api/sim_api.h>

#ifdef CYGHWR_HAL_ASIC_CLK
#include <cyg/hal/clk/clk.h>
#endif

// this is for debug trace,
#if (CYGPKG_DEVS_SIM_ARM_BP2016_DEBUG_LEVEL > 0)
#define sim_printf diag_printf
#else
#define sim_printf(fmt, ...)
#endif

// Init flag
typedef enum {
    SIM_INIT_TYPE_INIT_VAR = 0, // only init variables
    SIM_INIT_TYPE_INIT_HW,      // init hardware ok
} sim_init_process_t;

typedef enum {
    SIM_INIT_TYPE_NO = 0,  // no mode_
    SIM_INIT_TYPE_SYS,     // ecos system mode
    SIM_INIT_TYPE_POLLING, // polling mode
} sim_init_type_t;

typedef struct sim_api_data_t {
    cyg_io_handle_t simhandler;
    cyg_drv_mutex_t lock;

    sim_init_type_t init_type;

    sim_atr_req_t atr_req;
    sim_request_t req;

    cyg_uint32 atr_level; // save atr,0:rst low, 1: rst high
    cyg_uint32 init;
} sim_api_data_t;

static struct sim_api_data_t api_data[CYGVAR_DEVS_BP2016_SIM_TOTAL_NUM];

static Cyg_ErrNo SimCardOpen(cyg_uint32 index)
{
    Cyg_ErrNo res = 0;

    switch (index) {
        case SIM_INDEX_0:
            res = cyg_io_lookup("/dev/sim0", &api_data[0].simhandler);
            break;
        case SIM_INDEX_1:
            res = cyg_io_lookup("/dev/sim1", &api_data[1].simhandler);
            break;
        case SIM_INDEX_2:
            res = cyg_io_lookup("/dev/sim2", &api_data[2].simhandler);
            break;
        default:
            return (-ENODEV);
            break;
    }
    return res;
}

#ifdef CYGHWR_HAL_ASIC_CLK
static int clk_set_sim(cyg_uint32 index, int rate_kHz)
{
    switch (index) {
    case 0:
        hal_clk_set_rate_kHz(CLK_SIM0, rate_kHz);
        break;
    case 1:
        hal_clk_set_rate_kHz(CLK_SIM1, rate_kHz);
        break;
    case 2:
        hal_clk_set_rate_kHz(CLK_SIM2, rate_kHz);
        break;
    }
    return 0;
}
#else
static int clk_set_sim(cyg_uint32 index, int rate_kHz)
{
    return 0;
}
#endif

Cyg_ErrNo SimCardInit(cyg_uint32 index)
{
    Cyg_ErrNo res = 0;
    struct sim_api_data_t *api_dat;

    res = SimCardOpen(index);
    if (res != ENOERR) {
        return res;
    }
    api_dat = &api_data[index];
    api_dat->init = SIM_INIT_TYPE_INIT_VAR;
    api_dat->init_type = SIM_INIT_TYPE_SYS;
    api_dat->atr_level = 0;
    cyg_drv_mutex_init(&api_dat->lock);
    clk_set_sim(index, SIM_DEFAULT_CLK);
    return res;
}

Cyg_ErrNo SimCardAtr(cyg_uint32 index, cyg_uint32 etu, cyg_uint32 parity, cyg_uint8 *rbuf,
                     cyg_uint32 *rlen, cyg_uint32 timeout_ms)
{
    Cyg_ErrNo res = 0;
    cyg_uint32 len = *rlen;
    struct sim_api_data_t *api_dat;
    struct sim_atr_req_t *atr_req;
    cyg_uint32 atr_req_len;
    struct sim_request_t *req;
    cyg_uint32 req_len;
    cyg_io_handle_t simhdl;

    if ((index >= CYGVAR_DEVS_BP2016_SIM_TOTAL_NUM) || (NULL == rbuf)) {
        return (-EINVAL);
    }
    if ((rlen == NULL) || (*rlen == 0)) {
        sim_printf("\natr len is null\n");
        return -EINVAL;
    }

    api_dat = &api_data[index];

    if (api_dat->init_type != SIM_INIT_TYPE_SYS) {
        sim_printf("\nnot io sys mode\n");
        return -ENOSUPP;
    }

    req = &api_dat->req;
    atr_req = &api_dat->atr_req;
    simhdl = api_dat->simhandler;
    atr_req_len = sizeof(struct sim_atr_req_t);
    req_len = sizeof(struct sim_request_t);

    cyg_drv_mutex_lock(&api_dat->lock);
    atr_req->parity = parity;
    atr_req->etu = etu;
    atr_req->atr = 0;

    req->rbuf = (cyg_int8 *)rbuf;
    req->rsize = len;
    req->timeout = timeout_ms / 10;

    res = cyg_io_set_config(simhdl, SIM_ATR_LOW, atr_req, &atr_req_len);
    if (res != ENOERR) {
        cyg_drv_mutex_unlock(&api_dat->lock);
        return res;
    }

    res = cyg_io_read(simhdl, req, &req_len);
    if (res != ENOERR) {
        cyg_drv_mutex_unlock(&api_dat->lock);
        return res;
    }

    switch (req->op_stat) {
        case SIM_GPCNT_TIMEOUT:
            break;

        case SIM_RX_COMPLETE: 
            {
                if (req->rsize > 0) {
                    sim_printf("\natr low len>0\n");
                    atr_req->atr = 0;
                    res = cyg_io_set_config(simhdl, SIM_ATR_HIGH, atr_req, &atr_req_len);
                    if (res != ENOERR) {
                        cyg_drv_mutex_unlock(&api_dat->lock);
                        return res;
                    }

                    *rlen = req->rsize;
                    api_dat->init = SIM_INIT_TYPE_INIT_HW;
                    cyg_drv_mutex_unlock(&api_dat->lock);
                    return res;
                }
            } break;

        default:
            break;
    }

    atr_req->atr = 1;

    res = cyg_io_set_config(simhdl, SIM_ATR_HIGH, atr_req, &atr_req_len);
    if (res != ENOERR) {
        cyg_drv_mutex_unlock(&api_dat->lock);
        return res;
    }

    req->rsize = len;
    res = cyg_io_read(simhdl, req, &req_len);
    if (res == ENOERR) {
        *rlen = req->rsize;
        api_dat->init = SIM_INIT_TYPE_INIT_HW;
        api_dat->atr_level = 1;
    }

    if (req->op_stat == SIM_GPCNT_TIMEOUT) {
        res = -ETIMEDOUT;
    }
    cyg_drv_mutex_unlock(&api_dat->lock);
    return res;
}

Cyg_ErrNo SimCardSetetu(cyg_uint32 index, cyg_uint32 etu)
{
    Cyg_ErrNo res = 0;
    struct sim_api_data_t *api_dat;
    cyg_io_handle_t simhdl;
    struct sim_atr_req_t *atr_req;
    cyg_uint32 atr_req_len;

    if ((index >= CYGVAR_DEVS_BP2016_SIM_TOTAL_NUM) || (etu > 372) || (etu < 16)) {
        return (-EINVAL);
    }

    api_dat = &api_data[index];

    if (api_dat->init_type != SIM_INIT_TYPE_SYS) {
        sim_printf("\nnot io sys mode\n");
        return -ENOSUPP;
    }

    simhdl = api_dat->simhandler;
    atr_req = &api_dat->atr_req;

    cyg_drv_mutex_lock(&api_dat->lock);
    atr_req->etu = etu;
    res = cyg_io_set_config(simhdl, SIM_SET_ETU, atr_req, &atr_req_len);
    cyg_drv_mutex_unlock(&api_dat->lock);

    return res;
}

Cyg_ErrNo SimCardWrite(cyg_uint32 index, void *wbuf, cyg_uint32 *wlen)
{
    Cyg_ErrNo res = 0;
    struct sim_api_data_t *api_dat;
    struct sim_request_t *req;
    cyg_uint32 req_len;
    cyg_io_handle_t simhdl;

    if ((index >= CYGVAR_DEVS_BP2016_SIM_TOTAL_NUM) || (NULL == wbuf)) {
        return (-EINVAL);
    }
    api_dat = &api_data[index];

    if (api_dat->init_type != SIM_INIT_TYPE_SYS) {
        sim_printf("\nnot io sys mode\n");
        return -ENOSUPP;
    }

    simhdl = api_data[index].simhandler;
    req = &api_dat->req;
    req_len = sizeof(struct sim_request_t);

    if (api_dat->init != SIM_INIT_TYPE_INIT_HW) {
        sim_printf("\nwarning: need init!\n");
        return (-EINVAL);
    }

    cyg_drv_mutex_lock(&api_dat->lock);

    req->op_stat = SIM_NO_PROCESS;
    req->wbuf = wbuf;
    req->wsize = *wlen;
    req->timeout = 0;

    res = cyg_io_write(simhdl, req, &req_len);
    if (req->op_stat == SIM_GPCNT_TIMEOUT) {
        res = -ETIMEDOUT;
    }
    *wlen = req->wsize;

    cyg_drv_mutex_unlock(&api_dat->lock);
    return res;
}

Cyg_ErrNo SimCardRead(cyg_uint32 index, void *rbuf, cyg_uint32 *rlen, cyg_uint32 timeout_ms)
{
    Cyg_ErrNo res = 0;
    cyg_int32 i = 0;
    cyg_uint32 loop = 0;
    cyg_uint32 len = 0;

    struct sim_api_data_t *api_dat;
    cyg_io_handle_t simhdl;
    struct sim_request_t *req;
    cyg_uint32 req_len;

    if ((index >= CYGVAR_DEVS_BP2016_SIM_TOTAL_NUM) || (NULL == rbuf)) {
        return (-EINVAL);
    }
    if ((rlen == NULL) || (*rlen == 0)) {
        sim_printf("\natr len is null\n");
        return -EINVAL;
    }

    api_dat = &api_data[index];

    if (api_dat->init_type != SIM_INIT_TYPE_SYS) {
        sim_printf("\nnot io sys mode\n");
        return -ENOSUPP;
    }

    simhdl = api_data[index].simhandler;
    req_len = sizeof(struct sim_request_t);
    req = &api_dat->req;

    if (api_dat->init != SIM_INIT_TYPE_INIT_HW) {
        sim_printf("\nwarning: need init!\n");
        return (-EINVAL);
    }

    cyg_drv_mutex_lock(&api_dat->lock);
    req->op_stat = SIM_NO_PROCESS;
    loop = *rlen + CYGNUM_DEVS_SIM_BP2016_RX_BUFFER_SIZE - 1;
    loop = loop / CYGNUM_DEVS_SIM_BP2016_RX_BUFFER_SIZE;
    for (i = 0; i < loop; i++) {
        req->rbuf = (cyg_int8 *)rbuf + i * CYGNUM_DEVS_SIM_BP2016_RX_BUFFER_SIZE;
        req->rsize = (*rlen >= CYGNUM_DEVS_SIM_BP2016_RX_BUFFER_SIZE)
            ? CYGNUM_DEVS_SIM_BP2016_RX_BUFFER_SIZE
            : (*rlen);
        req->timeout = (timeout_ms + 9) / 10;

        res = cyg_io_read(simhdl, req, &req_len);
        if (res != ENOERR) {
            cyg_drv_mutex_unlock(&api_dat->lock);
            return res;
        }
        if (*rlen > CYGNUM_DEVS_SIM_BP2016_RX_BUFFER_SIZE) {
            *rlen -= CYGNUM_DEVS_SIM_BP2016_RX_BUFFER_SIZE;
        }
        len += req->rsize;
    }
    *rlen = len;
    if (req->op_stat == SIM_GPCNT_TIMEOUT) {
        res = -ETIMEDOUT;
    }
    cyg_drv_mutex_unlock(&api_dat->lock);
    return res;
}

Cyg_ErrNo SimCardRequest(cyg_uint32 index, void *wbuf, cyg_uint32 *wlen, void *rbuf,
                         cyg_uint32 *rlen, const cyg_uint32 timeout_ms)
{
    Cyg_ErrNo res = 0;
    return res;
}

Cyg_ErrNo SimCardGetConfig(cyg_uint32 index, void *rbuf, cyg_uint32 *rlen)
{
    Cyg_ErrNo res = 0;

    struct sim_api_data_t *api_dat;
    struct sim_atr_req_t *atr_req = (struct sim_atr_req_t *) rbuf;
    cyg_uint32 atr_req_len;
    cyg_io_handle_t simhdl;

    if ((index >= CYGVAR_DEVS_BP2016_SIM_TOTAL_NUM) || (NULL == rbuf)) {
        return (-EINVAL);
    }
    if ((rlen == NULL) || (*rlen == 0)) {
        sim_printf("\natr len is null\n");
        return -EINVAL;
    }
    api_dat = &api_data[index];

    if (api_dat->init_type != SIM_INIT_TYPE_SYS) {
        sim_printf("\nnot io sys mode\n");
        return -ENOSUPP;
    }

    simhdl = api_dat->simhandler;
    atr_req_len = sizeof(struct sim_atr_req_t);

    cyg_drv_mutex_lock(&api_dat->lock);
    res = cyg_io_get_config(simhdl, SIM_GET_CONFIG, atr_req, &atr_req_len);
    cyg_drv_mutex_unlock(&api_dat->lock);
    sim_printf("\netu = %d, atr = %d, parity = %d\n", atr_req->etu, atr_req->atr, atr_req->parity);

    return res;
}

// --------------- polling -------------------------------------------
#define SIM_POLLING_MODE
#ifdef SIM_POLLING_MODE
extern Cyg_ErrNo sim_hal_init_controller(cyg_uint32 index, cyg_uint32 etu, cyg_uint32 parity);
extern Cyg_ErrNo sim_hal_upper_reset(cyg_uint32 index);
extern Cyg_ErrNo sim_hal_tx_byte(cyg_uint32 index, cyg_int8 wdata);
extern Cyg_ErrNo sim_hal_rx_byte(cyg_uint32 index, cyg_int8 *rdata);
extern Cyg_ErrNo sim_hal_set_etu(cyg_uint32 index, cyg_uint32 etu);

Cyg_ErrNo SimCardPollingInit(cyg_uint32 index)
{
    struct sim_api_data_t *api_dat;

    if (index >= CYGVAR_DEVS_BP2016_SIM_TOTAL_NUM) {
        return (-ENODEV);
    }
    api_dat = &api_data[index];
    api_dat->init = SIM_INIT_TYPE_INIT_VAR;
    api_dat->init_type = SIM_INIT_TYPE_POLLING;
    cyg_drv_mutex_init(&api_dat->lock);

    return ENOERR;
}

Cyg_ErrNo SimCardPollingWrite(cyg_uint32 index, cyg_int8 wdata)
{
    Cyg_ErrNo res = 0;
    struct sim_api_data_t *api_dat;
    api_dat = &api_data[index];

    if (api_dat->init_type != SIM_INIT_TYPE_POLLING) {
        sim_printf("\nnot polling mode\n");
        return -ENOSUPP;
    }

    if (api_dat->init != SIM_INIT_TYPE_INIT_HW) {
        sim_printf("\nwarning: need init!\n");
        return (-EINVAL);
    }

    cyg_drv_mutex_lock(&api_dat->lock);
    res = sim_hal_tx_byte(index, wdata);

    cyg_drv_mutex_unlock(&api_dat->lock);
    return res;
}

Cyg_ErrNo SimCardPollingRead(cyg_uint32 index, cyg_int8 *rdata)
{
    Cyg_ErrNo res = 0;
    struct sim_api_data_t *api_dat;
    api_dat = &api_data[index];

    if (api_dat->init_type != SIM_INIT_TYPE_POLLING) {
        sim_printf("\nnot polling mode\n");
        return -ENOSUPP;
    }

    if (api_dat->init != SIM_INIT_TYPE_INIT_HW) {
        sim_printf("\nwarning: need init!\n");
        return (-EINVAL);
    }

    cyg_drv_mutex_lock(&api_dat->lock);
    res = sim_hal_rx_byte(index, rdata);
    cyg_drv_mutex_unlock(&api_dat->lock);
    return (res);
}

Cyg_ErrNo SimCardPollingAtr(cyg_uint32 index, cyg_uint32 etu, cyg_uint32 parity, cyg_uint8 *rbuf,
                            cyg_uint32 *rlen, cyg_uint32 timeout_ms)
{
    Cyg_ErrNo res = 0;
    cyg_uint32 len = 0;
    cyg_int8 rdata = 0;
    cyg_uint32 s, e;
    struct sim_api_data_t *api_dat;
    api_dat = &api_data[index];

    if (api_dat->init_type != SIM_INIT_TYPE_POLLING) {
        sim_printf("\nnot polling mode\n");
        return -ENOSUPP;
    }

    cyg_drv_mutex_lock(&api_dat->lock);

    res = sim_hal_init_controller(index, etu, parity);

    s = arch_counter_get_cntpct();
    while (1) {
        res = sim_hal_rx_byte(index, &rdata);
        if (res == ENOERR) {
            rbuf[len] = rdata;
            len++;
            if (len == *rlen) {
                break;
            }
        }

        e = arch_counter_get_cntpct();
        if (((unsigned long) (e - s) / (CONFIG_COUNTER_FREQ / 1000)) >= timeout_ms) {
            break;
        }
    }
    if (len > 0) {
        *rlen = len;
        api_dat->init = SIM_INIT_TYPE_INIT_HW;
        sim_hal_upper_reset(index);
        cyg_drv_mutex_unlock(&api_dat->lock);
        return 0;
    }
    sim_hal_upper_reset(index);
    s = arch_counter_get_cntpct();
    while (1) {
        res = sim_hal_rx_byte(index, &rdata);
        if (res == ENOERR) {
            rbuf[len] = rdata;
            len++;
            if (len == *rlen) {
                break;
            }
        }

        e = arch_counter_get_cntpct();
        if (((unsigned long) (e - s) / (CONFIG_COUNTER_FREQ / 1000)) >= timeout_ms) {
            res = -ETIMEDOUT;
            break;
        }
    }
    if (len > 0) {
        *rlen = len;
        api_dat->init = SIM_INIT_TYPE_INIT_HW;
    }
    cyg_drv_mutex_unlock(&api_dat->lock);
    return res;
}

Cyg_ErrNo SimCardPollingSetetu(cyg_uint32 index, cyg_uint32 etu)
{
    Cyg_ErrNo res = 0;
    struct sim_api_data_t *api_dat;
    api_dat = &api_data[index];

    if (api_dat->init_type != SIM_INIT_TYPE_POLLING) {
        sim_printf("\nnot polling mode\n");
        return -ENOSUPP;
    }

    cyg_drv_mutex_lock(&api_dat->lock);
    sim_hal_set_etu(index, etu);
    cyg_drv_mutex_unlock(&api_dat->lock);
    return res;
}

#endif /* SIM_POLLING_MODE */
#endif /* CYGPKG_IO_COMMON_SIM_ARM_BP2016 */
