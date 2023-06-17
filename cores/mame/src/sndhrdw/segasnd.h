/*************************************************************************

	Sega g80 common sound hardware

*************************************************************************/

WRITE_HANDLER( sega_sh_speechboard_w );

extern const struct Memory_ReadAddress  sega_speechboard_readmem[];
extern const struct Memory_WriteAddress sega_speechboard_writemem[];
extern const struct IO_ReadPort         sega_speechboard_readport[];
extern const struct IO_WritePort        sega_speechboard_writeport[];

extern struct sp0250_interface sega_sp0250_interface;
