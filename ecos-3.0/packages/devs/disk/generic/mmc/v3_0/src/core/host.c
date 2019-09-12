/*
 *  linux/drivers/mmc/core/host.c
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *  Copyright (C) 2007-2008 Pierre Ossman
 *  Copyright (C) 2010 Linus Walleij
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  MMC host class device management
 */
#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/drv_api.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <cyg/io/mmc/host.h>
#include <cyg/io/mmc/card.h>
#include <cyg/io/mmc/mmc_common.h>

#include "core.h"
#include "host.h"

static cyg_drv_mutex_t     mmc_host_lock;
static unsigned int mutex_init_flag = 0;
#define CARD_DETECT_THREAD_PRI      (4)
#define CARD_DISABLE_THREAD_PRI      (6)

#ifdef CONFIG_MMC_CLKGATE

/*
 * Enabling clock gating will make the core call out to the host
 * once up and once down when it performs a request or card operation
 * intermingled in any fashion. The driver will see this through
 * set_ios() operations with ios.clock field set to 0 to gate (disable)
 * the block clock, and to the old frequency to enable it again.
 */
static void mmc_host_clk_gate_delayed(struct mmc_host *host)
{
	unsigned long tick_ns;
	unsigned long freq = host->ios.clock;
	unsigned long flags;

	if (!freq) {
		pr_debug("%s: frequency set to 0 in disable function, "
			 "this means the clock is already disabled.\n",
			 mmc_hostname(host));
		return;
	}
	/*
	 * New requests may have appeared while we were scheduling,
	 * then there is no reason to delay the check before
	 * clk_disable().
	 */
	cyg_drv_mutex_lock(&host->clk_lock);

	/*
	 * Delay n bus cycles (at least 8 from MMC spec) before attempting
	 * to disable the MCI block clock. The reference count may have
	 * gone up again after this delay due to rescheduling!
	 */
	if (!host->clk_requests) {
		cyg_drv_mutex_unlock_irqrestore(&host->clk_lock, flags);
		tick_ns = DIV_ROUND_UP(1000000000, freq);
		ndelay(host->clk_delay * tick_ns);
	} else {
		/* New users appeared while waiting for this work */
		cyg_drv_mutex_unlock_irqrestore(&host->clk_lock, flags);
		return;
	}
	cyg_drv_mutex_lock(&host->clk_gate_mutex);
	//cyg_drv_mutex_lock_irqsave(&host->clk_lock, flags);
	if (!host->clk_requests) {
		//cyg_drv_mutex_unlock_irqrestore(&host->clk_lock, flags);
		/* This will set host->ios.clock to 0 */
		mmc_gate_clock(host);
		//cyg_drv_mutex_lock_irqsave(&host->clk_lock, flags);
		pr_debug("%s: gated MCI clock\n", mmc_hostname(host));
	}
	//cyg_drv_mutex_unlock_irqrestore(&host->clk_lock, flags);
	cyg_drv_mutex_unlock(&host->clk_gate_mutex);
}

/*
 * Internal work. Work to disable the clock at some later point.
 */
static void mmc_host_clk_gate_work(struct work_struct *work)
{
	struct mmc_host *host = container_of(work, struct mmc_host,
					      clk_gate_work);

	mmc_host_clk_gate_delayed(host);
}

/**
 *	mmc_host_clk_ungate - ungate hardware MCI clocks
 *	@host: host to ungate.
 *
 *	Makes sure the host ios.clock is restored to a non-zero value
 *	past this call.	Increase clock reference count and ungate clock
 *	if we're the first user.
 */
void mmc_host_clk_ungate(struct mmc_host *host)
{
	//unsigned long flags;

	cyg_drv_mutex_lock(&host->clk_gate_mutex);
	//cyg_drv_mutex_lock(&host->clk_lock, flags);
	if (host->clk_gated) {
		//cyg_drv_mutex_unlock(&host->clk_lock, flags);
		mmc_ungate_clock(host);
		//cyg_drv_mutex_lock(&host->clk_lock, flags);
		pr_debug("%s: ungated MCI clock\n", mmc_hostname(host));
	}
	host->clk_requests++;
	//cyg_drv_mutex_unlock(&host->clk_lock, flags);
	cyg_drv_mutex_unlock(&host->clk_gate_mutex);
}

/**
 *	mmc_host_may_gate_card - check if this card may be gated
 *	@card: card to check.
 */
static bool mmc_host_may_gate_card(struct mmc_card *card)
{
	/* If there is no card we may gate it */
	if (!card)
		return true;
	/*
	 * Don't gate SDIO cards! These need to be clocked at all times
	 * since they may be independent systems generating interrupts
	 * and other events. The clock requests counter from the core will
	 * go down to zero since the core does not need it, but we will not
	 * gate the clock, because there is somebody out there that may still
	 * be using it.
	 */
	return !(card->quirks & MMC_QUIRK_BROKEN_CLK_GATING);
}

/**
 *	mmc_host_clk_gate - gate off hardware MCI clocks
 *	@host: host to gate.
 *
 *	Calls the host driver with ios.clock set to zero as often as possible
 *	in order to gate off hardware MCI clocks. Decrease clock reference
 *	count and schedule disabling of clock.
 */
