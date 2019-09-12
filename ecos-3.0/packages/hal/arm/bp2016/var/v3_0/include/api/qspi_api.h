#ifndef __QSPI_API_20180314_H_
#define __QSPI_API_20180314_H_

/*
* Function Name: qspi_flash_init 
* Condition:    initialization flash 
* Input Para:  flash_handle: handle pointer which we want to get	
* 			   name: device name as ( /dev/flash/<deviceno>/<offset>[,<length>]) 
* Output Para:
* Return value:  ENOERR:  OK
*                -ENOENT: Not found
*/
int qspi_flash_init(cyg_io_handle_t *flash_handle, const char *name);

/*
* Function Name: qspi_flash_lock
* Condition: need init qspi module
* Input Para:  handle of flash device 
* 				offs: offset on flash
* 				len: operation len
* Output Para:
* Return value:  0: OK 
*                -EINVAL: failed 
*/
int qspi_flash_lock(cyg_io_handle_t handle, cyg_uint32 offs, cyg_uint32 len);

/*
* Function Name: qspi_flash_unlock
* Condition: need init qspi module
* Input Para:  handle of flash device 
* 				offs: offset on flash
* 				len: operation len
* Output Para:
* Return value:  0: OK 
*                -EINVAL: failed 
*/
int qspi_flash_unlock(cyg_io_handle_t handle, cyg_uint32 offs, cyg_uint32 len);

/*
* Function Name: qspi_flash_erase
* Condition: need init qspi module
* Input Para:  handle of flash device 
* 				offs: offset on flash
* 				len: operation len
* Output Para:
* Return value:  0: OK 
*                -EINVAL: failed 
*/
int qspi_flash_erase(cyg_io_handle_t handle, cyg_uint32 offs, cyg_uint32 len);

/*
* Function Name: qspi_flash_sector_erase
* Condition: need init qspi module
* Input Para:  handle of flash device 
* 				offs: offset on flash
* 				len: operation len
* Output Para:
* Return value:  0: OK 
*                -EINVAL: failed 
*/
int qspi_flash_sector_erase(cyg_io_handle_t handle, cyg_uint32 offs, cyg_uint32 len);

/*
* Function Name: qspi_flash_get_flashsize 
* Condition: need init qspi module
* Input Para:  handle of flash device 
* Output Para:
* Return value:  0: get flash size failed 
*                xx: flash size 
*/
cyg_uint32 qspi_flash_get_flashsize(cyg_io_handle_t handle);

/*
* Function Name: qspi_flash_get_sectorsize
* Condition:	need init qspi module
* Input Para:  handle of flash device 
* Output Para:
* Return value:  0: flash sector size is 0 
*                xx: flash sector size 
*/
cyg_uint32 qspi_flash_get_sectorsize(cyg_io_handle_t handle);

/*
* Function Name: qspi_flash_get_flashmem
* Condition: need init qspi module
* Input Para:  handle of flash device 
* Output Para:
* Return value:  0: get flash size failed 
*                xx: flash MEM access address 
*/
cyg_uint32 qspi_flash_get_flashmem(cyg_io_handle_t handle);

/*
* Function Name: qspi_flash_get_blocksize
* Condition:	need init qspi module
* Input Para:  handle of flash device 
* Output Para:
* Return value:  0: get flash block size failed 
*                xx: flash block size 
*/
cyg_uint32 qspi_flash_get_blocksize(cyg_io_handle_t handle);


/*
* Function Name: qspi_flash_read
* Condition:	need init qspi module
* Input Para:  handle of flash device 
               void *buff-buffer for storing data
               *len- length to read
               addr- read start addr from start
* Output Para: *buff- data
* Return value:  0: get flash block size failed 
*                xx: read length 
*/
cyg_uint32 qspi_flash_read(cyg_io_handle_t handle, void *buff, cyg_uint32 *len, cyg_uint32 addr);

/*
* Function Name: qspi_flash_write
* Condition:	need init qspi module
* Input Para:  handle of flash device 
               void *buff-buffer to write
               *len- length to write
               addr- write start addr from start
* Output Para: none
* Return value:  0: get flash block size failed 
*                xx: write length 
*/
cyg_uint32 qspi_flash_write(cyg_io_handle_t handle, void *buff, cyg_uint32 *len, cyg_uint32 addr);

/*
* Function Name: qspi_flash_chip_erase
* Condition:	need init qspi module
* Input Para:  handle of flash device 
* Output Para: none
* Return value:  ENOERR: erase all chip done 
*                -EINVAL: erase error
*/
int qspi_flash_chip_erase(cyg_io_handle_t handle);

/*
* Function Name: qspi_flash_read_id
* Condition:	need init qspi module
* Input Para:  handle of flash device 
* Output Para: none
* Return value:  0xFFFFFFFF: read id error 
*                XX: flash id
*/
cyg_uint32 qspi_flash_read_id(cyg_io_handle_t handle);

extern int flashiodev_region_unlock(cyg_io_handle_t handle, cyg_uint32 region);
extern int flashiodev_region_lock(cyg_io_handle_t handle, cyg_uint32 region);
extern int flashiodev_global_unlock(cyg_io_handle_t handle);
extern int flashiodev_global_lock(cyg_io_handle_t handle);

#endif //__QSPI_API_20180314_H_
