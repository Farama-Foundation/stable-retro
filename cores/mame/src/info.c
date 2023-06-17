#include <ctype.h>

#include "driver.h"
#include "sound/samples.h"
#include "info.h"
#include "hash.h"
#include "datafile.h"
#include "fileio.h"
#include "libretro.h"
#include "log.h"

#define XML_ROOT "mame"
#define XML_TOP "game"

/* Print a free format string */
static void print_free_string(FILE* out, const char* s)
{
    if (s)
    {
        while (*s)
        {
            switch (*s)
            {
                case '\"' : fprintf(out, "&quot;"); break;
                case '&'  : fprintf(out, "&amp;"); break;
                case '<'  : fprintf(out, "&lt;"); break;
                case '>'  : fprintf(out, "&gt;"); break;
                default:
                    if (*s>=' ' && *s<='~')
                        fprintf(out, "%c", *s);
                    else
                        fprintf(out, "&#%d;", (unsigned)(unsigned char)*s);
            }
            ++s;
        }
    }
}

static void print_game_switch(FILE* out, const struct GameDriver* game)
{
	const struct InputPortTiny* input = game->input_ports;

	while ((input->type & ~IPF_MASK) != IPT_END)
	{
		if ((input->type & ~IPF_MASK)==IPT_DIPSWITCH_NAME)
		{
			int def = input->default_value;

			fprintf(out, "\t\t<dipswitch name=\"");
			print_free_string(out, input->name);
			fprintf(out, "%s", "\">\n");
			++input;

			while ((input->type & ~IPF_MASK)==IPT_DIPSWITCH_SETTING)
			{
				fprintf(out, "\t\t\t<dipvalue name=\"");
				print_free_string(out, input->name);
				fprintf(out, "%s", "\"");
				if (def == input->default_value)
				{
					fprintf(out, " default=\"yes\"");
				}

				fprintf(out, "%s", "/>\n");

				++input;
			}

			fprintf(out, "\t\t</dipswitch>\n");
		}
		else
			++input;
	}
}

