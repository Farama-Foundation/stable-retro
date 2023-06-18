/*********************************************************/
/*    ricoh RF5C68(or clone) PCM controller              */
/*********************************************************/
#ifndef __RF5C68_H__
#define __RF5C68_H__

/******************************************/
#define  RF5C68_PCM_MAX    (8)


typedef struct rf5c68pcm
{
	int clock;
	unsigned char env[RF5C68_PCM_MAX];
	unsigned char pan[RF5C68_PCM_MAX];
	unsigned int  addr[RF5C68_PCM_MAX];
	unsigned int  start[RF5C68_PCM_MAX];
	unsigned int  step[RF5C68_PCM_MAX];
	unsigned int  loop[RF5C68_PCM_MAX];
	int pcmx[2][RF5C68_PCM_MAX];
	int flag[RF5C68_PCM_MAX];

	int pcmd[RF5C68_PCM_MAX];
	int pcma[RF5C68_PCM_MAX];
} RF5C68PCM;

struct RF5C68interface
{
	int clock;
	int volume;
};

/******************************************/
int RF5C68_sh_start( const struct MachineSound *msound );
void RF5C68_sh_stop( void );
WRITE_HANDLER( RF5C68_reg_w );

READ_HANDLER( RF5C68_r );
WRITE_HANDLER( RF5C68_w );


#endif
/**************** end of file ****************/
