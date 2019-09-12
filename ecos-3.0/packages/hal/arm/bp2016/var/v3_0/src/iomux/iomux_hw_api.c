#include <cyg/kernel/kapi.h>
#include <cyg/hal/iomux/iomux_hw.h>
#include "iomux_hw_regs.h"
#include <cyg/hal/api/iomux_api.h>


static cyg_mutex_t iomux_mutex;

void iomux_init(void)
{
    cyg_mutex_init(&iomux_mutex);
}

bool hal_iomux_spi1_en(int cs0, int cs1)
{
    if ((!cs0) && (!cs1)) {
        return false;
    }
    cyg_mutex_lock(&iomux_mutex);
    scm_spi1_en(cs0, cs1);
    cyg_mutex_unlock(&iomux_mutex);
    return true;
}

bool hal_iomux_spi2_en(int cs0, int cs1, int cs2, int cs3, int cs4, int cs5, int cs6, int cs7)
{
    if ((!cs0) && (!cs1) && (!cs2) && (!cs3) && (!cs4) && (!cs5) && (!cs6) && (!cs7)) {
        return false;
    }
    cyg_mutex_lock(&iomux_mutex);
    scm_spi2_en(cs0, cs1, cs2, cs3, cs4, cs5, cs6, cs7);
    cyg_mutex_unlock(&iomux_mutex);
    return true;
}

bool hal_iomux_spi4_en(int cs0, int cs1)
{
    if ((!cs0) && (!cs1)) {
        return false;
    }
    cyg_mutex_lock(&iomux_mutex);
    scm_spi4_en(cs0, cs1);
    cyg_mutex_unlock(&iomux_mutex);
    return true;
}

bool hal_iomux_gpio_en(GPIO_GROUP_ID_T group_id, int id)
{
    if((group_id > GPIO_GROUP_ID_D) || (id > 31))
    {
        return false;
    }
    cyg_mutex_lock(&iomux_mutex);
    switch(group_id){
    case GPIO_GROUP_ID_A:
        scm_gpioa_en(id);
        break;
    case GPIO_GROUP_ID_B:
        scm_gpiob_en(id);
        break;
    case GPIO_GROUP_ID_C:
        scm_gpioc_en(id);
        break;
    case GPIO_GROUP_ID_D:
        scm_gpiod_en(id);
        break;
    }
    cyg_mutex_unlock(&iomux_mutex);
    return true;
}

bool hal_iomux_en(IOMUX_ID_TYPE_T id)
{
    if(id >= IOMUX_ID_END){
        return false;
    }

    cyg_mutex_lock(&iomux_mutex);
    switch(id){
    case IOMUX_QSPI:
        scm_qspi_en();
        break;

    case IOMUX_SPI0:
        scm_spi0_en();
        break;
    case IOMUX_SPI3:
        scm_spi3_en();
        break;

    case IOMUX_IF0:
        scm_if_en(0);
        break;
    case IOMUX_IF1:
        scm_if_en(1);
        break;
    case IOMUX_IF2:
        scm_if_en(2);
        break;
    case IOMUX_IF3:
        scm_if_en(3);
        break;

    case IOMUX_MMC0:
        scm_mmc_en(0);
        break;
    case IOMUX_MMC1:
        scm_mmc_en(1);
        break;

    case IOMUX_PRM0:
        scm_prm0_en();
        break;
    case IOMUX_PRM1:
        scm_prm1_en();
        break;
 
    case IOMUX_PWM0:
        scm_hwpwm0();
        break;
    case IOMUX_PWM1:
        scm_hwpwm1();
        break;
    case IOMUX_PWM2:
        scm_hwpwm2();
        break;
    case IOMUX_PWM3:
        scm_hwpwm3();
        break;
 
    case IOMUX_SWPWM0:
        scm_swpwm0();
        break;
    case IOMUX_SWPWM1:
        scm_swpwm1();
        break;
 
    case IOMUX_PPS0:
        scm_pps0();
        break;
    case IOMUX_PPS1:
        scm_pps1();
        break;

    case IOMUX_CLK5M:
        scm_clk5m();
        break;
 
    case IOMUX_I2C0:
        scm_i2c0_en();
        break;
    case IOMUX_I2C1:
        scm_i2c1_en();
        break;

    case IOMUX_ODO:
        scm_odo_en();
        break;

    case IOMUX_IRIGB:
        scm_irigb_en();
        break;
 
    case IOMUX_SIM0:
        scm_sim_en(0);
        break;
    case IOMUX_SIM1:
        scm_sim_en(1);
        break;
    case IOMUX_SIM2:
        scm_sim_en(2);
        break;

    case IOMUX_ASRAM:
        scm_asram_en();
        break;

    case IOMUX_UART0:
        scm_uart_en(0, 0);
        break;
    case IOMUX_UART1:
        scm_uart_en(1, 0);
        break;
    case IOMUX_UART2:
        scm_uart_en(2, 0);
        break;
    case IOMUX_UART3:
        scm_uart_en(3, 0);
        break;
    case IOMUX_UART4:
        scm_uart_en(4, 0);
        break;
    case IOMUX_UART5:
        scm_uart_en(5, 0);
        break;
    case IOMUX_UART6:
        scm_uart_en(6, 1);
        break;
    case IOMUX_UART7:
        scm_uart_en(7, 0);
        break;
    case IOMUX_ID_END:
        break;
    }
    cyg_mutex_unlock(&iomux_mutex);
    return true;
}
