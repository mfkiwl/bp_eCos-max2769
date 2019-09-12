#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>

#include "mutexdemo.h"
#include "semdemo.h"
#include "conddemo.h"

/* we install our own startup routine which sets up threads */
void cyg_user_start(void)
{
    printf("Entering demo threads' cyg_user_start() function\n");

#if (MUTEX_TEST_BP2016 > 0)
    create_mutex_thread();
    resume_mutex_thread();
#endif

#if (SEM_TEST_BP2016 > 0)
    create_sem_thread();
    resume_sem_thread();
#endif

#if (COND_TEST_BP2016 > 0)
    create_cond_thread();
    resume_cond_thread();
#endif

    cyg_scheduler_start();
}
