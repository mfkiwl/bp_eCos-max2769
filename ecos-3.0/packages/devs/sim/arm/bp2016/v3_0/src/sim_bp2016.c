//==========================================================================
//
// Author(s):    huyue
// Contributors:
// Date:         2017-10-17
// Purpose:
// Description:  sim(iso7816) RDSS driver
//
//==========================================================================
#include <pkgconf/io_common_sim_arm_bp2016.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/devtab.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <string.h>

#ifdef CYGPKG_IO_COMMON_SIM_ARM_BP2016
#include <cyg/hal/regs/sim.h>
#include <cyg/io/sim/sim_bp2016.h>

// this is for debug trace,
#if (CYGPKG_DEVS_SIM_ARM_BP2016_DEBUG_LEVEL > 0)
#define sim_printf diag_printf
#else
#define sim_printf(fmt, ...)
#endif

enum sim_io_status_e {
    SIM_IDLE_STAT       = 0X00,
    SIM_PENDING_STAT
};

enum sim_atr_valid_e {
    SIM_ATR_LOW_VALID   = 0X00,
    SIM_ATR_HIGH_VALID
};

typedef struct sim_priv_chan_info {
    HWP_SIM_T           *hwp_sim;
    cyg_uint32          index;
    cyg_uint32          pending;
    cyg_vector_t        vector;     // interrupt vector number
    cyg_uint32          intprio;    // interrupt priority of sim
    cyg_drv_mutex_t     lock;
    cyg_drv_cond_t      signal;

    cyg_interrupt       sim_int;
    cyg_handle_t        sim_int_handle; // For initializing the interrupt

    sim_atr_req_t       atr_req;
    sim_request_t       req;
    cyg_uint32          op_stat;

    cyg_int8            rxbuf[CYGNUM_DEVS_SIM_BP2016_RX_BUFFER_SIZE];
    cyg_uint32          rlen;
    cyg_uint32          wlen;

    cyg_uint32          atr;
    cyg_uint32          etu;
    cyg_uint32          parity;
} sim_priv_chan_info;

// extern
extern inline void sim_disable(HWP_SIM_T *hwp_sim);
extern inline void sim_tx_halfempty_inten(HWP_SIM_T *hwp_sim, int en);
extern inline void sim_rx_notempty_inten(HWP_SIM_T *hwp_sim, int en);
extern inline void sim_set_gpcnt_en(HWP_SIM_T *hwp_sim, int en);
extern inline int sim_get_intr_stat(HWP_SIM_T *hwp_sim);
extern inline void sim_clr_rx_not_empty_flag(HWP_SIM_T *hwp_sim);
extern inline void sim_clr_gpcnt_flag(HWP_SIM_T *hwp_sim);
extern inline void sim_clr_tx_halfempty_flag(HWP_SIM_T *hwp_sim);
extern inline u8 sim_rx_byte(HWP_SIM_T *hwp_sim);
extern inline void sim_tx_byte(HWP_SIM_T *hwp_sim, const char c);
extern inline void sim_upper_reset(HWP_SIM_T *hwp_sim);
extern inline int sim_set_etu(HWP_SIM_T *hwp_sim, u32 simclk);
extern int sim_init_controller(HWP_SIM_T *hwp_sim, int etu, int parity);

// Functions in this module
static cyg_uint32 bp2016_sim_ISR(cyg_vector_t vector,
                                 cyg_addrword_t data);
static void bp2016_sim_DSR(cyg_vector_t vector,
                           cyg_ucount32 count,
                           cyg_addrword_t data);
static Cyg_ErrNo sim_write(cyg_io_handle_t handle,
                           const void *buffer,
                           cyg_uint32 *len);
static Cyg_ErrNo sim_read(cyg_io_handle_t handle,
                          void *buffer,
                          cyg_uint32 *len);
static cyg_bool  sim_select(cyg_io_handle_t handle,
                            cyg_uint32 which,
                            cyg_addrword_t info);
static Cyg_ErrNo sim_set_config(cyg_io_handle_t handle,
                                cyg_uint32 key,
                                const void *buffer,
                                cyg_uint32 *len);
static Cyg_ErrNo sim_get_config(cyg_io_handle_t handle,
                                cyg_uint32 key,
                                void *buffer,
                                cyg_uint32 *len);
static bool      sim_init(struct cyg_devtab_entry *tab);
static Cyg_ErrNo sim_lookup(struct cyg_devtab_entry **tab,
                            struct cyg_devtab_entry *st,
                            const char *name);

