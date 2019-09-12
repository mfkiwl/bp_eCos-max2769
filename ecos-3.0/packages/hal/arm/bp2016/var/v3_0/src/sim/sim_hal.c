#include <pkgconf/system.h>
#ifdef CYGPKG_IO_COMMON_SIM_ARM_BP2016

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/io/devtab.h>
#include <cyg/hal/regs/sim.h>
#include <pkgconf/io_common_sim_arm_bp2016.h>

// INDEX
#define SIM_INDEX_0 0X00
#define SIM_INDEX_1 0X01
#define SIM_INDEX_2 0X02

extern inline int sim_polling_raw_stat(HWP_SIM_T *hwp_sim);
extern inline void sim_clr_tx_halfempty_flag(HWP_SIM_T *hwp_sim);
extern inline void sim_tx_byte(HWP_SIM_T *hwp_sim, const char c);
extern int sim_init_controller(HWP_SIM_T *hwp_sim, int etu, int parity);
extern inline void sim_upper_reset(HWP_SIM_T *hwp_sim);
extern inline void sim_clr_rx_not_empty_flag(HWP_SIM_T *hwp_sim);
extern inline u8 sim_rx_byte(HWP_SIM_T *hwp_sim);
extern inline int sim_set_etu(HWP_SIM_T *hwp_sim, u32 simclk);

static HWP_SIM_T *sim_get_hwp(cyg_uint32 index)
{
    switch (index) {
    case SIM_INDEX_0:
        return hwp_sim0;
    case SIM_INDEX_1:
        return hwp_sim1;
    case SIM_INDEX_2:
        return hwp_sim2;
    default:
        return (NULL);
    }
}

Cyg_ErrNo sim_hal_init_controller(cyg_uint32 index, cyg_uint32 etu, cyg_uint32 parity)
{
    Cyg_ErrNo res = 0;
    HWP_SIM_T *hwp_sim = sim_get_hwp(index);
    if (hwp_sim == NULL) {
        return (-EINVAL);
    }

    res = sim_init_controller(hwp_sim, etu, parity);
    return res;
}

Cyg_ErrNo sim_hal_upper_reset(cyg_uint32 index)
{
    Cyg_ErrNo res = 0;
    HWP_SIM_T *hwp_sim = sim_get_hwp(index);
    if (hwp_sim == NULL) {
        return (-EINVAL);
    }

    sim_upper_reset(hwp_sim);
    return res;
}

Cyg_ErrNo sim_hal_tx_byte(cyg_uint32 index, cyg_int8 wdata)
{
    Cyg_ErrNo res = 0;
    cyg_int32 irq_stat = 0;
    HWP_SIM_T *hwp_sim = sim_get_hwp(index);
    if (hwp_sim == NULL) {
        return (-EINVAL);
    }
    irq_stat = sim_polling_raw_stat(hwp_sim);
    if (irq_stat & SIM_INT_STAT_TDNFI) {
        sim_clr_tx_halfempty_flag(hwp_sim);
        sim_tx_byte(hwp_sim, wdata);
        return ENOERR;
    } else {
        return (-EINVAL);
    }
    return res;
}

Cyg_ErrNo sim_hal_rx_byte(cyg_uint32 index, cyg_int8 *rdata)
{
    cyg_int32 irq_stat = 0;
    HWP_SIM_T *hwp_sim = sim_get_hwp(index);
    if (hwp_sim == NULL) {
        return (-EINVAL);
    }
    irq_stat = sim_polling_raw_stat(hwp_sim);

    if (irq_stat & SIM_INT_STAT_RFNEI) {
        sim_clr_rx_not_empty_flag(hwp_sim);
        *rdata = sim_rx_byte(hwp_sim);
        return ENOERR;
    } else {
        return (-EINVAL);
    }
}

Cyg_ErrNo sim_hal_set_etu(cyg_uint32 index, cyg_uint32 etu)
{
    Cyg_ErrNo res = 0;
    HWP_SIM_T *hwp_sim = sim_get_hwp(index);
    if (hwp_sim == NULL) {
        return (-EINVAL);
    }
    res = sim_set_etu(hwp_sim, etu);
    return res;
}
#endif /* CYGPKG_IO_COMMON_SIM_ARM_BP2016 */
