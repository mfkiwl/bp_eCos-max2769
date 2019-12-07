#include <cyg/kernel/kapi.h>           // C API
#include <cyg/io/devtab.h>
#include <cyg/hal/drv_api.h>
#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h> // base types
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_if.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/plf/iomap.h>
#include <cyg/hal/plf/irq_defs.h>
#include <cyg/hal/regs/dma.h>
#include <cyg/hal/reset/reset.h>
#include <cyg/hal/api/dma_api.h>
#include <stdio.h>
#include <pkgconf/io_common_dma_arm_bp2016.h>
#ifdef CYGHWR_HAL_ASIC_CLK
#include <cyg/hal/clk/clk.h>
#endif

// DMAC Peripheral enum
typedef enum {
    DMAC_PER_E_QSPI_TX = 0,
    DMAC_PER_E_QSPI_RX,
    DMAC_PER_E_SPI0_TX = 2,
    DMAC_PER_E_SPI0_RX,
    DMAC_PER_E_SPI3_TX = 4,
    DMAC_PER_E_SPI3_RX,
    DMAC_PER_E_UART6_TX = 6,
    DMAC_PER_E_UART6_RX,
    DMAC_PER_E_UART7_TX = 8,
    DMAC_PER_E_UART7_RX,
    DMAC_PER_E_I2C0_TX = 10,
    DMAC_PER_E_I2C0_RX,
    DMAC_PER_E_I2C1_TX = 12,
    DMAC_PER_E_I2C1_RX,
    DMAC_PER_E_UART1_TX = 14,
    DMAC_PER_E_UART1_RX,
    DMAC_PER_E_MEM,
    DMAC_PER_E_UNKNOWN,
}DMAC_PER_E;

typedef struct dma_slave_config {
    DMA_TRANS_DIR_E direction;
    cyg_uint32 src_addr;
    cyg_uint32 dst_addr;
    cyg_uint32 len;
    DMAC_CTL_TR_WIDTH_E src_addr_width;
    DMAC_CTL_TR_WIDTH_E dst_addr_width;
    DMAC_CTL_MSIZE_E src_burst;
    DMAC_CTL_MSIZE_E dst_burst;
    bool sync;
    DMAC_SLAVE_ID_E  slave_id;
}DMA_CONFIG_ST;

typedef struct dma_ch_manager{
    cyg_uint32 dmac_id;
    cyg_uint32 ch;
    volatile cyg_uint32 trans_err;
    cyg_uint32  int_unmask;
    cyg_mutex_t  mt;
    bool  ch_valid;  // get/free
    volatile bool  ch_idle;  // ch status
    DMA_CONFIG_ST config;
    cyg_sem_t sem;
    cyg_uint32 blk_ts;
    cyg_uint32 tt_fc;
    cyg_uint32 src_m_type;
    cyg_uint32 dst_m_type;
    cyg_uint32 src_addr_inc;
    cyg_uint32 dst_addr_inc;
    cyg_uint32 src_per_type;
    cyg_uint32 dst_per_type;
}DMA_CH_M;

typedef struct dmac_controller{
    cyg_uint32  id;
    cyg_uint32  base_addr;
    cyg_uint32  irq_num;
    cyg_uint32  irq_pri;
    SWRST_ID_TYPE_T  rwt;
}DMAC_PRI;

typedef struct dmac_manager{
    cyg_mutex_t  mt;
    cyg_interrupt  dmac_interrupt;
    cyg_handle_t   dmac_interrupt_handle;
    DMAC_PRI    *pri;
    DMA_CH_M    dma_ch_handle[HW_DMAC_CH_NUM_MAX];
}DMAC_M;

#if 1 == CYGPKG_DEVS_DMA_ARM_BP2016_DEBUG_LEVEL
  #define  dma_printf(...)  diag_printf(...)
#else
  #define  dma_printf(...)
#endif

#ifdef CYGNUM_DEVS_ARM_BP2016_SELECT_DMAC0
  static DMAC_PRI dmac_controller_pri = {
        0,
        HW_DMAC_CONTROLLER_BASE0,
        SYS_IRQ_ID_DMAC0,
        CYGNUM_DEVS_DMA_ARM_BP2016_INTPRIO,
        SWRST_DMAC0,
    };

#else
  static DMAC_PRI dmac_controller_pri = {
        1,
        HW_DMAC_CONTROLLER_BASE1,
        SYS_IRQ_ID_DMAC1,
        CYGNUM_DEVS_DMA_ARM_BP2016_INTPRIO,
        SWRST_DMAC1,
    };
#endif 

