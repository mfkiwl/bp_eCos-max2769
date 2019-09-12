#include <cyg/kernel/kapi.h>
#include "common.h"
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/hal/clk/clk.h>
#include <cyg/hal/reset/reset.h>
#include <cyg/hal/gpio/gpio_if.h>

#ifndef CYGPKG_HAL_ARM_BP2018
#include <cyg/hal/api/wdt_api.h>
#include <cyg/hal/api/pll_api.h>


extern int clk_get_rate_kHz(CLK_ID_TYPE_T id);

static u32  test_buf[8192];
static void print_buffer_info(void * base, int size)
{
   	int i = 0;
   	u32   addr;
    u32 * b = (u32*)base;
    addr = (u32)b;
	for(i = 0; i < size; i++) {
		if(i % 4 == 0) 
			printf("\r\n%08x: ", addr + i * 4);
		printf("%08x ", *b ++); 
	}
	printf("\r\n");
}

static void memory_testing(char *name, u32 addr, int size, int loops)
{
    u64  s, e;
    u32  us, i, j;
    u32 * va_s, count;
    
    va_s = (u32*)addr;
    count = size/4;
   
    for(j = 0; j < loops; j++) {
        cyg_interrupt_disable(); 
        s = arch_counter_get_cntpct();
        i = 0;
        for(i = 0; i< count; i++)
            va_s[i] = i;
        e = arch_counter_get_cntpct();

        us = ((u32)(e-s))/20;
        cyg_interrupt_enable(); 
        printf("%d: %s testing need %d us\n", j, name, us);
    }
    printf("\n\n");
}

int do_bbmem_cache_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    printf("************ttb*************\n");
    print_buffer_info((void*)hal_get_pagetable_base(), 20);
    memory_testing("bb", 0x600000, 8192 * 4, 10);
    memory_testing("iram", (u32)(test_buf), 8192 * 4, 10);
    hal_bbram_cache_enable();
    printf("enable bbram_mmu\n");
    printf("************ttb*************\n");
    print_buffer_info((void*)hal_get_pagetable_base(), 20);
    memory_testing("bb", 0x600000, 8192 * 4, 10);
    memory_testing("iram", (u32)(test_buf), 8192 * 4, 10);
    hal_bbram_cache_disable();

    printf("disable bbram_mmu\n");
    printf("************ttb*************\n");
    print_buffer_info((void*)hal_get_pagetable_base(), 20);
    memory_testing("bb", 0x600000, 8192 * 4, 10);
    memory_testing("iram", (u32)(test_buf), 8192 * 4, 10);

    printf("\r\n\r\n");
    return 0;
}

static int clk_get_rate_(CLK_ID_TYPE_T id)
{
    int rate_kHz;
    hal_clk_get_rate_kHz(id, &rate_kHz);
    return rate_kHz;
}

