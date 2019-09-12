#ifndef __COMMON_H__
#define __COMMON_H__
#include <stdio.h>
#include <stdlib.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/diag.h>

typedef  cyg_uint64 u64;
typedef  cyg_uint32 u32;
typedef  cyg_uint32 uint;
typedef  cyg_uint32 ulong;
typedef  cyg_uint64 uint64_t;
typedef  cyg_uint32 uint32_t;
typedef  cyg_uint16  uint16_t;
typedef  cyg_uint16  ushort;
typedef  cyg_uint8  uint8_t;
typedef  cyg_uint8  u_char;
typedef  cyg_int32  s32;
typedef  cyg_int64  s64;
typedef  cyg_int64  int64_t;
typedef unsigned long int uintptr_t;

#define   CONFIG_SYS_PBSIZE  768
#include "dvt_shell.h"
#include "dvt_common.h"
#include "dvt_cmd.h"

extern int printf_console(const char *fmt, ...);
#endif
