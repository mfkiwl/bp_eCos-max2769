#ifndef __MAX2112_BP2016_H__
#define	__MAX2112_BP2016_H__

enum sim_set_config_e {
    I2C_SET_REQUEST = 0,
    I2C_MODE_STD,
    I2C_MODE_FAST,
    I2C_MODE_HIGH,
	MAX2112_ATTACH_I2C,
	MAX2112_CONFIG_GPIO,
};

typedef struct max2112_req_t {
	cyg_uint8 	addr;
	cyg_uint32  len;
	void 		*buf;
}max2112_req_t;

typedef struct max2112_gpio_t {
	cyg_uint32  gpio_sel;
	cyg_uint32 	gpio0;
	cyg_uint32  gpio1;
}max2112_gpio_t;

#define  MAX2112_GET_CONFIG		(1) 

#endif //__MAX2112_BP2016_H__
