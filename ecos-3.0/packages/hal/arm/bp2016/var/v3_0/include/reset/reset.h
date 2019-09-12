#if !defined(__SWRST_ID__H)
#define __SWRST_ID__H
#include <cyg/infra/cyg_type.h>

typedef enum {
    SWRST_CPU1,       
    SWRST_DDR,        
    SWRST_62M,        
    SWRST_ROM,        
    SWRST_RAM,        
    SWRST_BB,         
    SWRST_MP,         
    SWRST_DMAC0,      
    SWRST_DMAC1,      
    SWRST_QSPI,       
    SWRST_SD,         
    SWRST_UART0,      
    SWRST_UART1,      
    SWRST_UART2,      
    SWRST_UART3,      
    SWRST_UART4,      
    SWRST_UART5,      
    SWRST_UART6,      
    SWRST_UART7,      
    SWRST_I2C0,       
    SWRST_I2C1,       
    SWRST_SPI0,       
    SWRST_SPI1,       
    SWRST_SPI2,       
    SWRST_SPI3,       
    SWRST_SIM0,       
    SWRST_SIM1,       
    SWRST_SIM2,       
    SWRST_PWM,        
    SWRST_GPIO,       
    SWRST_TIMER,     

    SWRST_GLOBAL,
} SWRST_ID_TYPE_T;

#if defined(__cplusplus)
extern "C" {
#endif
void hal_sw_reset(SWRST_ID_TYPE_T id);
#if defined(__cplusplus)
}
#endif


#endif
