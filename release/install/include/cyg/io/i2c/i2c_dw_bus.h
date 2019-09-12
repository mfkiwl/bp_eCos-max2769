#ifndef _I2C_DW_20180112_H__
#define _I2C_DW_20180112_H__

#include <pkgconf/devs_bp2016_i2c_dw.h>
#include <cyg/hal/regs/i2c_dw.h> 

#define BIT(nr) (1 << (nr))

#define dw_readl(addr) ((volatile u32)(addr))
#define dw_writel(val, addr) ((addr) = (val)) 

#define CYGNUM_HAL_INT_PRIO_I2C0	(1)
#define CYGNUM_HAL_INT_PRIO_I2C1	(1)

/**
 * struct i2c_msg - an I2C transaction segment beginning with START
 * @addr: Slave address, either seven or ten bits.  When this is a ten
 *	bit address, I2C_M_TEN must be set in @flags and the adapter
 *	must support I2C_FUNC_10BIT_ADDR.
 * @flags: I2C_M_RD is handled by all adapters.  No other flags may be
 *	provided unless the adapter exported the relevant I2C_FUNC_*
 *	flags through i2c_check_functionality().
 * @len: Number of data bytes in @buf being read from or written to the
 *	I2C slave address.  For read transactions where I2C_M_RECV_LEN
 *	is set, the caller guarantees that this buffer can hold up to
 *	32 bytes in addition to the initial length byte sent by the
 *	slave (plus, if used, the SMBus PEC); and this value will be
 *	incremented by the number of block data bytes received.
 * @buf: The buffer into which data is read, or from which it's written.
 *
 * An i2c_msg is the low level representation of one segment of an I2C
 * transaction.  It is visible to drivers in the @i2c_transfer() procedure,
 * to userspace from i2c-dev, and to I2C adapter drivers through the
 * @i2c_adapter.@master_xfer() method.
 *
 * Except when I2C "protocol mangling" is used, all I2C adapters implement
 * the standard rules for I2C transactions.  Each transaction begins with a
 * START.  That is followed by the slave address, and a bit encoding read
 * versus write.  Then follow all the data bytes, possibly including a byte
 * with SMBus PEC.  The transfer terminates with a NAK, or when all those
 * bytes have been transferred and ACKed.  If this is the last message in a
 * group, it is followed by a STOP.  Otherwise it is followed by the next
 * @i2c_msg transaction segment, beginning with a (repeated) START.
 *
 * Alternatively, when the adapter supports I2C_FUNC_PROTOCOL_MANGLING then
 * passing certain @flags may have changed those standard protocol behaviors.
 * Those flags are only for use with broken/nonconforming slaves, and with
 * adapters which are known to support the specific mangling options they
 * need (one or more of IGNORE_NAK, NO_RD_ACK, NOSTART, and REV_DIR_ADDR).
 */
struct i2c_msg {
	cyg_uint16 addr;	/* slave address			*/
	cyg_uint16 flags;
#define I2C_M_TEN		0x0010	/* this is a ten bit chip address */
#define I2C_M_RD		0x0001	/* read data, from slave to master */
#define I2C_M_STOP		0x8000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NOSTART		0x4000	/* if I2C_FUNC_NOSTART */
#define I2C_M_REV_DIR_ADDR	0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK	0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NO_RD_ACK		0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_RECV_LEN		0x0400	/* length will be first received byte */
	cyg_uint16 len;		/* msg length				*/
	cyg_uint8 *buf;		/* pointer to msg data			*/
};

typedef struct cyg_dw_i2c_bus {
	HWP_I2C_AP_T 		*hwp_i2c;

	int					cmd_err;
	struct i2c_msg		msgs[2];
	int					msgs_num;
	int					msg_write_idx;
	cyg_uint32			tx_buf_len;
	cyg_uint8		*tx_buf;
	int					msg_read_idx;
	cyg_uint32			rx_buf_len;
	cyg_uint8			*rx_buf;
	int					msg_err;
	cyg_uint32			status;
	cyg_uint32			abort_source;
	cyg_uint32			tx_fifo_depth;
	cyg_uint32			rx_fifo_depth;
	int					rx_outstanding;
	int 				master_cfg;
	int 				repeat;

    cyg_vector_t     	i2c_isrvec;
    int              	i2c_isrpri;
    cyg_uint8        	i2c_mode;
    cyg_drv_mutex_t  	i2c_lock; // For synchronizing between DSR and foreground
    cyg_drv_cond_t   	i2c_wait;
    cyg_handle_t     	i2c_interrupt_handle;// For initializing the interrupt
    cyg_interrupt    	i2c_interrupt_data;

}cyg_dw_i2c_bus;

//==========================================================================
// I2C driver interface
//==========================================================================
externC void        cyg_dw_i2c_init(struct cyg_i2c_bus*);
externC cyg_uint32  cyg_dw_i2c_tx(const cyg_i2c_device*, 
                                       cyg_bool, const cyg_uint8*, 
                                       cyg_uint32, cyg_bool);
externC cyg_uint32  cyg_dw_i2c_rx(const cyg_i2c_device*, 
                                       cyg_bool, cyg_uint8*, 
                                       cyg_uint32, cyg_bool, cyg_bool);
externC void        cyg_dw_i2c_stop(const cyg_i2c_device*);

#ifdef CYGVAR_DEVS_BP2016_I2C_BUS0_EN
externC cyg_i2c_bus bp2016_i2c_bus0;
#endif	//CYGVAR_DEVS_BP2016_I2C_BUS0_EN

#ifdef CYGVAR_DEVS_BP2016_I2C_BUS1_EN
externC cyg_i2c_bus bp2016_i2c_bus1;
#endif //CYGVAR_DEVS_BP2016_I2C_BUS1_EN

#endif //_I2C_DW_20180112_H__
