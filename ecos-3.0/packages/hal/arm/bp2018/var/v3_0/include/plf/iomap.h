#ifndef __IOMAP_HGBP2016_H
#define __IOMAP_HGBP2016_H

#define _ROM_BASE	    	(0x00000000) 
#define _SRAM_BASE	    	(0x00100000)
#define _ASRAM_BASE	    	(0x00400000)
#define _BB_BASE	    	(0x00500000)
#define _SPIFLASH_D_BASE       	(0x10000000)
#define _DDR_BASE       	(0x20000000)
#define	_GIC400_BASE	   	(0xa2000000)

#define _APB0_BASE           	(0xb0000000)	/*qspi/asram/sd/dmac0*/
#define _APB1_BASE           	(0xc0000000)    /*uart0~3, uart6~7/spi0~3/i2c0~1/wdt0~1*/
#define _APB2_BASE           	(0xc0100000)    /*sim0/gpio0/spi4/timer/pwm/ddr ctl/ddr phy/bb/scm*/
#define _AON_BASE               (0xf0000000)    /*sram/uart/scm/gpio/i2c/timer*/


#define APB0_BASE           _APB0_BASE
#define APB1_BASE           _APB1_BASE
#define APB2_BASE           _APB2_BASE
#define SPIFLASH_D_BASE       _SPIFLASH_D_BASE
#define ROM_BASE	    _ROM_BASE 
#define IRAM_BASE	    _SRAM_BASE
#define	GIC400_BASE	    _GIC400_BASE
#define AON_BASE        _AON_BASE
#define AON_SRAM        AON_BASE
#define AON_APB_BASE         (AON_BASE + 0x1000000)

#define	SCM_BASE	    (APB2_BASE + 0xd0000)
#ifndef CONFIG_USE_AP_UART_DEBUG 
#define	hwp_apUart	    	hwp_apUart0
#else
#define	hwp_apUart	    	hwp_apUart1
#endif

#define	hwp_dbgUart	   	    hwp_apUart0
#define	hwp_upvUart	   	    hwp_apUart1

#ifdef CONFIG_BUILD_AP_DVT
#define	hwp_apTimer	    	hwp_apTimer1
#else
#define	hwp_apTimer	    	hwp_apTimer0
#endif

#define hwp_apQspi	        hwp_apQspi0
//#define hwp_sim	        	hwp_sim0
//#define hwp_apWdt        	hwp_apWdt0
	

#endif /* __IOMAP_HGBP2016_H */
