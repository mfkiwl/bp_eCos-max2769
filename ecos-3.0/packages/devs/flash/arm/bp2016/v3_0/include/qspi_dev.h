#ifndef __CYG_QUAD_QSPI_H__
#define __CYG_QUAD_QSPI_H__

#include <pkgconf/devs_qspi_bp2016.h>

typedef unsigned long kernel_ulong_t;
#define SECTION(a) __attribute__((__section__(a)))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

struct flash_info {
    /* JEDEC id zero means "no ID" (most older chips); otherwise it has
     * a high byte of zero plus three data bytes: the manufacturer id,
     * then a two byte device id.
     */
    cyg_uint32 jedec_id;
    cyg_uint16 ext_id;

    /* The size listed here is what works with SPINOR_OP_SE, which isn't
     * necessarily called a "sector" by the vendor.
     */
    unsigned sector_size;
    cyg_uint16 n_sectors;

    cyg_uint16 page_size;
    cyg_uint16 addr_width;

    cyg_uint16 flags;
#define SECT_4K 0x01           /* SPINOR_OP_BE_4K works uniformly */
#define SPI_NOR_NO_ERASE 0x02  /* No erase command needed */
#define SST_WRITE 0x04         /* use SST byte programming */
#define SPI_NOR_NO_FR 0x08     /* Can't do fastread */
#define SECT_4K_PMC 0x10       /* SPINOR_OP_BE_4K_PMC works uniformly */
#define SPI_NOR_DUAL_READ 0x20 /* Flash supports Dual Read */
#define SPI_NOR_QUAD_READ 0x40 /* Flash supports Quad Read */
#define USE_FSR 0x80           /* use flag status register */
};

#define INFO(_jedec_id, _ext_id, _sector_size, _n_sectors, _flags)                                 \
    ((kernel_ulong_t) & (struct flash_info){                                                       \
                            .jedec_id = (_jedec_id),                                               \
                            .ext_id = (_ext_id),                                                   \
                            .sector_size = (_sector_size),                                         \
                            .n_sectors = (_n_sectors),                                             \
                            .page_size = 256,                                                      \
                            .flags = (_flags),                                                     \
                        })

#define SPI_NAME_SIZE 32
#define SPI_MODULE_PREFIX "spi:"

struct spi_device_id {
    char name[SPI_NAME_SIZE];
    kernel_ulong_t driver_data; /* Data private to the driver */
};

/* NOTE: double check command sets and memory organization when you add
 * more nor chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 */

static const struct spi_device_id spi_nor_ids[] = {
    /* Spansion -- single (large) sector size only, at least
         * for the chips listed here (without boot sectors).
         */
    {"s25sl032p", INFO(0x010215, 0x4d00, 64 * 1024, 64, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ)},
    {"s25sl064p", INFO(0x010216, 0x4d00, 64 * 1024, 128, 0)},
    {"s25fl256s0", INFO(0x010219, 0x4d00, 256 * 1024, 128, 0)},
    {"s25fl256s1", INFO(0x010219, 0x4d01, 64 * 1024, 512, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ)},
    {"s25fl512s", INFO(0x010220, 0x4d00, 256 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ)},
    {"s70fl01gs", INFO(0x010221, 0x4d00, 256 * 1024, 256, 0)},
    {"s25sl12800", INFO(0x012018, 0x0300, 256 * 1024, 64, 0)},
    {"s25sl12801", INFO(0x012018, 0x0301, 64 * 1024, 256, 0)},
    {"s25fl129p0", INFO(0x012018, 0x4d00, 256 * 1024, 64, 0)},
    {"s25fl129p1", INFO(0x012018, 0x4d01, 64 * 1024, 256, 0)},
    {"s25sl004a", INFO(0x010212, 0, 64 * 1024, 8, 0)},
    {"s25sl008a", INFO(0x010213, 0, 64 * 1024, 16, 0)},
    {"s25sl016a", INFO(0x010214, 0, 64 * 1024, 32, 0)},
    {"s25sl032a", INFO(0x010215, 0, 64 * 1024, 64, 0)},
    {"s25sl064a", INFO(0x010216, 0, 64 * 1024, 128, 0)},
    {"s25fl008k", INFO(0xef4014, 0, 64 * 1024, 16, SECT_4K)},
    {"s25fl016k", INFO(0xef4015, 0, 64 * 1024, 32, SECT_4K)},
    {"s25fl064k", INFO(0xef4017, 0, 64 * 1024, 128, SECT_4K)},

    /* GigaDevice */
    {"gd25q32", INFO(0xc84016, 0, 64 * 1024, 64, SECT_4K)},
    {"gd25q64", INFO(0xc84017, 0, 64 * 1024, 128, SECT_4K)},
};

