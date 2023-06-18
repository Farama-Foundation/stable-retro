/******************************************************************************
* FILE
*	Yamaha 3812 emulator interface - MAME VERSION
*
* CREATED BY
*	Ernesto Corvi
*
* UPDATE LOG
*	JB  28-04-2002  Fixed simultaneous usage of all three different chip types.
*                       Used real sample rate when resample filter is active.
*       AAT 12-28-2001  Protected Y8950 from accessing unmapped port and keyboard handlers.
*	CHS 1999-01-09	Fixes new ym3812 emulation interface.
*	CHS 1998-10-23	Mame streaming sound chip update
*	EC	1998		Created Interface
*
* NOTES
*
******************************************************************************/
#include "driver.h"
#include "3812intf.h"
#include "fm.h"
#include "sound/fmopl.h"


#if (HAS_YM3812)

static int  stream_3812[MAX_3812];
static void *Timer_3812[MAX_3812*2];
static const struct YM3812interface *intf_3812 = NULL;
static void IRQHandler_3812(int n,int irq)
{
	if (intf_3812->handler[n]) (intf_3812->handler[n])(irq ? ASSERT_LINE : CLEAR_LINE);
}
static void timer_callback_3812(int param)
{
	int n=param>>1;
	int c=param&1;
	YM3812TimerOver(n,c);
}

static void TimerHandler_3812(int c,double period)
{
	if( period == 0 )
	{	/* Reset FM Timer */
		timer_enable(Timer_3812[c], 0);
	}
	else
	{	/* Start FM Timer */
		timer_adjust(Timer_3812[c], period, c, 0);
	}
}


int YM3812_sh_start(const struct MachineSound *msound)
{
	int i;
	int rate = Machine->sample_rate;

	intf_3812 = msound->sound_interface;
	if( intf_3812->num > MAX_3812 ) return 1;

	rate = intf_3812->baseclock/72;

	/* Timer state clear */
	memset(Timer_3812,0,sizeof(Timer_3812));

	/* stream system initialize */
	if ( YM3812Init(intf_3812->num,intf_3812->baseclock,rate) != 0)
		return 1;

	for (i = 0;i < intf_3812->num;i++)
	{
		/* stream setup */
		char name[40];
		int vol = intf_3812->mixing_level[i];
		/* emulator create */
		/* stream setup */
		sprintf(name,"%s #%d",sound_name(msound),i);

		stream_3812[i] = stream_init(name,vol,rate,i,YM3812UpdateOne);

		/* YM3812 setup */
		YM3812SetTimerHandler (i, TimerHandler_3812, i*2);
		YM3812SetIRQHandler   (i, IRQHandler_3812, i);
		YM3812SetUpdateHandler(i, stream_update, stream_3812[i]);

		Timer_3812[i*2+0] = timer_alloc(timer_callback_3812);
		Timer_3812[i*2+1] = timer_alloc(timer_callback_3812);
	}
	return 0;
}

void YM3812_sh_stop(void)
{
	YM3812Shutdown();
}

/* reset */
void YM3812_sh_reset(void)
{
	int i;

	for (i = 0;i < intf_3812->num;i++)
		YM3812ResetChip(i);
}

WRITE_HANDLER( YM3812_control_port_0_w ) {
	YM3812Write(0, 0, data);
}
WRITE_HANDLER( YM3812_write_port_0_w ) {
	YM3812Write(0, 1, data);
}
READ_HANDLER( YM3812_status_port_0_r ) {
	return YM3812Read(0, 0);
}
READ_HANDLER( YM3812_read_port_0_r ) {
	return YM3812Read(0, 1);
}


WRITE_HANDLER( YM3812_control_port_1_w ) {
	YM3812Write(1, 0, data);
}
WRITE_HANDLER( YM3812_write_port_1_w ) {
	YM3812Write(1, 1, data);
}
READ_HANDLER( YM3812_status_port_1_r ) {
	return YM3812Read(1, 0);
}
READ_HANDLER( YM3812_read_port_1_r ) {
	return YM3812Read(1, 1);
}

