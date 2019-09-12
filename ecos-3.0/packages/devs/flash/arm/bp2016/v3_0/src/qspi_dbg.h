#ifndef __QSPI_DBG_20180204_H__
#define __QSPI_DBG_20180204_H__

#define qspi_printf(fmt, args...)		\
	diag_printf("[QSPI] %s %d: " fmt "\n", __func__, __LINE__,  ##args);

//#define QSPI_DEBUG
#ifdef QSPI_DEBUG
#define qspi_debug(fmt, args...)		\
	diag_printf("[QSPI dbg] %s %d: " fmt "\n", __func__, __LINE__,  ##args);
#else
#define qspi_debug(fmt, args...)
#endif

#endif