CHAR_DEVIO_TABLE(sim_handler,
                 sim_write,
                 sim_read,
                 sim_select,
                 sim_get_config,
                 sim_set_config);

#ifdef CYGVAR_DEVS_BP2016_SIM0_DEVTAB_ENTRY
sim_priv_chan_info sim_info0 = {
    hwp_sim:        hwp_sim0,
    index:          CYGVAR_DEVS_BP2016_SIM_INDEX0,
    vector:         SYS_IRQ_ID_SIM0,
    intprio:        CYGNUM_DEVS_SIM0_ARM_BP2016_INTPRIO,
};

CHAR_DEVTAB_ENTRY(sim0_device,
                  CYGDAT_DEVS_SIM_BP2016_NAME "0",
                  NULL,                                   // Base device name
                  &sim_handler,
                  sim_init,
                  sim_lookup,
                  (void*) &sim_info0);                                  // Private data pointer
#endif

#ifdef CYGVAR_DEVS_BP2016_SIM1_DEVTAB_ENTRY
sim_priv_chan_info sim_info1 = {
    hwp_sim:        hwp_sim1,
    index:          CYGVAR_DEVS_BP2016_SIM_INDEX1,
    vector:         SYS_IRQ_ID_SIM1,
    intprio:        CYGNUM_DEVS_SIM1_ARM_BP2016_INTPRIO,
};

CHAR_DEVTAB_ENTRY(sim1_device,
                  CYGDAT_DEVS_SIM_BP2016_NAME "1",
                  NULL,                                   // Base device name
                  &sim_handler,
                  sim_init,
                  sim_lookup,
                  (void*) &sim_info1);
#endif

#ifdef CYGVAR_DEVS_BP2016_SIM2_DEVTAB_ENTRY
sim_priv_chan_info sim_info2 = {
    hwp_sim:        hwp_sim2,
    index:          CYGVAR_DEVS_BP2016_SIM_INDEX2,
    vector:         SYS_IRQ_ID_SIM2,
    intprio:        CYGNUM_DEVS_SIM2_ARM_BP2016_INTPRIO,
};

CHAR_DEVTAB_ENTRY(sim2_device,
                  CYGDAT_DEVS_SIM_BP2016_NAME "2",
                  NULL,                                   // Base device name
                  &sim_handler,
                  sim_init,
                  sim_lookup,
                  (void*) &sim_info2);
#endif

static Cyg_ErrNo 
sim_write(cyg_io_handle_t handle,
          const void *buffer,
          cyg_uint32 *len)
{
    Cyg_ErrNo res = 0;
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *) handle;
    sim_priv_chan_info *chan = (sim_priv_chan_info *) t->priv;
    struct sim_request_t *config = (struct sim_request_t *) buffer;

    // sim_printf("\nwrite start \n");
    if (buffer == NULL) {
        return -EINVAL;
    }

    if ((len == NULL) || (*len == 0)) {
        return -EINVAL;
    }

    cyg_drv_mutex_lock(&chan->lock);

    chan->op_stat = SIM_NO_PROCESS;
    memcpy(&chan->req, buffer, sizeof(struct sim_request_t));
    chan->pending = SIM_PENDING_STAT;

    chan->wlen = 0;
    cyg_drv_cond_init(&chan->signal, &chan->lock);
    cyg_drv_dsr_lock();
    sim_tx_halfempty_inten(chan->hwp_sim, 1);
    res = cyg_drv_cond_wait(&chan->signal);
    cyg_drv_dsr_unlock();

    if (res != true) {
        cyg_drv_mutex_unlock(&chan->lock);
        sim_printf("\nwrite cond = %x\n", res);
        return res;
    }

    config->op_stat = chan->op_stat;
    config->wsize = chan->wlen;

    cyg_drv_mutex_unlock(&chan->lock);
    // sim_printf("\nwrite end\n");
    return ENOERR;
}

