#ifndef namco52_h
#define namco52_h

struct namco_52xx_interface
{
	int baseclock;		/* clock */
	int mixing_level;	/* volume */
	int region;			/* memory region */
};

int namco_52xx_sh_start(const struct MachineSound *msound);
void namco_52xx_sh_stop(void);

void namcoio_52XX_write(int data);

#endif