#endif


#if (HAS_YM3526)

static int  stream_3526[MAX_3526];
static void *Timer_3526[MAX_3526*2];
static const struct YM3526interface *intf_3526 = NULL;

/* IRQ Handler */
static void IRQHandler_3526(int n,int irq)
{
	if (intf_3526->handler[n]) (intf_3526->handler[n])(irq ? ASSERT_LINE : CLEAR_LINE);
}
/* Timer overflow callback from timer.c */
static void timer_callback_3526(int param)
{
	int n=param>>1;
	int c=param&1;
	YM3526TimerOver(n,c);
}
/* TimerHandler from fm.c */
static void TimerHandler_3526(int c,double period)
{
	if( period == 0 )
	{	/* Reset FM Timer */
		timer_enable(Timer_3526[c], 0);
	}
	else
	{	/* Start FM Timer */
		timer_adjust(Timer_3526[c], period, c, 0);
	}
}


int YM3526_sh_start(const struct MachineSound *msound)
{
	int i;
	int rate = Machine->sample_rate;


	intf_3526 = msound->sound_interface;
	if( intf_3526->num > MAX_3526 ) return 1;


	rate = intf_3526->baseclock/72;

	/* Timer state clear */
	memset(Timer_3526,0,sizeof(Timer_3526));

	/* stream system initialize */
	if ( YM3526Init(intf_3526->num,intf_3526->baseclock,rate) != 0)
		return 1;

	for (i = 0;i < intf_3526->num;i++)
	{
		/* stream setup */
		char name[40];
		int vol = intf_3526->mixing_level[i];
		/* emulator create */
		/* stream setup */
		sprintf(name,"%s #%d",sound_name(msound),i);
		stream_3526[i] = stream_init(name,vol,rate,i,YM3526UpdateOne);
		/* YM3526 setup */
		YM3526SetTimerHandler (i, TimerHandler_3526, i*2);
		YM3526SetIRQHandler   (i, IRQHandler_3526, i);
		YM3526SetUpdateHandler(i, stream_update, stream_3526[i]);

		Timer_3526[i*2+0] = timer_alloc(timer_callback_3526);
		Timer_3526[i*2+1] = timer_alloc(timer_callback_3526);
	}
	return 0;
}
void YM3526_sh_stop(void)
{
	YM3526Shutdown();
}

/* reset */
void YM3526_sh_reset(void)
{
	int i;

	for (i = 0;i < intf_3526->num;i++)
		YM3526ResetChip(i);
}

WRITE_HANDLER( YM3526_control_port_0_w ) {
	YM3526Write(0, 0, data);
}
WRITE_HANDLER( YM3526_write_port_0_w ) {
	YM3526Write(0, 1, data);
}
READ_HANDLER( YM3526_status_port_0_r ) {
	return YM3526Read(0, 0);
}
READ_HANDLER( YM3526_read_port_0_r ) {
	return YM3526Read(0, 1);
}


WRITE_HANDLER( YM3526_control_port_1_w ) {
	YM3526Write(1, 0, data);
}
WRITE_HANDLER( YM3526_write_port_1_w ) {
	YM3526Write(1, 1, data);
}
READ_HANDLER( YM3526_status_port_1_r ) {
	return YM3526Read(1, 0);
}
READ_HANDLER( YM3526_read_port_1_r ) {
	return YM3526Read(1, 1);
}

#endif


#if (HAS_Y8950)


static int  stream_8950[MAX_8950];
static void *Timer_8950[MAX_8950*2];
static const struct Y8950interface  *intf_8950 = NULL;
static void IRQHandler_8950(int n,int irq)
{
	if (intf_8950->handler[n]) (intf_8950->handler[n])(irq ? ASSERT_LINE : CLEAR_LINE);
}
static void timer_callback_8950(int param)
{
	int n=param>>1;
	int c=param&1;
	Y8950TimerOver(n,c);
}
static void TimerHandler_8950(int c,double period)
{
	if( period == 0 )
	{	/* Reset FM Timer */
		timer_enable(Timer_8950[c], 0);
	}
	else
	{	/* Start FM Timer */
		timer_adjust(Timer_8950[c], period, c, 0);
	}
}


