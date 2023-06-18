/*************************************************************************

	3dfx Voodoo Graphics SST-1 emulator

	driver by Aaron Giles

**************************************************************************/

extern data32_t *voodoo_regs;

VIDEO_START( voodoo_1x4mb );
VIDEO_START( voodoo_2x4mb );
VIDEO_START( voodoo2_1x4mb );
VIDEO_START( voodoo2_2x4mb );
VIDEO_STOP( voodoo );
VIDEO_UPDATE( voodoo );

void voodoo_set_init_enable(data32_t newval);

void voodoo_reset(void);

WRITE32_HANDLER( voodoo_regs_w );
WRITE32_HANDLER( voodoo2_regs_w );
READ32_HANDLER( voodoo_regs_r );

WRITE32_HANDLER( voodoo_framebuf_w );
READ32_HANDLER( voodoo_framebuf_r );

WRITE32_HANDLER( voodoo_textureram_w );