static bool    dmac_reset_flag = false;
static DMAC_M  dmac_manager;
//static void dma_reset(DMA_CH_M *handle);
static bool      dma_init(struct cyg_devtab_entry *tab);
static Cyg_ErrNo dma_lookup(struct cyg_devtab_entry **tab,
                            struct cyg_devtab_entry *st,
                            const char *name);

CHAR_DEVIO_TABLE(dma_handler,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL);


CHAR_DEVTAB_ENTRY(dma_device,
                  "dev/dma",
                  NULL,          // Base device name
                  &dma_handler,
                  dma_init,
                  dma_lookup,
                  (void*) &dmac_manager);   // Private data pointer
#define SRC_VALID(src)  (true)
#define DST_VALID(dst)  (true)
#define DMAC_COMMONREG_INT_MASK  (0x10000)

static cyg_uint32 dmac_isr(cyg_vector_t vector, cyg_addrword_t data);
static void       dmac_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data);


extern inline void dmac_ch_disable(U32 id, U32 ch);
extern inline void dmac_int_enable(U32 id);
extern inline void dma_enable(U32 id);
extern inline void dmac_reset(U32 id);
extern inline U32 dmac_ch_idle(U32 id, U32 ch);
extern inline void dmac_ch_int_clear(DMAC_CH_CONTROLLER_ST *hwp_dma, DMAC_CH_INT_TYPE_E int_type);
extern inline void dmac_ch_sar_set(DMAC_CH_CONTROLLER_ST *hwp_dma, U32 addr);
extern inline void dmac_ch_dar_set(DMAC_CH_CONTROLLER_ST *hwp_dma, U32 addr);
extern inline void dmac_ch_block_ts_set(DMAC_CH_CONTROLLER_ST *hwp_dma, U32 block_ts);
extern inline void dmac_ch_int_disable_all(DMAC_CH_CONTROLLER_ST *hwp_dma);
extern inline void dmac_ch_int_status_ctl(DMAC_CH_CONTROLLER_ST *hwp_dma, DMAC_CH_INT_TYPE_E int_type, U32 en_or_dis);
extern inline void dmac_ch_int_signal_ctl(DMAC_CH_CONTROLLER_ST *hwp_dma, DMAC_CH_INT_TYPE_E int_type, U32 en_or_dis);
extern inline void dmac_ch_enable(U32 id, U32 ch);
extern inline DMAC_CH_CONTROLLER_ST * get_hwp_dma(U32 id, U32 ch_id);
extern inline void dmac_ch_int_enable(DMAC_CH_CONTROLLER_ST *hwp_dma, DMAC_CH_INT_TYPE_E int_type);
extern inline void dmac_ch_int_disable(DMAC_CH_CONTROLLER_ST *hwp_dma, DMAC_CH_INT_TYPE_E int_type);

#ifdef CYGHWR_HAL_ASIC_DRV_LOW_POWER    
cyg_thread_entry_t dis_dma_clk;
cyg_handle_t dis_dma_clk_thread;
char dis_dma_clk_stack[4096*2];		/* space for two 4K stacks */
cyg_thread dis_dma_clk_s;		/* space for two thread objects */
static cyg_sem_t syn_dis_dma_clk;

void dis_dma_clk(cyg_addrword_t data)
{
    CLK_ID_TYPE_T id_type = (CLK_ID_TYPE_T)data;
    DMA_CH_M *dma_ch = NULL;
    cyg_uint32 ch_id = 0;
    while(1){
        cyg_semaphore_wait(&syn_dis_dma_clk);
        for(ch_id = 0; ch_id < HW_DMAC_CH_NUM_MAX; ch_id++){ 
            dma_ch = &(dmac_manager.dma_ch_handle[ch_id]);
            if(true == cyg_mutex_trylock(&(dma_ch->mt))){
                if(true == (dma_ch->ch_idle)){
                    cyg_mutex_unlock(&(dma_ch->mt)); 
                } else {
                    cyg_mutex_unlock(&(dma_ch->mt)); 
                    break;
                }
            } else {
                break;
            }
        }
        if(HW_DMAC_CH_NUM_MAX == ch_id){
            hal_clk_disable(id_type);
        }
    } 
}
#endif

