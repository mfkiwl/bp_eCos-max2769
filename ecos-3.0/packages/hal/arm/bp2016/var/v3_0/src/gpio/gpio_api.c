#include <cyg/kernel/kapi.h>           // C API
#include <cyg/hal/drv_api.h>
#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h> // base types
#include <cyg/hal/hal_if.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/plf/irq_defs.h>
#include <cyg/hal/regs/gpio.h>
#include <cyg/hal/gpio/gpio_if.h>
#include <cyg/hal/api/gpio_api.h>

typedef struct gpio_m {
    cyg_drv_mutex_t  lock;
    cyg_uint32  in_use_flag;
}GPIO_M_ST;

static bool gpio_api_init_flag = false;
static GPIO_M_ST  gpio_manager[4];

typedef struct bp2016_gpio_int_info {
    GPIO_INT_TYPE_E  int_type;
    cyg_interrupt  serial_interrupt;
    cyg_handle_t   serial_interrupt_handle;
} GPIO_INT_M_ST;

GPIO_INT_M_ST  gpio_int_manager_st[32];

void gpio_api_init(void)
{
    int i;
    if(true == gpio_api_init_flag) return;
    for(i = 0; i < 4; i++){
        gpio_manager[i].in_use_flag = 0;
        cyg_drv_mutex_init(&gpio_manager[i].lock);
    }
    //gpio_debounce_enable();
    *(volatile unsigned int *)(0xc0130048) = 0xFFFFFFFF;
    gpio_api_init_flag = true;
}

bool gpio_api_set_output(unsigned int gpio_num)
{
    bool ret;
    cyg_uint32 index;
    if(false == gpio_api_init_flag) return false;
    if(gpio_num > 127) return false;
    index = gpio_num / 32;

    cyg_drv_mutex_lock(&gpio_manager[index].lock);
    ret = gpio_set_output_direction(gpio_num);
    cyg_drv_mutex_unlock(&gpio_manager[index].lock);
    return ret;
}

bool gpio_api_set_high(unsigned int gpio_num)
{
    bool ret;
    cyg_uint32 index;
    if(false == gpio_api_init_flag) return false;
    if(gpio_num > 127) return false;
    index = gpio_num / 32;

    cyg_drv_mutex_lock(&gpio_manager[index].lock);
    ret = gpio_set_output_high(gpio_num);
    cyg_drv_mutex_unlock(&gpio_manager[index].lock);
    return ret;
}

bool gpio_api_set_low(unsigned int gpio_num)
{
    bool ret;
    cyg_uint32 index;
    if(false == gpio_api_init_flag) return false;
    if(gpio_num > 127) return false;
    index = gpio_num / 32;

    cyg_drv_mutex_lock(&gpio_manager[index].lock);
    ret = gpio_set_output_low(gpio_num);
    cyg_drv_mutex_unlock(&gpio_manager[index].lock);
    return ret;
}

bool gpio_api_set_input(unsigned int gpio_num)
{
    bool ret;
    cyg_uint32 index;
    if(false == gpio_api_init_flag) return false;
    if(gpio_num > 127) return false;
    index = gpio_num / 32;

    cyg_drv_mutex_lock(&gpio_manager[index].lock);
    ret = gpio_set_input_direction(gpio_num);
    cyg_drv_mutex_unlock(&gpio_manager[index].lock);
    return ret;
}

bool gpio_api_get_data(unsigned int gpio_num, cyg_uint32 *v)
{
    bool ret;
    cyg_uint32 index;
    if(false == gpio_api_init_flag) return false;
    if((gpio_num > 127) || (NULL == v)) return false;
    index = gpio_num / 32;

    cyg_drv_mutex_lock(&gpio_manager[index].lock);
    ret = gpio_get_input_data(gpio_num, v);
    cyg_drv_mutex_unlock(&gpio_manager[index].lock);
    return ret;
}

bool gpio_api_get_int_status(unsigned int gpio_num, bool *ret)
{
    cyg_uint32 mask;
    if(gpio_num > 31) return false;

    mask = gpio_interupt_get_intstatus();
    if(mask & (0x1 << gpio_num)){
        *ret = true;
    }else{
        *ret = false;
    }
    return true;
}

void gpio_api_int_clear(unsigned int gpio_num)
{
    cyg_uint32 mask;
    if(gpio_num > 31) return;
    mask = (1 << gpio_num);
    gpio_interrupt_clr(mask);
}

bool gpio_api_set_irq(unsigned int gpio_num, void *isr, void *dsr, cyg_uint32 data, GPIO_INT_TYPE_E int_type)
{
    cyg_uint32 irq_num;
    if(false == gpio_api_init_flag) return false;
    if((gpio_num > 31) || (NULL == isr) || (NULL == dsr) || (GPIO_INT_LOW_LEVEL < int_type)) return false;
    cyg_drv_mutex_lock(&gpio_manager[0].lock);
    // set gpio to irq mode
    gpio_enable(gpio_num);
    gpio_enable_interrupt(gpio_num);
    gpio_set_input_direction(gpio_num);
    if(int_type < GPIO_INT_HIGH_LEVEL){
        gpio_interrupt_type_set_edge(gpio_num);
        if(GPIO_INT_RISING_EDGE == int_type)
            gpio_interrupt_polarity_set_rising_edge(gpio_num);
        else
            gpio_interrupt_polarity_set_falling_edge(gpio_num);
    } else {
        gpio_interrupt_type_set_level(gpio_num);
        if(GPIO_INT_LOW_LEVEL == int_type)
            gpio_interrupt_polarity_set_lowlevel(gpio_num);
        else
            gpio_interrupt_polarity_set_highlevel(gpio_num);
    }
    gpio_int_manager_st[gpio_num].int_type = int_type;
    irq_num = SYS_IRQ_ID_GPIO0 + gpio_num;
    cyg_drv_interrupt_create(irq_num,
            INT_PRI_DEFAULT,      // Priority - default
            (cyg_addrword_t)data, //  Data item passed to interrupt handler
            isr,
            dsr,
            &gpio_int_manager_st[gpio_num].serial_interrupt_handle,
            &gpio_int_manager_st[gpio_num].serial_interrupt);
    cyg_drv_interrupt_attach(gpio_int_manager_st[gpio_num].serial_interrupt_handle);
    cyg_drv_interrupt_unmask(irq_num);
    gpio_interrupt_unmask(gpio_num);
    cyg_drv_mutex_unlock(&gpio_manager[0].lock);
    return true;
}






