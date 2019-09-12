#ifndef __MEMMAP_HGBP2016_H
#define __MEMMAP_HGBP2016_H

#include <cyg/hal/plf/iomap.h>

/*some memory definitons*/

#define CONFIG_IRAM_KSIZE            (2048)
#define CONFIG_CPU_SHARE_MEM_KSIZE   (120)  // not use for now wwzz
#define CONFIG_BOOT_RSVADDR_KSIZE    (8)
#define CONFIG_BP_IRAM_KSIZE         (1024 * 2)

// not use for now wwzz  only for AP
#define CONFIG_AP_IRAM_KSIZE         (CONFIG_IRAM_KSIZE - CONFIG_BP_IRAM_KSIZE - CONFIG_CPU_SHARE_MEM_KSIZE - CONFIG_BOOT_RSVADDR_KSIZE)
#define CONFIG_CPU_SHARE_MEM_START (IRAM_BASE + (CONFIG_BP_IRAM_KSIZE + CONFIG_AP_IRAM_KSIZE) * 1024) 

#define CONFIG_RAM0_BASE            IRAM_BASE
#define CONFIG_OS_IRAM_KSIZE        CONFIG_BP_IRAM_KSIZE 

   
#define CONFIG_FIXED_VECT_RAM_KSIZE       (2)      /*real vect size + rsv_data size*/
#define CONFIG_RSV_DATA_COUNT             (1)      /*fixed size = 8KBytes*/ 

#define CONFIG_RAM0_KSIZE       (CONFIG_OS_IRAM_KSIZE - CONFIG_BOOT_RSVADDR_KSIZE - CONFIG_FIXED_VECT_RAM_KSIZE)
#define CONFIG_RAM1_BASE        (CONFIG_RAM0_BASE + CONFIG_RAM0_KSIZE * 1024)
#define CONFIG_RAM1_KSIZE       CONFIG_FIXED_VECT_RAM_KSIZE

# define CYGHWR_HAL_VECTOR_TABLE_BASE     CONFIG_RAM0_BASE

#endif /* __MEMMAP_HGBP2016_H */
