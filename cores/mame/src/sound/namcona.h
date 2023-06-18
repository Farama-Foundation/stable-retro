/* namcona1.h */
#ifndef _NAMCONA_
#define _NAMCONA_

int NAMCONA_sh_start( const struct MachineSound *msound );
void NAMCONA_sh_stop( void );

struct NAMCONAinterface {
    int frequency;
    int region;
    int mixing_level;
};

#endif
