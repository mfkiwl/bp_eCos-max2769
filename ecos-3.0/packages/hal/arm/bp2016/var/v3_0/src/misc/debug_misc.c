#include <cyg/kernel/kapi.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <cyg/hal/a7/cortex_a7.h>

#ifdef CYGFUN_KERNEL_THREADS_CPULOAD
extern cyg_uint64 tick_sch_start;
#endif

extern unsigned char * _stext, *_etext, *__exception_stack_base, *__exception_stack, *__undef_exception_stack;
extern unsigned char * cyg_interrupt_stack_base, * cyg_interrupt_stack;
extern unsigned char * __startup_stack_base, *__startup_stack;
extern unsigned char * __ram_data_start, *__ram_data_end;
static bool sp_addr_cross(cyg_uint32 addr)
{
    if(addr >= (cyg_uint32)&__exception_stack_base && addr < (cyg_uint32)&__exception_stack)
            return true;
    if(addr >= (cyg_uint32)&cyg_interrupt_stack_base && addr < (cyg_uint32)&cyg_interrupt_stack)
            return true;
    if(addr >= (cyg_uint32)&__startup_stack_base && addr < (cyg_uint32)&__startup_stack)
            return true;
    if(addr >= (cyg_uint32)&__exception_stack && addr < (cyg_uint32)&__undef_exception_stack)
            return true;
    return false;
}

bool run_in_int_env(void)
{
    cyg_uint32 cur_sp;
    HAL_GET_CURR_SP(cur_sp);
    return sp_addr_cross(cur_sp);
}

void show_all_thread_infor(void)
{
    cyg_thread_info info;
    cyg_handle_t thread;
    cyg_uint16 id;

    thread = 0;
    id = 0;

	cyg_scheduler_lock();
    while(cyg_thread_get_next(&thread, &id) == true) {
        if(cyg_thread_get_info(thread, id, &info) == true) {
            if(info.name != NULL)
                printf("****************Thread Name: %s \r\n", info.name);
            printf("     Id = %d\r\n", info.id);
            switch(info.state) {
                case 0:
                    printf("     State = %d, RUNNING\r\n", info.state);
                    break;
                case 1:
                    printf("     State = %d, SLEEPING\r\n", info.state);
                    break;
                case 2:
                    printf("     State = %d, COUNTSLEEP\r\n", info.state);
                    break;
                case 4:
                    printf("     State = %d, SUSPENDED\r\n", info.state);
                    break;
                case 8:
                    printf("     State = %d, CREATING\r\n", info.state);
                    break;
                case 16:
                     printf("     State = %d, EXITED\r\n", info.state);
                    break;
                default:
                    printf("     State = %d\r\n", info.state);
                    break;
            }
            printf("     Set pri = %d\r\n", info.set_pri);
            printf("     Cur pri = %d\r\n", info.cur_pri);
            printf("     statck_base = 0x%08x\r\n", info.stack_base);
            printf("     statck_size= %d\r\n", info.stack_size);
            printf("     statck_used= %d\r\n", info.stack_used);
        }
        printf("\r\n\r\n");
    }
    printf("********************End\r\n\r\n");
	cyg_scheduler_unlock();
}

#ifdef CYGFUN_KERNEL_THREADS_CPULOAD
extern cyg_uint64 tick_sch_start;
float cyg_thread_get_cpuload(void)
{
    cyg_uint64 curr_tick;
    cyg_handle_t thread;
    cyg_uint16 id;
    double rate = 0.0;
    cyg_thread_info info;
    thread = cyg_thread_idle_thread();
    //id = ((Cyg_Thread *)thread)->get_unique_id();
    id = cyg_thread_get_id(thread);
    cyg_scheduler_lock();

    if(cyg_thread_get_info(thread, id, &info) == true) {
        //curr_tick = arch_counter_get_cntpct();
        curr_tick = arch_counter_get_current();
        rate = (info.tick_total*1.0)/(curr_tick - tick_sch_start);
    }
    cyg_scheduler_unlock();
    return (1 - rate);
}

void cyg_thread_cpuload_reset(void)
{
    cyg_handle_t thread;
    cyg_uint16 id;

    cyg_handle_t idlethread = cyg_thread_idle_thread();
    thread = 0;
    id = 0;

    cyg_scheduler_lock();
    //cyg_interrupt_disable();
    while(cyg_thread_get_next(&thread, &id) == true) {
        //((Cyg_Thread *)thread)->tick_total = 0;
        ((cyg_thread *)thread)->tick_total = 0;
    }
    //tick_sch_start = arch_counter_get_cntpct();
    tick_sch_start = arch_counter_get_current();
    //Cyg_Thread *t = Cyg_Scheduler::get_current_thread();
    cyg_thread *t = cyg_thread_current();
    //t->tick_start = arch_counter_get_cntpct();
    t->tick_start = arch_counter_get_current();

    cyg_scheduler_unlock();
    //cyg_interrupt_enable();
}