/*
 * Note on opcode nomenclature: some opcodes have a format like
 * SPINOR_OP_FUNCTION{4,}_x_y_z. The numbers x, y, and z stand for the number
 * of I/O lines used for the opcode, address, and data (respectively). The
 * FUNCTION has an optional suffix of '4', to represent an opcode which
 * requires a 4-byte (32-bit) address.
 */

/* Flash opcodes. */
#define SPINOR_OP_WREN 0x06         /* Write enable */
#define SPINOR_OP_WRDI 0x04         /* Write disable */
#define SPINOR_OP_RDSR 0x05         /* Read status register */
#define SPINOR_OP_WRSR 0x01         /* Write status register 1 byte */
#define SPINOR_OP_READ 0x03         /* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST 0x0b    /* Read data bytes (high frequency) */
#define SPINOR_OP_READ_1_1_2 0x3b   /* Read data bytes (Dual SPI) */
#define SPINOR_OP_READ_1_1_4 0x6b   /* Read data bytes (Quad SPI) */
#define SPINOR_OP_READ_DUAL_IO 0xbb /* Read data bytes (Dual io SPI */
#define SPINOR_OP_READ_QUAD_IO 0xeb /* Read data bytes (Quad SPI*/
#define SPINOR_OP_PP 0x02           /* Page program (up to 256 bytes) */
#define SPINOR_OP_BE_4K 0x20        /* Erase 4KiB block */
#define SPINOR_OP_BE_4K_PMC 0xd7    /* Erase 4KiB block on PMC chips */
#define SPINOR_OP_BE_32K 0x52       /* Erase 32KiB block */
#define SPINOR_OP_CHIP_ERASE 0xc7   /* Erase whole flash chip */
#define SPINOR_OP_SE 0xd8           /* Sector erase (usually 64KiB) */
#define SPINOR_OP_RD_DEVID 0x90     /* Read Manu ID/ DEV ID */
#define SPINOR_OP_RDID 0x9f         /* Read JEDEC ID */
#define SPINOR_OP_RDCR 0x35         /* Read configuration register */
#define SPINOR_OP_RDFSR 0x70        /* Read flag status register */
#define SPINOR_OP_RESET 0x99
#define SPINOR_OP_ENABLE_RESET 0x66

/* 4-byte address opcodes - used on Spansion and some Macronix flashes. */
#define SPINOR_OP_READ4 0x13       /* Read data bytes (low frequency) */
#define SPINOR_OP_READ4_FAST 0x0c  /* Read data bytes (high frequency) */
#define SPINOR_OP_READ4_1_1_2 0x3c /* Read data bytes (Dual SPI) */
#define SPINOR_OP_READ4_1_1_4 0x6c /* Read data bytes (Quad SPI) */
#define SPINOR_OP_PP_4B 0x12       /* Page program (up to 256 bytes) */
#define SPINOR_OP_SE_4B 0xdc       /* Sector erase (usually 64KiB) */

/* Used for SST flashes only. */
#define SPINOR_OP_BP 0x02     /* Byte program */
#define SPINOR_OP_WRDI 0x04   /* Write disable */
#define SPINOR_OP_AAI_WP 0xad /* Auto address increment word program */

/* Used for Macronix and Winbond flashes. */
#define SPINOR_OP_EN4B 0xb7 /* Enter 4-byte mode */
#define SPINOR_OP_EX4B 0xe9 /* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define SPINOR_OP_BRWR 0x17    /* Bank register write */
#define SPINOR_OP_BRRD 0x16    /* Bank register read*/
#define SPINOR_OP_BR_BA25 0x02 /* Bank register: Address 25-bit enable*/
#define SPINOR_OP_BR_BA24 0x01 /* Bank register: Address 24-bit enable*/

