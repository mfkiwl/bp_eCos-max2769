#include <cyg/kernel/kapi.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

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
    printf("\nRelease Message:\n\tMajor Version: %d\n\tMinor Version: %d\n\t"   \
            "Date: %d\n\tGit Version: %x\n", \
            MAJOR_VERSION, MINOR_VERSION, RELEASE_DATE, RELEASE_GIT);
}

