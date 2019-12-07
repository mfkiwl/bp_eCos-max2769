#include <pkgconf/hal.h>

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>
#include <cyg/hal/plf/common_def.h> 
#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // necessary?
#include <cyg/hal/a7/cortex_a7.h>           // necessary?


static unsigned int  mmu_table_v[4096] __attribute__ ((aligned (16384)));

static unsigned long ttb_addr = (unsigned long)(mmu_table_v);
void hal_bbram_cache_disable(void);
/*      
 * CR1 bits (CP#15 CR1)
 */
#define CR_M    (1 << 0)        /* MMU enable                           */
#define CR_A    (1 << 1)        /* Alignment abort enable               */
#define CR_C    (1 << 2)        /* Dcache enable                        */
#define CR_W    (1 << 3)        /* Write buffer enable                  */
#define CR_P    (1 << 4)        /* 32-bit exception handler             */
#define CR_D    (1 << 5)        /* 32-bit data address range            */
#define CR_L    (1 << 6)        /* Implementation defined               */
#define CR_B    (1 << 7)        /* Big endian                           */
#define CR_S    (1 << 8)        /* System MMU protection                */
#define CR_R    (1 << 9)        /* ROM MMU protection                   */
#define CR_F    (1 << 10)       /* Implementation defined               */
#define CR_Z    (1 << 11)       /* Implementation defined               */
#define CR_I    (1 << 12)       /* Icache enable                        */
#define CR_V    (1 << 13)       /* Vectors relocated to 0xffff0000      */
#define CR_RR   (1 << 14)       /* Round Robin cache replacement        */
#define CR_L4   (1 << 15)       /* LDR pc can set T bit                 */
#define CR_DT   (1 << 16)
#define CR_IT   (1 << 18)
#define CR_ST   (1 << 19)
#define CR_FI   (1 << 21)       /* Fast interrupt (lower latency mode)  */
#define CR_U    (1 << 22)       /* Unaligned access operation           */
#define CR_XP   (1 << 23)       /* Extended page tables                 */
#define CR_VE   (1 << 24)       /* Vectored interrupts                  */
#define CR_EE   (1 << 25)       /* Exception (Big) Endian               */
#define CR_TRE  (1 << 28)       /* TEX remap enable                     */
#define CR_AFE  (1 << 29)       /* Access flag enable                   */
#define CR_TE   (1 << 30)       /* Thumb exception enable               */

#define PGTABLE_SIZE            (4096 * 4)

enum dcache_option {
        DCACHE_OFF = 0x12,
        DCACHE_WRITETHROUGH = 0x1a,
        DCACHE_WRITEBACK = 0x1e,
        DCACHE_WRITEALLOC = 0x16,
};

/* Size of an MMU section */
enum {
        MMU_SECTION_SHIFT       = 20,
        MMU_SECTION_SIZE        = 1 << MMU_SECTION_SHIFT,
};
 

static inline unsigned int get_cr(void)
{
        unsigned int val;
        asm("mrc p15, 0, %0, c1, c0, 0  @ get CR" : "=r" (val) : : "cc");
        return val;
}

static inline void set_cr(unsigned int val)
{
        asm volatile("mcr p15, 0, %0, c1, c0, 0 @ set CR"
          : : "r" (val) : "cc");
        isb();
}

static inline void dis_actlr_smp(void)
{
    /*
     * Disable actlr.SMP 
     */                                                                                                                                  
    asm volatile ("mrc   p15, 0, r0, c1, c0, 1");
    asm volatile ("bic   r0, r0, #(1 << 6)   @ Disable SMP/nAMP mode");
    asm volatile ("mcr   p15, 0, r0, c1, c0, 1");

    isb();
}

static void set_section_dcache(int section, enum dcache_option option)
{
        unsigned int *page_table = (unsigned int *)ttb_addr;
        unsigned int value;

        value = (section << MMU_SECTION_SHIFT) | (3 << 10);
        value |= option;
        page_table[section] = value;
}

void modify_section_dcache(int va, int pa, enum dcache_option option)
{
    unsigned int *page_table = (unsigned int *) ttb_addr;
    unsigned int value;

    value = (pa << MMU_SECTION_SHIFT) | (3 << 10);
    value |= option;
    page_table[va] = value;
}

static void dram_bank_mmu_setup(void)
{
    unsigned int start = CONFIG_DRAM_START;
    unsigned int size = CONFIG_DRAM_SIZE;
    int     i;

    for (i = start >> 20;
            i < (start >> 20) + (size >> 20);
            i++) {
        set_section_dcache(i, DCACHE_WRITEBACK);
    }
}

static void iram_mmu_setup(void)
{
    unsigned int start = CONFIG_SRAM_START;
    unsigned int size = CONFIG_SRAM_SIZE;
    int          i;
    int          section_start = start >> 20;
    int          section_num = (start >> 20) + ((size + 0xFFFFF) >> 20);

    for (i = section_start; i < section_num; i++) {
        set_section_dcache(i, DCACHE_WRITEBACK);
    }
}

/* Invalidate TLB */
static void v7_inval_tlb(void)
{
	/* Invalidate entire unified TLB */
	asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));
	/* Invalidate entire data TLB */
	asm volatile ("mcr p15, 0, %0, c8, c6, 0" : : "r" (0));
	/* Invalidate entire instruction TLB */
	asm volatile ("mcr p15, 0, %0, c8, c5, 0" : : "r" (0));

	dsb();
	isb();
}

unsigned int  hal_get_pagetable_base(void)
{
    return ttb_addr;
}

