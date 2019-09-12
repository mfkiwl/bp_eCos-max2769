//==========================================================================
//
//        thread_test.c
//
//        thread performance test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2017-10-18
// Description:   thread performance test for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <cyg/kernel/kapi.h>
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/cyg_trac.h>

#define TH_DEBUG

// this is for debug trace,
#ifdef TH_DEBUG
#define th_printf(fmt, args...)                                                               \
    diag_printf("[thread test] %s:%d " fmt "\n", __func__, __LINE__, ##args);
#else
#define th_printf(fmt, ...)
#endif

//===========================================================================
//                               DATA TYPES
//===========================================================================

#define REG_READ_UINT32( _reg_ )        (*(volatile unsigned int*)(_reg_))
void trigger_swi (void)
{
	__asm__ __volatile__("swi #4\n" ::: "memory");
}
void trigger_undefined(void)
{
	__asm__ __volatile__(".inst 0xdeff\n" ::: "memory");
}

void trigger_prefetch(void)
{
	__asm__ __volatile__("subs pc, r14, #4\n" ::: "memory");
    //__asm__ __volatile__("bkpt #3\n" ::: "memory");
}

long pointer_exception(unsigned int *p)
{
    REG_READ_UINT32(p);
    dsb();
	//__asm__ __volatile__("dsb" ::: "memory");
	return *p = 10;
}

static unsigned long test_do_go_exec (unsigned long (*entry)(int), unsigned size)
{
        return entry(0);
}

extern void show_all_thread_infor(void);

char excu[100] = {};
int do_thread_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    char mem[3] = {};
    int i;
    unsigned int ptr=9;

    th_printf("\n-------------thread thread in----------------------\n");

    if (strcmp(argv[1], "exp") == 0) {
        pointer_exception(&ptr);
        return ENOERR;
    }

    if (strcmp(argv[1], "prefetch") == 0) {

        for(i=100; i>0;i--)
            excu[i-1] = i;

        test_do_go_exec((void *)excu, 0);
        //trigger_prefetch();
        return ENOERR;
    }

    if (strcmp(argv[1], "undefine") == 0) {
        trigger_undefined();
        return ENOERR;
    }

    if (strcmp(argv[1], "swi") == 0) {
        trigger_swi ();
        return ENOERR;
    }

    if (strcmp(argv[1], "abort") == 0) {
        memset(mem, 1, 10);

        for(i=10;i>=0; i--)
        {
            th_printf(" abort %d \n", mem[10-i]/i);
        }
        return ENOERR;
    }

    if (strcmp(argv[1], "info") == 0) {
        show_all_thread_infor();
        return ENOERR;
    }

    if (strcmp(argv[1], "stack") == 0) {
        CYG_TRACE_DUMP();
        return ENOERR;
    }

    return ENOERR;
}
