/*************************************************************************

	Art & Magic hardware

**************************************************************************/

/*----------- defined in vidhrdw/artmagic.c -----------*/

extern data16_t *artmagic_vram0;
extern data16_t *artmagic_vram1;

extern int artmagic_xor[16], artmagic_is_stoneball;

VIDEO_START( artmagic );

void artmagic_to_shiftreg(offs_t address, data16_t *data);
void artmagic_from_shiftreg(offs_t address, data16_t *data);

READ16_HANDLER( artmagic_blitter_r );
WRITE16_HANDLER( artmagic_blitter_w );

VIDEO_UPDATE( artmagic );
