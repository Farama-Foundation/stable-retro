#ifndef gaelco_snd_h
#define gaelco_snd_h

struct gaelcosnd_interface
{
	int region;				/* memory region */
	int banks[4];			/* start of each ROM bank */
	int volume[2];			/* playback volume */
};

#ifdef __cplusplus
extern "C" {
#endif

int gaelco_cg1v_sh_start(const struct MachineSound *msound);
int gaelco_gae1_sh_start(const struct MachineSound *msound);
void gaelcosnd_sh_stop(void);

WRITE16_HANDLER( gaelcosnd_w );
READ16_HANDLER( gaelcosnd_r );

#ifdef __cplusplus
}
#endif

#endif
