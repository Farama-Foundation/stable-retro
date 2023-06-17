/*********************************************************/
/*    SEGA 8bit PCM                                      */
/*********************************************************/

#ifndef __SEGAPCM_H__
#define __SEGAPCM_H__

#define   BANK_256    (11)
#define   BANK_512    (12)
#define   BANK_12M    (13)
#define   BANK_MASK7    (0x70<<16)
#define   BANK_MASKF    (0xf0<<16)
#define   BANK_MASKF8   (0xf8<<16)

struct SEGAPCMinterface
{
	int  mode;
	int  bank;
	int  region;
	int  volume;
};

enum SEGAPCM_samplerate
{
	SEGAPCM_SAMPLE15K,
	SEGAPCM_SAMPLE32K
};

int SEGAPCM_sh_start( const struct MachineSound *msound );
void SEGAPCM_sh_stop( void );

WRITE_HANDLER( SegaPCM_w );
READ_HANDLER( SegaPCM_r );

#endif