static void print_game_input(FILE* out, const struct GameDriver* game)
{
	const struct InputPortTiny* input = game->input_ports;
	int nplayer = 0;
	const char* control = 0;
	int nbutton = 0;
	int ncoin = 0;
	const char* service = 0;
	const char* tilt = 0;

	while ((input->type & ~IPF_MASK) != IPT_END)
	{
		/* skip analog extension fields */
		if ((input->type & ~IPF_MASK) != IPT_EXTENSION)
		{
			switch (input->type & IPF_PLAYERMASK)
			{
				case IPF_PLAYER1:
					if (nplayer<1) nplayer = 1;
					break;
				case IPF_PLAYER2:
					if (nplayer<2) nplayer = 2;
					break;
				case IPF_PLAYER3:
					if (nplayer<3) nplayer = 3;
					break;
				case IPF_PLAYER4:
					if (nplayer<4) nplayer = 4;
					break;
				case IPF_PLAYER5:
					if (nplayer<5) nplayer = 5;
					break;
				case IPF_PLAYER6:
					if (nplayer<6) nplayer = 6;
					break;
				case IPF_PLAYER7:
					if (nplayer<7) nplayer = 7;
					break;
				case IPF_PLAYER8:
					if (nplayer<8) nplayer = 8;
					break;
			}
			switch (input->type & ~IPF_MASK)
			{
				case IPT_JOYSTICK_UP:
				case IPT_JOYSTICK_DOWN:
				case IPT_JOYSTICK_LEFT:
				case IPT_JOYSTICK_RIGHT:
					if (input->type & IPF_2WAY)
						control = "joy2way";
					else if (input->type & IPF_4WAY)
						control = "joy4way";
					else
						control = "joy8way";
					break;
				case IPT_JOYSTICKRIGHT_UP:
				case IPT_JOYSTICKRIGHT_DOWN:
				case IPT_JOYSTICKRIGHT_LEFT:
				case IPT_JOYSTICKRIGHT_RIGHT:
				case IPT_JOYSTICKLEFT_UP:
				case IPT_JOYSTICKLEFT_DOWN:
				case IPT_JOYSTICKLEFT_LEFT:
				case IPT_JOYSTICKLEFT_RIGHT:
					if (input->type & IPF_2WAY)
						control = "doublejoy2way";
					else if (input->type & IPF_4WAY)
						control = "doublejoy4way";
					else
						control = "doublejoy8way";
					break;
				case IPT_BUTTON1:
					if (nbutton<1) nbutton = 1;
					break;
				case IPT_BUTTON2:
					if (nbutton<2) nbutton = 2;
					break;
				case IPT_BUTTON3:
					if (nbutton<3) nbutton = 3;
					break;
				case IPT_BUTTON4:
					if (nbutton<4) nbutton = 4;
					break;
				case IPT_BUTTON5:
					if (nbutton<5) nbutton = 5;
					break;
				case IPT_BUTTON6:
					if (nbutton<6) nbutton = 6;
					break;
				case IPT_BUTTON7:
					if (nbutton<7) nbutton = 7;
					break;
				case IPT_BUTTON8:
					if (nbutton<8) nbutton = 8;
					break;
				case IPT_BUTTON9:
					if (nbutton<9) nbutton = 9;
					break;
				case IPT_BUTTON10:
					if (nbutton<10) nbutton = 10;
					break;
				case IPT_PADDLE:
					control = "paddle";
					break;
				case IPT_DIAL:
					control = "dial";
					break;
				case IPT_TRACKBALL_X:
				case IPT_TRACKBALL_Y:
					control = "trackball";
					break;
				case IPT_AD_STICK_X:
				case IPT_AD_STICK_Y:
					control = "stick";
					break;
				case IPT_LIGHTGUN_X:
				case IPT_LIGHTGUN_Y:
					control = "lightgun";
					break;
				case IPT_COIN1:
					if (ncoin < 1) ncoin = 1;
					break;
				case IPT_COIN2:
					if (ncoin < 2) ncoin = 2;
					break;
				case IPT_COIN3:
					if (ncoin < 3) ncoin = 3;
					break;
				case IPT_COIN4:
					if (ncoin < 4) ncoin = 4;
					break;
				case IPT_COIN5:
					if (ncoin < 5) ncoin = 5;
					break;
				case IPT_COIN6:
					if (ncoin < 6) ncoin = 6;
					break;
				case IPT_COIN7:
					if (ncoin < 7) ncoin = 7;
					break;
				case IPT_COIN8:
					if (ncoin < 8) ncoin = 8;
					break;
				case IPT_SERVICE :
					service = "yes";
					break;
				case IPT_TILT :
					tilt = "yes";
					break;
			}
		}
		++input;
	}

	fprintf(out, "\t\t<input");
	fprintf(out, " players=\"%d\"", nplayer );
	if (control)
		fprintf(out, " control=\"%s\"", control );
	if (nbutton)
		fprintf(out, " buttons=\"%d\"", nbutton );
	if (ncoin)
		fprintf(out, " coins=\"%d\"", ncoin );
	if (service)
		fprintf(out, " service=\"%s\"", service );
	if (tilt)
		fprintf(out, " tilt=\"%s\"", tilt );
	fprintf(out, "/>\n");
}

static void print_game_bios(FILE* out, const struct GameDriver* game)
{
	const struct SystemBios *thisbios;

	if(!game->bios)
		return;

	thisbios = game->bios;

	/* Match against bios short names */
	while(!BIOSENTRY_ISEND(thisbios))
	{
		fprintf(out, "\t\t<biosset");

		if (thisbios->_name)
			fprintf(out, " name=\"%s\"", thisbios->_name);
		if (thisbios->_description)
			fprintf(out, " description=\"%s\"", thisbios->_description);
		if (thisbios->value == 0)
			fprintf(out, " default=\"yes\"");

		fprintf(out, "/>\n");

		thisbios++;
	}
}

