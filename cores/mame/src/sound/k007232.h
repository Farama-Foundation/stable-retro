/*********************************************************/
/*    Konami PCM controller                              */
/*********************************************************/
#ifndef __KDAC_A_H__
#define __KDAC_A_H__

#define MAX_K007232		3


struct K007232_interface
{
	int num_chips;			/* Number of chips */
	int baseclock;          /* chip clock */
	int bank[MAX_K007232];	/* memory regions */
	int volume[MAX_K007232];/* volume */
	void (*portwritehandler[MAX_K007232])(int);
};

#define K007232_VOL(LVol,LPan,RVol,RPan) ((LVol)|((LPan)<<8)|((RVol)<<16)|((RPan)<<24))

int K007232_sh_start(const struct MachineSound *msound);

WRITE_HANDLER( K007232_write_port_0_w );
WRITE_HANDLER( K007232_write_port_1_w );
WRITE_HANDLER( K007232_write_port_2_w );
READ_HANDLER( K007232_read_port_0_r );
READ_HANDLER( K007232_read_port_1_r );
READ_HANDLER( K007232_read_port_2_r );

void K007232_set_bank( int chip, int chABank, int chBBank );

/*
  The 007232 has two channels and produces two outputs. The volume control
  is external, however to make it easier to use we handle that inside the
  emulation. You can control volume and panning: for each of the two channels
  you can set the volume of the two outputs. If panning is not required,
  then volumeB will be 0 for channel 0, and volumeA will be 0 for channel 1.
  Volume is in the range 0-255.
*/
void K007232_set_volume(int chip,int channel,int volumeA,int volumeB);

#endif
