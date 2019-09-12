#include "common.h"
#ifdef CYGPKG_DEVS_DISK_MMC
#include <pkgconf/devs_disk_mmc.h>
#endif
#include "ctype.h"
#include <pkgconf/hal_arm.h>

static uint	dp_last_addr, dp_last_size;
static uint	dp_last_length = 0x40;

static	ulong	base_address = 0;

unsigned long simple_strtoul(const char *cp, char **endp,
				unsigned int base)
{
	unsigned long result = 0;
	unsigned long value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}

		if (!base)
			base = 8;
	}

	if (!base)
		base = 10;

	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;

	return result;
}


#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
int print_buffer (ulong addr, void* data, uint width, uint count, uint linelen)
{
	/* linebuf as a union causes proper alignment */
	union linebuf {
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
		uint64_t uq[MAX_LINE_LENGTH_BYTES/sizeof(uint64_t) + 1];
#endif
		uint32_t ui[MAX_LINE_LENGTH_BYTES/sizeof(uint32_t) + 1];
		uint16_t us[MAX_LINE_LENGTH_BYTES/sizeof(uint16_t) + 1];
		uint8_t  uc[MAX_LINE_LENGTH_BYTES/sizeof(uint8_t) + 1];
	} lb;
	int i;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	uint64_t x;
#else
	uint32_t x;
#endif

	if (linelen*width > MAX_LINE_LENGTH_BYTES)
		linelen = MAX_LINE_LENGTH_BYTES / width;
	if (linelen < 1)
		linelen = DEFAULT_LINE_LENGTH_BYTES / width;

	while (count) {
		printf_console("%08lx:", addr);

		/* check for overflow condition */
		if (count < linelen)
			linelen = count;

		/* Copy from memory into linebuf and print hex values */
		for (i = 0; i < linelen; i++) {
			if (width == 4)
				x = lb.ui[i] = *(volatile uint32_t *)data;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
			else if (width == 8)
				x = lb.uq[i] = *(volatile uint64_t *)data;
#endif
			else if (width == 2)
				x = lb.us[i] = *(volatile uint16_t *)data;
			else
				x = lb.uc[i] = *(volatile uint8_t *)data;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
			printf_console(" %0*llx", width * 2, (long long)x);
#else
			printf_console(" %0*x", width * 2, x);
#endif
			data += width;
		}

		/* Print data in ASCII characters */
		for (i = 0; i < linelen * width; i++) {
			if (!isprint(lb.uc[i]) || lb.uc[i] >= 0x80)
				lb.uc[i] = '.';
		}
		lb.uc[i] = '\0';
		printf_console("    %s\r\n", lb.uc);

		/* update references */
		addr += linelen * width;
		count -= linelen;
	}

	return 0;
}

int cmd_get_data_size(const char* arg, int default_size)
{
        /* Check for a size specification .b, .w or .l.
         */
        int len = strlen(arg);
        if (len > 2 && arg[len-2] == '.') {
                switch(arg[len-1]) {
                case 'b':
                        return 1;
                case 'w':
                        return 2;
                case 'l':
                        return 4;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
				case 'q':
						return 8;
#endif
                case 's':
                        return -2;
                default:
                        return -1;
                }
        }
        return default_size;
}

/* Memory Display
 *
 * Syntax:
 *	md{.b, .w, .l} {addr} {len}
 */
#define DISP_LINE_LEN	16
int do_mem_md ( shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
	ulong	addr, length;
	int	size;
	int rc = 0;

	/* We use the last specified parameters, unless new ones are
	 * entered.
	 */
	addr = dp_last_addr;
	size = dp_last_size;
	length = dp_last_length;

	if (argc < 2)
		return -EINVAL;

	/* New command specified.  Check for a size specification.
	 * Defaults to long if no or incorrect specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return -EINVAL;

	/* Address is specified since argc > 1
	 */
	addr = simple_strtoul(argv[1], NULL, 16);
	addr += base_address;

	/* If another parameter, it is the length to display.
	 * Length is the number of objects, not number of bytes.
	 */
	if (argc > 2)
		length = simple_strtoul(argv[2], NULL, 16);

	/* Print the lines. */
	print_buffer(addr, (void*)addr, size, length, DISP_LINE_LEN/size);
	addr += size*length;

	dp_last_addr = addr;
	dp_last_length = length;
	dp_last_size = size;
	return (rc);
}