static void print_game_rom(FILE* out, const struct GameDriver* game)
{
	const struct RomModule *region, *rom, *chunk;
	const struct RomModule *pregion, *prom, *fprom=NULL;
/*	extern struct GameDriver driver_0;*/

	if (!game->rom)
		return;

	for (region = rom_first_region(game); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			int offset, length, in_parent, is_disk, is_bios, found_bios, i;
			char name[100], bios_name[100];

			strcpy(name,ROM_GETNAME(rom));
			offset = ROM_GETOFFSET(rom);
			is_disk = ROMREGION_ISDISKDATA(region);
			is_bios = ROM_GETBIOSFLAGS(rom);

			in_parent = 0;
			length = 0;
			for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
				length += ROM_GETLENGTH(chunk);


			if (!ROM_NOGOODDUMP(rom) && game->clone_of)
			{
				fprom=NULL;
				for (pregion = rom_first_region(game->clone_of); pregion; pregion = rom_next_region(pregion))
					for (prom = rom_first_file(pregion); prom; prom = rom_next_file(prom))
						if (hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(prom), 0))
						{
							if (!fprom || !strcmp(ROM_GETNAME(prom), name))
								fprom=prom;
							in_parent = 1;
						}
			}

			found_bios = 0;
			if(!is_disk && is_bios && game->bios)
			{
				const struct SystemBios *thisbios = game->bios;

				/* Match against bios short names */
				while(!found_bios && !BIOSENTRY_ISEND(thisbios) )
				{
					if((is_bios-1) == thisbios->value) /* Note '-1' */
					{
						strcpy(bios_name,thisbios->_name);
						found_bios = 1;
					}

					thisbios++;
				}
			}


			if (!is_disk)
				fprintf(out, "\t\t<rom");
			else
				fprintf(out, "\t\t<disk");

			if (*name)
				fprintf(out, " name=\"%s\"", name);
			if (!is_disk && in_parent)
				fprintf(out, " merge=\"%s\"", ROM_GETNAME(fprom));
			if (!is_disk && found_bios)
				fprintf(out, " bios=\"%s\"", bios_name);
			if (!is_disk)
				fprintf(out, " size=\"%d\"", length);

			/* dump checksum information only if there is a known dump */
			if (!hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
			{
				for (i=0;i<HASH_NUM_FUNCTIONS;i++)
				{
					int func = 1<<i;
					const char* func_name = hash_function_name(func);
					char checksum[1000];

					if (hash_data_extract_printable_checksum(ROM_GETHASHDATA(rom), func, checksum))
					{
						fprintf(out, " %s=\"%s\"", func_name, checksum);
					}
				}
			}

			switch (ROMREGION_GETTYPE(region))
			{
				case REGION_CPU1:   fprintf(out, " region=\"cpu1\""); break;
				case REGION_CPU2:   fprintf(out, " region=\"cpu2\""); break;
				case REGION_CPU3:   fprintf(out, " region=\"cpu3\""); break;
				case REGION_CPU4:   fprintf(out, " region=\"cpu4\""); break;
				case REGION_CPU5:   fprintf(out, " region=\"cpu5\""); break;
				case REGION_CPU6:   fprintf(out, " region=\"cpu6\""); break;
				case REGION_CPU7:   fprintf(out, " region=\"cpu7\""); break;
				case REGION_CPU8:   fprintf(out, " region=\"cpu8\""); break;
				case REGION_GFX1:   fprintf(out, " region=\"gfx1\""); break;
				case REGION_GFX2:   fprintf(out, " region=\"gfx2\""); break;
				case REGION_GFX3:   fprintf(out, " region=\"gfx3\""); break;
				case REGION_GFX4:   fprintf(out, " region=\"gfx4\""); break;
				case REGION_GFX5:   fprintf(out, " region=\"gfx5\""); break;
				case REGION_GFX6:   fprintf(out, " region=\"gfx6\""); break;
				case REGION_GFX7:   fprintf(out, " region=\"gfx7\""); break;
				case REGION_GFX8:   fprintf(out, " region=\"gfx8\""); break;
				case REGION_PROMS:  fprintf(out, " region=\"proms\""); break;
				case REGION_SOUND1: fprintf(out, " region=\"sound1\""); break;
				case REGION_SOUND2: fprintf(out, " region=\"sound2\""); break;
				case REGION_SOUND3: fprintf(out, " region=\"sound3\""); break;
				case REGION_SOUND4: fprintf(out, " region=\"sound4\""); break;
				case REGION_SOUND5: fprintf(out, " region=\"sound5\""); break;
				case REGION_SOUND6: fprintf(out, " region=\"sound6\""); break;
				case REGION_SOUND7: fprintf(out, " region=\"sound7\""); break;
				case REGION_SOUND8: fprintf(out, " region=\"sound8\""); break;
				case REGION_USER1:  fprintf(out, " region=\"user1\""); break;
				case REGION_USER2:  fprintf(out, " region=\"user2\""); break;
				case REGION_USER3:  fprintf(out, " region=\"user3\""); break;
				case REGION_USER4:  fprintf(out, " region=\"user4\""); break;
				case REGION_USER5:  fprintf(out, " region=\"user5\""); break;
				case REGION_USER6:  fprintf(out, " region=\"user6\""); break;
				case REGION_USER7:  fprintf(out, " region=\"user7\""); break;
				case REGION_USER8:  fprintf(out, " region=\"user8\""); break;
				case REGION_DISKS:  fprintf(out, " region=\"disks\""); break;
				default:            fprintf(out, " region=\"0x%x\"", ROMREGION_GETTYPE(region));
		}

		if (!is_disk)
		{

            if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
                fprintf(out, " status=\"nodump\"");
            if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP))
                fprintf(out, " status=\"baddump\"");
            if (ROMREGION_GETFLAGS(region) & ROMREGION_DISPOSE)
                fprintf(out, " dispose=\"yes\"");
            if (ROMREGION_GETFLAGS(region) & ROMREGION_SOUNDONLY)
                fprintf(out, " soundonly=\"yes\"");

			fprintf(out, " offset=\"%x\"", offset);
			fprintf(out, "/>\n");
		}
		else
		{
			fprintf(out, " index=\"%x\"", DISK_GETINDEX(rom));
			fprintf(out, "/>\n");
		}
	}

}