void DMA_init(void)
{
    U32 i, id;
    SWRST_ID_TYPE_T  rwt;
#ifdef CYGHWR_HAL_ASIC_DRV_LOW_POWER    
    CLK_ID_TYPE_T id_type = CLK_DMAC0;
#endif

    memset(&dmac_manager, 0x0, sizeof(DMAC_M));
    dmac_manager.pri = &dmac_controller_pri;

    id = dmac_manager.pri->id;
    /* controller reset */
    rwt = dmac_manager.pri->rwt;
    hal_sw_reset(rwt);

    cyg_mutex_init(&(dmac_manager.mt));
    /* init channel */
    for(i = 0; i < HW_DMAC_CH_NUM_MAX; i++){
        dmac_manager.dma_ch_handle[i].dmac_id = id;
        dmac_manager.dma_ch_handle[i].ch = i;
        cyg_mutex_init(&(dmac_manager.dma_ch_handle[i].mt));
        cyg_semaphore_init(&(dmac_manager.dma_ch_handle[i].sem), 0);
        dmac_manager.dma_ch_handle[i].ch_valid = true;
        dmac_manager.dma_ch_handle[i].ch_idle = true;
    }

    cyg_interrupt_create(dmac_manager.pri->irq_num,
            dmac_manager.pri->irq_pri,                    // Priority - unused
            (cyg_addrword_t)dmac_manager.pri, //  Data item passed to interrupt handler
            dmac_isr,
            dmac_dsr,
            &dmac_manager.dmac_interrupt_handle,
            &dmac_manager.dmac_interrupt);
    cyg_interrupt_attach(dmac_manager.dmac_interrupt_handle);
    cyg_interrupt_unmask(dmac_manager.pri->irq_num);

#ifdef CYGHWR_HAL_ASIC_DRV_LOW_POWER    
    cyg_semaphore_init(&syn_dis_dma_clk, 0);
    cyg_thread_create(0, dis_dma_clk, (cyg_addrword_t) id_type,
		 "for disable dma clk", (void *)dis_dma_clk_stack, 4096*2,
		 &dis_dma_clk_thread, &dis_dma_clk_s);

    cyg_thread_resume(dis_dma_clk_thread);
#endif

    dmac_int_enable(id);
    dma_enable(id);
}

static bool dma_init(struct cyg_devtab_entry *tab)
{
    dma_printf("dma init...\n");
    DMA_init();
    return ENOERR;
}

static Cyg_ErrNo dma_lookup(struct cyg_devtab_entry **tab,
           struct cyg_devtab_entry *st,
           const char *name)
{
    return ENOERR;
}
#if 1 == CYGNUM_DEVS_DMA_ARM_BP2016_CH_FIX
void * DMA_alloc(DMAC_SLAVE_ID_E slave_id)
{
    DMA_CH_M * dma_handle = NULL;
    cyg_uint32 i, loop_s, loop_e;

    if(slave_id >= DMAC_SLAVE_ID_E_UNKNOWN){
        return NULL;
    }

    if(true == dmac_reset_flag){
        return NULL;
    }

    cyg_mutex_lock(&(dmac_manager.mt));
    switch(slave_id){
        case DMAC_SLAVE_ID_E_QSPI: // qspiflash
        case DMAC_SLAVE_ID_E_MEM: // MEM TO MEM 
            loop_s = 0;
            loop_e = 4;
            break;

        default:
            loop_s = 4;
            loop_e = 8;
            break;
    }

    for(i = loop_s; i < loop_e; i++){
        dma_handle = &dmac_manager.dma_ch_handle[i];
        if((true == dma_handle->ch_idle) && (true == dma_handle->ch_valid)){
            dma_handle->ch_valid = false;
            dma_handle->config.slave_id = slave_id;
            dma_handle->config.sync = true;
            break;
        }
    }

    // loop end, no match channel, return NULL
    if(i == loop_e){
        dma_handle = NULL;
    }

    cyg_mutex_unlock(&(dmac_manager.mt));
    return dma_handle;
}
#else
void * DMA_alloc(DMAC_SLAVE_ID_E slave_id)
{
    if(true == dmac_reset_flag){
        return NULL;
    }
    return NULL;
}
#endif
cyg_uint32 DMA_free(void * dma_handle)
{
    cyg_uint32 ret = DMAC_ERR_E_OK;
    DMA_CH_M *handle = NULL;
    if (NULL == dma_handle)
        return DMAC_ERR_E_PARA_INVALID;

    handle = (DMA_CH_M *)dma_handle;
    cyg_mutex_lock(&(dmac_manager.mt));
    if (false == handle->ch_idle){
        ret = DMAC_ERR_E_CH_BUSY;
        goto out;
    }

    if (true == handle->ch_valid){
        ret = DMAC_ERR_E_FREE_NOT_ALLOC_CH;
        goto out;
    }

    handle->ch_valid = true;
    handle->ch_idle = true;

out:
    cyg_mutex_unlock(&(dmac_manager.mt));
    dma_handle = NULL;
    return ret;
}

