#include "driver.h"

/* Game writes here to set ADPCM ROM address */
WRITE_HANDLER( rastan_adpcm_trigger_w )
{
	UINT8 *rom = memory_region(REGION_SOUND1);
	int len = memory_region_length(REGION_SOUND1);
	int start = data << 8;
	int end;

	/* look for end of sample */
	end = (start + 3) & ~3;
	while (end < len && *((UINT32 *)(&rom[end])) != 0x08080808)
		end += 4;

	ADPCM_play(0,start,(end-start)*2);
}

/* Game writes here to START ADPCM_voice playing */
WRITE_HANDLER( rastan_c000_w )
{
}

/* Game writes here to STOP ADPCM_voice playing */
WRITE_HANDLER( rastan_d000_w )
{
#if 0
	if (Machine->samples == 0) return;
	if (data==0)
		mixer_stop_sample(channel);
#endif
}