static int sampleof;
static void print_game_sampleof(FILE* out, const struct GameDriver* game)
{
#if (HAS_SAMPLES)
	struct InternalMachineDriver drv;
	int i=0;
	sampleof =0;
	expand_machine_driver(game->drv, &drv);

	for( i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++ )	{
		const char **samplenames = NULL;
		if( drv.sound[i].sound_type == SOUND_SAMPLES )
			samplenames = ((struct Samplesinterface *)drv.sound[i].sound_interface)->samplenames;
		if (samplenames != 0 && samplenames[0] != 0){
			int k = 0;
			if (samplenames[k][0]=='*'){
				/* output sampleof only if different from game name */
				if (strcmp(samplenames[k] + 1, game->name)!=0){
					fprintf(out, " sampleof=\"%s\"", samplenames[k] + 1);
					sampleof=1;
					++k;
				}
			}
		}
	}
#endif
}

static void print_game_sample(FILE* out, const struct GameDriver* game)
{
#if (HAS_SAMPLES)
	struct InternalMachineDriver drv;
	int i=0;

  if (!sampleof || true){

		expand_machine_driver(game->drv, &drv);

		for( i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++ )
		{
			const char **samplenames = NULL;
			if( drv.sound[i].sound_type == SOUND_SAMPLES )
				samplenames = ((struct Samplesinterface *)drv.sound[i].sound_interface)->samplenames;
			if (samplenames != 0 && samplenames[0] != 0) {
				int k = 0;
				if (samplenames[k][0]=='*')
				{
					++k;
				}
				while (samplenames[k] != 0) {
					/* check if is not empty */
					if (*samplenames[k]) {
						/* check if sample is duplicate */
						int l = 0;
						while (l<k && strcmp(samplenames[k],samplenames[l])!=0)
							++l;
						if (l==k)
							fprintf(out, "\t\t<sample name=\"%s\"/>\n", samplenames[k]);
					}
						++k;
				}
			}
		}
  }
#endif
}

