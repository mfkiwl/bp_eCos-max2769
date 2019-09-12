#ifndef __CHIP_CONFIG_H__
#define __CHIP_CONFIG_H__

#include <cyg/hal/plf/iomap.h>

#define CONFIG_COUNTER_FREQ		20000000	/* 20M  Hz */
#define CONFIG_TIMER_FREQ		95000000	/* 95M Hz */
#define	 TICKS_PER_US			(95)
#define  TICKS_PER_MS			(95000)
#define	 TICKS_PER_S			(95000000)

#define	 CONFIG_APB_CLOCK		(95000000)	
#define	 CONFIG_SERIAL_BAUDRATE		(115200)

#define tick_to_second(tick)		(tick/TICKS_PER_S) 
#define second_to_tick(second)		(second * TICKS_PER_S)
#define tick_to_ms(tick)		(tick/TICKS_PER_MS) 
#define ms_to_tick(ms)			(ms * TICKS_PER_MS)
#define tick_to_us(tick)		(tick/TICKS_PER_US)
#define us_to_tick(us)			(us * TICKS_PER_US)
#define archtimer_perus_to_tick	(20)


#define CONFIG_ROM_START	ROM_BASE	
#define CONFIG_ROM_SIZE		0x00010000	/* 64k */

#define CONFIG_SRAM_START	IRAM_BASE	
#define CONFIG_SRAM_SIZE	0x00200000	/* 2M */

#define CONFIG_DRAM_START       _DDR_BASE
#define CONFIG_DRAM_SIZE        0x08000000 /* 128M */

#define CONFIG_BB_MEM_START  0x00500000	
#define CONFIG_BB_MEM_SIZE	 0x00400000	/* 4MB */

#define	CONFIG_BP_FASTBOOT_JUMP_MAGIC	0xFABBE000		
#define	CONFIG_BP_FASTBOOT_JUMP_ADDR    (SCM_BASE + 0x1100)
#define	CONFIG_AP_FASTBOOT_JUMP_MAGIC	0xFAAE1000			
#define	CONFIG_AP_FASTBOOT_JUMP_ADDR    (SCM_BASE + 0x1108)

#define CONFIG_AP_LINUX_DTB_START_MAGIC   0xFAAE2000

#define	CONFIG_DATA_RESEVED_SIZE	(0x2000)      /* 8k */
#define	CONFIG_DATA_RESEVED_ADDR	(CONFIG_SRAM_START + 0x200000 - CONFIG_DATA_RESEVED_SIZE) 

#endif // __CHIP_CONFIG_H__

