//==========================================================================
//
//      spi3wire_bp2016_init.cxx
//
//      BP2016 SPI 3wire bus init
//
//==========================================================================
//
//#####DESCRIPTIONBEGIN####
//
// Author(s):     huyue <huyue@haigebeidou.cn>
// Date:          2018-01-29
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <cyg/infra/cyg_type.h>

// This file is not needed if we support CYGBLD_ATTRIB_C_INIT_PRI, as the
// init happens directly in spi_bp2016.c then.
#ifndef CYGBLD_ATTRIB_C_INIT_PRI

// -------------------------------------------------------------------------

externC void cyg_spi3wire_bp2016_bus_init(void);

class cyg_spi3wire_bp2016_bus_init_class {
  public:
    cyg_spi3wire_bp2016_bus_init_class(void)
    {
        cyg_spi3wire_bp2016_bus_init();
    }
};

// -------------------------------------------------------------------------

static cyg_spi3wire_bp2016_bus_init_class spi3wire_bp2016_bus_init CYGBLD_ATTRIB_INIT_PRI(CYG_INIT_BUS_SPI);

#endif // ifndef CYGBLD_ATTRIB_C_INIT_PRI
