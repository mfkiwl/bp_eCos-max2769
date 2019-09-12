
#include <cyg/hal/regs/asram.h>

void asram_init(void)
{
    // set SMTMGR register
    // t_prc(22:19)--(b'000)
    // t_bta(18:16)--(b'001)
    // t_wp(15:10)--(b'0000 01)
    // t_wr(9:8)--(b'01)
    // t_as(7:6)--(b'01)
    // t_rc(5:0)--(b'0000 01)
    hwp_apAsram->timing_r_set0 = 0x00010541;
}
