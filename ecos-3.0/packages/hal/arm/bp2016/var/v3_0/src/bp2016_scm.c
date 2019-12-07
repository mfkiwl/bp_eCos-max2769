#include <cyg/kernel/kapi.h>
#include <pkgconf/hal.h>

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>

#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/clk/clk.h>           
#include <cyg/hal/reset/reset.h>           

extern int clk_get_rate_kHz(CLK_ID_TYPE_T id);
extern int clk_set_rate_kHz(CLK_ID_TYPE_T id, int rate_kHz);
extern int clk_disable(CLK_ID_TYPE_T id);
extern int clk_enable(CLK_ID_TYPE_T id);

static cyg_mutex_t clklock;
static cyg_mutex_t rstlock;

/******************************clk*************************/
void  hal_clk_init(void)
{
    cyg_mutex_init(&clklock);
}

int hal_clk_get_rate_kHz(CLK_ID_TYPE_T id, int *rate_kHz)
{
    int ret;
    cyg_mutex_lock(&clklock); 
    ret = clk_get_rate_kHz(id);
    cyg_mutex_unlock(&clklock);

    if(ret >= 0) {
        if(rate_kHz != NULL)
            *rate_kHz = ret;
       ret = CLK_OP_SUCCESS;
    }
    return ret;
}

int hal_clk_set_rate_kHz(CLK_ID_TYPE_T id, int rate_kHz)
{
    int ret;
    cyg_mutex_lock(&clklock); 
    ret = clk_set_rate_kHz(id, rate_kHz);
    cyg_mutex_unlock(&clklock);
    return ret;
}

int hal_clk_disable(CLK_ID_TYPE_T id)
{
    int ret;
    // diag_printf("hal_clk_disable id = %d\n", id);
    cyg_mutex_lock(&clklock); 
    ret = clk_disable(id);
    cyg_mutex_unlock(&clklock);
    return ret;
}

int hal_clk_enable(CLK_ID_TYPE_T id)
{
    int ret;
    // diag_printf("hal_clk_enable id = %d\n", id);
    cyg_mutex_lock(&clklock); 
    ret = clk_enable(id);
    cyg_mutex_unlock(&clklock);
    return ret;
}

/******************************reset*************************/
extern void swrst_cpu1_reset(void);
extern void swrst_ddr_reset(void);
extern void swrst_62M_reset(void);
extern void swrst_rom_reset(void);
extern void swrst_ram_reset(void);
extern void swrst_bb_reset(void);
extern void swrst_mp_reset(void);
extern void swrst_dmac0_reset(void);
extern void swrst_dmac1_reset(void);
extern void swrst_qspi_reset(void);
extern void swrst_sd_reset(void);
extern void swrst_uart0_reset(void);
extern void swrst_uart1_reset(void);
extern void swrst_uart2_reset(void);
extern void swrst_uart3_reset(void);
extern void swrst_uart4_reset(void);
extern void swrst_uart5_reset(void);
extern void swrst_uart6_reset(void);
extern void swrst_uart7_reset(void);
extern void swrst_i2c0_reset(void);
extern void swrst_i2c1_reset(void);
extern void swrst_spi0_reset(void);
extern void swrst_spi1_reset(void);
extern void swrst_spi2_reset(void);
extern void swrst_spi3_reset(void);
extern void swrst_sim0_reset(void);
extern void swrst_sim1_reset(void);
extern void swrst_sim2_reset(void);
extern void swrst_pwm_reset(void);
extern void swrst_gpio_reset(void);
extern void swrst_timer_reset(void);
extern void swrst_global_reset(void);

void  hal_reset_init(void)
{
    cyg_mutex_init(&rstlock);
}

void hal_sw_reset(SWRST_ID_TYPE_T id)
{
    cyg_mutex_lock(&rstlock);
    switch(id) {
    case SWRST_CPU1: 
        swrst_cpu1_reset();
        break;
    case SWRST_DDR: 
        swrst_ddr_reset();
        break;
    case SWRST_62M: 
        swrst_62M_reset();
        break;
    case SWRST_ROM: 
        swrst_rom_reset();
        break;
    case SWRST_RAM: 
        swrst_ram_reset();
        break;
    case SWRST_BB: 
        swrst_bb_reset();
        break;
    case SWRST_MP: 
        swrst_mp_reset();
        break;
    case SWRST_DMAC0: 
        swrst_dmac0_reset();
        break;
    case SWRST_DMAC1: 
        swrst_dmac1_reset();
        break;
    case SWRST_QSPI: 
        swrst_qspi_reset();
        break;
    case SWRST_SD: 
        swrst_sd_reset();
        break;
    case SWRST_UART0: 
        swrst_uart0_reset();
        break;
    case SWRST_UART1: 
        swrst_uart1_reset();
        break;
    case SWRST_UART2: 
        swrst_uart2_reset();
        break;
    case SWRST_UART3: 
        swrst_uart3_reset();
        break;
    case SWRST_UART4: 
        swrst_uart4_reset();
        break;
    case SWRST_UART5: 
        swrst_uart5_reset();
        break;
    case SWRST_UART6: 
        swrst_uart6_reset();
        break;
    case SWRST_UART7: 
        swrst_uart7_reset();
        break;
    case SWRST_I2C0: 
        swrst_i2c0_reset();
        break;
    case SWRST_I2C1: 
        swrst_i2c1_reset();
        break;
    case SWRST_SPI0: 
        swrst_spi0_reset();
        break;
    case SWRST_SPI1: 
        swrst_spi1_reset();
        break;
    case SWRST_SPI2: 
        swrst_spi2_reset();
        break;
    case SWRST_SPI3: 
        swrst_spi3_reset();
        break;
    case SWRST_SIM0: 
        swrst_sim0_reset();
        break;
    case SWRST_SIM1: 
        swrst_sim1_reset();
        break;
    case SWRST_SIM2: 
        swrst_sim2_reset();
        break;
    case SWRST_PWM: 
        swrst_pwm_reset();
        break;
    case SWRST_GPIO: 
        swrst_gpio_reset();
        break;
    case SWRST_TIMER: 
        swrst_timer_reset();
        break;
    case SWRST_GLOBAL:
        swrst_global_reset();   //not return, and the whole system reboot
        break;
    default:
        break;
    }
    cyg_mutex_unlock(&rstlock);
}

