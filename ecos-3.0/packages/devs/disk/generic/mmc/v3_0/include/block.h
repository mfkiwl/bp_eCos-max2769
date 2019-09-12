#ifndef __MMC_CARD_BLOCK_H__
#define __MMC_CARD_BLOCK_H__

#include <cyg/io/mmc/card.h>
#include <cyg/io/mmc/core.h>

int mmc_blk_probe(struct mmc_card *card);
int mmc_blk_issue_rq(struct mmc_card *card, struct request *req);


#endif //__MMC_CARD_BLOCK_H__