static Cyg_ErrNo 
sim_read(cyg_io_handle_t handle,
         void *buffer,
         cyg_uint32 *len)
{
    Cyg_ErrNo res = 0;
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *) handle;
    sim_priv_chan_info *chan = (sim_priv_chan_info *) t->priv;
    struct sim_request_t *config = (struct sim_request_t *) buffer;

    // sim_printf("\nread start 0\n");

    if (buffer == NULL) {
        sim_printf("\nread buffer is null\n");
        return -EINVAL;
    }
    if ((len == NULL) || (*len == 0)) {
        sim_printf("\nread len is null\n");
        return -EINVAL;
    }
    if (config->rsize > CYGNUM_DEVS_SIM_BP2016_RX_BUFFER_SIZE) {
        sim_printf("\nread len is null\n");
        return -EINVAL;
    }
    if (config->timeout == 0) {
        return -EINVAL;
    }

    cyg_drv_mutex_lock(&chan->lock);

    chan->op_stat = SIM_NO_PROCESS;
    memcpy(&chan->req, buffer, sizeof(struct sim_request_t));
    chan->pending = SIM_PENDING_STAT;

    chan->rlen = 0;
    cyg_drv_cond_init(&chan->signal, &chan->lock);
    cyg_drv_dsr_lock();
    sim_set_gpcnt_en(chan->hwp_sim, 1);
    sim_rx_notempty_inten(chan->hwp_sim, 1);
    res = cyg_drv_cond_wait(&chan->signal);
    cyg_drv_dsr_unlock();
    if (res != true) {
        cyg_drv_mutex_unlock(&chan->lock);
        sim_printf("\ncond res = %x\n", res);
        return res;
    }

    config->op_stat = chan->op_stat;
    memcpy(config->rbuf, chan->rxbuf, chan->rlen);
    config->rsize = chan->rlen;

    cyg_drv_mutex_unlock(&chan->lock);
    return ENOERR;
}

static cyg_bool  
sim_select(cyg_io_handle_t handle,
           cyg_uint32 which,
           cyg_addrword_t info)
{
    return -EINVAL;
}

static Cyg_ErrNo 
sim_set_config(cyg_io_handle_t handle,
               cyg_uint32 key,
               const void *buffer,
               cyg_uint32 *len)
{
    Cyg_ErrNo res = 0;
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *) handle;
    sim_priv_chan_info *chan = (sim_priv_chan_info *) t->priv;
    struct sim_atr_req_t *atr_req = &chan->atr_req;

    if (buffer == NULL) {
        sim_printf("\nread buffer is null\n");
        return -EINVAL;
    }

    if ((len == NULL) || (*len == 0)) {
        sim_printf("\nread len is null\n");
        return -EINVAL;
    }

    cyg_drv_mutex_lock(&chan->lock);

    switch (key) {
        case SIM_ATR_LOW:
            memcpy(atr_req, buffer, sizeof(struct sim_atr_req_t));
            res = sim_init_controller(chan->hwp_sim, atr_req->etu, atr_req->parity);
            if (res != ENOERR) {
                cyg_drv_mutex_unlock(&chan->lock);
                return res;
            }
            break;

        case SIM_ATR_HIGH:
            memcpy(atr_req, buffer, sizeof(struct sim_atr_req_t));
            if (atr_req->atr == 0) {
                chan->atr = SIM_ATR_LOW_VALID;
            } else {
                chan->atr = SIM_ATR_HIGH_VALID;
            }
            chan->etu = atr_req->etu;
            chan->parity = atr_req->parity;
            sim_upper_reset(chan->hwp_sim);
            break;

        case SIM_SET_ETU: // etu
            memcpy(atr_req, buffer, sizeof(struct sim_atr_req_t));
            res = sim_set_etu(chan->hwp_sim, atr_req->etu);
            if (res != ENOERR) {
                cyg_drv_mutex_unlock(&chan->lock);
                return res;
            }
            chan->etu = atr_req->etu;
            break;

        case SIM_SET_REQUEST: // request
            break;

        default:
            break;
    }

    cyg_drv_mutex_unlock(&chan->lock);
    return ENOERR;
}

static Cyg_ErrNo 
sim_get_config(cyg_io_handle_t handle,
               cyg_uint32 key,
               void *buffer,
               cyg_uint32 *len)
{
    cyg_devtab_entry_t *t = (cyg_devtab_entry_t *) handle;
    sim_priv_chan_info *chan = (sim_priv_chan_info *) t->priv;

    if (buffer == NULL) {
        sim_printf("\nread buffer is null\n");
        return -EINVAL;
    }

    if ((len == NULL) || (*len == 0)) {
        sim_printf("\nread len is null\n");
        return -EINVAL;
    }

    cyg_drv_mutex_lock(&chan->lock);

    switch (key) {
        case SIM_GET_CONFIG:
            memcpy(buffer, &chan->atr_req, sizeof(struct sim_atr_req_t));
            break;

        default:
            break;
    }

    cyg_drv_mutex_unlock(&chan->lock);
    return ENOERR;
}

