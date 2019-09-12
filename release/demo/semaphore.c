#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>

#include "semdemo.h"

#if (SEM_TEST_BP2016 > 0)

typedef struct thread_data_bp2016 {
    cyg_uint32 *name;
    cyg_thread thread;
    cyg_uint32 stack[SEM_STACK_SIZE_BP2016];
    cyg_handle_t handle;
} thread_data_bp2016;

cyg_sem_t sem_bp2016;
cyg_thread_entry_t sem_thread_0, sem_thread_1, sem_thread_2;
thread_data_bp2016 bp2016_sem_data[3];

cyg_uint32 stackusg = 0;

void sem_thread_0(cyg_addrword_t data)
{
    cyg_uint32 start_time;
    cyg_uint32 end_time, v, v_update;
    int base = 0;

    printf("thread0 start \n");

    while (1) {
        base = cyg_thread_get_stack_base(bp2016_sem_data[0].handle);
        printf("th0 base = %x\n", base);
        start_time = cyg_current_time();
        cyg_semaphore_peek(&sem_bp2016, &v);
        cyg_semaphore_wait(&sem_bp2016);
        cyg_semaphore_peek(&sem_bp2016, &v_update);
        end_time = cyg_current_time();
        printf("             <=====sem_th 0 wait %d \n", (end_time - start_time));
        printf("             <=====sem_th 0 %d-->%d \n\n", v, v_update);
        //stackusg = cyg_thread_measure_stack_usage(bp2016_sem_data[0].handle);
    }
}

void sem_thread_2(cyg_addrword_t data)
{
    cyg_int32 time_v, count_v;
    printf("thread2 start \n");
    while (1) {
        cyg_thread_delay(200);
        time_v = cyg_current_time();
        cyg_semaphore_peek(&sem_bp2016, &count_v);
        printf("%d : sem_th 2: post ===>, sem count %d\n", time_v, count_v );
        printf("th0 usage %d\n", stackusg);
        cyg_semaphore_post(&sem_bp2016);
        cyg_semaphore_peek(&sem_bp2016, &count_v);
        printf("    post after     ===>, sem ount %d\n\n",  count_v );
    }
}

void create_sem_thread(void)
{
    cyg_semaphore_init(&sem_bp2016, 0);

    cyg_thread_create(10, sem_thread_0, (cyg_addrword_t) 0, "bp2016_sem_thread0",
                      (void *) bp2016_sem_data[0].stack, SEM_STACK_SIZE_BP2016 * sizeof(cyg_uint32),
                      &bp2016_sem_data[0].handle, &bp2016_sem_data[0].thread);

    cyg_thread_create(2, sem_thread_2, (cyg_addrword_t) 0, "bp2016_sem_thread2",
                      (void *) bp2016_sem_data[2].stack, SEM_STACK_SIZE_BP2016 * sizeof(cyg_uint32),
                      &bp2016_sem_data[2].handle, &bp2016_sem_data[2].thread);
    printf("thread0 pr = 10, thread2 = 2\n");
}

void resume_sem_thread(void)
{
    cyg_thread_resume(bp2016_sem_data[0].handle);
    cyg_thread_resume(bp2016_sem_data[2].handle);
}

#endif
