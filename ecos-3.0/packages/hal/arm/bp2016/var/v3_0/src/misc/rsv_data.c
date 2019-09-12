#include <pkgconf/hal.h>
#include <string.h>
#include <stdio.h>

#include <cyg/infra/cyg_type.h>         // base types

#include <cyg/hal/plf/common_def.h>
#include <cyg/hal/plf/chip_config.h>
#include <cyg/hal/misc/mbr_if.h>
#include <cyg/hal/misc/auth_if.h>
#include <cyg/hal/misc/rsv_save.h>
#include <cyg/hal/regs/efuse.h>

static struct reserved_area_data * rsv_save;
static struct auth_fmt_data * auth_config;

static u16 crc16(u8 *buf, int len)
{
    int i;
    u16 cksum;
    u8 s, t;
    u32 r;

    cksum = 0;
    for (i = 0;  i < len;  i++) {
        s = *buf++ ^ (cksum >> 8);
        t = s ^ (s >> 4);
        r = (cksum << 8) ^
            t       ^
            (t << 5) ^
            (t << 12);
        cksum = r;
    }
    return cksum;
}

void rsv_data_init(void)
{
    rsv_save =(struct reserved_area_data*)CONFIG_DATA_RESEVED_ADDR;
    auth_config = (struct auth_fmt_data  *)rsv_save->auth_code;
}

bool is_auth_data_valid(u32 chipid)
{
	u16 crc16_v;

	if(auth_config->magic != AUTH_MAGIC) {
		return  false;
	}

	crc16_v = crc16((u8 *)auth_config, sizeof(struct auth_fmt_data) - sizeof(u16));		
	if(crc16_v != auth_config->crc16) {
		return false;
	}

	if(chipid != auth_config->chipid) {
		return false;
    }

	return true;
}

bool is_auth_bbjtag_disabled(void)
{
	return (auth_config->se_flags & SE_BB_JTAG_DISABLE) > 0 ? true:false;
}

bool is_auth_apjtag_disabled(void)
{
	return (auth_config->se_flags & SE_AP_JTAG_DISABLE) > 0 ? true:false;
}

bool is_auth_bl_verf_disabled(void)
{
	return (auth_config->se_flags & SE_BL_VERF_DISABLE) > 0 ? true:false;
}

bool is_auth_fw_verf_disabled(void)
{
	return (auth_config->se_flags & SE_FW_VERF_DISABLE) > 0 ? true:false;
}

bool is_auth_ap_key_enabled(void)
{
	return (auth_config->se_flags & SE_AP_KEY_ENABLE) > 0 ? true:false;
}

bool is_auth_trace_disabled(void)
{
	return (auth_config->se_flags & SE_TRACE_DISABLE) > 0 ? true:false;
}

int get_auth_key_index(void)
{
	return (auth_config->se_flags & SE_KEY_INDEX_MASK) >> SE_KEY_INDEX_SHIFT;
}

u32 get_auth_version(void)
{
	return auth_config->version;
}

u32 get_auth_serial_number(void)
{
	return auth_config->sn;
}

void print_authcode_info(void)
{
	struct auth_fmt_data  * auth = auth_config;
	printf("\r\n*************Authcode Info***********\n\n");
	printf("version        : %d.%dv\n", 
		(auth->version >> 16) & 0xffff, auth->version & 0xffff);
	printf("chipid         : %02x-%02x-%02x-%02x\n", 
		(auth->chipid >> 24) & 0xff, 
		(auth->chipid >> 16) & 0xff, 
		(auth->chipid >> 8) & 0xff, 
		auth->chipid & 0xff);
	printf("sn             : %02x-%02x-%02x-%02x\n", 
		(auth->sn >> 24) & 0xff, 
		(auth->sn >> 16) & 0xff, 
		(auth->sn >> 8) & 0xff, 
		auth->sn & 0xff);	

	printf("start date     : %04x-%02x-%02x \n", 
		auth->start_date >> 16, 
		(auth->start_date >> 8) & 0xff, 
		auth->start_date & 0xff);	
	printf("end date       : %04x-%02x-%02x \n", 
		auth->end_date >> 16, 
		(auth->end_date >> 8) & 0xff, 
		auth->end_date & 0xff);	

	printf("*************security flags ***************\n");
	printf("*%s *\n", 
		is_auth_bbjtag_disabled() ? " disable bbjtag":" enable bbjtag");
	printf("*%s *\n", 
		is_auth_apjtag_disabled() ? " disable apjtag":" enable apjtag");
	printf("*%s *\n", 
		is_auth_bl_verf_disabled() ? " bl bypass securiy verify" : " bl need verify");
	printf("*%s *\n", 
		is_auth_fw_verf_disabled() ? " fw bypass securiy verify" : " fw need verify");
	printf("* key index %d *\n", get_auth_key_index());
	printf("*%s *\n", 
		is_auth_ap_key_enabled() ? " user key enable ":" user key disable" );
	printf("*%s *\n", 
		is_auth_trace_disabled() ? " trace disable ":" trace enable" );

	printf("*************security flags end ***********\n\n");
}