static cyg_uint32 dma_wait_ch_done(DMA_CH_M *handle)
{
    bool ret;
    // wait semaphore here
  #ifdef CYGFUN_KERNEL_THREADS_TIMER
    ret =  cyg_semaphore_timed_wait(&handle->sem, cyg_current_time()+100);
  #else
    ret = cyg_semaphore_wait(&handle->sem);
  #endif
    if(true == ret) {
        if(true == handle->ch_idle){
            return DMAC_ERR_E_OK;
        } else {
            // maybe dma_abort post the sem
            handle->trans_err = DMAC_ERR_E_NOT_END;
            handle->ch_idle = true;
            return DMAC_ERR_E_NOT_END;
        }
    } else {
        // first disable channel
        dmac_ch_disable(handle->dmac_id, handle->ch);
        handle->ch_idle = true;
        // reset channel or whole DMAC
        //dmac_reset(handle->dmac_id);
        //dma_reset(handle);
        handle->trans_err = DMAC_ERR_E_NOT_END;
        return DMAC_ERR_E_NOT_END;
    }
}

cyg_uint32 DMA_wait_ch_done(void * dma_handle)
{
    DMA_CH_M *handle = NULL;
    if (NULL == dma_handle)
        return DMAC_ERR_E_PARA_INVALID;

    handle = (DMA_CH_M *)dma_handle;
    if (true == handle->config.sync)
        return DMAC_ERR_E_OK;

    return dma_wait_ch_done(handle);
}

static cyg_uint32 dmac_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);
    return CYG_ISR_CALL_DSR;  // Cause DSR to be run
}

static void dmac_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    U32 i, ch_id;
    DMAC_PRI *dmac_pri = (DMAC_PRI *)data;
    U32 dmac_base = dmac_pri->base_addr;
    DMAC_CONTROLLER_ST *hwp_dma_ctrl = (DMAC_CONTROLLER_ST *)(dmac_base);
    U32 id = dmac_pri->id;
    DMAC_CH_CONTROLLER_ST *hwp_dma_ch = NULL;
    U32 int_status = hwp_dma_ctrl->DMAC_INTSTATUS;
    U32 ch_int_status = 0, ch_int_unknown = 0, ch_int_valid = 0;

    DMA_CH_M *dma_ch = NULL;

    //printf("int_status:0x%x!\n", int_status);

    if (int_status & DMAC_COMMONREG_INT_MASK) {
        // clear common reg int
        HAL_WRITE_UINT32((dmac_base + HW_DMAC_COMMONREG_INTCLEAR_REG), 0xFF);
    }

    for (i = 0; i < HW_DMAC_CH_NUM_MAX; i++) {
        if (int_status & (0x1 << i)) {
            ch_id = i;
            hwp_dma_ch = get_hwp_dma(id, ch_id);
            dma_ch = &(dmac_manager.dma_ch_handle[ch_id]);
            ch_int_status = hwp_dma_ch->INTSTATUS;
            ch_int_valid = ch_int_status & dma_ch->int_unmask;
            ch_int_unknown = (ch_int_status & (~dma_ch->int_unmask));

            //diag_printf("ch(%d),int_st(0x%x), int_valid(0x%x), int_unknown(0x%x).\n", ch_id, ch_int_status, ch_int_valid, ch_int_unknown);
            //if (ch_int_status & DMAC_CH_INTSTATUS_DMA_TFR_DONE_MASK) {
            if (ch_int_valid & DMAC_CH_INTSTATUS_DMA_TFR_DONE_MASK) {
                dma_ch->ch_idle = true;
                dma_ch->trans_err = 0;
                // clear ch int status
                dmac_ch_int_clear(hwp_dma_ch, DMAC_CH_INT_TYPE_E_DMA_TFR_DONE);
                // post semaphore to resume thread
                cyg_semaphore_post(&dma_ch->sem);
                // dma_printf("not post, for test only.\n");
            }

            if (ch_int_valid & (~DMAC_CH_INTSTATUS_DMA_TFR_DONE_MASK)) {
                dmac_ch_int_clear(hwp_dma_ch, ch_int_valid);
                diag_printf("clear other valid int(0x%x).\n", ch_int_valid);
            }
            if(ch_int_unknown){
                // unknown interrupt, clear the interrupt
                dmac_ch_int_clear(hwp_dma_ch, ch_int_unknown);
                dma_ch->trans_err = ch_int_unknown;
                diag_printf("unknown int(0x%x).\n", ch_int_unknown);
            }
        }
    }
#ifdef CYGHWR_HAL_ASIC_DRV_LOW_POWER    
    cyg_semaphore_post(&syn_dis_dma_clk);
#endif
    cyg_interrupt_unmask(vector);
    return ;
}