static void print_game_micro(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;
	const struct MachineCPU* cpu;
	const struct MachineSound* sound;
	int j;

	expand_machine_driver(game->drv, &driver);
	cpu = driver.cpu;
	sound = driver.sound;

	for(j=0;j<MAX_CPU;++j)
	{
		if (cpu[j].cpu_type!=0)
		{
			fprintf(out, "\t\t<chip");
			if (cpu[j].cpu_flags & CPU_AUDIO_CPU)
				fprintf(out, " type=\"cpu\" soundonly=\"yes\"");
			else
				fprintf(out, " type=\"cpu\"");

			fprintf(out, " name=\"");
			print_free_string(out, cputype_name(cpu[j].cpu_type));
			fprintf(out, "%s", "\"");

			fprintf(out, " clock=\"%d\"/>\n", cpu[j].cpu_clock);
		}
	}

	for(j=0;j<MAX_SOUND;++j) if (sound[j].sound_type)
	{
		if (sound[j].sound_type)
		{
			int num = sound_num(&sound[j]);
			int l;

			if (num == 0) num = 1;

			for(l=0;l<num;++l)
			{
				fprintf(out, "\t\t<chip type=\"audio\" name=\"");
				print_free_string(out, sound_name(&sound[j]));
				fprintf(out, "%s", "\"");
				if (sound_clock(&sound[j]))
					fprintf(out, " clock=\"%d\"", sound_clock(&sound[j]));
				fprintf(out, "/>\n");
			}
		}
	}
}

static void print_game_video(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;

	int dx;
	int dy;
	int ax;
	int ay;
	int showxy;
	int orientation;

	expand_machine_driver(game->drv, &driver);

	fprintf(out, "\t\t<video");
	if (driver.video_attributes & VIDEO_TYPE_VECTOR)
	{
		fprintf(out, " screen=\"vector\"");
		showxy = 0;
	}
	else
	{
		fprintf(out, " screen=\"raster\"");
		showxy = 1;
	}

	if (game->flags & ORIENTATION_SWAP_XY)
	{
		ax = driver.aspect_y;
		ay = driver.aspect_x;
		if (ax == 0 && ay == 0) {
			ax = 3;
			ay = 4;
		}
		dx = driver.default_visible_area.max_y - driver.default_visible_area.min_y + 1;
		dy = driver.default_visible_area.max_x - driver.default_visible_area.min_x + 1;
		orientation = 1;
	}
	else
	{
		ax = driver.aspect_x;
		ay = driver.aspect_y;
		if (ax == 0 && ay == 0) {
			ax = 4;
			ay = 3;
		}
		dx = driver.default_visible_area.max_x - driver.default_visible_area.min_x + 1;
		dy = driver.default_visible_area.max_y - driver.default_visible_area.min_y + 1;
		orientation = 0;
	}

	fprintf(out, " orientation=\"%s\"", orientation ? "vertical" : "horizontal" );
	if (showxy)
	{
		fprintf(out, " width=\"%d\" height=\"%d\"", dx, dy);
	}

	fprintf(out, " aspectx=\"%d\" aspecty=\"%d\" refresh=\"%f\"/>\n", ax, ay, driver.frames_per_second);
}

