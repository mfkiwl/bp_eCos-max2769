#ifndef __MEMMAP_HGBP2018_H
#define __MEMMAP_HGBP2018_H

#include <cyg/hal/plf/iomap.h>

/*some memory definitons*/
#define CONFIG_BOOT_RSVADDR_KSIZE    (8)

#ifdef  CYGHWR_HAL_BP2018_FPGA_SRAM_512KB
  #define CONFIG_RAM0_BASE            IRAM_BASE
  #define CONFIG_OS_IRAM_KSIZE        (512)
#else //CYGHWR_HAL_BP2018_FPGA_SRAM_512KB, default sram is 2MB
  #define CONFIG_RAM0_BASE            IRAM_BASE
  #define CONFIG_OS_IRAM_KSIZE        (2048)
#endif //CYGHWR_HAL_BP2018_FPGA_SRAM_512KB
   
#define CONFIG_FIXED_VECT_RAM_KSIZE       (2)      /*real vect size + rsv_data size*/
#define CONFIG_RSV_DATA_COUNT             (1)      /*fixed size = 8KBytes*/ 

#define CONFIG_RAM0_KSIZE       (CONFIG_OS_IRAM_KSIZE - CONFIG_BOOT_RSVADDR_KSIZE - CONFIG_FIXED_VECT_RAM_KSIZE)
#define CONFIG_RAM1_BASE        (CONFIG_RAM0_BASE + CONFIG_RAM0_KSIZE * 1024)
#define CONFIG_RAM1_KSIZE       CONFIG_FIXED_VECT_RAM_KSIZE

# define CYGHWR_HAL_VECTOR_TABLE_BASE     CONFIG_RAM0_BASE

#endif /* __MEMMAP_HGBP2018_H */