static void clk_get_rate_testing(void)
{
    print_buffer_info((void*)0xc01d0000, 8);

    printf("cpu0 run at %d KHz\n",  clk_get_rate_(CLK_CPU0));
    printf("cpu1 run at %d KHz\n",  clk_get_rate_(CLK_CPU1));
    printf("axi  run at %d KHz\n",  clk_get_rate_(CLK_AXI));
    printf("ahb  run at %d KHz\n",  clk_get_rate_(CLK_AHB));
    printf("apb  run at %d KHz\n",  clk_get_rate_(CLK_APB));
    printf("ddr  run at %d KHz\n",  clk_get_rate_(CLK_DDR));
    printf("62M  run at %d KHz\n",  clk_get_rate_(CLK_62M));
    printf("ram  run at %d KHz\n",  clk_get_rate_(CLK_RAM));

    printf("\r\n");

    printf("sd    run at %d KHz\n",  clk_get_rate_(CLK_SD));
    printf("bb    run at %d KHz\n",  clk_get_rate_(CLK_BB));
    printf("mp    run at %d KHz\n",  clk_get_rate_(CLK_MP));
    printf("qspi  run at %d KHz\n",  clk_get_rate_(CLK_QSPI));
    printf("i2c   run at %d KHz\n",  clk_get_rate_(CLK_I2C0));
    printf("uart6 run at %d KHz\n",  clk_get_rate_(CLK_UART6));
    printf("uart7 run at %d KHz\n",  clk_get_rate_(CLK_UART7));

    printf("\r\n");

    printf("spi0 run at %d KHz\n",  clk_get_rate_(CLK_SPI0));
    printf("spi1 run at %d KHz\n",  clk_get_rate_(CLK_SPI1));
    printf("spi2 run at %d KHz\n",  clk_get_rate_(CLK_SPI2));
    printf("spi3 run at %d KHz\n",  clk_get_rate_(CLK_SPI3));
    printf("sim0 run at %d KHz\n",  clk_get_rate_(CLK_SIM0));
    printf("sim1 run at %d KHz\n",  clk_get_rate_(CLK_SIM1));
    printf("sim2 run at %d KHz\n",  clk_get_rate_(CLK_SIM2));
    printf("pwm  run at %d KHz\n",  clk_get_rate_(CLK_PWM));
}

static int clk_set_rate(char* name, CLK_ID_TYPE_T id, int rate_KHz)
{
    int ret; 
    print_buffer_info((void*)0xc01d0004, 4);
    printf("set %s to %dKHz --->", name, rate_KHz);
    ret = hal_clk_set_rate_kHz(id, rate_KHz);
    if(ret < 0) {
        printf("\n");
        if(ret == CLK_OP_HW_NOT_SUPPORT_FAILED)
             printf("Error, Hardware limit for divider bits\n");
    }
    else 
        printf("%s run at %d KHz\n", name, clk_get_rate_kHz(id));
    print_buffer_info((void*)0xc01d0004, 4);
    printf("\r\n");
    return ret;
}

static void clk_set_rate_testing(void)
{
    clk_set_rate("sd", CLK_SD, 100 * 1000);
    clk_set_rate("bb", CLK_BB, 240 * 1000);
    clk_set_rate("mp", CLK_MP, 200 * 1000);
    clk_set_rate("qspi", CLK_QSPI, 80 * 1000);
    clk_set_rate("i2c", CLK_I2C0, 60 * 1000);       //error
    clk_set_rate("i2c", CLK_I2C0, 120 * 1000);
    clk_set_rate("uart6", CLK_UART6, 45600);
    clk_set_rate("uart7", CLK_UART7, 30 * 1000);

    clk_set_rate("spi0", CLK_SPI0, 100 * 1000);
    clk_set_rate("spi1", CLK_SPI1, 150 * 1000);
    clk_set_rate("spi2", CLK_SPI2, 200 * 1000);
    clk_set_rate("spi3", CLK_SPI3, 50 * 1000);
    clk_set_rate("spi3", CLK_SPI3, 90 * 1000);
    clk_set_rate("sim0", CLK_SIM0, 5 * 1000);
    clk_set_rate("sim1", CLK_SIM1, 5000);
    clk_set_rate("sim2", CLK_SIM2, 5000);
    clk_set_rate("pwm", CLK_PWM, 64 * 1000);        //error
    clk_set_rate("pwm", CLK_PWM, 64 *2 * 1000);

    clk_set_rate("CPU0", CLK_CPU0, 400 * 1000);
    clk_set_rate("cpu1", CLK_CPU1, 200 * 1000);
    clk_set_rate("axi", CLK_AXI, 380 * 1000);
    clk_set_rate("ahb", CLK_AHB,  190* 1000);
    clk_set_rate("apb", CLK_APB, 95 * 1000);
    clk_set_rate("ddr", CLK_DDR, 400 * 1000);
    clk_set_rate("62m", CLK_62M, 400 * 1000);
    clk_set_rate("ram", CLK_RAM, 380 * 1000);
}

