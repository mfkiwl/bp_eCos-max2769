#include <pkgconf/hal.h>
#include <string.h>

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>

#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/plf/chip_config.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/hal/reset/reset.h>
#include <cyg/hal/regs/scm.h>

#define FASTBOOT_MAX_SIZE        (CONFIG_SRAM_SIZE - 0x10000)
#define FASTBOOT_FW_RSV_ADDR     (CONFIG_SRAM_START)

#define EXEC_ADDR_IN_DDR(addr)   (((addr) >= CONFIG_DRAM_START) && ((addr) < CONFIG_DRAM_START + CONFIG_DRAM_SIZE))

typedef volatile struct{
    REG32  en;  // user set, api fastboot_reset
    REG32  type;  // copy or not
    REG32  src;  // copy src
    REG32  dst;  // copy dst
    REG32  len;  // copy len(bin len)
    REG32  jump_addr;  // bin exec addr
    REG32  flash_addr;  // bin store addr
    REG32  bak;  // main or bak
    REG32  boot_cnt;  // count of fastboot
} FASTBOOT_ST;

#define REG_FASTBOOT_BASE    (SCM_BASE + 0x1000)
#define hwp_fastboot         ((FASTBOOT_ST*) (REG_FASTBOOT_BASE))

#define  FASTBOOT_RESET          (0x55AA)
#define  FASTBOOT_EN             (0x5A5A)
#define  FASTBOOT_NOT_SUPPORT    (0xAAAA)
#define  FASTBOOT_COPY           (0xAA55)
#define  FASTBOOT_DIRECT         (0xA5A5)


extern void v7_inv_l1_l2_dcache(void);
extern void cyg_interrupt_disable( void );
extern void cache_disable(void);
extern void flush_dcache_all(void);

static unsigned long go_exec (unsigned long (*entry)(int), unsigned size)
{
    return entry(0);
}

void fastboot_reset(bool fastboot)
{
    // 1. disable irq
    cyg_interrupt_disable();
    //diag_printf("cyg_interrupt_disable done!\n");

    // 2. set para
    if(true == fastboot){
        hwp_fastboot->en = FASTBOOT_EN;
    }else{
        hwp_fastboot->en = 0x0;
        hwp_apSCM->sw_bp_magic = 0;
    }

    // 3. inv all cachea
    v7_inv_l1_l2_dcache();

    // 4. disable mmu
    cache_disable();

    // 5. go 0
    go_exec(0, 0);

}