int do_mem_mw (shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	u64 writeval;
#else
	ulong writeval;
#endif
	ulong	addr, count;
	int	size;

	if ((argc < 3) || (argc > 4))
		return -EINVAL;

	/* Check for size specification.
	*/
	if ((size = cmd_get_data_size(argv[0], 4)) < 1)
		return -EINVAL;

	/* Address is specified since argc > 1
	*/
	addr = simple_strtoul(argv[1], NULL, 16);
	addr += base_address;

	/* Get the value to write.
	*/
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
	writeval = simple_strtoull(argv[2], NULL, 16);
#else
	writeval = simple_strtoul(argv[2], NULL, 16);
#endif

	/* Count ? */
	if (argc == 4) {
		count = simple_strtoul(argv[3], NULL, 16);
	} else {
		count = 1;
	}

	while (count-- > 0) {
		if (size == 4)
			*((u32  *)addr) = (u32)writeval;
#ifdef CONFIG_SYS_SUPPORT_64BIT_DATA
		else if (size == 8)
			*((u64 *)addr) = (u64)writeval;
#endif
		else if (size == 2)
			*((ushort *)addr) = (ushort)writeval;
		else
			*((u_char *)addr) = (u_char)writeval;
		addr += size;
	}
	return 0;
}

extern int do_interrupt_stat(shell_cmd_t *, const unsigned int, const char **);
extern int do_bbmem_cache_testing(shell_cmd_t *, const unsigned int, const char **);
extern int do_arch_timer_testing(shell_cmd_t *, const unsigned int, const char **);
extern int do_clk_testing(shell_cmd_t *, const unsigned int, const char **);
extern int do_reset_testing(shell_cmd_t *, const unsigned int, const char **);
extern int do_sim_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_max2112_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_qspi_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_gpio_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_wdt_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_pll_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
//extern int do_spi_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_flash_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_spi3wire_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_jffs2_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_fat_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_mmc_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_timer_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_i2c_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_rtc_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_w5500_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_max2769_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_uart_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_mbox_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_dma_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_hardware_timer_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_thread_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_mthread_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_spi4_gpio_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_mcp2515_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_bmi160_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);
extern int do_pwm_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv);

