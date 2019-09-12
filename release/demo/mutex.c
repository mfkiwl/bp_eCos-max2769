#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>

#include "mutexdemo.h"

#if (MUTEX_TEST_BP2016 > 0)
typedef struct thread_data_bp2016 {
    cyg_uint32 *name;
    cyg_thread thread;
    cyg_uint32 stack[MUTEX_STACK_SIZE_BP2016];
    cyg_handle_t handle;
} thread_data_bp2016;

cyg_mutex_t cliblock;

cyg_thread_entry_t mutex_thread_0, mutex_thread_1, mutex_thread_2;
thread_data_bp2016 bp2016_mutex_data[3];

void mutex_thread_0(cyg_addrword_t data)
{
    while (1) {
        cyg_mutex_lock(&cliblock);
        printf("\nmutex_th 0\n");
        cyg_thread_delay(400);
        cyg_mutex_unlock(&cliblock);
    }
}

void mutex_thread_1(cyg_addrword_t data)
{
    cyg_thread_delay(100);
    while (1) {
        cyg_mutex_lock(&cliblock);
        printf("\nmutex_th 1\n");
        cyg_thread_delay(100);
        cyg_mutex_unlock(&cliblock);
    }
}

void mutex_thread_2(cyg_addrword_t data)
{
    cyg_thread_delay(200);
    while (1) {
        cyg_mutex_lock(&cliblock);
        printf("\nmutex_th 2\n");
        cyg_thread_delay(200);
        cyg_mutex_unlock(&cliblock);
    }
}

void create_mutex_thread(void)
{
    cyg_mutex_init(&cliblock);

    cyg_thread_create(14, mutex_thread_0, (cyg_addrword_t) 0, "bp2016_mutex_thread0",
                      (void *) bp2016_mutex_data[0].stack, 1024 * sizeof(cyg_uint32),
                      &bp2016_mutex_data[0].handle, &bp2016_mutex_data[0].thread);

    cyg_thread_create(3, mutex_thread_1, (cyg_addrword_t) 0, "bp2016_mutex_thread1",
                      (void *) bp2016_mutex_data[1].stack, 1024 * sizeof(cyg_uint32),
                      &bp2016_mutex_data[1].handle, &bp2016_mutex_data[1].thread);

    cyg_thread_create(5, mutex_thread_2, (cyg_addrword_t) 0, "bp2016_mutex_thread2",
                      (void *) bp2016_mutex_data[2].stack, 1024 * sizeof(cyg_uint32),
                      &bp2016_mutex_data[2].handle, &bp2016_mutex_data[2].thread);
}

void resume_mutex_thread(void)
{
    cyg_thread_resume(bp2016_mutex_data[0].handle);
    cyg_thread_resume(bp2016_mutex_data[1].handle);
    cyg_thread_resume(bp2016_mutex_data[2].handle);
}

#endif