/* Status Register bits. */
#define SR_WIP 1 /* Write in progress */
#define SR_WEL 2 /* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define SR_BP0 4     /* Block protect 0 */
#define SR_BP1 8     /* Block protect 1 */
#define SR_BP2 0x10  /* Block protect 2 */
#define SR_SRWD 0x80 /* SR write protect */

#define SR_QUAD_EN_MX 0x40 /* Macronix Quad I/O */

/* Flag Status Register bits */
#define FSR_READY 0x80

/* Configuration Register bits. */
#define CR_QUAD_EN_SPAN 0x2    /* Spansion Quad I/O */
#define CR_BPNV_EN_SPAN 0x8    /* Spansion BPNV I/O: 1 = BP2-0 volatile*/
#define CR_TBPROT_EN_SPAN 0x20 /* Spansion TBProt I/O: 1=low address */

#define SR_WIP_SPAN 0x0    /* Spansion Quad I/O */
#define SR_E_ERR_SPAN 0x20 /* Spansion BPNV I/O: 1 = BP2-0 volatile*/
#define SR_P_ERR_SPAN 0x40 /* Spansion TBProt I/O: 1=low address */
#define SR_BP_BITS_MASK 0x1C
#define SR_BP_BITS_SHIFT(v) (v << 2)

enum read_mode {
    SPI_NOR_NORMAL = 0,
    SPI_NOR_FAST,
    SPI_NOR_DUAL,
    SPI_NOR_QUAD,
};

#if 0
/**
 * struct spi_nor_xfer_cfg - Structure for defining a Serial Flash transfer
 * @wren:		command for "Write Enable", or 0x00 for not required
 * @cmd:		command for operation
 * @cmd_pins:		number of pins to send @cmd (1, 2, 4)
 * @addr:		address for operation
 * @addr_pins:		number of pins to send @addr (1, 2, 4)
 * @addr_width:		number of address bytes
 *			(3,4, or 0 for address not required)
 * @mode:		mode data
 * @mode_pins:		number of pins to send @mode (1, 2, 4)
 * @mode_cycles:	number of mode cycles (0 for mode not required)
 * @dummy_cycles:	number of dummy cycles (0 for dummy not required)
 */
struct spi_nor_xfer_cfg {
	cyg_uint8		wren;
	cyg_uint8		cmd;
	cyg_uint8		cmd_pins;
	cyg_uint32		addr;
	cyg_uint8		addr_pins;
	cyg_uint8		addr_width;
	cyg_uint8		mode;
	cyg_uint8		mode_pins;
	cyg_uint8		mode_cycles;
	cyg_uint8		dummy_cycles;
};

#define SPI_NOR_MAX_CMD_SIZE 8
enum spi_nor_ops {
	SPI_NOR_OPS_READ = 0,
	SPI_NOR_OPS_WRITE,
	SPI_NOR_OPS_ERASE,
	SPI_NOR_OPS_LOCK,
	SPI_NOR_OPS_UNLOCK,
};

/**
 * struct spi_nor - Structure for defining a the SPI NOR layer
 * @page_size:		the page size of the SPI NOR
 * @addr_width:		number of address bytes
 * @erase_opcode:	the opcode for erasing a sector
 * @read_opcode:	the read opcode
 * @read_dummy:		the dummy needed by the read operation
 * @program_opcode:	the program opcode
 * @flash_read:		the mode of the read
 * @sst_write_second:	used by the SST write operation
 * @cfg:		used by the read_xfer/write_xfer
 * @cmd_buf:		used by the write_reg
 * @prepare:		[OPTIONAL] do some preparations for the
 *			read/write/erase/lock/unlock operations
 * @unprepare:		[OPTIONAL] do some post work after the
 *			read/write/erase/lock/unlock operations
 * @read_xfer:		[OPTIONAL] the read fundamental primitive
 * @write_xfer:		[OPTIONAL] the writefundamental primitive
 * @read_reg:		[DRIVER-SPECIFIC] read out the register
 * @write_reg:		[DRIVER-SPECIFIC] write data to the register
 * @read_id:		[REPLACEABLE] read out the ID data, and find
 *			the proper spi_device_id
 * @wait_till_ready:	[REPLACEABLE] wait till the NOR becomes ready
 * @read:		[DRIVER-SPECIFIC] read data from the SPI NOR
 * @write:		[DRIVER-SPECIFIC] write data to the SPI NOR
 * @erase:		[DRIVER-SPECIFIC] erase a sector of the SPI NOR
 *			at the offset @offs
 * @priv:		the private data
 */
