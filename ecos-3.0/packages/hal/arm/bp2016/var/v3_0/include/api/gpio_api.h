#ifndef __GPIO_API_H__
#define __GPIO_API_H__

// INDEX
#define HWP_GPIO_PORT_MAX          (128)

// 
typedef enum {
    GPIO_INT_RISING_EDGE = 0,
    GPIO_INT_FALLING_EDGE = 1,
    GPIO_INT_HIGH_LEVEL = 2,
    GPIO_INT_LOW_LEVEL = 3,
} GPIO_INT_TYPE_E;

// global functions
/*
* Function Name: gpio_api_init
* Input Para:    void
* Return value:  void
*/
void gpio_api_init(void);

/*
* Function Name: gpio_api_set_output
* Input Para:    unsigned int gpio_num
* Return value:  bool-- true/false
*/
bool gpio_api_set_output(unsigned int gpio_num);

/*
* Function Name: gpio_api_set_high
* Input Para:    unsigned int gpio_num
* Return value:  bool-- true/false
*/
bool gpio_api_set_high(unsigned int gpio_num);

/*
* Function Name: gpio_api_set_low
* Input Para:    unsigned int gpio_num
* Return value:  bool-- true/false
*/
bool gpio_api_set_low(unsigned int gpio_num);

/*
* Function Name: gpio_api_set_input
* Input Para:    unsigned int gpio_num
* Return value:  bool-- true/false
*/
bool gpio_api_set_input(unsigned int gpio_num);

/*
* Function Name: gpio_api_get_data
* Input Para:    unsigned int gpio_num
*                cyg_uint32 *v-- store output data
* Return value:  bool-- true/false
*/
bool gpio_api_get_data(unsigned int gpio_num, cyg_uint32 *v);

/*
* Function Name: gpio_api_get_int_status
* Input Para:    unsigned int gpio_num
*                bool *ret
* Return value:  bool-- true/false
*/
bool gpio_api_get_int_status(unsigned int gpio_num, bool *ret);

/*
* Function Name: gpio_api_int_clear
* Input Para:    unsigned int gpio_num
* Return value:  void
*/
void gpio_api_int_clear(unsigned int gpio_num);
/*
* Function Name: gpio_api_set_irq
* Input Para:    unsigned int gpio_num
*                void *isr -- isr function
*                void *dsr -- dsr function
*                cyg_uint32 data -- data for isr and dsr
*                GPIO_INT_TYPE_E int_type -- int trigger type
* Return value:  bool-- true/false
*/
bool gpio_api_set_irq(unsigned int gpio_num, void *isr, void *dsr, cyg_uint32 data, GPIO_INT_TYPE_E int_type);




#endif /* __GPIO_API_H__ */
