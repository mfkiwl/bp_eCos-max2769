#ifndef IRQ_DEFS__H__
#define	IRQ_DEFS__H__

typedef enum
{
    SYS_IRQ_ID_TIMER0                      = 0,
    SYS_IRQ_ID_TIMER1                      = 1,
    SYS_IRQ_ID_TIMER2                      = 2,
    SYS_IRQ_ID_TIMER3                      = 3,

    SYS_IRQ_ID_UART0                       = 4,
    SYS_IRQ_ID_UART1                       = 5,
    SYS_IRQ_ID_UART2                       = 6,
    SYS_IRQ_ID_UART3                       = 7,
    SYS_IRQ_ID_UART4                       = 8,
    SYS_IRQ_ID_UART5                       = 9,
    SYS_IRQ_ID_UART6                       = 10,
    SYS_IRQ_ID_UART7                       = 11,

    SYS_IRQ_ID_SPI0                        = 12,
    SYS_IRQ_ID_SPI1                        = 13,
    SYS_IRQ_ID_SPI2                        = 14,
    SYS_IRQ_ID_SPI3                        = 15,

    SYS_IRQ_ID_I2C0                        = 16,
    SYS_IRQ_ID_I2C1                        = 17,
 
    SYS_IRQ_ID_SIM0                        = 18,
    SYS_IRQ_ID_SIM1                        = 19,
    SYS_IRQ_ID_SIM2                        = 20,

    SYS_IRQ_ID_GPIO0                      = 21,
    SYS_IRQ_ID_GPIO1                      = 22,
    SYS_IRQ_ID_GPIO2                      = 23,
    SYS_IRQ_ID_GPIO3                      = 24,
    SYS_IRQ_ID_GPIO4                      = 25,
    SYS_IRQ_ID_GPIO5                      = 26,
    SYS_IRQ_ID_GPIO6                      = 27,
    SYS_IRQ_ID_GPIO7                      = 28,
    SYS_IRQ_ID_GPIO8                      = 29,
    SYS_IRQ_ID_GPIO9                      = 30,
    SYS_IRQ_ID_GPIO10                     = 31,
    SYS_IRQ_ID_GPIO11                     = 32,
    SYS_IRQ_ID_GPIO12                     = 33,
    SYS_IRQ_ID_GPIO13                     = 34,
    SYS_IRQ_ID_GPIO14                     = 35,
    SYS_IRQ_ID_GPIO15                     = 36,
    SYS_IRQ_ID_GPIO16                     = 37,
    SYS_IRQ_ID_GPIO17                     = 38,
    SYS_IRQ_ID_GPIO18                     = 39,
    SYS_IRQ_ID_GPIO19                     = 40,
    SYS_IRQ_ID_GPIO20                     = 41,
    SYS_IRQ_ID_GPIO21                     = 42,
    SYS_IRQ_ID_GPIO22                     = 43,
    SYS_IRQ_ID_GPIO23                     = 44,
    SYS_IRQ_ID_GPIO24                     = 45,
    SYS_IRQ_ID_GPIO25                     = 46,
    SYS_IRQ_ID_GPIO26                     = 47,
    SYS_IRQ_ID_GPIO27                     = 48,
    SYS_IRQ_ID_GPIO28                     = 49,
    SYS_IRQ_ID_GPIO29                     = 50,
    SYS_IRQ_ID_GPIO30                     = 51,
    SYS_IRQ_ID_GPIO31                     = 52,

    SYS_IRQ_ID_SD                     	   = 53,
    SYS_IRQ_ID_QSPI                    	   = 54,
    SYS_IRQ_ID_WDT0                    	   = 55,
    SYS_IRQ_ID_WDT1                    	   = 56,
    SYS_IRQ_ID_DMAC0                   	   = 57,
    SYS_IRQ_ID_DMAC1                   	   = 58,

    SYS_IRQ_ID_SGI0			   = 59,
    SYS_IRQ_ID_SGI1			   = 60,
    SYS_IRQ_ID_SGI2			   = 61,
    SYS_IRQ_ID_SGI3			   = 62,
    SYS_IRQ_ID_SGI4			   = 63,
    SYS_IRQ_ID_SGI5			   = 64,
    SYS_IRQ_ID_SGI6			   = 65,
    SYS_IRQ_ID_SGI7			   = 66,
    SYS_IRQ_ID_SGI8			   = 67,
    SYS_IRQ_ID_SGI9			   = 68,
    SYS_IRQ_ID_SGI10			   = 69,
    SYS_IRQ_ID_SGI11			   = 70,
    SYS_IRQ_ID_SGI12			   = 71,
    SYS_IRQ_ID_SGI13			   = 72,
    SYS_IRQ_ID_SGI14			   = 73,
    SYS_IRQ_ID_SGI15			   = 74,
    SYS_IRQ_ID_SGI16			   = 75,
    SYS_IRQ_ID_SGI17			   = 76,
    SYS_IRQ_ID_SGI18			   = 77,
    SYS_IRQ_ID_SGI19			   = 78,
    SYS_IRQ_ID_SGI20			   = 79,
    SYS_IRQ_ID_SGI21			   = 80,
    SYS_IRQ_ID_SGI22			   = 81,
    SYS_IRQ_ID_SGI23			   = 82,
    SYS_IRQ_ID_SGI24			   = 83,
    SYS_IRQ_ID_SGI25			   = 84,
    SYS_IRQ_ID_SGI26			   = 85,
    SYS_IRQ_ID_SGI27			   = 86,
    SYS_IRQ_ID_SGI28			   = 87,
    SYS_IRQ_ID_SGI29			   = 88,
    SYS_IRQ_ID_SGI30			   = 89,
    SYS_IRQ_ID_SGI31			   = 90,
   
    SYS_IRQ_ID_AE_TE            = 91,
    SYS_IRQ_ID_MP_INT           = 92,
    SYS_IRQ_ID_VITERBI          = 93,
    SYS_IRQ_ID_EXT0             = 94,
    SYS_IRQ_ID_EXT1             = 95,
    SYS_IRQ_ID_PPS0             = 96,
    SYS_IRQ_ID_PPS1             = 97,
    SYS_IRQ_ID_LDPC0            = 98,
    SYS_IRQ_ID_LDPC1            = 99,
    SYS_IRQ_ID_IRIG             = 100,

} SYS_IRQ_ID_T;

