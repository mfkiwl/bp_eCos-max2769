#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>

#include "conddemo.h"

#if (COND_TEST_BP2016 > 0)

typedef struct thread_data_bp2016 {
    cyg_uint32 *name;
    cyg_thread thread;
    cyg_uint32 stack[COND_STACK_SIZE_BP2016];
    cyg_handle_t handle;
} thread_data_bp2016;

cyg_mutex_t mutex_lock;
cyg_cond_t cond_wait;

cyg_thread_entry_t cond_thread_0, cond_thread_1, cond_thread_2;
thread_data_bp2016 bp2016_cond_data[3];

void cond_thread_0(cyg_addrword_t data)
{
    cyg_uint32 start_time;
    cyg_uint32 end_time;

    while (1) {
        printf("\ncond_th 0\n");
        cyg_mutex_lock(&mutex_lock);
        start_time = cyg_current_time();
        cyg_cond_wait(&cond_wait);
        cyg_thread_delay(100);
        end_time = cyg_current_time();
        cyg_mutex_unlock(&mutex_lock);
        printf("\ncon0 starttime = %d, endtime = %d, usetime = %d", start_time, end_time,
               (end_time - start_time));
    }
}

void cond_thread_1(cyg_addrword_t data)
{
    cyg_bool_t wait_res;
    cyg_uint32 start_time;
    cyg_uint32 end_time;

    while (1) {
        printf("\ncond_th 1\n");
        start_time = cyg_current_time();
        cyg_mutex_lock(&mutex_lock);
        cyg_thread_delay(200);

        wait_res = cyg_cond_timed_wait(&cond_wait, 100);
        cyg_mutex_unlock(&mutex_lock);
        end_time = cyg_current_time();
        printf("\ncon1 stime = %d, etime = %d, usetime = %d", start_time, end_time,
               (end_time - start_time));
    }
}

void cond_thread_2(cyg_addrword_t data)
{
    cyg_uint32 start_time;
    cyg_uint32 end_time;

    while (1) {
        printf("\ncond_th 2\n");
        start_time = cyg_current_time();
        cyg_thread_delay(400);
        cyg_cond_signal(&cond_wait);
        end_time = cyg_current_time();
        printf("\ncon2 stime = %d, etime = %d, usetime = %d", start_time, end_time,
               (end_time - start_time));
    }
}

void create_cond_thread(void)
{
    cyg_mutex_init(&mutex_lock);
    cyg_cond_init(&cond_wait, &mutex_lock);

    cyg_thread_create(10, cond_thread_0, (cyg_addrword_t) 0, "bp2016_cond_thread0",
                      (void *) bp2016_cond_data[0].stack, 1024 * sizeof(cyg_uint32),
                      &bp2016_cond_data[0].handle, &bp2016_cond_data[0].thread);

    cyg_thread_create(13, cond_thread_1, (cyg_addrword_t) 0, "bp2016_cond_thread1",
                      (void *) bp2016_cond_data[1].stack, 1024 * sizeof(cyg_uint32),
                      &bp2016_cond_data[1].handle, &bp2016_cond_data[1].thread);

    cyg_thread_create(5, cond_thread_2, (cyg_addrword_t) 0, "bp2016_cond_thread2",
                      (void *) bp2016_cond_data[2].stack, 1024 * sizeof(cyg_uint32),
                      &bp2016_cond_data[2].handle, &bp2016_cond_data[2].thread);
}

void resume_cond_thread(void)
{
    cyg_thread_resume(bp2016_cond_data[0].handle);
    cyg_thread_resume(bp2016_cond_data[1].handle);
    cyg_thread_resume(bp2016_cond_data[2].handle);
}

#endif