int do_clk_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
	CLK_ID_TYPE_T cur_id=CLK_ID_NUM;
	int rate_kHz=0;
	int ret=0;
	char name[10];

	if(argc < 3)
	{
		printf("command error!, too few arguments\n");
		return -EINVAL; 
	}

	memset(name, 0, 10);
	if(strcmp(argv[2], "cpu0") == 0)
	{
		cur_id = CLK_CPU0;
		memcpy(name, "cpu0", 4);
	}

	if(strcmp(argv[2], "cpu1") == 0)
	{
		cur_id = CLK_CPU1;
		memcpy(name, "cpu1", 4);
	}

	if(strcmp(argv[2], "ddr") == 0)
	{
		cur_id = CLK_DDR;
		memcpy(name, "ddr", 3);
	}

	if(strcmp(argv[2], "62m") == 0)
	{
		cur_id = CLK_62M;
		memcpy(name, "62m", 3);
	}

	if(strcmp(argv[2], "rom") == 0)
	{
		cur_id = CLK_ROM;
		memcpy(name, "rom", 3);
	}

	if(strcmp(argv[2], "ram") == 0)
	{
		cur_id = CLK_RAM;
		memcpy(name, "ram", 3);
	}

	if(strcmp(argv[2], "dmac0") == 0)
	{
		cur_id = CLK_DMAC0;
		memcpy(name, "dmac0", 5);
	}

	if(strcmp(argv[2], "dmac1") == 0)
	{
		cur_id = CLK_DMAC1;
		memcpy(name, "dmac1", 5);
	}

	if(strcmp(argv[2], "axi") == 0)
	{
		cur_id = CLK_AXI;
		memcpy(name, "axi", 3);
	}

	if(strcmp(argv[2], "ahb") == 0)
	{
		cur_id = CLK_AHB;
		memcpy(name, "ahb", 3);
	}

	if(strcmp(argv[2], "apb") == 0)
	{
		cur_id = CLK_APB;
		memcpy(name, "apb", 3);
	}

	if(strcmp(argv[2], "ram") == 0)
	{
		cur_id = CLK_RAM;
		memcpy(name, "ram", 3);
	}

	if(strcmp(argv[2], "sd") == 0)
	{
		cur_id = CLK_SD;
		memcpy(name, "sd", 2);
	}

	if(strcmp(argv[2], "mp") == 0)
	{
		cur_id = CLK_MP;
		memcpy(name, "mp", 2);
	}

	if(strcmp(argv[2], "qspi") == 0)
	{
		cur_id = CLK_QSPI;
		memcpy(name, "qspi", 4);
	}

	if(strcmp(argv[2], "i2c0") == 0)
	{
		cur_id = CLK_I2C0;
		memcpy(name, "i2c0", 4);
	}

	if(strcmp(argv[2], "i2c1") == 0)
	{
		cur_id = CLK_I2C1;
		memcpy(name, "i2c1", 4);
	}

	if(strcmp(argv[2], "uart0") == 0)
	{
		cur_id = CLK_UART0;
		memcpy(name, "uart0", 5);
	}

	if(strcmp(argv[2], "uart1") == 0)
	{
		cur_id = CLK_UART1;
		memcpy(name, "uart1", 5);
	}

	if(strcmp(argv[2], "uart2") == 0)
	{
		cur_id = CLK_UART2;
		memcpy(name, "uart2", 5);
	}

	if(strcmp(argv[2], "uart3") == 0)
	{
		cur_id = CLK_UART3;
		memcpy(name, "uart3", 5);
	}

	if(strcmp(argv[2], "uart4") == 0)
	{
		cur_id = CLK_UART4;
		memcpy(name, "uart4", 5);
	}

	if(strcmp(argv[2], "uart5") == 0)
	{
		cur_id = CLK_UART5;
		memcpy(name, "uart5", 5);
	}

	if(strcmp(argv[2], "uart6") == 0)
	{
		cur_id = CLK_UART6;
		memcpy(name, "uart6", 5);
	}

	if(strcmp(argv[2], "uart7") == 0)
	{
		cur_id = CLK_UART7;
		memcpy(name, "uart7", 5);
	}

	if(strcmp(argv[2], "spi0") == 0)
	{
		cur_id = CLK_SPI0;
		memcpy(name, "spi0", 4);
	}

	if(strcmp(argv[2], "spi1") == 0)
	{
		cur_id = CLK_SPI1;
		memcpy(name, "spi1", 4);
	}

	if(strcmp(argv[2], "spi2") == 0)
	{
		cur_id = CLK_SPI2;
		memcpy(name, "spi2", 4);
	}

	if(strcmp(argv[2], "spi3") == 0)
	{
		cur_id = CLK_SPI3;
		memcpy(name, "spi3", 4);
	}

	if(strcmp(argv[2], "pwm") == 0)
	{
		cur_id = CLK_PWM;
		memcpy(name, "pwm", 3);
	}

	if(strcmp(argv[2], "bb") == 0)
	{
		cur_id = CLK_BB;
		memcpy(name, "bb", 2);
	}

	if(strcmp(argv[2], "rdss") == 0)
	{
		cur_id = CLK_RDSS;
		memcpy(name, "RDSS", 4);
	}

	if(strcmp(argv[2], "sim0") == 0)
	{
		cur_id = CLK_SIM0;
		memcpy(name, "sim0", 4);
	}

	if(strcmp(argv[2], "sim1") == 0)
	{
		cur_id = CLK_SIM1;
		memcpy(name, "sim1", 4);
	}

	if(strcmp(argv[2], "sim2") == 0)
	{
		cur_id = CLK_SIM2;
		memcpy(name, "sim2", 4);
	}

	if(strcmp(argv[2], "gpio") == 0)
	{
		cur_id = CLK_GPIO;
		memcpy(name, "gpio", 4);
	}

	if(strcmp(argv[2], "timer") == 0)
	{
		cur_id = CLK_TIMER;
		memcpy(name, "timer", 5);
	}

	if(strcmp(argv[2], "efuse") == 0)
	{
		cur_id = CLK_EFUSE;
		memcpy(name, "efuse", 5);
	}

	if(strcmp(argv[2], "sys32k") == 0)
	{
		cur_id = CLK_SYS32K;
		memcpy(name, "sys32k", 6);
	}

	if(strcmp(argv[2], "5m") == 0)
	{
		cur_id = CLK_5M;
		memcpy(name, "5M", 2);
	}

	if(cur_id == CLK_ID_NUM)
	{
		printf("Clock ID ERROR! exit... \n");
		return -1; 
	}

	if ((argc >= 4) && (strcmp(argv[1], "set") == 0)) {
        rate_kHz = simple_strtoul(argv[3], NULL, 10);
		ret = hal_clk_set_rate_kHz(cur_id, rate_kHz);
		if(CLK_OP_SUCCESS != ret)
		{
			printf("set %s clock rate to %d kHz failed!\n", name, rate_kHz);
			return -1;
		}
		return ENOERR;
	}

	if ((argc >= 3) && (strcmp(argv[1], "get") == 0)) {

		ret = clk_get_rate_(cur_id);

		if(CLK_OP_INV_FAILED != ret)
		{
			printf("%s: %d kHz\n", name, ret);
			return ENOERR;
		}else{
			printf("get %s clock rate failed!\n", name);
			return -1;
		}
	}

	if ((argc >= 3) && (strcmp(argv[1], "en") == 0)) {
		ret = hal_clk_enable(cur_id);
		if(CLK_OP_SUCCESS != ret)
		{
			printf("%s: enable failed!\n", name);
			return -1;
		}else{
			printf("%s: enable OK!\n", name);
			return ENOERR;
		}
	}

	if ((argc >= 3) && (strcmp(argv[1], "dis") == 0)) {
		ret = hal_clk_disable(cur_id);
		if(CLK_OP_SUCCESS != ret)
		{
			printf("%s: disable failed!\n", name);
			return -1;
		}else{
			printf("%s: disable OK!\n", name);
			return ENOERR;
		}
	}

	if ((argc >= 2) && (strcmp(argv[1], "auto") == 0)) {
        clk_get_rate_testing();
        clk_set_rate_testing();
        return ENOERR;
    }

    return -EINVAL; 
}

