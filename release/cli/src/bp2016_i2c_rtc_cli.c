//==========================================================================
//
//        bp2016_i2c_rtc_cli.c
//
//        i2c base interface test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2019-05-06
// Description:   i2c base interface test for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <cyg/kernel/kapi.h>
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/infra/diag.h>

#ifdef CYGPKG_DEVS_BP2016_I2C_DW
#include <cyg/hal/api/i2c_api.h>
#include <cyg/hal/api/iomux_api.h>
#include <cyg/hal/api/gpio_api.h>
#include <cyg/hal/gpio/gpio_if.h>

#define SLV_ADDR    0x68
#define DW_IC_CON_SPEED_STD 0x2

#define TIME_SEC    0x00
#define TIME_MIN    0x01
#define TIME_HOUR   0x02
#define TIME_DAY    0x03
#define TIME_DATE   0x04
#define TIME_MONTH  0x05
#define TIME_YEAR   0x06
#define ALARM1_SEC  0x07
#define ALARM1_MIN  0x08
#define ALARM1_HOUR 0x09
#define ALARM1_DATE 0x0a
#define ALARM2_MIN  0x0b
#define ALARM2_HOUR 0x0c
#define ALARM2_DATE 0x0d
#define CTRL_REG    0x0e
#define STATUS_REG  0x0f

#define CTRL_A1IE   (0)
#define CTRL_A2IE   (1)
#define CTRL_INTCN  (2)

#define STATUS_A1F  (0)
#define STATUS_A2F  (1)
#define STATUS_ECLK (2)

#define rtc_debug printf

//===========================================================================
//                               DATA TYPES
//===========================================================================
static void * i2c_handle = NULL;
static cyg_uint8 wbuf[256];
cyg_thread thread_s_rtc;		/* space for two thread objects */
char stack_rtc[4096*2];		/* space for two 4K stacks */

/* now the handles for the threads */
cyg_handle_t cli_thread_rtc;

static cyg_sem_t synrtc;
void dump_time_info(void);

static cyg_int32 set_reg(cyg_uint8 addr, cyg_uint8 sec)
{
    int res = 0;
    wbuf[0] = addr; 
    wbuf[1] = sec; 
    res = I2C_dev_write(i2c_handle, wbuf, 2);
    if(res != 2){
        rtc_debug("set rtc reg 0x%08x failed\n", addr);
        return -1;
    }
    return 0;
}

static cyg_int32 read_reg(cyg_uint8 addr, cyg_uint8 *rbuf)
{
    int res = 0;
    wbuf[0] = addr;
    res = I2C_dev_read(i2c_handle, wbuf, 1, rbuf, 1);
    if(res != 1){
        rtc_debug("read reg 0x%08x failed\n", addr);
        return -1;
    }
    return 0;
}

static cyg_uint32 gpio_isr(cyg_uint32 vector, cyg_uint32 data)   
{
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);

    return CYG_ISR_HANDLED | CYG_ISR_CALL_DSR;  // Cause DSR to be run
}

static void gpio_dsr(cyg_uint32 vector, cyg_uint32 cnt, cyg_uint32 data)
{
    cyg_uint32 st;
    st = gpio_interupt_get_intstatus();
    if (st & (1 << (vector - SYS_IRQ_ID_GPIO0))) {
        gpio_interrupt_clr(1 << (vector - SYS_IRQ_ID_GPIO0));
    }
    cyg_interrupt_unmask(vector);
    cyg_semaphore_post(&synrtc);  //release a semaphore
    //rtc_debug("dsr handle end\n");
}

static cyg_int32 gpio_interrupt_init(cyg_int32 gpio_num)
{
    gpio_api_init();
    if(false == gpio_api_set_irq((gpio_num - SYS_IRQ_ID_GPIO0), gpio_isr, gpio_dsr, 0, GPIO_INT_FALLING_EDGE)){
        rtc_debug("in rtc testing, gpio%d Set irq mode error!\n", (gpio_num - SYS_IRQ_ID_GPIO0));
        return -1;
    }
    return 0;
} 

//init
void rtc_clr_interrupt(cyg_addrword_t data);
static cyg_int32 init_for_rtc(void)
{
    static cyg_uint32 rtc_init_flag = 0;
    if(rtc_init_flag == 0){
        rtc_debug("create thread for clear rtc interrupt\n");
        cyg_semaphore_init(&synrtc, 0);
        cyg_thread_create(0, rtc_clr_interrupt, (cyg_addrword_t) 0,
		    "Thread rtc", (void *) stack_rtc, 4096 *2,
		    &cli_thread_rtc, &thread_s_rtc);

        cyg_thread_resume(cli_thread_rtc);
        rtc_init_flag = 1;
    }
    hal_iomux_en(IOMUX_I2C0);
    i2c_handle = I2C_dev_init(0, SLV_ADDR, DW_IC_CON_SPEED_STD);
    if(NULL == i2c_handle){
        rtc_debug("rtc Init failed!\n");
        return -1;
    } else {
        rtc_debug("rtc Init success!\n");
    }
    return 0;
}

