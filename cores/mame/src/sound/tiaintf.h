#ifndef TIA_H
#define TIA_H

#define TIA_DEFAULT_GAIN 16

struct TIAinterface {
    unsigned int clock;
	int volume;
	int gain;
	int baseclock;
};

int tia_sh_start (const struct MachineSound *msound);
void tia_sh_stop (void);
void tia_sh_update (void);

#endif
