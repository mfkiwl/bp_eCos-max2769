#ifndef __PLL_API__
#define __PLL_API__

void pll_api_init(void);
int pll_api_get_rate_kHz(unsigned int pll_num);
bool pll_api_set_rate_kHz(unsigned int pll_num, unsigned int rate_kHz);
bool pll_api_set_pll3_ref_KHz(unsigned int rate_kHz);
unsigned int pll_api_get_pll3_ref_KHz(void);

#endif
