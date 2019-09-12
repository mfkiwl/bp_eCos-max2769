#include <cyg/kernel/kapi.h>
#include <pkgconf/hal.h>

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>

#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info


static cyg_mutex_t gpiolock;

extern bool gpio_set_output_direction(unsigned int);
extern bool gpio_set_output_high(unsigned int);
extern bool gpio_set_output_low(unsigned int);
extern bool gpio_set_input_direction(unsigned int);
extern bool gpio_get_input_data(unsigned int, unsigned int *);
extern void gpio_interrupt_clr(int irq);
extern unsigned int gpio_interupt_get_intstatus(void);
extern bool gpio_get_output_data(int gpio_num, unsigned int *v);

/******************************clk*************************/
void  hal_gpio_init(void)
{
    cyg_mutex_init(&gpiolock);
}

bool hal_gpio_set_output(unsigned int gpio_num)
{
    bool ret;
    if(gpio_num > 127) return false;

    cyg_mutex_lock(&gpiolock); 
    ret = gpio_set_output_direction(gpio_num);
    cyg_mutex_unlock(&gpiolock);
    return ret;
}

bool hal_gpio_set_high(unsigned int gpio_num)
{
    bool ret;
    if(gpio_num > 127) return false;

    cyg_mutex_lock(&gpiolock); 
    ret = gpio_set_output_high(gpio_num);
    cyg_mutex_unlock(&gpiolock);
    return ret;
}

bool hal_gpio_set_low(unsigned int gpio_num)
{
    bool ret;
    cyg_mutex_lock(&gpiolock); 
    ret = gpio_set_output_low(gpio_num);
    cyg_mutex_unlock(&gpiolock);
    return ret;
}

bool hal_gpio_set_input(unsigned int gpio_num)
{
    bool ret;
    if(gpio_num > 127) return false;

    cyg_mutex_lock(&gpiolock); 
    ret = gpio_set_input_direction(gpio_num);
    cyg_mutex_unlock(&gpiolock); 
    return ret;
}

bool hal_gpio_get_input_data(unsigned int gpio_num, cyg_uint32 *v)
{
    bool ret;
    if((gpio_num > 127) || (NULL == v)) return false;

    cyg_mutex_lock(&gpiolock); 
    ret = gpio_get_input_data(gpio_num, v);
    cyg_mutex_unlock(&gpiolock); 
    return ret;
}

bool hal_gpio_get_output_data(unsigned int gpio_num, cyg_uint32 *v)
{
    bool ret;
    if((gpio_num > 127) || (NULL == v)) return false;

    cyg_mutex_lock(&gpiolock); 
    ret = gpio_get_output_data(gpio_num, v);
    cyg_mutex_unlock(&gpiolock); 
    return ret;
}

bool hal_gpio_get_int_status(unsigned int gpio_num)
{
    cyg_uint32 mask;
    if(gpio_num > 31) return false;
    mask = gpio_interupt_get_intstatus();
    if(mask & (0x1 << gpio_num))
        return true;
    return false;
}

void hal_gpio_int_clear(unsigned int gpio_num)
{
    cyg_uint32 mask;
    if(gpio_num > 31) return;
    mask = (1 << gpio_num);
    gpio_interrupt_clr(mask);
}

