/***************************************************************************

  namcond1.h

  Common functions & declarations for the Namco ND-1 driver

***************************************************************************/

/* VIDHRDW */

#define GFX_8X8_4BIT    0
#define GFX_16X16_4BIT  1
#define GFX_32X32_4BIT  2
#define GFX_64X64_4BIT  3
#define GFX_8X8_8BIT    4
#define GFX_16X16_8BIT  5

extern void nvsram( offs_t offset, data16_t data );

/* MACHINE */
extern int namcond1_gfxbank;

extern unsigned short int *namcond1_shared_ram;
extern unsigned short int *namcond1_eeprom;

extern READ16_HANDLER( namcond1_shared_ram_r );
extern READ16_HANDLER( namcond1_cuskey_r );
extern WRITE16_HANDLER( namcond1_shared_ram_w );
extern WRITE16_HANDLER( namcond1_cuskey_w );

NVRAM_HANDLER( namcond1 );
MACHINE_INIT( namcond1 );

/* VIDHRDW */

/* to be removed*/
extern READ16_HANDLER( debug_trigger );