static bool DMA_set_config(void * dma_handle, DMA_TRANS_DIR_E dir, cyg_uint32 src, 
                    cyg_uint32 dst, cyg_uint32 len, 
                    DMAC_CTL_TR_WIDTH_E src_width, DMAC_CTL_TR_WIDTH_E dst_width, 
                    DMAC_CTL_MSIZE_E src_burst, DMAC_CTL_MSIZE_E dst_burst, bool sync)
{
    DMA_CH_M *handle = NULL;
    cyg_uint32 blk_ts, tt_fc;
    cyg_uint32 src_inc = DMAC_CTL_ADDR_INC_E_INC, dst_inc = DMAC_CTL_ADDR_INC_E_INC;

    //if (NULL == dma_handle)
    //    return false;

    if(true == dmac_reset_flag){
        return false;
    }

    handle = (DMA_CH_M *)dma_handle;
    if((src_width > DMAC_CTL_TR_WIDTH_E_512)
       || (dst_width > DMAC_CTL_TR_WIDTH_E_512)
       || (src_burst >= DMAC_CTL_MSIZE_E_ERR)
       || (dst_burst >= DMAC_CTL_MSIZE_E_ERR)
       || (dir >= DMA_TRANS_NONE)){
        return false;
    }

    // channel4~7 support 8bit width only, check para here
    if((handle->ch >=4) && ((DMAC_CTL_TR_WIDTH_E_8 != src_width) 
                            || (DMAC_CTL_TR_WIDTH_E_8 != dst_width))){
        dma_printf("ch:0x%x support 8 bit width only, input(src_width:0x%x, dst_width:0x%x) does not OK!\n", handle->ch, src_width, dst_width);
        return false;
    }

/*
    if(len % (0x1 << src_width)){
        dma_printf("len:0x%x and src_width:0x%x does not OK!\n", len, src_width);
        return false;
    }

    if(src % (0x1 << src_width)){
        dma_printf("src:0x%x and src_width:0x%x does not OK!\n", src, src_width);
        return false;
    }

    if(len % (0x1 << dst_width)){
        dma_printf("len:0x%x and dst_width:0x%x does not OK!\n", len, dst_width);
        return false;
    }
    if(dst % (0x1 << dst_width)){
        dma_printf("dst:0x%x and dst_width:0x%x does not OK!\n", dst, dst_width);
        return false;
    }
*/

    if((!SRC_VALID(src)) || (!DST_VALID(dst))){
        return false;
    }

    cyg_mutex_lock(&(handle->mt));
    handle->config.direction = dir;
    handle->config.src_addr = src;
    handle->config.dst_addr = dst;
    handle->config.len = len;
    handle->config.src_addr_width = src_width;
    handle->config.dst_addr_width = dst_width;
    handle->config.src_burst = src_burst;
    handle->config.dst_burst = dst_burst;
    handle->config.sync = sync;

    // caculate para for dma
    blk_ts = len / (0x1 << src_width);
    if (blk_ts > HW_DMAC_CH_BLK_MAX) {
        handle->ch_idle = true;
        cyg_mutex_unlock(&(handle->mt));
        return false;
    }
    handle->blk_ts = blk_ts;
    handle->src_m_type = 0;
    handle->dst_m_type = 0;

    switch(dir){
    case DMA_MEM_TO_MEM:
        tt_fc = DMAC_CFG_TT_FC_E_M_TO_M_DMA;
        handle->src_per_type = DMAC_PER_E_MEM;
        handle->dst_per_type = DMAC_PER_E_MEM;
        break;
    case DMA_MEM_TO_DEV:
        handle->src_per_type = DMAC_PER_E_MEM;
        dst_inc = DMAC_CTL_ADDR_INC_E_NO_CHANGE;
        tt_fc = DMAC_CFG_TT_FC_E_M_TO_P_DMA;
        switch(handle->config.slave_id){
        case DMAC_SLAVE_ID_E_QSPI:
            handle->dst_per_type = DMAC_PER_E_QSPI_TX;
            break;
        case DMAC_SLAVE_ID_E_SPI0:
            handle->dst_per_type = DMAC_PER_E_SPI0_TX;
            break;
        case DMAC_SLAVE_ID_E_SPI3:
            handle->dst_per_type = DMAC_PER_E_SPI3_TX;
            break;
        case DMAC_SLAVE_ID_E_UART6:
            handle->dst_per_type = DMAC_PER_E_UART6_TX;
            break;
        case DMAC_SLAVE_ID_E_UART7:
            handle->dst_per_type = DMAC_PER_E_UART7_TX;
            break;
        case DMAC_SLAVE_ID_E_I2C0:
            handle->dst_per_type = DMAC_PER_E_I2C0_TX;
            break;
        case DMAC_SLAVE_ID_E_I2C1:
            handle->dst_per_type = DMAC_PER_E_I2C1_TX;
            break;
        case DMAC_SLAVE_ID_E_UART1:
            handle->dst_per_type = DMAC_PER_E_UART1_TX;
            break;
        default:
            diag_printf("slave id error 0x%x\n", handle->config.slave_id);
            cyg_mutex_unlock(&(handle->mt));
            return false;
        }
        break;
    case DMA_DEV_TO_MEM:
        handle->dst_per_type = DMAC_PER_E_MEM;
        src_inc = DMAC_CTL_ADDR_INC_E_NO_CHANGE;
        tt_fc = DMAC_CFG_TT_FC_E_P_TO_M_DMA;
        switch(handle->config.slave_id){
        case DMAC_SLAVE_ID_E_QSPI:
            handle->src_per_type = DMAC_PER_E_QSPI_RX;
            break;
        case DMAC_SLAVE_ID_E_SPI0:
            handle->src_per_type = DMAC_PER_E_SPI0_RX;
            break;
        case DMAC_SLAVE_ID_E_SPI3:
            handle->src_per_type = DMAC_PER_E_SPI3_RX;
            break;
        case DMAC_SLAVE_ID_E_UART6:
            handle->src_per_type = DMAC_PER_E_UART6_RX;
            break;
        case DMAC_SLAVE_ID_E_UART7:
            handle->src_per_type = DMAC_PER_E_UART7_RX;
            break;
        case DMAC_SLAVE_ID_E_I2C0:
            handle->src_per_type = DMAC_PER_E_I2C0_RX;
            break;
        case DMAC_SLAVE_ID_E_I2C1:
            handle->src_per_type = DMAC_PER_E_I2C1_RX;
            break;
        case DMAC_SLAVE_ID_E_UART1:
            handle->src_per_type = DMAC_PER_E_UART1_RX;
            break;
        default:
            diag_printf("slave id error 0x%x\n", handle->config.slave_id);
            cyg_mutex_unlock(&(handle->mt));
            return false;
        }

        break;
    default:
        cyg_mutex_unlock(&(handle->mt));
        return false;
    }

    handle->src_addr_inc = src_inc;
    handle->dst_addr_inc = dst_inc;
    handle->tt_fc = tt_fc;

    cyg_mutex_unlock(&(handle->mt));
    return true;
}

