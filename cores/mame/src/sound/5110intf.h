#ifndef intf5110_h
#define intf5110_h

struct TMS5110interface
{
	int baseclock;				/* clock rate = 80 * output sample rate,     */
								/* usually 640000 for 8000 Hz sample rate or */
								/* usually 800000 for 10000 Hz sample rate.  */
	int mixing_level;
	void (*irq)(int state);		/* IRQ callback function */
	int (*M0_callback)(void);	/* function to be called when chip requests another bit*/
};

int tms5110_sh_start(const struct MachineSound *msound);
void tms5110_sh_stop(void);
void tms5110_sh_update(void);

WRITE_HANDLER( tms5110_CTL_w );
WRITE_HANDLER( tms5110_PDC_w );

READ_HANDLER( tms5110_status_r );
int tms5110_ready_r(void);

void tms5110_reset(void);
void tms5110_set_frequency(int frequency);

#endif