void bbiram_mmu_setup(void)
{
    unsigned int start = CONFIG_BB_MEM_START;
    unsigned int size = CONFIG_BB_MEM_SIZE;
    int          i;
    int          section_start = start >> 20;
    int          section_num = (start >> 20) + ((size + 0xFFFFF) >> 20);

    for (i = section_start; i < section_num; i++) {
        set_section_dcache(i, DCACHE_WRITEBACK);
    }
}

void per_common_mmu_setup(unsigned int start, unsigned int size)
{
    int          i;
    int          section_start = start >> 20;
    int          section_num = (start >> 20) + ((size + 0xFFFFF) >> 20);

    for (i = section_start; i < section_num; i++) {
        set_section_dcache(i, DCACHE_OFF);
    }
}

void dap_mmu_setup(void)
{
    unsigned int start = 0xA0000000;
    unsigned int size = 0x1000000;  // actually 16M space

    per_common_mmu_setup(start, size);
}

void gic_mmu_setup(void)
{
    unsigned int start = _GIC400_BASE;
    unsigned int size = 0x100000;  // actually 64K space

    per_common_mmu_setup(start, size);
}

void apb0_mmu_setup(void)
{
    unsigned int start = _APB0_BASE;
    unsigned int size = 0x100000;  // actually 1M space

    per_common_mmu_setup(start, size);
}

void apb1_mmu_setup(void)
{
    unsigned int start = _APB1_BASE;
    unsigned int size = 0x100000;  // actually 1M space

    per_common_mmu_setup(start, size);
}

void apb2_mmu_setup(void)
{
    unsigned int start = _APB2_BASE;
    unsigned int size = 0x100000;  // actually 1M space

    per_common_mmu_setup(start, size);
}

void asram_mmu_setup(void)
{
    unsigned int start = _ASRAM_BASE;
    unsigned int size = 0x100000;  // actually 16bytes space

    per_common_mmu_setup(start, size);
}

void qspi_mem_mmu_setup(void)
{
    unsigned int start = _SPIFLASH_D_BASE;
    unsigned int size = 0x2000000;  // actually 32M space

    per_common_mmu_setup(start, size);
}


void peripheral_mmu_setup(void)
{
    apb0_mmu_setup();
    apb1_mmu_setup();
    apb2_mmu_setup();
    //asram_mmu_setup();
    //qspi_mem_mmu_setup();
}

void mmu_setup(void)
{
    //int i;
    unsigned int reg;

    reg = get_cr(); /* get control reg. */
    if(reg & CR_M) {
        return ;
    }

#if 0
    /* Set up an identity-mapping for all 4GB, rw for everyone */
    for (i = 0; i < 4096; i++)
        set_section_dcache(i, DCACHE_OFF);
#else
    // set valid mmu item
    //dap_mmu_setup();
    gic_mmu_setup();
    peripheral_mmu_setup();
    hal_bbram_cache_disable();
#endif

#if defined(CYGHAL_RUN_ENV_DDR) || defined(CYGHAL_RUN_ENV_TEXT_SRAM_BSS_DDR)
    dram_bank_mmu_setup();
#endif
    iram_mmu_setup();
//    bbiram_mmu_setup();
    set_section_dcache(0, DCACHE_WRITEBACK );

    // wwzz add new item for debug
    modify_section_dcache(0xD05, 5, DCACHE_OFF);
    modify_section_dcache(0xD06, 6, DCACHE_OFF);
    modify_section_dcache(0xD07, 7, DCACHE_OFF);
    modify_section_dcache(0xD08, 8, DCACHE_OFF);
    

    /* Copy the page table address to cp15 */
    asm volatile("mcr p15, 0, %0, c2, c0, 0" 
            : : "r" (ttb_addr) : "memory");
    /* Set the access control to all-supervisor */
    asm volatile("mcr p15, 0, %0, c3, c0, 0"
            : : "r" (~0));

    /* and enable the mmu */
    reg = get_cr(); /* get control reg. */
    set_cr(reg | CR_M| CR_C | CR_I | CR_Z);
}

void hal_bbram_cache_enable(void) 
{
    unsigned int start = CONFIG_BB_MEM_START;
    unsigned int size = CONFIG_BB_MEM_SIZE;
    int          i;
    int          s = start >> 20;
    int          e = (start >> 20) + ((size + 0xFFFFF) >> 20);

    for (i = s; i < e; i++) {
        set_section_dcache(i, DCACHE_WRITEBACK);
    }
    v7_dma_clean_range(ttb_addr + s * 4, ttb_addr + e * 4);

    v7_inval_tlb();
}

void hal_bbram_cache_disable(void) 
{
    unsigned int start = CONFIG_BB_MEM_START;
    unsigned int size = CONFIG_BB_MEM_SIZE;
    int          i;
    int          s = start >> 20;
    int          e = (start >> 20) + ((size + 0xFFFFF) >> 20);

    for (i = s; i < e; i++) {
        set_section_dcache(i, DCACHE_OFF);
    }
    v7_dma_clean_range(ttb_addr + s * 4, ttb_addr + e * 4);

    v7_inval_tlb();
}
 
static void cp15_cache_disable(unsigned int cache_bit) 
{ 
    unsigned int reg; 
 
    reg = get_cr(); 
 
    /* if disabling data cache, disable mmu too */ 
    if ((cache_bit & reg) & CR_C) { 
        cache_bit |= CR_M; 
        //flush_dcache_all(); 
    } 
 
    set_cr(reg & ~cache_bit); 
}

void cache_disable(void)
{
    cp15_cache_disable(CR_I | CR_C);
    dis_actlr_smp();
}


