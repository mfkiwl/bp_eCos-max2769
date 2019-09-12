#ifndef RSV_SAVE_H__
#define	RSV_SAVE_H__
/*Need include some header file*/
struct bl2fw_rsv_data {
	u32 load_addr;
	u32 exec_addr;
	u32 fl_addr;
	u32 dsize;
	u16 flags;
	u16 valid;
	int err_return;
};

struct reserved_area_data {
	struct	bl2fw_rsv_data bl;
	struct	bl2fw_rsv_data bbfw;
	struct	bl2fw_rsv_data apfw;

	u8   efuse_valid;
	u8   authcode_valid;
	u8   bl_se;
	u8   bbfw_se;
	u8   apfw_se;
	u8   apfw_key_en;
    u8   boot_type;
	u8   reserved_d8;
    
    u32 init0_time_count;
    u32 init1_time_count;
    u32 auth_time_count;
    u32 bl_time_count;
    u32 bbfw_time_count;
    u32 apfw_time_count;
    u32 done_time_count;
	
	u8  auth_code[AUTH_CODE_DBUF_SIZE];
	u8  mbr_data[MBR_SIZE];
};


// auth 
#define AUTH_MAGIC	0x68747561		/*"auth"*/

struct auth_fmt_data {
	u32 	magic;
	u32 	version;
	u32 	chipid;
	u32 	sn;

	u32 	feature[6];
	u32    	start_date;
	u32 	end_date;

	u16  	se_flags;
	u16  	crc16;
};



#endif
