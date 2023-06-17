#ifndef namco54_h
#define namco54_h

struct namco_54xx_interface
{
	int baseclock;			/* clock */
	int mixing_level[3];	/* volume of the 3 output ports */
};

int namco_54xx_sh_start(const struct MachineSound *msound);
void namco_54xx_sh_stop(void);

void namcoio_54XX_write(int data);

#endif