static void reset_testing_sub_func(char * name, SWRST_ID_TYPE_T id)
{
    printf("%s reset ...\n", name);
    hal_sw_reset(id);

}
static void reset_testing(void)
{
    reset_testing_sub_func("cpu1", SWRST_CPU1);
    reset_testing_sub_func("ddr", SWRST_DDR);
    reset_testing_sub_func("62M", SWRST_62M);
    reset_testing_sub_func("rom", SWRST_ROM);
//    reset_testing_sub_func("ram", SWRST_RAM);

    reset_testing_sub_func("bb", SWRST_BB);
    reset_testing_sub_func("mp", SWRST_MP);
    reset_testing_sub_func("dmac0", SWRST_DMAC0);
    reset_testing_sub_func("dmac1", SWRST_DMAC1);
    reset_testing_sub_func("qspi", SWRST_QSPI);
    reset_testing_sub_func("sd", SWRST_SD);
 //   reset_testing_sub_func("uart0", SWRST_UART0);
    reset_testing_sub_func("uart1", SWRST_UART1);
    reset_testing_sub_func("uart2", SWRST_UART2);
    reset_testing_sub_func("uart3", SWRST_UART3);
    reset_testing_sub_func("uart4", SWRST_UART4);
    reset_testing_sub_func("uart5", SWRST_UART5);
    reset_testing_sub_func("uart6", SWRST_UART6);
    reset_testing_sub_func("uart7", SWRST_UART7);
    reset_testing_sub_func("i2c0", SWRST_I2C0);
    reset_testing_sub_func("i2c1", SWRST_I2C1);
    reset_testing_sub_func("spi0", SWRST_SPI0);
    reset_testing_sub_func("spi1", SWRST_SPI1);
    reset_testing_sub_func("spi2", SWRST_SPI2);
    reset_testing_sub_func("spi3", SWRST_SPI3);
    reset_testing_sub_func("sim0", SWRST_SIM0);
    reset_testing_sub_func("sim1", SWRST_SIM1);
    reset_testing_sub_func("sim2", SWRST_SIM2);
    reset_testing_sub_func("pwm", SWRST_PWM);
    reset_testing_sub_func("gpio", SWRST_GPIO);
  //  reset_testing_sub_func("timer", SWRST_TIMER);
}


