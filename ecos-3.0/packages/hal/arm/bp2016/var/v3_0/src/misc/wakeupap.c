#include <pkgconf/hal.h>
#include <string.h>

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>

#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/plf/chip_config.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/hal/reset/reset.h>
#include <cyg/hal/misc/mbr_if.h>
#include <cyg/hal/misc/auth_if.h>
#include <cyg/hal/misc/bl2fw_if.h>
#include <cyg/hal/misc/rsv_save.h>
#include <cyg/hal/regs/scm.h>

struct reserved_area_data * reserved_save;
static void wakeup_cpu1(u32 exec_addr, u32 dtb_addr)
{
    u32 magic_v = CONFIG_AP_LINUX_DTB_START_MAGIC;
    if(dtb_addr == 0) 
        magic_v = 0;

    hwp_apSCM->cpu1right_ctrl = 0x1fffffff; //access permission
    hwp_apSCM->cpu1trap_addr = 0xFFFFFFFF;

    hwp_apSCM->cpu1zero_addr = 0xf000;
    hwp_apSCM->cpu1rom_baddr = 0xf000;
    hwp_apSCM->cpu1rom_eaddr = 0x10000;  

    hwp_apSCM->rsv_1160_11ec[0] = magic_v;
    hwp_apSCM->rsv_1160_11ec[1] = dtb_addr;
    hwp_apSCM->sw_ap_jaddr = exec_addr;
    hwp_apSCM->sw_ap_magic = CONFIG_AP_FASTBOOT_JUMP_MAGIC;

    diag_printf("de-assert cpu1 reset\n");
    hal_sw_reset(SWRST_CPU1);
}

void boot_ap_fw_ecos2linux(void)
{
    struct  bl2fw_rsv_data *apfw;
    reserved_save =(struct reserved_area_data*)CONFIG_DATA_RESEVED_ADDR ;
    apfw = &reserved_save->apfw;
    if((apfw->valid == 1) && (apfw->err_return == BL2FW_SUCCESS)) {
         if(apfw->exec_addr != apfw->load_addr) {
            diag_printf("1. memcpy ...\n");
            memcpy((void*)apfw->exec_addr, (void*)apfw->load_addr, apfw->dsize);
            diag_printf("2. flush\n\n\n");
            v7_dma_flush_range(apfw->exec_addr, apfw->exec_addr + apfw->dsize);
        }
        wakeup_cpu1(apfw->exec_addr, 0);
    }
}

