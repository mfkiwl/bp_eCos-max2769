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
#include <cyg/hal/regs/scm.h>
#include <cyg/hal/reset/reset.h>
#include <cyg/hal/api/comm_api.h>
#include <stdio.h>
#include <pkgconf/io_common_comm_arm_bp2016.h>


#define  COMM_CH_NUM   (16)  // max 16
#ifdef CYGNUM_DEVS_ARM_BP2016_SELECT_CPU0
  #define  COMM_RECV_IRQ_CH0       (SYS_IRQ_ID_SGI16)
  #define  COMM_SEND_IRQ_CH0       (SYS_IRQ_ID_SGI0)
#else
  #define  COMM_RECV_IRQ_CH0       (SYS_IRQ_ID_SGI0)
  #define  COMM_SEND_IRQ_CH0       (SYS_IRQ_ID_SGI16)
#endif

#define  COMM_ADDR_VALID(addr, len) \
         ((addr >= CONFIG_CPU_SHARE_MEM_START) \
        &&((addr + len) < (CONFIG_CPU_SHARE_MEM_START + CONFIG_CPU_SHARE_MEM_KSIZE * 1024)))

static cyg_mutex_t  comm_mt;

typedef enum {
    CPU_SEND = 0,
    CPU_RECV = 1,
}CPU_TRANS;

typedef struct comm_ch_m{
    volatile cyg_uint32  addr;
    volatile cyg_uint32  len;
    volatile CH_ST_S     status;
    volatile bool        idle;
    volatile bool        sync;
    volatile cyg_uint32  ret;
    cyg_sem_t            sem;
    cyg_uint32           irq_num;
    cyg_uint32           irq_pri;
    cyg_interrupt        comm_interrupt;
    cyg_handle_t         comm_interrupt_handle;
}COMM_CH_ST;

typedef struct comm_m_st{
    cyg_uint32           ch;
    bool                 valid;
    CPU_TRANS            last_op;
    callback_func        call_back;  // recv callback
    COMM_CH_ST           send;
    COMM_CH_ST           recv;
}COMM_M_ST;

static COMM_M_ST  comm_m[COMM_CH_NUM];

extern void v7_dma_clean_range(U32, U32);
extern void v7_dma_inv_range(U32, U32);

extern void cpu_comm_clr_irq(U32 irq_num);
extern void cpu_comm_int_clr_all(void);
extern void cpu_comm_ch_ack_ret_set(U32 ch, U32 ret);
extern cyg_uint32 cpu_comm_ch_ret_get(U32 ch);
extern void cpu_comm_ch_para_get(U32 ch, volatile U32 *addr, volatile U32 *len);
extern void cpu_comm_ch_para_set(U32 ch, U32 addr, U32 len);
extern void cpu_comm_send_trigger(U32 ch);
extern void cpu_comm_ack_trigger(U32 ch);


static bool      comm_init(struct cyg_devtab_entry *tab);
static Cyg_ErrNo comm_lookup(struct cyg_devtab_entry **tab,
                            struct cyg_devtab_entry *st,
                            const char *name);

CHAR_DEVIO_TABLE(comm_handler,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL);

CHAR_DEVTAB_ENTRY(comm_device,
                  "dev/comm",
                  NULL,          // Base device name
                  &comm_handler,
                  comm_init,
                  comm_lookup,
                  (void*) &comm_m[0]);   // Private data pointer

static cyg_uint32 comm_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);
    return CYG_ISR_CALL_DSR;  // Cause DSR to be run
}

static void comm_send_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    // recv ack int, release channel
    COMM_M_ST *comm = (COMM_M_ST *)data;
    cyg_uint32 ch = comm->ch;
    // clear device int
    cpu_comm_clr_irq(vector);

    if(false == comm->send.idle){
        comm->send.ret = cpu_comm_ch_ret_get(ch);
        comm->send.idle = true;
        if((true == comm->send.sync) && (comm->send.status == CH_ST_SENDING)){
            cyg_semaphore_post(&comm->send.sem);
            comm->send.status = CH_ST_SEND_RECV_ACK;
        }else{
            comm->send.status = CH_ST_SEND_RECV_ACK_AFTER_TIMEOUT;
        }
        //diag_printf("send_comm[%d] recv ack\n", ch);
    }
    cyg_interrupt_unmask(vector);
}

static void comm_recv_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    COMM_M_ST *comm = (COMM_M_ST *)data;
    cyg_uint32 ch = comm->ch;
    // clear device int
    cpu_comm_clr_irq(vector);

    cpu_comm_ch_para_get(ch, &comm->recv.addr, &comm->recv.len);
    comm->recv.status = CH_ST_RECV_DATA;
    //diag_printf("comm[%d] recv data, addr:0x%x, len:0x%x\n", ch, comm->recv.addr, comm->recv.len);
    cyg_semaphore_post(&comm->recv.sem);
    cyg_interrupt_unmask(vector);
}

