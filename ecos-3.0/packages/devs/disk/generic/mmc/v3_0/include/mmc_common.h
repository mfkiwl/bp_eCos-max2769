#ifndef __MMC_COMMON_20180813_H__
#define __MMC_COMMON_20180813_H__

#include <cyg/infra/diag.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/kernel/kapi.h>

#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/testcase.h>

typedef unsigned long sector_t;

#define printk(fmt, args...)     diag_printf("[sdmmc] %s:%d " fmt "\n", __func__, __LINE__, ##args);
#ifdef PR_DEBUG
#define pr_debug(fmt, ...)  diag_printf((fmt), ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)
#endif

#define pr_warning(fmt, ...)  diag_printf((fmt), ##__VA_ARGS__)

# define likely(x)	__builtin_expect(!!(x), 1)
# define unlikely(x)	__builtin_expect(!!(x), 0)
#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BIT(nr) (1 << (nr))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define min(x,y) (x<y?x:y)
#define max(x,y) (x<y?y:x)

#define SDIO_ANY_ID         (~0)
#define PAGE_SIZE           (1<<12)
#define PAGE_CACHE_SIZE     (PAGE_SIZE)

#define MAX_SLOTS	2	
#define SD_CARD     1
#define MMC_CARD    2
#define EMMC_CARD   4
#define SDIO_CARD   8  

#define jiffies 100
#define HZ      100

/*
 * Don't use BUG() or BUG_ON() unless there's really no way out; one
 * example might be detecting data structure corruption in the middle
 * of an operation that can't be backed out of.  If the (sub)system
 * can somehow continue operating, perhaps with reduced functionality,
 * it's probably not BUG-worthy.
 *
 * If you're tempted to BUG(), think again:  is completely giving up
 * really the *only* solution?  There are usually better options, where
 * users don't need to reboot ASAP and can mostly shut down cleanly.
 */

void cyg_panic(const char *msg, ...);

#ifndef HAVE_ARCH_BUG
#define BUG() do { \
	printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
	cyg_panic("BUG!"); \
} while (0)
#endif

#define BUG_ON(condition) do { if (unlikely(condition)) BUG(); } while(0)

#define __WARN()    printf("Warnning: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__)

#ifndef WARN_ON
#define WARN_ON(condition) ({						\
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on))					\
		__WARN();						\
	unlikely(__ret_warn_on);					\
})
#endif

#define IS_ALIGNED(x, a)        (((x) & ((typeof(x))(a) - 1)) == 0)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define ERR_PTR(err) ((void*)(err))
#define PTR_ERR(err) ((unsigned long)(err))
#define IS_ERR(err) ((unsigned long)err > (unsigned long)-1000L)

#define rmb()       dsb()
#define wmb()       dsb()

#endif //__MMC_COMMON_20180813_H__