#define NB_SYS_IRQ              (160)

typedef enum
{
    INT_PRIORITY_HIGHEST = 0x00,
    INT_PRIORITY_2 = 0x08,
    INT_PRIORITY_3 = 0x10,
    INT_PRIORITY_4 = 0x18,
    INT_PRIORITY_5 = 0x20,
    INT_PRIORITY_6 = 0x28,
    INT_PRIORITY_7 = 0x30,
    INT_PRIORITY_8 = 0x38,
    INT_PRIORITY_9 = 0x40,
    INT_PRIORITY_10 = 0x48,
    INT_PRIORITY_11 = 0x50,
    INT_PRIORITY_12 = 0x58,
    INT_PRIORITY_13 = 0x60,
    INT_PRIORITY_14 = 0x68,
    INT_PRIORITY_15 = 0x70,
    INT_PRIORITY_16 = 0x78,
    INT_PRIORITY_17 = 0x80,
    INT_PRIORITY_18 = 0x88,
    INT_PRIORITY_19 = 0x90,
    INT_PRIORITY_20 = 0x98,
    INT_PRIORITY_21 = 0xa0,
    INT_PRIORITY_22 = 0xa8,
    INT_PRIORITY_23 = 0xb0,
    INT_PRIORITY_24 = 0xb8,
    INT_PRIORITY_25 = 0xc0,
    INT_PRIORITY_26 = 0xc8,
    INT_PRIORITY_27 = 0xd0,
    INT_PRIORITY_28 = 0xd8,
    INT_PRIORITY_29 = 0xe0,
    INT_PRIORITY_30 = 0xe8,
    INT_PRIORITY_31 = 0xf0,
    INT_PRIORITY_LOWEST = 0xF8,
}INT_PRIORITY_E;

#define   INT_PRI_DEFAULT       (INT_PRIORITY_21)

#endif
