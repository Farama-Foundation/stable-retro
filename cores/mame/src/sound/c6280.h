#ifndef _C6280_H_
#define _C6280_H_

#define MAX_C6280 2

struct C6280_interface
{
    int num;
    int volume[MAX_C6280];
    int clock[MAX_C6280];
};

typedef struct {
    UINT16 frequency;
    UINT8 control;
    UINT8 balance;
    UINT8 waveform[32];
    UINT8 index;
    INT16 dda;
    UINT8 noise_control;
    UINT32 noise_counter;
    UINT32 counter;
} t_channel;

typedef struct {
    UINT8 select;
    UINT8 balance;
    UINT8 lfo_frequency;
    UINT8 lfo_control;
    t_channel channel[8];
    INT16 volume_table[32];
    UINT32 noise_freq_tab[32];
    UINT32 wave_freq_tab[4096];
} c6280_t;

/* Function prototypes */
int c6280_sh_start(const struct MachineSound *msound);
void c6280_sh_stop(void);
WRITE_HANDLER( C6280_0_w );
WRITE_HANDLER( C6280_1_w );

#endif /* _C6280_H_ */
