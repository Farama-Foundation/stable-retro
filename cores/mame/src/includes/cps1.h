#ifndef _CPS1_H_
#define _CPS1_H_

extern data16_t *cps1_gfxram;     /* Video RAM */
extern data16_t *cps1_output;     /* Output ports */
extern size_t cps1_gfxram_size;
extern size_t cps1_output_size;

extern const struct Memory_ReadAddress qsound_readmem[];
extern const struct Memory_WriteAddress qsound_writemem[];

READ16_HANDLER( qsound_sharedram1_r );
WRITE16_HANDLER( qsound_sharedram1_w );

READ16_HANDLER( cps1_eeprom_port_r );
WRITE16_HANDLER( cps1_eeprom_port_w );

READ16_HANDLER( cps1_output_r );
WRITE16_HANDLER( cps1_output_w );

WRITE16_HANDLER( cps1_gfxram_w );

WRITE16_HANDLER( cps2_objram_bank_w );
READ16_HANDLER( cps2_objram1_r );
READ16_HANDLER( cps2_objram2_r );
WRITE16_HANDLER( cps2_objram1_w );
WRITE16_HANDLER( cps2_objram2_w );

VIDEO_START( cps1 );
VIDEO_UPDATE( cps1 );
VIDEO_EOF( cps1 );
DRIVER_INIT( cps1 );
DRIVER_INIT( cps2 );

INTERRUPT_GEN( cps1_qsound_interrupt );

extern struct QSound_interface qsound_interface;

extern int scanline;

#endif