static inline DMAC_CTL_TR_WIDTH_E get_tr_width(U32 addr)
{
    if ((addr & 0x0F) == 0) {
        return DMAC_CTL_TR_WIDTH_E_128;
    } else if ((addr & 0x7) == 0) {
        return DMAC_CTL_TR_WIDTH_E_64;
    } else if ((addr & 0x3) == 0) {
        return DMAC_CTL_TR_WIDTH_E_32;
    } else if ((addr & 0x1) == 0) {
        return DMAC_CTL_TR_WIDTH_E_16;
    } else {
        return DMAC_CTL_TR_WIDTH_E_8;
    }
}

bool DMA_m2m_set_config(void * dma_handle, cyg_uint32 src, 
                    cyg_uint32 dst, cyg_uint32 len, bool sync)
{
    DMA_CH_M *handle = NULL;
    DMA_TRANS_DIR_E dir = DMA_MEM_TO_MEM;
    DMAC_CTL_TR_WIDTH_E src_width, dst_width;
    DMAC_CTL_MSIZE_E src_burst = DMAC_CTL_MSIZE_E_8, dst_burst = DMAC_CTL_MSIZE_E_8;

    if (NULL == dma_handle)
        return false;

    handle = (DMA_CH_M *)dma_handle;

    src_width = get_tr_width(src|len);
    dst_width = get_tr_width(dst);

    if(handle->ch >= 4){
        src_width = DMAC_CTL_TR_WIDTH_E_8;
        dst_width = DMAC_CTL_TR_WIDTH_E_8;
    }

    return DMA_set_config(dma_handle, dir, src, dst, len, src_width, dst_width, src_burst, dst_burst, sync);
}

bool DMA_m2p_set_config(void * dma_handle, cyg_uint32 src, 
                    cyg_uint32 dst, cyg_uint32 len, 
                    DMAC_CTL_TR_WIDTH_E dst_width, 
                    DMAC_CTL_MSIZE_E dst_burst, bool sync)
{
    DMA_CH_M *handle = NULL;
    DMA_TRANS_DIR_E dir = DMA_MEM_TO_DEV;
    DMAC_CTL_TR_WIDTH_E src_width;
    DMAC_CTL_MSIZE_E src_burst = DMAC_CTL_MSIZE_E_4;

    if (NULL == dma_handle)
        return false;

    handle = (DMA_CH_M *)dma_handle;

    src_width = get_tr_width(src|len);
    if(handle->ch >= 4){
        src_width = DMAC_CTL_TR_WIDTH_E_8;
        if(DMAC_CTL_TR_WIDTH_E_8 != dst_width){
            diag_printf("dst_width(%d) not match channel(%d).\n", dst_width, handle->ch);
            return false;
        }
    }

    if(len % (0x1 << dst_width)){
        dma_printf("len:0x%x and dst_width:0x%x does not match!\n", len, dst_width);
        return false;
    }
    if(dst % (0x1 << dst_width)){
        dma_printf("dst:0x%x and dst_width:0x%x does not match!\n", dst, dst_width);
        return false;
    }

    return DMA_set_config(dma_handle, dir, src, dst, len, src_width, dst_width, src_burst, dst_burst, sync);
}