static bool 
sim_init(struct cyg_devtab_entry *tab)
{
    sim_priv_chan_info *chan = (sim_priv_chan_info *) tab->priv;

    sim_disable(chan->hwp_sim);
    cyg_drv_interrupt_create(chan->vector,
                            chan->intprio,
                            (cyg_addrword_t) chan,
                            bp2016_sim_ISR,
                            bp2016_sim_DSR,
                            &chan->sim_int_handle,
                            &chan->sim_int);
    cyg_drv_interrupt_attach(chan->sim_int_handle);
    cyg_drv_interrupt_unmask(chan->vector);

    chan->pending = SIM_IDLE_STAT;
    chan->op_stat = SIM_NO_PROCESS;
    cyg_drv_mutex_init(&chan->lock);
    cyg_drv_cond_init(&chan->signal, &chan->lock);

    return ENOERR;
}

static Cyg_ErrNo 
sim_lookup(struct cyg_devtab_entry **tab,
           struct cyg_devtab_entry *st,
           const char *name)
{
    return ENOERR;
}

static cyg_uint32 
bp2016_sim_ISR(cyg_vector_t vector,
               cyg_addrword_t data)
{
    cyg_drv_interrupt_mask(vector);
    cyg_drv_interrupt_acknowledge(vector);
    return (CYG_ISR_CALL_DSR);
}

static void 
bp2016_sim_DSR(cyg_vector_t vector,
               cyg_ucount32 count,
               cyg_addrword_t data)
{
    sim_priv_chan_info *chan = (sim_priv_chan_info *) data;
    int i = 0;
    struct sim_request_t *req = &chan->req;
    int status = sim_get_intr_stat(chan->hwp_sim);

    // sim_printf("DSR st=0x%x , chan data = 0x%x\n", status, chan);

    if (status & SIM_INT_STAT_GPCNTI) {
        sim_clr_gpcnt_flag(chan->hwp_sim);
        req->timeout--;
        if (req->timeout == 0) {
            sim_set_gpcnt_en(chan->hwp_sim, 0);
            chan->pending = SIM_IDLE_STAT;
            chan->op_stat = SIM_GPCNT_TIMEOUT;
            sim_printf("\n\ngpcnt\n");
            cyg_drv_cond_signal(&chan->signal);
        }
    }

    if (status & SIM_INT_STAT_RFNEI) {
        sim_clr_rx_not_empty_flag(chan->hwp_sim);
        if (chan->pending == SIM_PENDING_STAT) {
            chan->rxbuf[chan->rlen] = sim_rx_byte(chan->hwp_sim);
            chan->rlen++;
            if (req->rsize == chan->rlen) {
                sim_set_gpcnt_en(chan->hwp_sim, 0);
                sim_rx_notempty_inten(chan->hwp_sim, 0);
                chan->pending = SIM_IDLE_STAT;
                chan->op_stat = SIM_RX_COMPLETE;
                sim_printf("\ndsr rlen =0x%x\n", chan->rlen);
                cyg_drv_cond_signal(&chan->signal);
            }
        } else {
            sim_rx_byte(chan->hwp_sim);
        }
    }

    if (status & SIM_INT_STAT_TDHFI) {
        sim_clr_tx_halfempty_flag(chan->hwp_sim);
        if (chan->pending == SIM_PENDING_STAT) {
            int loops =
                (req->wsize > SIM_XMT_FIFO_HALF_DEPTH) ? SIM_XMT_FIFO_HALF_DEPTH : req->wsize;
            for (i = 0; i < loops; i++) {
                sim_tx_byte(chan->hwp_sim, req->wbuf[chan->wlen++]);
            }
            if (req->wsize > SIM_XMT_FIFO_HALF_DEPTH) {
                req->wsize -= SIM_XMT_FIFO_HALF_DEPTH;
            } else {
                sim_tx_halfempty_inten(chan->hwp_sim, 0);
                req->wsize = 0;
                chan->pending = SIM_IDLE_STAT;
                chan->op_stat = SIM_TX_COMPLETE;
                cyg_drv_cond_signal(&chan->signal);
            }
        }
    }

    cyg_drv_interrupt_unmask(vector);
}

#endif