void register_common_cmd(DYN_SHELL_ITEM_T* p_common_shell)
{
	DVT_SHELL_ADD_CMD(p_common_shell,
			"mw",0,
			"mw             [.b/.w/.h] [addr] [value] [length]",
			NULL,
			do_mem_mw); 
	DVT_SHELL_ADD_CMD(p_common_shell,
			"md",0,
			"md             [.b/./w/.h] [addr] [length]",
			NULL,
			do_mem_md);

    DVT_SHELL_ADD_CMD(p_common_shell,
			"interrupt",0,
			"interrupt      display interrupt stats of eCos os",
			NULL,
			do_interrupt_stat); 

#ifndef CYGPKG_HAL_ARM_BP2018
	DVT_SHELL_ADD_CMD(p_common_shell,
			"bbmem",0,
			"bbmem          enable bbmem cache, and do some testing",
			NULL,
			do_bbmem_cache_testing); 
	DVT_SHELL_ADD_CMD(p_common_shell,
			"archtimer",0,
			"archtimer      archtimer testing",
			NULL,
			do_arch_timer_testing);

   	DVT_SHELL_ADD_CMD(p_common_shell,
			"clk",0,
			"clk            clk_set/clk_get testing",
			NULL,
			do_clk_testing); 

    DVT_SHELL_ADD_CMD(p_common_shell,
			"reset",0,
			"reset          reset testing",
			NULL,
			do_reset_testing); 

	DVT_SHELL_ADD_CMD(p_common_shell,
			"gpio",0,
			"gpio            num output[o=0, i=1]",
			NULL,
			do_gpio_testing);

    DVT_SHELL_ADD_CMD(p_common_shell,
			"wdt",0,
			"wdt            init_time[ms] modify_time[ms]",
			NULL,
			do_wdt_testing);
    
    DVT_SHELL_ADD_CMD(p_common_shell,
			"pll",0,
			"pll            set/get pll_num[0..3] rate_kHz(for set PLL)",
			NULL,
			do_pll_testing);

    DVT_SHELL_ADD_CMD(p_common_shell,
			"timer",0,
			"timer 10",
			NULL,
			do_timer_testing);
#endif

#ifdef CYGPKG_IO_COMMON_SIM_ARM_BP2016
    DVT_SHELL_ADD_CMD(p_common_shell,
			"sim",0,
			"sim            sim auto",
			NULL,
			do_sim_testing);
#endif

#ifdef CYGPKG_BP2016_I2C_DEV_MAX2112
    DVT_SHELL_ADD_CMD(p_common_shell,
			"max2112",0,
			"max2112            max2112 auto",
			NULL,
			do_max2112_testing);
#endif

    DVT_SHELL_ADD_CMD(p_common_shell,
            "bang",0,
            "bang            bang auto",
            NULL,
            do_spi4_gpio_testing);

#ifdef CYGPKG_DEVS_SPI_ARM_BP2016
#if 0
    DVT_SHELL_ADD_CMD(p_common_shell,
			"spi",0,
			"spi            spi auto",
			NULL,
			do_spi_testing);
#endif

    DVT_SHELL_ADD_CMD(p_common_shell,
			"spi4",0,
			"spi4       spi4 auto",
			NULL,
			do_spi3wire_testing);

    DVT_SHELL_ADD_CMD(p_common_shell,
			"flash",0,
			"flash init/program/read",
			NULL,
			do_flash_testing);

    DVT_SHELL_ADD_CMD(p_common_shell,
            "w5500",0,
            "w5500 reset/init/write/read",
            NULL,
            do_w5500_testing);

    DVT_SHELL_ADD_CMD(p_common_shell,
			"max2769",0,
			"max2769 init/config",
			NULL,
			do_max2769_testing);

    DVT_SHELL_ADD_CMD(p_common_shell,
            "mcp",0,
            "mcp reset/init/write/read",
            NULL,
            do_mcp2515_testing);

    DVT_SHELL_ADD_CMD(p_common_shell,
            "bmi",0,
            "bmi reset/init/write/read",
            NULL,
            do_bmi160_testing);
#endif

#ifdef CYGPKG_DEVS_QSPI_BP2016
    DVT_SHELL_ADD_CMD(p_common_shell,
			"qspi",0,
			"qspi			qspi auto",
			NULL,
			do_qspi_testing);
#endif

#ifdef CYGPKG_FS_JFFS2
    DVT_SHELL_ADD_CMD(p_common_shell,
			"jffs2",0,
			"jffs mount/mkdir/rm/ls/umount/read/test",
			NULL,
			do_jffs2_testing);
#endif

#ifdef CYGVAR_DEVS_BP2016_EVATRONIX_SDMMC_EN
    DVT_SHELL_ADD_CMD(p_common_shell,
			"mmc",0,
			"mmc init/rd/wr",
			NULL,
			do_mmc_testing);
#endif

#ifdef CYGPKG_FS_FAT
    DVT_SHELL_ADD_CMD(p_common_shell,
			"fat",0,
			"fat mount/auto/mkdir/rm/ls/umount/read/test",
			NULL,
			do_fat_testing);
#endif

#ifdef CYGPKG_IO_I2C
    DVT_SHELL_ADD_CMD(p_common_shell,
			"i2c",0,
			"i2c init/read/write",
			NULL,
			do_i2c_testing);

    DVT_SHELL_ADD_CMD(p_common_shell,
			"rtc",0,
			"rtc",
			NULL,
			do_rtc_testing);
#endif

#ifdef CYGPKG_IO_SERIAL_ARM_BP2016
    DVT_SHELL_ADD_CMD(p_common_shell,
			"uart",0,
			"uart [port] [baud] [read/write/rtimeout] [len] [timeout<tick>]",
			NULL,
			do_uart_testing);
#endif

    DVT_SHELL_ADD_CMD(p_common_shell,
            "mbox",0,
            "mbox test",
            NULL,
            do_mbox_testing);

#ifdef CYGPKG_IO_COMMON_DMA_ARM_BP2016
    DVT_SHELL_ADD_CMD(p_common_shell,
            "dma",0,
            "dma [loop]",
            NULL,
            do_dma_testing);
#endif

    DVT_SHELL_ADD_CMD(p_common_shell,
            "pwm",0,
            "pwm [id<0/1>] enable/disable/freq/duty/polarity",
            NULL,
            do_pwm_testing);

#ifdef CYGPKG_IO_COMMON_TIMER_ARM_BP2016
    DVT_SHELL_ADD_CMD(p_common_shell,
            "htimer",0,
            "htimer [id<0/1>] [timeout<us>]",
            NULL,
            do_hardware_timer_testing);

#endif

    DVT_SHELL_ADD_CMD(p_common_shell,
            "thread",0,
            "thread info",
            NULL,
            do_thread_testing);
#ifdef CYGFUN_KERNEL_THREADS_CPULOAD
    DVT_SHELL_ADD_CMD(p_common_shell,
            "mthread",0,
            "mthread [info] [loop]\r\nmthread [rand]",
            NULL,
            do_mthread_testing);
#endif
}