static cyg_int32 init_interrupt(void)
{
    cyg_uint8 value = 0;
    cyg_int32 res = 0;
    
    //set alarm1 and alarm2 interrupt out by SQW/INTB PIN
    res = read_reg(CTRL_REG, &value);
    if(res != 0){
        return -1;
    }
    value |= (1 << CTRL_INTCN);
    res = set_reg(CTRL_REG, value);
    if(res != 0){
        return -1;
    }
    res = read_reg(STATUS_REG, &value);
    if(res != 0){
        return -1;
    }
    value |= (1 << STATUS_ECLK);
    res = set_reg(STATUS_REG, value);
    if(res != 0){
        return -1;
    }

    //clear interrupt flags
    res = read_reg(STATUS_REG, &value);
    if(res != 0){
        return -1;
    }
    value &= ~(3 << STATUS_A1F);
    res = set_reg(STATUS_REG, value); //clear alarm1 alarm2 int bit
    if(res != 0){
        return -1;
    }

    //enable alarm1 interrupt
    res = read_reg(CTRL_REG, &value);
    if(res != 0){
        return -1;
    }
    value |= (1 << CTRL_A1IE);
    res = set_reg(CTRL_REG, value);
    if(res != 0){
        return -1;
    }

    //enable gpio10 interrupt 
    res = gpio_interrupt_init(SYS_IRQ_ID_GPIO10); 
    if(res != 0){
        return -1;
    }
    return 0;
}

void dump_time_info(void)
{
    cyg_uint8 buf[16];
    cyg_uint8 addr = 0;
    for(addr = 0; addr <= 0x0a; addr++){
        read_reg(addr, &buf[addr]);
    }
    rtc_debug("time is %04d-%02d-%02d %02d %02d:%02d:%02d\n",(1976+buf[6]),buf[5],buf[4],buf[3],buf[2],buf[1],buf[0]);
    rtc_debug("alarm1 is %02d %02d:%02d:%02d\n", buf[0xa],buf[9],buf[8],buf[7]);
}

int do_rtc_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    cyg_int32 res = 0;

    res = init_for_rtc();
    if(res != 0){
        return -1;
    }
    res = init_interrupt();
    if(res != 0){
        return -1;
    }

    //set sec  :1
    res = set_reg(TIME_SEC, 1);
    if(res != 0){
        return -1;
    }
    //set min  :1
    res = set_reg(TIME_MIN, 1);
    if(res != 0){
        return -1;
    }
    //set hour :08
    res = set_reg(TIME_HOUR, 8);
    if(res != 0){
        return -1;
    }
    //set day  :07
    res = set_reg(TIME_DAY, 7);
    if(res != 0){
        return -1;
    }
    //set date :05
    res = set_reg(TIME_DATE, 5);
    if(res != 0){
        return -1;
    }
    //set month:05
    res = set_reg(TIME_MONTH, 5);
    if(res != 0){
        return -1;
    }
    //set year :3
    res = set_reg(TIME_YEAR, 3);
    if(res != 0){
        return -1;
    }

    //set alarm1 sec  :04
    res = set_reg(ALARM1_SEC, 4);
    if(res != 0){
        return -1;
    }
    //set alarm1 min  :01
    res = set_reg(ALARM1_MIN, 1);
    if(res != 0){
        return -1;
    }
    //set alarm1 hour :08
    res = set_reg(ALARM1_HOUR, 8);
    if(res != 0){
        return -1;
    }
    //set alarm1 date
    res = set_reg(ALARM1_DATE, 5);
    if(res != 0){
        return -1;
    }

    dump_time_info();

    return ENOERR;
}

void rtc_clr_interrupt(cyg_addrword_t data)
{
    cyg_uint8 value = 0;
    while(1){
        rtc_debug("enter rtc interrupt clear thread\n");
        cyg_semaphore_wait(&synrtc);
        rtc_debug("get sem\n");
        
        read_reg(STATUS_REG, &value);
        value &= ~(1 << STATUS_A1F);
        set_reg(STATUS_REG, value); //clear alarm1 int
        dump_time_info();
    }
}
#endif