char *null_name = "NONE";
void show_all_thread_cpuload(void)
{
    cyg_uint64 curr_tick;
    double rate = 0.0;
    cyg_thread_info info;
    cyg_handle_t thread;
    cyg_uint16 id;
    cyg_uint32 index = 0;

    thread = 0;
    id = 0;

	cyg_scheduler_lock();
    printf("\n\n********************Start\n");
    printf("start:%lld\n", tick_sch_start);
    printf("Index\tID\t\tThread Name\tpri\tcpuload \n");
    while(cyg_thread_get_next(&thread, &id) == true) {
        index++;
        if(cyg_thread_get_info(thread, id, &info) == true) {
            if(info.name == NULL){
                info.name = null_name;
            }

            curr_tick = arch_counter_get_cntpct();
            rate = (info.tick_total*1.0)/(curr_tick - tick_sch_start) * 100;
        printf("[%d]\t%d\t%20s\t%d\t%.2f%%\n", index, info.id, info.name, info.cur_pri, rate);
        }else{
            printf("[%d]--thread[%d] get info error!\n", index, info.id);
        }
    }
    printf("********************End\n\n");
    printf("cyg_thread_get_cpuload = %f\n\n",cyg_thread_get_cpuload());
    cyg_scheduler_unlock();
}

#endif

#ifdef CYGFUN_KERNEL_THREADS_SWITCH_STATS
extern void * cyg_get_thread_trans_table(void); 
static cyg_ucount32 tt[CYGNUM_KERNEL_TREAHD_STATS][CYGNUM_KERNEL_TREAHD_STATS];
static cyg_handle_t tlist[CYGNUM_KERNEL_TREAHD_STATS];

static cyg_ucount16 cyg_get_stat_thread_list(void)
{
	cyg_handle_t thread;
	cyg_uint16 id, len;

	thread = 0;
	id = 0;
	len = 0;
	memset(tlist, 0, sizeof(cyg_handle_t) * CYGNUM_KERNEL_TREAHD_STATS);
	while(cyg_thread_get_next(&thread, &id) == true) {
		tlist[len] = thread;
		len ++;
	}

	return len;
}

void show_thread_trans_table(void)
{
	int  dsize = sizeof(cyg_ucount32) * CYGNUM_KERNEL_TREAHD_STATS * CYGNUM_KERNEL_TREAHD_STATS;
	cyg_uint16  i, j;
	cyg_uint16 len;
	cyg_int8   name_t[11];
	cyg_int32  name_len;

	cyg_thread_info info;
    cyg_handle_t thread;
    cyg_uint16 id, tmp_id;

    thread = 0;
    id = 0;

	cyg_interrupt_disable();
	memcpy(tt, cyg_get_thread_trans_table(), dsize);
	cyg_interrupt_enable();
	len = cyg_get_stat_thread_list();

	if(len == 0)
		return;

	cyg_scheduler_lock();
	printf("\n");
	printf("%11s:%-11s\n", "From   ", "    To");
	printf("%11s:", " ");

	//output line name_of table
	for(i = 0; i < len; i++) {
		thread = tlist[i];
		id = cyg_thread_get_id(thread);
        if(cyg_thread_get_info(thread, id, &info) == true) {
			memset(name_t, 0, 11);
            if(info.name != NULL) {
				name_len = strlen(info.name);
				if(name_len > 10)
					name_len = 10;
				memcpy(name_t, info.name, name_len);
				printf("%10s", name_t);
			}
       }
	}
	printf("\n");

    for(i = 0; i < len; i++) {
		thread = tlist[i];
		id = cyg_thread_get_id(thread);
		if(cyg_thread_get_info(thread, id, &info) == true) {
			memset(name_t, 0, 11);
			if(info.name != NULL) {
				name_len = strlen(info.name);
				if(name_len > 10)
					name_len = 10;
				memcpy(name_t, info.name, name_len);
				printf("%11s:", name_t);
			}
		}

		if((id >=1) && (id < CYGNUM_KERNEL_TREAHD_STATS)) {	
			for(j = 0; j< len; j++) {
				tmp_id = cyg_thread_get_id(tlist[j]);
				printf("%10d", tt[id-1][tmp_id - 1]);
			}
		}else {
			printf("beyond the scope of the stat, id = %d", id);
		}

		printf("\n");
	}
	printf("\n\n");
	cyg_scheduler_unlock();
}
#else
void show_thread_trans_table(void)
{
	printf("Need enable_CYGFUN_KERNEL_THREADS_SWITCH_STATS\n");
}
#endif

#include <cyg/hal/version.h>
cyg_uint32 cyg_get_major_version(void)
{
    return MAJOR_VERSION;
}

cyg_uint32 cyg_get_minor_version(void)
{
    return MINOR_VERSION;
}

cyg_uint32 cyg_get_git_version(void)
{
    return RELEASE_GIT;
}

cyg_uint32 cyg_get_release_date(void)
{
    return RELEASE_DATE;
}

void show_release_message(void)
{
    printf("\nRelease Message:\n\tsupport board: %s\n\tMajor Version: %d\n\tMinor Version: %d\n\t"   \
            "Date: %d\n\tGit Version: %x\n", \
            SUPPORT_BOARD, MAJOR_VERSION, MINOR_VERSION, RELEASE_DATE, RELEASE_GIT);
}

