#ifndef _MISC_H__
#define _MISC_H__

extern void show_all_thread_infor(void);
extern void show_all_thread_cpuload(void);
extern void show_thread_trans_table(void);
extern cyg_uint32 cyg_get_major_version(void);
extern cyg_uint32 cyg_get_minor_version(void);
extern cyg_uint32 cyg_get_git_version(void);
extern cyg_uint32 cyg_get_release_date(void);
extern void show_release_message(void);
extern bool run_in_int_env(void);

#ifdef CYGFUN_KERNEL_THREADS_CPULOAD
float cyg_thread_get_cpuload(void);
void cyg_thread_cpuload_reset(void);
#endif

/*
 * this is for enable and disable diag_printf and printf
 */
extern void enable_hal_printf(void);
extern void disable_hal_printf(void);

#endif  /* _MISC_H__ */