static void print_game_sound(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;
	const struct MachineCPU* cpu;
	const struct MachineSound* sound;

	/* check if the game have sound emulation */
	int has_sound = 0;
	int i;

	expand_machine_driver(game->drv, &driver);
	cpu = driver.cpu;
	sound = driver.sound;

	i = 0;
	while (i < MAX_SOUND && !has_sound)
	{
		if (sound[i].sound_type)
			has_sound = 1;
		++i;
	}
	i = 0;
	while (i < MAX_CPU && !has_sound)
	{
		if  ((cpu[i].cpu_flags & CPU_AUDIO_CPU)!=0)
			has_sound = 1;
		++i;
	}

	fprintf(out, "\t\t<sound");

	/* sound channel */
	if (has_sound)
	{
		if (driver.sound_attributes & SOUND_SUPPORTS_STEREO)
			fprintf(out, " channels=\"2\"");
		else
			fprintf(out, " channels=\"1\"");
	}
	else
		fprintf(out, " channels=\"0\"");

	fprintf(out, "/>\n");
}

#define HISTORY_BUFFER_MAX 16384

static void print_game_history(FILE* out, const struct GameDriver* game)
{
	char buffer[HISTORY_BUFFER_MAX];

	if (load_driver_history(game,buffer,HISTORY_BUFFER_MAX)==0)
	{
		fprintf(out, "\t\t<history>");
		print_free_string(out, buffer);
		fprintf(out, "</history>\n");
	}
}

static void print_game_driver(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;

	expand_machine_driver(game->drv, &driver);

	fprintf(out, "\t\t<driver");
	if (game->flags & GAME_NOT_WORKING)
		fprintf(out, " status=\"preliminary\"");
	else if (game->flags & GAME_UNEMULATED_PROTECTION)
		fprintf(out, " status=\"protection\"");
	else
		fprintf(out, " status=\"good\"");

	if (game->flags & GAME_WRONG_COLORS)
		fprintf(out, " color=\"preliminary\"");
	else if (game->flags & GAME_IMPERFECT_COLORS)
		fprintf(out, " color=\"imperfect\"");
	else
		fprintf(out, " color=\"good\"");

	if (game->flags & GAME_NO_SOUND)
		fprintf(out, " sound=\"preliminary\"");
	else if (game->flags & GAME_IMPERFECT_SOUND)
		fprintf(out, " sound=\"imperfect\"");
	else
		fprintf(out, " sound=\"good\"");

	if (game->flags & GAME_IMPERFECT_GRAPHICS)
		fprintf(out, " graphic=\"imperfect\"");
	else
		fprintf(out, " graphic=\"good\"");

	fprintf(out, " palettesize=\"%d\"/>\n", driver.total_colors);

}

/* Print the MAME info record for a game */
static void print_game_info(FILE* out, const struct GameDriver* game)
{
	extern struct GameDriver driver_0;
	const char *start;

	fprintf(out, "\t<" XML_TOP);

	fprintf(out, " name=\"%s\"", game->name ); /* use GameDrv "name" field as the filename */

	start = strrchr(game->source_file, '/');
	if (!start)
		start = strrchr(game->source_file, '\\');
	if (!start)
		start = game->source_file - 1;
	fprintf(out, " sourcefile=\"%s\"", start + 1);


	if (game->clone_of && !(game->clone_of->flags & NOT_A_DRIVER))
		fprintf(out, " cloneof=\"%s\"", game->clone_of->name);

	if (game->clone_of && game->clone_of != &driver_0)
		fprintf(out, " romof=\"%s\"", game->clone_of->name);

	print_game_sampleof(out, game);

	fprintf(out, "%s", ">\n");

	fprintf(out, "\t\t<description>");
	print_free_string(out, game->description);
	fprintf(out, "</description>\n");

	/* print the year */
	if (game->year != NULL)
		fprintf(out, "\t\t<year>%s</year>\n", game->year );

	if (game->manufacturer)
	{
		fprintf(out, "\t\t<manufacturer>");
		print_free_string(out, game->manufacturer);
		fprintf(out, "</manufacturer>\n");
	}

/*	print_game_history(out, game); */
	print_game_bios(out, game);
	print_game_rom(out, game);
	print_game_sample(out, game);
	print_game_micro(out, game);
	print_game_video(out, game);
	print_game_sound(out, game);
	print_game_input(out, game);
	print_game_switch(out, game);
	print_game_driver(out, game);

	fprintf(out, "\t</" XML_TOP ">\n");
}

