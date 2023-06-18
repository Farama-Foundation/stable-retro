#include "driver.h"
#include "tiaintf.h"
#include "tiasound.h"

static int channel = -1;
static const struct TIAinterface *intf;

int tia_sh_start(const struct MachineSound *msound)
{
    intf = msound->sound_interface;

    if (Machine->sample_rate == 0)
        return 0;

	channel = stream_init("TIA", intf->volume, Machine->sample_rate, 0, tia_process);
	if (channel == -1)
        return 1;

	tia_sound_init(intf->clock, Machine->sample_rate, intf->gain);

    return 0;
}


void tia_sh_stop(void)
{
    /* Nothing to do here */
}

void tia_sh_update(void)
{
	stream_update(channel, 0);
}