static unsigned char Y8950PortHandler_r(int chip)
{
	if (intf_8950->portread[chip])
		return intf_8950->portread[chip](chip);
	return 0;
}

static void Y8950PortHandler_w(int chip,unsigned char data)
{
	if (intf_8950->portwrite[chip])
		intf_8950->portwrite[chip](chip,data);
}

static unsigned char Y8950KeyboardHandler_r(int chip)
{
	if (intf_8950->keyboardread[chip])
		return intf_8950->keyboardread[chip](chip);
	return 0;
}

static void Y8950KeyboardHandler_w(int chip,unsigned char data)
{
	if (intf_8950->keyboardwrite[chip])
		intf_8950->keyboardwrite[chip](chip,data);
}

int Y8950_sh_start(const struct MachineSound *msound)
{
	int i;
	int rate = Machine->sample_rate;

	intf_8950 = msound->sound_interface;
	if( intf_8950->num > MAX_8950 ) return 1;

	rate = intf_8950->baseclock/72;

	/* Timer state clear */
	memset(Timer_8950,0,sizeof(Timer_8950));

	/* stream system initialize */
	if ( Y8950Init(intf_8950->num,intf_8950->baseclock,rate) != 0)
		return 1;

	for (i = 0;i < intf_8950->num;i++)
	{
		/* stream setup */
		char name[40];
		int vol = intf_8950->mixing_level[i];

		/* stream setup */
		sprintf(name,"%s #%d",sound_name(msound),i);

		/* ADPCM ROM data */
		Y8950SetDeltaTMemory(i,
			(void *)(memory_region(intf_8950->rom_region[i])),
				memory_region_length(intf_8950->rom_region[i]) );

		stream_8950[i] = stream_init(name,vol,rate,i,Y8950UpdateOne);

		/* port and keyboard handler */
		Y8950SetPortHandler(i, Y8950PortHandler_w, Y8950PortHandler_r, i);
		Y8950SetKeyboardHandler(i, Y8950KeyboardHandler_w, Y8950KeyboardHandler_r, i);

		/* Y8950 setup */
		Y8950SetTimerHandler (i, TimerHandler_8950, i*2);
		Y8950SetIRQHandler   (i, IRQHandler_8950, i);
		Y8950SetUpdateHandler(i, stream_update, stream_8950[i]);

		Timer_8950[i*2+0] = timer_alloc(timer_callback_8950);
		Timer_8950[i*2+1] = timer_alloc(timer_callback_8950);
	}
	return 0;
}
void Y8950_sh_stop(void)
{
	Y8950Shutdown();
}

/* reset */
void Y8950_sh_reset(void)
{
	int i;

	for (i = 0;i < intf_8950->num;i++)
		Y8950ResetChip(i);
}

WRITE_HANDLER( Y8950_control_port_0_w ) {
	Y8950Write(0, 0, data);
}
WRITE_HANDLER( Y8950_write_port_0_w ) {
	Y8950Write(0, 1, data);
}
READ_HANDLER( Y8950_status_port_0_r ) {
	return Y8950Read(0, 0);
}
READ_HANDLER( Y8950_read_port_0_r ) {
	return Y8950Read(0, 1);
}


WRITE_HANDLER( Y8950_control_port_1_w ) {
	Y8950Write(1, 0, data);
}
WRITE_HANDLER( Y8950_write_port_1_w ) {
	Y8950Write(1, 1, data);
}
READ_HANDLER( Y8950_status_port_1_r ) {
	return Y8950Read(1, 0);
}
READ_HANDLER( Y8950_read_port_1_r ) {
	return Y8950Read(1, 1);
}

#endif
