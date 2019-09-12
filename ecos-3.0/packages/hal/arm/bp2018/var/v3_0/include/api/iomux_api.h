#ifndef __IOMUX_API_H__
#define __IOMUX_API_H__

typedef enum {
    GPIO_GROUP_ID_A,
    GPIO_GROUP_ID_B,
    GPIO_GROUP_ID_C,
    GPIO_GROUP_ID_D,
}GPIO_GROUP_ID_T;

typedef enum {
   IOMUX_QSPI,       

   IOMUX_SPI0,       
   IOMUX_SPI3,       

   IOMUX_IF0,       
   IOMUX_IF1,       
   IOMUX_IF2,       
   IOMUX_IF3,       

   IOMUX_MMC0,       
   IOMUX_MMC1,       

   IOMUX_PRM0,
   IOMUX_PRM1,

   IOMUX_PWM0,
   IOMUX_PWM1,
   IOMUX_PWM2,
   IOMUX_PWM3,

   IOMUX_SWPWM0,
   IOMUX_SWPWM1,

   IOMUX_PPS0,
   IOMUX_PPS1,

   IOMUX_CLK5M,

   IOMUX_I2C0,       
   IOMUX_I2C1,       

   IOMUX_ODO,       

   IOMUX_IRIGB,       

   IOMUX_SIM0,       
   IOMUX_SIM1,       
   IOMUX_SIM2,       

   IOMUX_ASRAM,       

   IOMUX_UART0,      
   IOMUX_UART1,      
   IOMUX_UART2,      
   IOMUX_UART3,      
   IOMUX_UART4,      
   IOMUX_UART5,      
   IOMUX_UART6,      
   IOMUX_UART7,      

   IOMUX_ID_END,      
} IOMUX_ID_TYPE_T;

#if defined(__cplusplus)
extern "C" {
#endif

bool hal_iomux_spi1_en(int cs0, int cs1);
bool hal_iomux_spi2_en(int cs0, int cs1, int cs2, int cs3, int cs4, int cs5, int cs6, int cs7);
bool hal_iomux_spi4_en(int cs0, int cs1);

bool hal_iomux_gpio_en(GPIO_GROUP_ID_T group_id, int id);

bool hal_iomux_en(IOMUX_ID_TYPE_T id);
#if defined(__cplusplus)
}
#endif


#endif /* __IOMUX_API_H__ */