void mmc_host_clk_gate(struct mmc_host *host)
{
	unsigned long flags;

	cyg_drv_mutex_lock_irqsave(&host->clk_lock, flags);
	host->clk_requests--;
	if (mmc_host_may_gate_card(host->card) &&
	    !host->clk_requests)
		schedule_work(&host->clk_gate_work);
	cyg_drv_mutex_unlock_irqrestore(&host->clk_lock, flags);
}

/**
 *	mmc_host_clk_rate - get current clock frequency setting
 *	@host: host to get the clock frequency for.
 *
 *	Returns current clock frequency regardless of gating.
 */
unsigned int mmc_host_clk_rate(struct mmc_host *host)
{
	unsigned long freq;
	unsigned long flags;

	cyg_drv_mutex_lock_irqsave(&host->clk_lock, flags);
	if (host->clk_gated)
		freq = host->clk_old;
	else
		freq = host->ios.clock;
	cyg_drv_mutex_unlock_irqrestore(&host->clk_lock, flags);
	return freq;
}

/**
 *	mmc_host_clk_init - set up clock gating code
 *	@host: host with potential clock to control
 */
static inline void mmc_host_clk_init(struct mmc_host *host)
{
	host->clk_requests = 0;
	/* Hold MCI clock for 8 cycles by default */
	host->clk_delay = 8;
	host->clk_gated = false;
	INIT_WORK(&host->clk_gate_work, mmc_host_clk_gate_work);
	cyg_drv_mutex_lock_init(&host->clk_lock);
	mutex_init(&host->clk_gate_mutex);
}

/**
 *	mmc_host_clk_exit - shut down clock gating code
 *	@host: host with potential clock to control
 */
static inline void mmc_host_clk_exit(struct mmc_host *host)
{
	/*
	 * Wait for any outstanding gate and then make sure we're
	 * ungated before exiting.
	 */
	if (cancel_work_sync(&host->clk_gate_work))
		mmc_host_clk_gate_delayed(host);
	if (host->clk_gated)
		mmc_host_clk_ungate(host);
	/* There should be only one user now */
	WARN_ON(host->clk_requests > 1);
}

#else

static inline void mmc_host_clk_init(struct mmc_host *host)
{
}

static inline void mmc_host_clk_exit(struct mmc_host *host)
{
}

#endif

/**
 *	mmc_alloc_host - initialise the per-host structure.
 *	@extra: sizeof private data structure
 *	@dev: pointer to host device model structure
 *
 *	Initialise the per-host structure.
 */
//struct mmc_host *mmc_alloc_host(int extra, struct device *dev)
void mmc_alloc_host(struct mmc_host *host, int id)
{
	//struct mmc_host *host;

    if(!mutex_init_flag)
    {
       cyg_drv_mutex_init(&mmc_host_lock);
       mutex_init_flag = 1;
    }

	cyg_drv_mutex_lock(&mmc_host_lock);
    host->index = id;

	cyg_drv_mutex_unlock(&mmc_host_lock);

    sprintf(host->dev_name, "mmc%d", host->index);

	mmc_host_clk_init(host);

	cyg_drv_mutex_init(&host->lock);
    cyg_drv_cond_init(&host->completion_data, &host->lock);

#ifdef CYGDAT_DEVS_MMC_HOTPLUG
    //bp2016 sdmmc controller not support hotplug
    cyg_thread_create(CARD_DETECT_THREAD_PRI, mmc_rescan, (cyg_addrword_t)host, 
            "Card detect thread", (void *)host->stack[0], DEF_THREAD_SIZE,
            &(host->detect), &(host->thread_cd));
#endif

#if 0
    cyg_thread_create(CARD_DISABLE_THREAD_PRI, mmc_host_deeper_disable, (cyg_addrword_t)host, 
            "Card disable thread", (void *)host->stack[1], DEF_THREAD_SIZE,
            &(host->disable), &(host->thread_disable));
#endif

	//INIT_DELAYED_WORK_DEFERRABLE(&host->disable, mmc_host_deeper_disable);
#ifdef CONFIG_PM
	host->pm_notify.notifier_call = mmc_pm_notify;
#endif

	/*
	 * By default, hosts do not support SGIO or large requests.
	 * They have to set these according to their abilities.
	 */
	host->max_segs = 1;
	host->max_seg_size = PAGE_CACHE_SIZE;

	host->max_req_size = PAGE_CACHE_SIZE;
	host->max_blk_size = 512;
	host->max_blk_count = PAGE_CACHE_SIZE / 512;
}

/**
 *	mmc_add_host - initialise host hardware
 *	@host: mmc host
 *
 *	Register the host with the driver model. The host must be
 *	prepared to start servicing requests before this function
 *	completes.
 */
int mmc_add_host(struct mmc_host *host)
{
	WARN_ON((host->caps & MMC_CAP_SDIO_IRQ) &&
		!host->ops->enable_sdio_irq);

#ifdef CONFIG_DEBUG_FS
	mmc_add_host_debugfs(host);
#endif

	mmc_start_host(host);

	return 0;
}

/**
 *	mmc_remove_host - remove host hardware
 *	@host: mmc host
 *
 *	Unregister and remove all cards associated with this host,
 *	and power down the MMC bus. No new requests will be issued
 *	after this function has returned.
 */
void mmc_remove_host(struct mmc_host *host)
{
	mmc_stop_host(host);
	mmc_host_clk_exit(host);
}