static void comm_int_init(COMM_M_ST *data, COMM_CH_ST *comm, bool send)
{
    cyg_DSR_t * dsr = NULL;
    if(true == send){
        dsr = comm_send_dsr;
    } else {
        dsr = comm_recv_dsr;
    }

    cyg_interrupt_create(comm->irq_num,
            comm->irq_pri,                    // Priority - unused
            (cyg_addrword_t)data, //  Data item passed to interrupt handler
            comm_isr,
            dsr,
            &comm->comm_interrupt_handle,
            &comm->comm_interrupt);
    cyg_interrupt_attach(comm->comm_interrupt_handle);
    cyg_interrupt_unmask(comm->irq_num);
}

static void send_ch_init(cyg_uint32 id, COMM_CH_ST *comm, COMM_M_ST *data)
{
    comm->irq_num = id + COMM_SEND_IRQ_CH0;
    comm->irq_pri = 160;
    comm->idle = true;
    comm->status = CH_ST_IDLE;
    cyg_semaphore_init(&(comm->sem), 0);
    comm_int_init(data, comm, true);
}

static void recv_ch_init(cyg_uint32 id, COMM_CH_ST *comm, COMM_M_ST *data)
{
    comm->irq_num = id + COMM_RECV_IRQ_CH0;
    comm->irq_pri = 160;
    comm->idle = true;
    comm->status = CH_ST_IDLE;
    cyg_semaphore_init(&(comm->sem), 0);
    comm_int_init(data, comm, false);
}

void  cyg_cpu_comm_init(void)
{
    cyg_uint32 i;

    cyg_mutex_init(&comm_mt);

    // clear all interrupt
    cpu_comm_int_clr_all();

    for(i = 0; i < COMM_CH_NUM; i++){
        //diag_printf("comm[%d] addr 0x%x\n", i, (cyg_uint32)&comm_m[i]);
        comm_m[i].ch = i;
        comm_m[i].valid = true;
        send_ch_init(i, &comm_m[i].send, &comm_m[i]);
        recv_ch_init(i, &comm_m[i].recv, &comm_m[i]);
    }
}

static bool comm_init(struct cyg_devtab_entry *tab)
{
    //comm_printf("comm module init...\n");
    cyg_cpu_comm_init();
    return ENOERR;
}

static Cyg_ErrNo comm_lookup(struct cyg_devtab_entry **tab,
           struct cyg_devtab_entry *st,
           const char *name)
{
    return ENOERR;
}

void * cyg_cpu_comm_alloc(FUNC_T fun)
{
    COMM_M_ST *comm = NULL;

    if(fun >= FUN_END){
        diag_printf("func err, max(%d), given(%d)\n", FUN_END-1, fun);
        return NULL;
    }

    comm = &comm_m[fun];

    cyg_mutex_lock(&comm_mt);
    if(true == comm->valid){
        comm->valid = false;
        cyg_semaphore_init(&(comm->send.sem), 0);
        cyg_semaphore_init(&(comm->recv.sem), 0);
    }else{
        comm = NULL;
    }

    cyg_mutex_unlock(&comm_mt);
    return comm;
}

void cyg_cpu_comm_free(void *handle)
{
    cyg_uint32 ch;
    if(NULL == handle) return ;
    COMM_M_ST *comm = (COMM_M_ST *)handle;
    ch = comm->ch;
    if(false == comm->send.idle){
        diag_printf("free not idle channel(%d)!\n", ch);
    }

    cyg_mutex_lock(&comm_mt);
    comm->valid = true;
    comm->send.idle = true;
    comm->send.status = CH_ST_IDLE;
    comm->recv.status = CH_ST_IDLE;
    cyg_mutex_unlock(&comm_mt);
    handle = NULL;
}

bool cyg_cpu_comm_send_msg(void * handle, char * buff, cyg_uint32 len, bool sync, cyg_uint32 timeout)
{
    cyg_uint32 ch;
    COMM_M_ST *comm;
    COMM_CH_ST *send;
    bool  sem_ret = false;
    cyg_uint32 addr = (cyg_uint32)buff;
    if(NULL == handle) return false;
    comm = (COMM_M_ST *)handle;
    send = &comm->send;
    ch = comm->ch;
    send->sync = sync;
    send->status = CH_ST_SENDING;

    // record last operation
    comm->last_op = CPU_SEND;

    if(false == send->idle){
        diag_printf("send msg by not idle channel(%d)!\n", ch);
    }

    if(!COMM_ADDR_VALID(addr, len)){
        diag_printf("para invalid, addr(0x%x), len(0x%x)!\n", addr, len);
        return false;
    }

    send->idle = false;
    send->ret = COMM_ERR_E_OK;  // clear last err to 0
    cpu_comm_ch_para_set(ch, addr, len);
    // clean data to ram
    v7_dma_clean_range(addr, addr + len);
    cpu_comm_send_trigger(ch);

    if(true == sync){
#ifdef CYGFUN_KERNEL_THREADS_TIMER
        sem_ret = cyg_semaphore_timed_wait(&send->sem, cyg_current_time() + timeout);
#else
        sem_ret = cyg_semaphore_wait(&send->sem);
#endif
        if(true == sem_ret){
            return true;
        }else{
            send->status = CH_ST_SEND_TIMEOUT_OR_QUIT;
            send->ret = -COMM_ERR_E_SEM_TO;
            return false;
        }
    }

    return true;
}

