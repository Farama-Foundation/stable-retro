#ifndef k051649_h
#define k051649_h

struct k051649_interface
{
	int master_clock;	/* master clock */
	int volume;			/* playback volume */
};

int K051649_sh_start(const struct MachineSound *msound);
void K051649_sh_stop(void);
void K051649_sh_reset(void);

WRITE_HANDLER( K051649_waveform_w );
READ_HANDLER( K051649_waveform_r );
WRITE_HANDLER( K051649_volume_w );
WRITE_HANDLER( K051649_frequency_w );
WRITE_HANDLER( K051649_keyonoff_w );
WRITE_HANDLER( K052539_waveform_w );

#endif
