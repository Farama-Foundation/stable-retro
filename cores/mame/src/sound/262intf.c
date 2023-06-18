/***************************************************************************

  262intf.c

  MAME interface for YMF262 (OPL3) emulator

***************************************************************************/
#include "driver.h"
#include "262intf.h"
#include "ymf262.h"




#if (HAS_YMF262)

static int  stream_262[MAX_262];
static void *Timer_262[MAX_262*2];
static const struct YMF262interface *intf_262 = NULL;
static void IRQHandler_262(int n,int irq)
{
	if (intf_262->handler[n]) (intf_262->handler[n])(irq);
}
static void timer_callback_262(int param)
{
	int n=param>>1;
	int c=param&1;
	YMF262TimerOver(n,c);
}

static void TimerHandler_262(int c,double period)
{
	if( period == 0 )
	{	/* Reset FM Timer */
		timer_enable(Timer_262[c], 0);
	}
	else
	{	/* Start FM Timer */
		timer_adjust(Timer_262[c], period, c, 0);
	}
}


int YMF262_sh_start(const struct MachineSound *msound)
{
	int i,chip;
	int rate = Machine->sample_rate;

	intf_262 = msound->sound_interface;
	if( intf_262->num > MAX_262 ) return 1;

	rate = intf_262->baseclock/288;

	/* Timer state clear */
	memset(Timer_262,0,sizeof(Timer_262));

	/* stream system initialize */
	if ( YMF262Init(intf_262->num,intf_262->baseclock,rate) != 0)
		return 1;

	for (chip = 0;chip < intf_262->num; chip++)
	{
		int mixed_vol;
		int vol[4];		/* four separate outputs */
		char buf[4][40];
		const char *name[4];

		mixed_vol = intf_262->mixing_levelAB[chip];
		for (i=0; i<4; i++)
		{
			if (i==2) /*channels C ad D use separate field */
				mixed_vol = intf_262->mixing_levelCD[chip];
			vol[i] = mixed_vol & 0xffff;
			mixed_vol >>= 16;
			name[i] = buf[i];
			sprintf(buf[i],"%s #%d ch%c",sound_name(msound),chip,'A'+i);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%s #%d ch%c",sound_name(msound),chip,'A'+i);
		}
		stream_262[chip] = stream_init_multi(4,name,vol,rate,chip,YMF262UpdateOne);

		/* YMF262 setup */
		YMF262SetTimerHandler (chip, TimerHandler_262, chip*2);
		YMF262SetIRQHandler   (chip, IRQHandler_262, chip);
		YMF262SetUpdateHandler(chip, stream_update, stream_262[chip]);

		Timer_262[chip*2+0] = timer_alloc(timer_callback_262);
		Timer_262[chip*2+1] = timer_alloc(timer_callback_262);
	}
	return 0;
}

void YMF262_sh_stop(void)
{
	YMF262Shutdown();
}

/* reset */
void YMF262_sh_reset(void)
{
	int i;

	for (i = 0;i < intf_262->num;i++)
		YMF262ResetChip(i);
}

/* chip #0 */
READ_HANDLER( YMF262_status_0_r ) {
	return YMF262Read(0, 0);
}
WRITE_HANDLER( YMF262_register_A_0_w ) {
	YMF262Write(0, 0, data);
}
WRITE_HANDLER( YMF262_data_A_0_w ) {
	YMF262Write(0, 1, data);
}
WRITE_HANDLER( YMF262_register_B_0_w ) {
	YMF262Write(0, 2, data);
}
WRITE_HANDLER( YMF262_data_B_0_w ) {
	YMF262Write(0, 3, data);
}

/* chip #1 */
READ_HANDLER( YMF262_status_1_r ) {
	return YMF262Read(1, 0);
}
WRITE_HANDLER( YMF262_register_A_1_w ) {
	YMF262Write(1, 0, data);
}
WRITE_HANDLER( YMF262_data_A_1_w ) {
	YMF262Write(1, 1, data);
}
WRITE_HANDLER( YMF262_register_B_1_w ) {
	YMF262Write(1, 2, data);
}
WRITE_HANDLER( YMF262_data_B_1_w ) {
	YMF262Write(1, 3, data);
}

#endif