int do_reset_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    reset_testing();
    return 0;
}

static int gpio_testing(int gpio_num, int input)
{
    int i;
    u32 v;
    if (input == 0) {
        printf("gpio %d, output testing .\n", gpio_num);
        gpio_enable(gpio_num);
        gpio_disable_interrupt(gpio_num);
        gpio_set_output_direction(gpio_num);

        for (i = 0; i < 60; i++) {
            gpio_set_output_high(gpio_num);
            cyg_thread_delay(10);
            gpio_set_output_low(gpio_num);
            cyg_thread_delay(10);
        }
        gpio_disable(gpio_num);

        print_buffer_info((u32 *) 0xc0130000,  64);
        return 0;
    }

    printf("gpio %d, input testing .\n", gpio_num);
    gpio_enable(gpio_num);
    gpio_disable_interrupt(gpio_num);
    gpio_set_input_direction(gpio_num);

    for (i = 0; i < 60; i++) {
        gpio_get_input_data(gpio_num, &v);
        printf("gpio %d, input v = %d\n", gpio_num, v);
        cyg_thread_delay(10);
    }
    gpio_disable(gpio_num);

   print_buffer_info((u32 *) 0xc0130000,  64);
   return 0;
}

int do_gpio_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int gpio_num, input;

    gpio_num = 0;
    input = 0;

    if (argc >= 2)
        gpio_num = simple_strtoul(argv[1], NULL, 10);
    if (argc >= 3)
        input = simple_strtoul(argv[2], NULL, 10);

    printf("gpio num: %d, mode %s\n", gpio_num, input > 0 ? "input" : "output");

    gpio_testing(gpio_num, input);
    return 0;
}

