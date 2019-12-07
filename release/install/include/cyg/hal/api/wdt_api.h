#ifndef __WDT_API__
#define __WDT_API__

typedef enum {
    T_0MS     = 0,
    T_1MS     = 1,
    T_3MS,     
    T_6MS,     
    T_11MS,    
    T_22MS,  
    T_44MS,    
    T_88MS,   
    T_177MS,   
    T_353MS,   
    T_706MS,   
    T_1413MS,  
    T_2826MS,  
    T_5651MS,  
    T_11303MS, 
    T_22605MS, 
}WDT_TIME;

void wdt_api_init(void);
bool wdt_api_enable(void);
bool wdt_api_feed(void);
bool wdt_api_set_pause(WDT_TIME time);
cyg_uint32 wdt_api_get_pause(void);

#endif


