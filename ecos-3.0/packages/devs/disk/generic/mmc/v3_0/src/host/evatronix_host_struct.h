#ifndef __EVATRONIX_HOST_STRUCT_H__
#define __EVATRONIX_HOST_STRUCT_H__

#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/mmc/host.h>
#include <cyg/io/mmc/list.h>
#include <cyg/io/mmc/mmc.h>
#include <cyg/io/mmc/card.h>
#include <cyg/io/mmc/host.h>
#include <cyg/io/mmc/mmc_common.h>

#include "evatronix_host.h"

//#define	DEBUG
//#define	ERROR_RECOVERY
//#define	DATA_ERROR_DEBUG

#define HOST_THREAD_SIZE     (8192)

typedef struct DmaDescStruct {  
	unsigned int desc_1;           
	unsigned int desc_2;           
}DmaDesc;

struct evatronix_sdio_slot_data {
    u8	ctype;
    bool	force_rescan;
    u32	caps;
    u32 	pm_caps;
    u32	freq;
    u32	ocr_avail;

    int (*voltage_switch)(void);
};

struct evtx_mmc_platform_data {
	u8		nr_slots;
	unsigned int 	ref_clk;
	unsigned int 	detect_delay_ms;
	int (*slot_attr_init) (struct evatronix_sdio_slot_data *sd,
					struct mmc_host *mmc, unsigned int id);

	struct evatronix_sdio_slot_data slots[MAX_SLOTS];
};

struct evatronix_slot;	

struct evatronix_host {
	//unsigned char		initialized;
	unsigned char		suspended;

	struct mmc_request	*mrq;
	unsigned char		cur_slot;

	enum    evatronix_state state;
	struct list_head        queue;
	cyg_drv_mutex_t         lock;

	//DMA
	u32     		sg_dma;
	DmaDesc         *desc;
	unsigned int		blocks;

    cyg_vector_t        sdmmc_isrvec;
    int                 sdmmc_isrpri;
    cyg_handle_t        sdmmc_interrupt_handle; // For initializing the interrupt
    cyg_interrupt       sdmmc_interrupt_data;

	unsigned char   *virt_base;
	unsigned int		phys_base;
	struct clk		*fclk;
	unsigned int		clk_rate;
    
    struct evtx_mmc_platform_data *pdata;
	struct evatronix_slot	*slots[4];
	unsigned int		nr_slots;

	unsigned int		flag;
#ifdef DEBUG
	u32			    prev_slot;
	u32 			prev_cmd;
#endif
};

struct evatronix_slot {
	struct evatronix_host	*host;
	struct mmc_host		*mmc;
	struct mmc_request 	*mrq;
	struct mmc_command	*cmd;
	struct mmc_data		*data;

	/* exported members */
	int			id;
    int         initialized;
    int         mmc_connected;
	u8			ctype;
	u32			voltage;
	u32			freq;

	u32			irq_status;
	unsigned long long      reqid;
	unsigned int		cur_clk;

	struct list_head        queue_node;
	unsigned long		pending_events;
	unsigned long		completed_events;

	struct evatronix_sdio_slot_data *pdata;
	int			last_detect_state;
#define EVA_MMC_CARD_PRESENT	0
	unsigned long       cd_flags;
	unsigned int		step;
#ifdef	REQUEST_STAT
	u32			tm_start;
	u32			tm_mid1;
	u32			tm_mid2;
	u32			tm_mid3;
	u32			tm_end;
#endif
    int read_only;
    unsigned int mmc_block_count;

};

int evatronix_slot_init(struct evatronix_slot *slot);
int evatronix_get_cd(struct mmc_host *mmc, struct mmc_ios *ios);

#endif //__EVATRONIX_HOST_STRUCT_H__
