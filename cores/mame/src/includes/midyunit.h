/*************************************************************************

	Williams/Midway Y/Z-unit system

**************************************************************************/

/* these are accurate for MK Rev 5 according to measurements done by Bryan on a real board */
/* due to the way the TMS34010 core operates, however, we need to use 0 for our VBLANK */
/* duration (263ms is the measured value) */
#define MKLA5_VBLANK_DURATION		0
#define MKLA5_FPS					53.204950


/*----------- defined in machine/midyunit.c -----------*/

extern data16_t *midyunit_code_rom;
extern data16_t *midyunit_scratch_ram;

extern data16_t *midyunit_cmos_ram;
extern UINT32 	midyunit_cmos_page;

extern offs_t 	midyunit_speedup_pc;
extern offs_t 	midyunit_speedup_offset;
extern offs_t 	midyunit_speedup_spin[3];
extern data16_t *midyunit_speedup_base;

WRITE16_HANDLER( midyunit_cmos_w );
READ16_HANDLER( midyunit_cmos_r );

WRITE16_HANDLER( midyunit_cmos_enable_w );
READ16_HANDLER( midyunit_protection_r );

READ16_HANDLER( midyunit_input_r );

READ16_HANDLER( midyunit_generic_speedup_1_16bit );
READ16_HANDLER( midyunit_generic_speedup_1_mixedbits );
READ16_HANDLER( midyunit_generic_speedup_1_32bit );
READ16_HANDLER( midyunit_generic_speedup_3 );

DRIVER_INIT( narc );
DRIVER_INIT( narc3 );

DRIVER_INIT( trog );
DRIVER_INIT( trog3 );
DRIVER_INIT( trogpa6 );
DRIVER_INIT( trogp );

DRIVER_INIT( smashtv );
DRIVER_INIT( smashtv4 );

DRIVER_INIT( hiimpact );
DRIVER_INIT( shimpact );
DRIVER_INIT( shimpacp );

DRIVER_INIT( strkforc );

DRIVER_INIT( mkprot9 );
DRIVER_INIT( mkla1 );
DRIVER_INIT( mkla2 );
DRIVER_INIT( mkla3 );
DRIVER_INIT( mkla4 );

DRIVER_INIT( term2 );
DRIVER_INIT( term2la2 );
DRIVER_INIT( term2la1 );

DRIVER_INIT( totcarn );
DRIVER_INIT( totcarnp );

MACHINE_INIT( midyunit );

WRITE16_HANDLER( midyunit_sound_w );


/*----------- defined in vidhrdw/midyunit.c -----------*/

extern UINT8 *	midyunit_gfx_rom;
extern size_t	midyunit_gfx_rom_size;

VIDEO_START( midyunit_4bit );
VIDEO_START( midyunit_6bit );
VIDEO_START( midzunit );

READ16_HANDLER( midyunit_gfxrom_r );

WRITE16_HANDLER( midyunit_vram_w );
READ16_HANDLER( midyunit_vram_r );

void midyunit_to_shiftreg(UINT32 address, UINT16 *shiftreg);
void midyunit_from_shiftreg(UINT32 address, UINT16 *shiftreg);

WRITE16_HANDLER( midyunit_control_w );
WRITE16_HANDLER( midyunit_paletteram_w );

READ16_HANDLER( midyunit_dma_r );
WRITE16_HANDLER( midyunit_dma_w );

void midyunit_display_addr_changed(UINT32 offs, int rowbytes, int scanline);
void midyunit_display_interrupt(int scanline);

WRITE16_HANDLER( midyunit_io_register_w );

VIDEO_EOF( midyunit );
VIDEO_UPDATE( midyunit );