bool DMA_p2m_set_config(void * dma_handle, cyg_uint32 src, 
                    cyg_uint32 dst, cyg_uint32 len, 
                    DMAC_CTL_TR_WIDTH_E src_width, 
                    DMAC_CTL_MSIZE_E src_burst, bool sync)
{
    DMA_CH_M *handle = NULL;
    DMA_TRANS_DIR_E dir = DMA_DEV_TO_MEM;
    DMAC_CTL_TR_WIDTH_E dst_width;
    DMAC_CTL_MSIZE_E dst_burst = DMAC_CTL_MSIZE_E_4;

    if (NULL == dma_handle)
        return false;

    handle = (DMA_CH_M *)dma_handle;

    dst_width = get_tr_width(dst|len);
    if(handle->ch >= 4){
        dst_width = DMAC_CTL_TR_WIDTH_E_8;
        if(DMAC_CTL_TR_WIDTH_E_8 != src_width){
            diag_printf("src_width(%d) not match channel(%d).\n", src_width, handle->ch);
            return false;
        }
    }

    if(len % (0x1 << src_width)){
        dma_printf("len:0x%x and src_width:0x%x does not match!\n", len, src_width);
        return false;
    }
    if(src % (0x1 << src_width)){
        dma_printf("src:0x%x and src_width:0x%x does not match!\n", src, src_width);
        return false;
    }

    return DMA_set_config(dma_handle, dir, src, dst, len, src_width, dst_width, src_burst, dst_burst, sync);
}

#if 0
static void dma_reset(DMA_CH_M *handle)
{
    cyg_uint32  i = 0;
    cyg_uint32  id = handle->dmac_id;
    cyg_uint32  ch = handle->ch;
    //cyg_uint32  ch_en_status = 0;
    //cyg_uint32  ch_idle[HW_DMAC_CH_NUM_MAX] = {0};

    dmac_reset_flag = true;
    //ch_en_status = 
    //for(i = 0; i < HW_DMAC_CH_NUM_MAX; i++){
    for( ; i < HW_DMAC_CH_NUM_MAX; ){
        if(i == ch){
            i++;
            continue;
        }
        //ch_idle[i] = dmac_ch_idle(id, i);
        //if(0 == ch_idle[i]){
        if(0 == dmac_ch_idle(id, i)){
            i = 0;
            continue;
        }
        i++;
    }

    // reset controller now
    if(0 == dmac_ch_idle(id, ch)){  // 问题通道busy，reset
        dma_printf("reset controller!\n");
        diag_printf("diag-reset controller!\n");
        dmac_reset(handle->dmac_id);
        dmac_int_enable(handle->dmac_id);
        dma_enable(handle->dmac_id);

        // reset all channel to idle
        for(i = 0; i < HW_DMAC_CH_NUM_MAX; i++){
            dmac_manager.dma_ch_handle[i].ch_idle = true;
        }
    }else{
        diag_printf("ch(%d) err and already idle(fixed by controller)!\n", handle->ch);
        dmac_manager.dma_ch_handle[ch].ch_idle = true;
    }
    dmac_reset_flag = false;
}
#endif

