/*
 *  linux/drivers/mmc/core/bus.c
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *  Copyright (C) 2007 Pierre Ossman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  MMC card bus driver model
 */
#include <stdlib.h>
#include <stdio.h>
#include <cyg/io/mmc/card.h>
#include <cyg/io/mmc/host.h>
#include <cyg/io/mmc/mmc_common.h>

#include "core.h"
#include "sdio_cis.h"
#include "bus.h"

#ifdef CONFIG_PM_RUNTIME

static int mmc_runtime_suspend(struct device *dev)
{
	struct mmc_card *card = mmc_dev_to_card(dev);

	return mmc_power_save_host(card->host);
}

static int mmc_runtime_resume(struct device *dev)
{
	struct mmc_card *card = mmc_dev_to_card(dev);

	return mmc_power_restore_host(card->host);
}

static int mmc_runtime_idle(struct device *dev)
{
	return pm_runtime_suspend(dev);
}

static const struct dev_pm_ops mmc_bus_pm_ops = {
	.runtime_suspend	= mmc_runtime_suspend,
	.runtime_resume		= mmc_runtime_resume,
	.runtime_idle		= mmc_runtime_idle,
};

#define MMC_PM_OPS_PTR	(&mmc_bus_pm_ops)

#else /* !CONFIG_PM_RUNTIME */

#define MMC_PM_OPS_PTR	NULL

#endif /* !CONFIG_PM_RUNTIME */

/*
 * Allocate and initialise a new MMC card structure.
 */
//struct mmc_card *mmc_alloc_card(struct mmc_host *host, struct device_type *type)
struct mmc_card all_cards[MAX_SLOTS];

struct mmc_card *mmc_alloc_card(struct mmc_host *host)
{
	struct mmc_card *card;

    memset(&all_cards[host->index], 0, sizeof(struct mmc_card));
	card = &all_cards[host->index]; 

	card->host = host;

	return card;
}

/*
 * Register a new MMC card with the driver model.
 */
int mmc_add_card(struct mmc_card *card)
{
	const char *type;

	sprintf(card->card_name, "%s:%04x", mmc_hostname(card->host), card->rca);

	switch (card->type) {
	case MMC_TYPE_MMC:
		type = "MMC";
		break;
	case MMC_TYPE_SD:
		type = "SD";
		if (mmc_card_blockaddr(card)) {
			if (mmc_card_ext_capacity(card))
				type = "SDXC";
			else
				type = "SDHC";
		}
		break;
	case MMC_TYPE_SDIO:
		type = "SDIO";
		break;
	case MMC_TYPE_SD_COMBO:
		type = "SD-combo";
		if (mmc_card_blockaddr(card))
			type = "SDHC-combo";
		break;
	default:
		type = "?";
		break;
	}

	if (mmc_host_is_spi(card->host)) {
		printk("%s: new %s%s%s card on SPI\n",
			mmc_hostname(card->host),
			mmc_card_highspeed(card) ? "high speed " : "",
			mmc_card_ddr_mode(card) ? "DDR " : "",
			type);
	} else {
		printk("%s: new %s%s%s card at address %04x\n",
			mmc_hostname(card->host),
			mmc_sd_card_uhs(card) ? "ultra high speed " :
			(mmc_card_highspeed(card) ? "high speed " : ""),
			mmc_card_ddr_mode(card) ? "DDR " : "",
			type, card->rca);
	}

#ifdef CONFIG_DEBUG_FS
	mmc_add_card_debugfs(card);
#endif

	mmc_card_set_present(card);

	return 0;
}

/*
 * Unregister a new MMC card with the driver model, and
 * (eventually) free it.
 */
void mmc_remove_card(struct mmc_card *card)
{
#ifdef CONFIG_DEBUG_FS
	mmc_remove_card_debugfs(card);
#endif

	if (mmc_card_present(card)) {
		if (mmc_host_is_spi(card->host)) {
			printk("%s: SPI card removed\n",
				mmc_hostname(card->host));
		} else {
			printk("%s: card %04x removed\n",
				mmc_hostname(card->host), card->rca);
		}
	}

}
