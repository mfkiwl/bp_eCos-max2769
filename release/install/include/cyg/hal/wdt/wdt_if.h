#ifndef __WDT_IF__
#define __WDT_IF__

inline void wdt_cr(HWP_WDT_AP_T* hwp_apWdt, u32 en, u32 reponse_mode, u32 rpl);
inline u32 wdt_ccvr(HWP_WDT_AP_T* hwp_apWdt);
inline void wdt_crr(HWP_WDT_AP_T* hwp_apWdt, u32 rst_v); // 0x76 restart wdt counter
inline u32 wdt_stat(HWP_WDT_AP_T* hwp_apWdt);
inline u32 wdt_eoi(HWP_WDT_AP_T* hwp_apWdt);
inline void wdt_torr(HWP_WDT_AP_T* hwp_apWdt, u32 top, u32 top_init);
u32 get_torr_index(u32 max_v);
inline u32 get_torr_range_max_v(int index);
inline u32 get_time2torr(u32 time);

#endif