int do_wdt_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    cyg_uint32 pause_count = 0;
    cyg_uint32 wait_ms = 0, t_time = 0, reset_time = 0;
    int i = 0;
   
    if (argc != 3){
        printf("parameter is error\n"); 
    } else {
        t_time = simple_strtoul(argv[1], NULL, 10);
        reset_time = simple_strtoul(argv[2], NULL, 10);
    }
    
    printf("wdt api test enter!\n");
    wdt_api_init();
    printf("wdt api init done!\n");
    wait_ms = t_time / 10;
    wdt_api_set_pause((WDT_TIME)t_time);
    wdt_api_enable();

    for(i = 0; i < 8; i++){
        cyg_thread_delay(wait_ms);
        pause_count = wdt_api_get_pause();
        printf("%d:feed, wdt_torr = 0x%x\n", i, pause_count);
        wdt_api_feed();
    }
    
    wdt_api_set_pause((WDT_TIME)reset_time);
    wait_ms = reset_time / 10;
    printf("modify wdt restart value\n");
    i = 0;
    printf("System will restart\n");
    while (1) {
        cyg_thread_delay(wait_ms);
        printf("%d\n", i);
        pause_count = wdt_api_get_pause();
        printf("%d: wdt_torr = 0x%x\n", i, pause_count);
        i++;
    }
}

int do_pll_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
	int rate_kHz = 0;
    int pll_num = 0;
	int ret = 0;
	char name[10];

	if(argc < 3){
		printf("command error!, too few arguments\n");
		return false; 
	}
    printf("pll api test enter!\n");
    pll_api_init();
    printf("pll api init done!\n");

	memset(name, 0, 10);
    pll_num = simple_strtoul(argv[2], NULL, 10);
    if ((argc >= 4) && (strcmp(argv[1], "set") == 0)) {
        rate_kHz = simple_strtoul(argv[3], NULL, 10);
		ret = pll_api_set_rate_kHz(pll_num, rate_kHz);
		if(true != ret){
			printf("set %c clock rate to %d kHz failed!\n", *argv[2], rate_kHz);
			return false;
		}
		return true;
	} else if ((argc >= 3) && (strcmp(argv[1], "get") == 0)) {
		ret = pll_api_get_rate_kHz(pll_num);
		if(0 != ret){
			printf("pll%c: %d kHz\n", *argv[2], ret);
			return true;
		} else {
			printf("get pll%c clock rate failed!\n", *argv[2]);
			return false;
		}
	}
    return 0;
}
#endif   // #ifnde CYGPKG_HAL_ARM_BP2018

int do_interrupt_stat(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    u32 isr_l, isr, dsr_l, dsr, us, avg_us;
    int ret;
    hal_interrupt_stats_display();
    printf("\r\n\r\n");

    ret =hal_get_interrupt_stats(0, &isr_l, &isr, &dsr_l, &dsr, &us, &avg_us);
    if(ret < 0)
        printf("get_interrupt_stats return error\n");
    else    
        printf("%d, %d, %d, %d, %d, %d\n", isr_l, isr, dsr_l, dsr, us, avg_us);

    return 0;
}

int do_arch_timer_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int i;
    u64 t0, t1;
    u32 t32;

    for (i = 0; i < 10; i++) {
        t0 = arch_counter_get_cntpct();
        cyg_thread_delay(100);
        t1 = arch_counter_get_cntpct();
        t32 = t1 - t0;
        printf("Delay %d S , %d\n", i, t32);
    }
    printf("\r\n\r\n");
    return 0;
}


