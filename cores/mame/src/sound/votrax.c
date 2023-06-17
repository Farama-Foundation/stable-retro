/**************************************************************************

	Votrax SC-01 Emulator

 	Mike@Dissfulfils.co.uk

**************************************************************************

sh_votrax_start  - Start emulation, load samples from Votrax subdirectory
sh_votrax_stop   - End emulation, free memory used for samples
votrax_w		 - Write data to votrax port
votrax_status    - Return busy status (-1 = busy)

If you need to alter the base frequency (i.e. Qbert) then just alter
the variable VotraxBaseFrequency, this is defaulted to 8000

**************************************************************************/

#include "driver.h"

int		VotraxBaseFrequency;		/* Some games (Qbert) change this */
int 	VotraxBaseVolume;
int 	VotraxChannel;

struct  GameSamples *VotraxSamples;

/****************************************************************************
 * 64 Phonemes - currently 1 sample per phoneme, will be combined sometime!
 ****************************************************************************/

static const char *VotraxTable[65] =
{
 "EH3","EH2","EH1","PA0","DT" ,"A1" ,"A2" ,"ZH",
 "AH2","I3" ,"I2" ,"I1" ,"M"  ,"N"  ,"B"  ,"V",
 "CH" ,"SH" ,"Z"  ,"AW1","NG" ,"AH1","OO1","OO",
 "L"  ,"K"  ,"J"  ,"H"  ,"G"  ,"F"  ,"D"  ,"S",
 "A"  ,"AY" ,"Y1" ,"UH3","AH" ,"P"  ,"O"  ,"I",
 "U"  ,"Y"  ,"T"  ,"R"  ,"E"  ,"W"  ,"AE" ,"AE1",
 "AW2","UH2","UH1","UH" ,"O2" ,"O1" ,"IU" ,"U1",
 "THV","TH" ,"ER" ,"EH" ,"E1" ,"AW" ,"PA1","STOP",
 0
};

void sh_votrax_start(int Channel)
{
	VotraxSamples = readsamples(VotraxTable,"votrax");
    VotraxBaseFrequency = 8000;
    VotraxBaseVolume = 230;
    VotraxChannel = Channel;
}

void sh_votrax_stop(void)
{
	freesamples(VotraxSamples);
}

void votrax_w(int data)
{
	int Phoneme,Intonation;

    Phoneme = data & 0x3F;
    Intonation = data >> 6;

  	log_cb(RETRO_LOG_DEBUG, LOGPRE "Speech : %s at intonation %d\n",VotraxTable[Phoneme],Intonation);

    if(Phoneme==63)
   		mixer_stop_sample(VotraxChannel);

    if(VotraxSamples->sample[Phoneme])
	{
		mixer_set_volume(VotraxChannel,VotraxBaseVolume+(8*Intonation)*100/255);
		mixer_play_sample(VotraxChannel,VotraxSamples->sample[Phoneme]->data,
				  VotraxSamples->sample[Phoneme]->length,
				  VotraxBaseFrequency+(256*Intonation),
				  0);
	}
}

int votrax_status_r(void)
{
    return mixer_is_sample_playing(VotraxChannel);
}