/* Print the resource info */
static void print_resource_info(FILE* out, const struct GameDriver* game)
{
	fprintf(out, "\t<" XML_TOP " runnable=\"no\" name=\"%s\">\n", game->name);

	if (game->description)
	{
		fprintf(out, "\t\t<description>");
		print_free_string(out, game->description);
		fprintf(out, "</description>\n");
	}

	/* print the year */
	if (game->year != NULL)
		fprintf(out, "\t\t<year>%s</year>\n", game->year );

	if (game->manufacturer)
	{
		fprintf(out, "\t\t<manufacturer>");
		print_free_string(out, game->manufacturer);
		fprintf(out, "</manufacturer>\n");
	}

	print_game_bios(out, game);
	print_game_rom(out, game);
	print_game_sample(out, game);

	fprintf(out, "\t</" XML_TOP ">\n");
}

/* Import the driver object and print it as a resource */
#define PRINT_RESOURCE(s) \
	{ \
		extern struct GameDriver driver_##s; \
		print_resource_info(xml_dat, &driver_##s); \
	}


/* Print the MAME database in XML format */
void print_mame_xml()
{
    int driver_index = 0;
    FILE *xml_dat = osd_fopen(FILETYPE_XML_DAT, 1, APPNAME".xml", "w+b");

    if (xml_dat != NULL)
    {
	    log_cb(RETRO_LOG_INFO, LOGPRE "Generating mame2003-plus.xml\n");
    } else {
      log_cb(RETRO_LOG_WARN, LOGPRE "Unable to open mame2003-plus.xml for writing.\n");
      return;
    }

  fprintf(xml_dat,
		"<?xml version=\"1.0\"?>\n"
		"<!DOCTYPE " XML_ROOT " [\n"
		"<!ELEMENT " XML_ROOT " (" XML_TOP "+)>\n"
		"\t<!ELEMENT " XML_TOP " (description, year?, manufacturer, history?, biosset*, rom*, disk*, sample*, chip*, video?, sound?, input?, dipswitch*, driver?)>\n"
		"\t\t<!ATTLIST " XML_TOP " name CDATA #REQUIRED>\n"
		"\t\t<!ATTLIST " XML_TOP " sourcefile CDATA #IMPLIED>\n"
		"\t\t<!ATTLIST " XML_TOP " runnable (yes|no) \"yes\">\n"
		"\t\t<!ATTLIST " XML_TOP " cloneof CDATA #IMPLIED>\n"
		"\t\t<!ATTLIST " XML_TOP " romof CDATA #IMPLIED>\n"
		"\t\t<!ATTLIST " XML_TOP " sampleof CDATA #IMPLIED>\n"
		"\t\t<!ELEMENT description (#PCDATA)>\n"
/*		"\t\t<!ELEMENT driver (#PCDATA)>\n" */
		"\t\t<!ELEMENT year (#PCDATA)>\n"
		"\t\t<!ELEMENT manufacturer (#PCDATA)>\n"
		/*"\t\t<!ELEMENT history (#PCDATA)>\n"*/
		"\t\t<!ELEMENT biosset EMPTY>\n"
		"\t\t\t<!ATTLIST biosset name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST biosset description CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST biosset default (yes|no) \"no\">\n"
		"\t\t<!ELEMENT rom EMPTY>\n"
		"\t\t\t<!ATTLIST rom name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST rom bios CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom size CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST rom crc CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom md5 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom sha1 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom merge CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom region CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom offset CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom status (baddump|nodump|good) \"good\">\n"
		"\t\t\t<!ATTLIST rom dispose (yes|no) \"no\">\n"
		"\t\t\t<!ATTLIST rom soundonly (yes|no) \"no\">\n"
		"\t\t<!ELEMENT disk EMPTY>\n"
		"\t\t\t<!ATTLIST disk name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST disk md5 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST disk sha1 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST disk region CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST disk index CDATA #IMPLIED>\n"
		"\t\t<!ELEMENT sample EMPTY>\n"
		"\t\t\t<!ATTLIST sample name CDATA #REQUIRED>\n"
		"\t\t<!ELEMENT chip EMPTY>\n"
		"\t\t\t<!ATTLIST chip name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST chip type (cpu|audio) #REQUIRED>\n"
		"\t\t\t<!ATTLIST chip soundonly (yes|no) \"no\">\n"
		"\t\t\t<!ATTLIST chip clock CDATA #IMPLIED>\n"
		"\t\t<!ELEMENT video EMPTY>\n"
		"\t\t\t<!ATTLIST video screen (raster|vector) #REQUIRED>\n"
		"\t\t\t<!ATTLIST video orientation (vertical|horizontal) #REQUIRED>\n"
		"\t\t\t<!ATTLIST video width CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video height CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video aspectx CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video aspecty CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video refresh CDATA #REQUIRED>\n"
		"\t\t<!ELEMENT sound EMPTY>\n"
		"\t\t\t<!ATTLIST sound channels CDATA #REQUIRED>\n"
		"\t\t<!ELEMENT input EMPTY>\n"
		"\t\t\t<!ATTLIST input service (yes|no) \"no\">\n"
		"\t\t\t<!ATTLIST input tilt (yes|no) \"no\">\n"
		"\t\t\t<!ATTLIST input players CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST input control CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST input buttons CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST input coins CDATA #IMPLIED>\n"
		"\t\t<!ELEMENT dipswitch (dipvalue*)>\n"
		"\t\t\t<!ATTLIST dipswitch name CDATA #REQUIRED>\n"
		"\t\t\t<!ELEMENT dipvalue EMPTY>\n"
		"\t\t\t\t<!ATTLIST dipvalue name CDATA #REQUIRED>\n"
		"\t\t\t\t<!ATTLIST dipvalue default (yes|no) \"no\">\n"
		"\t\t<!ELEMENT driver EMPTY>\n"
		"\t\t\t<!ATTLIST driver status (good|preliminary|protection) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver color (good|imperfect|preliminary) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver sound (good|imperfect|preliminary) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver graphic (good|imperfect) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver palettesize CDATA #REQUIRED>\n"
		"]>\n\n"
		"<" XML_ROOT ">\n"
	);

	/* print games */
	for(driver_index = 0;drivers[driver_index];++driver_index)
		print_game_info(xml_dat, drivers[driver_index]);
#ifndef SPLIT_CORE
	/* print the resources (only if linked) */
	PRINT_RESOURCE(neogeo);
	PRINT_RESOURCE(cvs);
	PRINT_RESOURCE(decocass);
	PRINT_RESOURCE(playch10);
	PRINT_RESOURCE(pgm);
	PRINT_RESOURCE(skns);
	PRINT_RESOURCE(stvbios);
	PRINT_RESOURCE(konamigx);
	PRINT_RESOURCE(nss);
	PRINT_RESOURCE(megatech);
	PRINT_RESOURCE(megaplay);
	PRINT_RESOURCE(cpzn1);
	PRINT_RESOURCE(cpzn2);
	PRINT_RESOURCE(tps);
	PRINT_RESOURCE(taitofx1);
	PRINT_RESOURCE(acpsx);
#endif
	fprintf(xml_dat, "</" XML_ROOT ">\n");
    fclose(xml_dat);
}
