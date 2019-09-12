#include <pkgconf/hal.h>
#include <cyg/io/devtab.h>

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>

#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // necessary?
#include <cyg/hal/a7/cortex_a7.h>


extern unsigned char * _stext, *_etext, *__exception_stack_base, *__exception_stack;
extern unsigned char * cyg_interrupt_stack_base, * cyg_interrupt_stack;
extern unsigned char * __startup_stack_base, *__startup_stack;
extern unsigned char * __ram_data_start, *__ram_data_end;

bool kernel_text_address(cyg_uint32 addr)
{
    if(addr >= (cyg_uint32)&_stext && addr < (cyg_uint32)&_etext)
        return true;
    
    return false;
}

bool kernel_sp_addr_cross(cyg_uint32 addr)
{
    if(addr >= (cyg_uint32)&__exception_stack_base && addr < (cyg_uint32)&__exception_stack)
            return true;
    if(addr >= (cyg_uint32)&cyg_interrupt_stack_base && addr < (cyg_uint32)&cyg_interrupt_stack)
            return true;
    if(addr >= (cyg_uint32)&__startup_stack_base && addr < (cyg_uint32)&__startup_stack)
            return true;
    return false;
}

//for pabort/undef
static inline u32 v7_get_ifar(void)
{
	u32 ifar;
	asm volatile("mrc p15, 0, %0, c6, c0, 2" : "=r" (ifar));
	return ifar;
}

static inline u32 v7_get_ifsr(void)
{
	u32 ifsr;
	asm volatile("mrc p15, 0, %0, c5, c0, 1" : "=r" (ifsr));
	return ifsr;
}

//for dabort
static inline u32 v7_get_dfar(void)
{
	u32 dfar;
	asm volatile("mrc p15, 0, %0, c6, c0, 0" : "=r" (dfar));
	return dfar;
}

static inline u32 v7_get_dfsr(void)
{
	u32 dfsr;
	asm volatile("mrc p15, 0, %0, c5, c0, 0" : "=r" (dfsr));
	return dfsr;
}

static void print_cp15_debug_reg(EXCEPTION_ENUM ex)
{
    switch(ex)
    {
    case ERR_UNDEF_INT:
    case ERR_PREFETCH:
        diag_printf("IFAR 0x%08x, IFSR 0x%08x\n", 
                v7_get_ifar(), v7_get_ifsr());
        break;
    case ERR_SOFT_INT:
    case ERR_DATA_ABORT:
        diag_printf("DFAR 0x%08x DFSR 0x%08x\n",
                v7_get_dfar(), v7_get_dfsr());
        break;
    default:
        break;
    }
}

//    if(vector == 107)
//        cyg_hg_dump_frame(frame);
void cyg_hg_dump_frame(unsigned char *frame, EXCEPTION_ENUM exp)
{
    HAL_SavedRegisters *rp = (HAL_SavedRegisters *)frame;
    int i;
    static  int init__ = 0;
    if(init__ == 0) {
        init__ = 1;
        diag_printf("_stext = 0x%x\n", (cyg_uint32)&_stext);
        diag_printf("_etext = 0x%x\n", (cyg_uint32)&_etext);
        diag_printf("__exception_stack_base = 0x%x\n", (cyg_uint32)&__exception_stack_base);
        diag_printf("__exception_stack = 0x%x\n", (cyg_uint32)&__exception_stack);
        diag_printf("__interrupt_stack_base = 0x%x\n", (cyg_uint32)&cyg_interrupt_stack_base);
        diag_printf("__interrupt_stack = 0x%x\n", (cyg_uint32)&cyg_interrupt_stack);
        diag_printf("__startup_stack_base = 0x%x\n", (cyg_uint32)&__startup_stack_base);
        diag_printf("__startup_stack = 0x%x\n", (cyg_uint32)&__startup_stack);
        diag_printf("__ram_data_start= 0x%x\n", (cyg_uint32)&__ram_data_start);
        diag_printf("__ram_data_end= 0x%x\n", (cyg_uint32)&__ram_data_end);
        print_cp15_debug_reg(exp);
    }

    diag_printf("Registers:\n");
    for (i = 0;  i <= 10;  i++) {
        if ((i == 0) || (i == 6)) diag_printf("R%d: ", i);
        diag_printf("%08X ", rp->d[i]);
        if ((i == 5) || (i == 10)) diag_printf("\n");
    }
    diag_printf("FP: %08X, SP: %08X, LR: %08X, PC: %08X, PSR: %08X\n",
            rp->fp, rp->sp, rp->lr, rp->pc, rp->cpsr);

	if((rp->sp >0 ) && ((rp->sp & 7) == 0)) {
		if(kernel_sp_addr_cross(rp->sp) == true) 
			diag_printf("SP is in kernel space\r\n");
		else
			diag_printf("SP is in thread space\r\n");
	}
	else 
		diag_printf("SP invalid\r\n");

    if(kernel_text_address(rp->pc) == false)
        diag_printf("PC is invalid address\r\n");
    else
        diag_printf("PC is valid\r\n");

    if(kernel_text_address(rp->lr) == false)
        diag_printf("LR is invalid address\r\n");
    else
        diag_printf("LR is valid\r\n");

    diag_printf("\r\n");
}