bool cyg_cpu_comm_wait_ack(void * handle, cyg_uint32 timeout)
{
    //cyg_uint32 ch;
    COMM_M_ST *comm;
    COMM_CH_ST *send;
    bool  sem_ret = false;
    if(NULL == handle) return false;
    comm = (COMM_M_ST *)handle;
    send = &comm->send;
    //ch = comm->ch;

    if(false == send->sync){
#ifdef CYGFUN_KERNEL_THREADS_TIMER
        sem_ret = cyg_semaphore_timed_wait(&send->sem, cyg_current_time() + timeout);
#else
        sem_ret = cyg_semaphore_wait(&send->sem);
#endif
        if(true == sem_ret){
            return true;
        }else{
            send->status = CH_ST_SEND_TIMEOUT_OR_QUIT;
            send->ret = -COMM_ERR_E_SEM_TO;
            return false;
        }
    }

    return true;
}

void cyg_cpu_comm_recv_callback_register(void * handle, callback_func callback)
{
    COMM_M_ST *comm;
    //COMM_CH_ST *recv;
    if(NULL == handle) return ;
    comm = (COMM_M_ST *)handle;
    //recv = &comm->recv;

    // if NULL == callback, means unregister
    comm->call_back = callback;
}

static void cpu_comm_ack(cyg_uint32 ch, cyg_uint32 ret)
{
    //diag_printf("ack ch(%d)!\n", ch);
    cpu_comm_ch_ack_ret_set(ch, ret);
    cpu_comm_ack_trigger(ch);
}

bool cyg_cpu_comm_ack(void * handle, cyg_uint32 ret)
{
    cyg_uint32 ch;
    COMM_M_ST *comm;
    if(NULL == handle) return false;
    comm = (COMM_M_ST *)handle;
    ch = comm->ch;

    cpu_comm_ack(ch, ret);
    return true;
}

bool cyg_cpu_comm_recv(void * handle, cyg_uint32 timeout)
{
    COMM_M_ST *comm;
    COMM_CH_ST *recv;
    cyg_uint32 ret = COMM_ERR_E_OK;
    bool  sem_ret = false;
    if(NULL == handle) return false;
    comm = (COMM_M_ST *)handle;
    recv = &comm->recv;
    recv->ret = COMM_ERR_E_OK;
    recv->status = CH_ST_RECVING;

    if(NULL == comm->call_back){
        ret = -COMM_ERR_E_NO_CALLBACK;
        recv->ret = ret;
        recv->status = CH_ST_IDLE;
        cpu_comm_ack(comm->ch, ret);
        return false;
    }

    // record last operation
    comm->last_op = CPU_RECV;

#ifdef CYGFUN_KERNEL_THREADS_TIMER
    sem_ret = cyg_semaphore_timed_wait(&recv->sem, cyg_current_time() + timeout);
#else
    sem_ret = cyg_semaphore_wait(&recv->sem);
#endif
    if(true == sem_ret){
        if(!COMM_ADDR_VALID(recv->addr, recv->len)){
            diag_printf("recv para invalid, addr(0x%x), len(0x%x)!\n", recv->addr, recv->len);
            ret = -COMM_ERR_E_PARA_INV;
        }else{
            // inv cache
            v7_dma_inv_range(recv->addr, recv->addr + recv->len);
            // call user's call back function 
            ret = comm->call_back(recv->addr, recv->len);
        }
        recv->ret = ret;
        cpu_comm_ack(comm->ch, ret);
        recv->status = CH_ST_RECV_SEND_ACK;
        return true;
    } else {
        recv->ret = -COMM_ERR_E_SEM_TO;
        recv->status = CH_ST_RECV_TIMEOUT;
        cpu_comm_ack(comm->ch, ret);
        recv->status = CH_ST_RECV_TIMEOUT_SEND_ACK;
        return false;
    }
}

cyg_uint32 cyg_cpu_comm_get_last_err(void * handle)
{
    COMM_M_ST *comm;
    COMM_CH_ST *recv, *send;
    if(NULL == handle) return -COMM_ERR_E_PARA_INV;
    comm = (COMM_M_ST *)handle;
    recv = &comm->recv;
    send = &comm->send;

    if(CPU_SEND == comm->last_op){
        return send->ret;
    } else {
        return recv->ret;
    }
}

cyg_uint32 cyg_cpu_comm_get_status(void * handle, CH_ST_S *send_st, CH_ST_S *recv_st)
{
    COMM_M_ST *comm;
    COMM_CH_ST *recv, *send;
    if(NULL == handle){
        return -COMM_ERR_E_PARA_INV;
    }
    comm = (COMM_M_ST *)handle;
    recv = &comm->recv;
    send = &comm->send;

    *send_st = send->status;
    *recv_st = recv->status;
    return COMM_ERR_E_OK;
}

