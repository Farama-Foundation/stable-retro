
#ifndef ASTROCADE_H
#define ASTROCADE_H

#define MAX_ASTROCADE_CHIPS 2   /* max number of emulated chips */

struct astrocade_interface
{
	int num;			/* total number of sound chips in the machine */
	int baseclock;			/* astrocade clock rate  */
	int volume[MAX_ASTROCADE_CHIPS];			/* master volume */
};

int astrocade_sh_start(const struct MachineSound *msound);
void astrocade_sh_stop(void);
void astrocade_sh_update(void);

WRITE_HANDLER( astrocade_sound1_w );
WRITE_HANDLER( astrocade_sound2_w );

#endif
