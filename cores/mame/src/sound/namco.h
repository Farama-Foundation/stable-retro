#ifndef namco_h
#define namco_h

struct namco_interface
{
	int samplerate;	/* sample rate */
	int voices;		/* number of voices */
	int volume;		/* playback volume */
	int region;		/* memory region; -1 to use RAM (pointed to by namco_wavedata) */
	int stereo;		/* set to 1 to indicate stereo (e.g., System 1) */
};

int namco_sh_start(const struct MachineSound *msound);
void namco_sh_stop(void);

WRITE_HANDLER( pengo_sound_enable_w );
WRITE_HANDLER( pengo_sound_w );

void polepos_sound_enable(int enable);
WRITE_HANDLER( polepos_sound_w );

WRITE_HANDLER( mappy_sound_enable_w ); 
void mappy_sound_enable(int enable);
WRITE_HANDLER( namco_15xx_w );

WRITE_HANDLER( namcos1_sound_w );
WRITE_HANDLER( namcos1_wavedata_w );
READ_HANDLER( namcos1_sound_r );
READ_HANDLER( namcos1_wavedata_r );

extern unsigned char *namco_soundregs;
extern unsigned char *namco_wavedata;


#define pengo_soundregs namco_soundregs
#define polepos_soundregs namco_soundregs

#endif