struct spi_nor {
	cyg_uint32			page_size;
	cyg_uint8			addr_width;
	cyg_uint8			erase_opcode;
	cyg_uint8			read_opcode;
	cyg_uint8			read_dummy;
	cyg_uint8			program_opcode;
	enum read_mode		flash_read;
	bool			sst_write_second;
	struct spi_nor_xfer_cfg	cfg;
	cyg_uint8			cmd_buf[SPI_NOR_MAX_CMD_SIZE];

	int (*prepare)(struct spi_nor *nor, enum spi_nor_ops ops);
	void (*unprepare)(struct spi_nor *nor, enum spi_nor_ops ops);
	int (*read_xfer)(struct spi_nor *nor, struct spi_nor_xfer_cfg *cfg,
			 cyg_uint8 *buf, size_t len);
	int (*write_xfer)(struct spi_nor *nor, struct spi_nor_xfer_cfg *cfg,
			  cyg_uint8 *buf, size_t len);
	int (*read_reg)(struct spi_nor *nor, cyg_uint8 opcode, cyg_uint8 *buf, int len);
	int (*write_reg)(struct spi_nor *nor, cyg_uint8 opcode, cyg_uint8 *buf, int len,
			int write_enable);
	const struct spi_device_id *(*read_id)(struct spi_nor *nor);
	int (*wait_till_ready)(struct spi_nor *nor);

	int (*read)(struct spi_nor *nor, loff_t from,
			size_t len, size_t *retlen, u_char *read_buf);
	void (*write)(struct spi_nor *nor, loff_t to,
			size_t len, size_t *retlen, const u_char *write_buf);
	int (*erase)(struct spi_nor *nor, loff_t offs);

	void *priv;
};
#endif

struct qspi_fl_info {
    char name[16];
    cyg_uint32 jedec; /*0x010220 s25fl512s; 0xc84020 gd*/
    cyg_uint32 blk_size;
    cyg_uint32 blk_num;
    cyg_uint32 sector_size;

    int (*read_rdid)(cyg_uint8 rdid_cmd, int rdid_len, cyg_uint8 *dbuf);
    int (*read_devid)(cyg_uint8 devid_cmd, int devid_len, cyg_uint8 *dbuf);
#if (CYGPKG_DEVS_QSPI_DMA_EN > 0)
    int (*dma_read_page)(void *rx_ch, cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *rbuf, int len);
    int (*dma_write_page)(void *tx_ch, cyg_uint32 addr, cyg_uint8 *dbuf, int len);
#else
    int (*read_page)(cyg_uint8 cmd, cyg_uint32 addr, cyg_uint8 *rbuf, int len, int dfs);
    int (*write_page)(cyg_uint32 addr, cyg_uint8 *dbuf, int len);
#endif
    int (*erase)(cyg_uint8 cmd, cyg_uint32 addr);
    int (*erase_chip)(void);

    int (*config_4byte_extend)(cyg_uint32 addr, int en);

    int (*config_quad)(int en);
    int (*config_prot_region)(int region, int en);
    cyg_uint8 (*read_status)(void);
    int (*reset)(void);
    int (*block_lock)(cyg_uint32 offs);
    int (*block_unlock)(cyg_uint32 offs);
    int (*get_block_lock_status)(cyg_uint32 offs);
    int (*global_lock)(void);
    int (*global_unlock)(void);
};

#define QSPI_HW_STAT

#define QSPI_OP_SUCCESS 0
#define QSPI_OP_FAILED -1

#define BLOCK_PROTECTED 	(1)
#define BLOCK_UNPROTECTED	(0)

typedef struct qspi_timeout_t {
	cyg_uint32	ers_sec_tm;		//erase sector timeout
	cyg_uint32	ers_blk32k_tm;  //erase 32KB block timeout
	cyg_uint32	ers_blk64k_tm;	//erase 64KB block timeout
	cyg_uint32	ers_chip_tm;	//chip erase timeout
	cyg_uint32	prg_page_tm;	//page programming timeout
	cyg_uint32	wr_status_tm;	//write status register timeout
} qspi_timeout_t;

extern qspi_timeout_t tm_array[];
#endif
