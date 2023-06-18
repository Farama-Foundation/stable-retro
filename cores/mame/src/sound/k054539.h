/*********************************************************

	Konami 054539 PCM Sound Chip

*********************************************************/
#ifndef __K054539_H__
#define __K054539_H__

#define MAX_054539 2

struct K054539interface {
	int num;									/* number of chips */
	int clock;									/* clock (usually 48000) */
	int region[MAX_054539];						/* memory regions of sample ROM(s) */
	int mixing_level[MAX_054539][2];			/* Mixing levels */
	void (*apan[MAX_054539])(double, double);	/* Callback for analog output mixing levels (0..1 for each channel) */
	void (*irq[MAX_054539])( void );
};


int K054539_sh_start( const struct MachineSound *msound );
void K054539_sh_stop( void );
WRITE_HANDLER( K054539_0_w );
READ_HANDLER( K054539_0_r );
WRITE_HANDLER( K054539_1_w );
READ_HANDLER( K054539_1_r );

/** control flags, may be set at DRIVER_INIT().*/
#define K054539_RESET_FLAGS     0
#define K054539_REVERSE_STEREO  1
#define K054539_DISABLE_REVERB  2
#define K054539_UPDATE_AT_KEYON 4

void K054539_init_flags(int flags);

/*
	Note that the eight PCM channels of a K054539 do not have seperate
	volume controls. Considering the global attenuation equation may not
	be entirely accurate, K054539_set_gain() provides means to control
	channel gain. It can be called anywhere but preferrably from
	DRIVER_INIT().

	Parameters:
		chip    : 0 / 1
		channel : 0 - 7
		gain    : 0.0=silent, 1.0=no gain, 2.0=twice as loud, etc.
*/
void K054539_set_gain(int chip, int channel, double gain);

#endif /* __K054539_H__ */
