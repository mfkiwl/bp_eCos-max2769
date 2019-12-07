// eCos memory layout

// =============================================================================

#ifndef __ASSEMBLER__
#include <cyg/infra/cyg_type.h>
#include <stddef.h>
#include <cyg/hal/mem_map.h>
#endif
#define CYGMEM_REGION_ram         CONFIG_RAM0_BASE  //sram start
#define CYGMEM_REGION_ram_SIZE    (CONFIG_RAM0_KSIZE * 1024) //2M
#define CYGMEM_REGION_ram_ATTR    (CYGMEM_REGION_ATTR_R | CYGMEM_REGION_ATTR_W)

#ifndef __ASSEMBLER__
extern char CYG_LABEL_NAME (__heap1) [];
#endif
#define CYGMEM_SECTION_heap1 (CYG_LABEL_NAME (__heap1))
#define CYGMEM_SECTION_heap1_SIZE (CYGMEM_REGION_ram + CYGMEM_REGION_ram_SIZE - (size_t) CYG_LABEL_NAME (__heap1))