static cyg_uint32 hal_dma_trans(DMA_CH_M *handle)
{
    U32 cfg_upper, cfg, ctl_upper, ctl, id, ch;
    U32 src, dst, src_width, dst_width, src_m_size, dst_m_size;
    bool sync;
    U32 blk_ts, tt_fc, src_m_type, dst_m_type, src_inc, dst_inc, src_per_type, dst_per_type;
    DMAC_CH_CONTROLLER_ST *hwp_dma = NULL;

    handle->ch_idle = false;  // channel in use
    id = handle->dmac_id;
    ch = handle->ch;

    src = handle->config.src_addr;
    dst = handle->config.dst_addr;
    src_width = handle->config.src_addr_width;
    dst_width = handle->config.dst_addr_width;
    src_m_size = handle->config.src_burst;
    dst_m_size = handle->config.dst_burst;
    sync = handle->config.sync;

    blk_ts = handle->blk_ts;
    tt_fc = (handle->tt_fc & 0x7);
    src_m_type = (handle->src_m_type & 0x3);
    dst_m_type = (handle->dst_m_type & 0x3);
    src_inc = (handle->src_addr_inc & 0x1);
    dst_inc = (handle->dst_addr_inc & 0x1);
    src_per_type = (handle->src_per_type & 0xF);
    dst_per_type = (handle->dst_per_type & 0xF);
    hwp_dma = get_hwp_dma(id, ch);

    // config regs for dma
    cfg_upper = DMAC_CH_CFG_DST_OSR_LMT_MASK | DMAC_CH_CFG_SRC_OSR_LMT_MASK |
          (tt_fc << DMAC_CH_CFG_TT_FC_BIT) | (src_per_type << DMAC_CH_CFG_SRC_PER_BIT) | (dst_per_type << DMAC_CH_CFG_DEST_PER_BIT);
    cfg = (dst_m_type << DMAC_CH_DST_MULTBLK_TYPE_BIT) |
          (src_m_type << DMAC_CH_SRC_MULTBLK_TYPE_BIT);
    hwp_dma->CFG = cfg;
    hwp_dma->CFG_UPPER = cfg_upper;

    dmac_ch_sar_set(hwp_dma, src);
    dmac_ch_dar_set(hwp_dma, dst);
    dmac_ch_block_ts_set(hwp_dma, blk_ts);

    ctl = (src_width << DMAC_CH_CTL_SRC_TR_WIDTH_BIT) |
          (dst_width << DMAC_CH_CTL_DST_TR_WIDTH_BIT) |
          (src_m_size << DMAC_CH_CTL_SRC_MSIZE_BIT) |
          (dst_m_size << DMAC_CH_CTL_DST_MSIZE_BIT) |
          (src_inc << DMAC_CH_CTL_SINC_BIT) |
          (dst_inc << DMAC_CH_CTL_DINC_BIT);
    ctl_upper = 0;

    hwp_dma->CTL = ctl;
    hwp_dma->CTL_UPPER = ctl_upper;

    // clear int status
    HAL_WRITE_UINT32(((U32) hwp_dma + HW_DMAC_CH_INTCLEAR_REG_OFFSET), 0xFFFFFFFF);
    // enable int
    dmac_ch_int_disable_all(hwp_dma);
    handle->int_unmask = 0;
    dmac_ch_int_enable(hwp_dma, DMAC_CH_INT_TYPE_E_DMA_TFR_DONE);
    //dmac_ch_int_status_ctl(hwp_dma, DMAC_CH_INT_TYPE_E_DMA_TFR_DONE, 0x1);
    //dmac_ch_int_signal_ctl(hwp_dma, DMAC_CH_INT_TYPE_E_DMA_TFR_DONE, 0x1);
    handle->int_unmask |= (0x1 << DMAC_CH_INT_TYPE_E_DMA_TFR_DONE);
    asm volatile("ISB");
    asm volatile("DMB");
    asm volatile("DSB");
    dmac_ch_enable(id, ch);

    if (true == sync) {
        return dma_wait_ch_done(handle);
    } else {
        // printf("ch %d trans start!\n", ch);
    }

    return DMAC_ERR_E_OK;
}

int DMA_trans(void * dma_handle)
{
    DMA_CH_M *handle = NULL;
    cyg_uint32 ret = DMAC_ERR_E_OK;
#ifdef CYGHWR_HAL_ASIC_DRV_LOW_POWER    
    CLK_ID_TYPE_T id_type = CLK_DMAC0;
#endif

    if (NULL == dma_handle)
        return DMAC_ERR_E_PARA_INVALID;

    if(true == dmac_reset_flag){
        return DMAC_ERR_E_RESETTING;
    }

    handle = (DMA_CH_M *)dma_handle;

    cyg_mutex_lock(&(handle->mt));
#ifdef CYGHWR_HAL_ASIC_DRV_LOW_POWER    
#ifdef CYGNUM_DEVS_ARM_BP2016_SELECT_DMAC0
    if(0 == handle->dmac_id){
        id_type = CLK_DMAC0;
    }
#else
    if(1 == handle->dmac_id){
        id_type = CLK_DMAC1;
    }
#endif
    hal_clk_enable(id_type);
#endif
    cyg_semaphore_init(&(handle->sem), 0);
    ret = hal_dma_trans(handle);
    cyg_mutex_unlock(&(handle->mt));

    return ret;
}

bool DMA_abort(void * dma_handle)
{
    DMA_CH_M *handle = NULL;

    if (NULL == dma_handle)
        return false;

    handle = (DMA_CH_M *)dma_handle;
    cyg_mutex_lock(&(handle->mt));
    // first disable channel
    dmac_ch_disable(handle->dmac_id, handle->ch);
    cyg_semaphore_post(&(handle->sem));
    //dmac_ch_abort(handle->dmac_id, handle->ch);
    cyg_mutex_unlock(&(handle->mt));

    return true;
}
