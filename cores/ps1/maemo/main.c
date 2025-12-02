/*
 * (C) notaz, 2010-2011
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "../frontend/main.h"
#include "../frontend/menu.h"
#include "../frontend/plugin.h"
#include "../frontend/plugin_lib.h"
#include "../libpcsxcore/misc.h"
#include "../libpcsxcore/cdriso.h"
#include "../libpcsxcore/new_dynarec/new_dynarec.h"
#include "../plugins/dfsound/spu_config.h"
#include "maemo_common.h"

extern int in_enable_vibration;
extern int in_type1, in_type2;

accel_option accelOptions;
int ready_to_go, g_emu_want_quit, g_emu_resetting;
int g_menuscreen_w, g_menuscreen_h;
int g_scaler, soft_filter;
int g_opts = 0;
int g_maemo_opts;
int cornerActions[4] = {0,0,0,0};
int bKeepDisplayOn = FALSE;
int bAutosaveOnExit = FALSE;
char file_name[MAXPATHLEN];
char keys_config_file[MAXPATHLEN] = "/opt/psx4m/keys";

enum sched_action emu_action;
void do_emu_action(void);

static void ChangeWorkingDirectory(char *exe)
{
	char exepath[1024];
	char *s;
	snprintf(exepath, sizeof(exepath), "%s", exe);
	s = strrchr(exepath, '/');
	if (s != NULL) {
		*s = '\0';
		chdir(exepath);
	}
}

void PrintHelp()
{
	printf("PCSX-ReARMed version %s for Maemo\n\n", PACKAGE_VERSION);

	printf("Usage:\n");
	printf("  pcsx [options] -cdfile FILE\n\n");

	printf("Options:\n");
	printf("  -help          : This help\n");
	printf("  -disc VALUE    : Disc number for multi discs images\n");
	printf("  -fullscreen    : Run fullscreen\n");
	printf("  -frameskip     : Frameskip\n");
	printf("                   -1=Auto (Default)\n");
	printf("                    0=Disabled\n");
	printf("                    1=Set to 1\n");
	printf("                    ...\n");
 	printf("  -autosave      : Enable auto save on exit\n");
	printf("  -accel         : Enable accelerometer\n");
	printf("  -analog        : Use analog pad for accel\n");
	printf("  -vibration     : Activate vibration\n");
	printf("  -sens VALUE    : Set accelerometer sens [0-1000]\n");
    printf("                   (Default 150)\n");
	printf("  -ydef VALUE    : Set accelerometer y zero [0-1000]\n");
    printf("                   (Default 500)\n");
	printf("  -max VALUE     : Set accelerometer max value[0-1000]\n");
    printf("                   (Default 500)\n");
	printf("  -nosound       : No sound output\n");
	printf("  -bdir PATH     : Set the bios path\n");
	printf("  -pdir PATH     : Set the plugins path\n");
	printf("  -bios          : Set the bios\n");
	printf("  -cdda          : Disable CD Audio for a performance boost\n");
	printf("  -xa            : Disables XA sound, which can sometimes\n");
	printf("                   improve performance\n");
	printf("  -sio           : SIO IRQ Always Enabled\n");
	printf("  -spuirq        : SPU IRQ Always Enabled\n");
	printf("  -fps           : Show fps\n");
	printf("  -cpu           : Show CPU load\n");
	printf("  -spu           : Show SPU channels\n");
	printf("  -nofl          : Disable Frame Limiter\n");
	printf("  -mcd1 FILE     : Set memory card 1 file\n");
	printf("  -mcd2 FILE     : Set memory card 2 file\n");
	printf("  -region VALUE  : Set PSX region\n");
	printf("                   -1=Auto (Default)\n");
	printf("                    0=NTSC\n");
	printf("                    1=PAL\n");
	printf("  -cpuclock VALUE: PSX CPU clock %% [1-500]\n");
    printf("                   (Default 50)\n");
	printf("  -displayon     : Prevent display from blanking\n");
    printf("                   (Default disabled)\n");
	printf("  -keys FILE     : File with keys configuration\n");
    printf("                   (Default /opt/psx4m/keys)\n");
	printf("  -corners VALUE : Define actions for click on the\n");
    printf("                   display corners\n");
    printf("                   VALUE is a four digit number, each number\n");
    printf("                   represent a corner (topleft, topright,\n");
    printf("                   bottomright and bottomleft\n");
    printf("                   Actions:\n");
    printf("                   0=No action\n");
    printf("                   1=Save\n");
    printf("                   2=Load\n");
    printf("                   3=Change slot (+1)\n");
    printf("                   4=Change slot (-1)\n");
    printf("                   5=Quit\n");
	printf("  -guncon        : Set the controller to guncon\n");
	printf("  -gunnotrigger  : Don't trigger (shoot) when touching screen\n");
    printf("                   0=Auto (Default)\n");
    printf("                   1=On\n");
    printf("                   2=Off\n");


	printf("\nGPU Options:\n");
	printf("  -gles          : Use the GLES plugin (gpu_gles.so)\n");
	printf("  -oldgpu        : Use the peops plugin (gpu_peops.so)\n");
	printf("  -unai          : Use the unai plugin (gpu_unai.so)\n");

	printf("\nSound Options:\n");
	printf("  -spu_reverb VALUE        : Enable/disable reverb [0/1]\n");
    printf("                             (Default disabled)\n");
	printf("  -spu_interpolation VALUE : Set interpolation mode\n");
	printf("                             0=None (Default)\n");
    printf("                             1=Simple\n");
	printf("                             2=Gaussian\n");
	printf("                             3=Cubic\n");

	printf("\nNeon Options (default GPU):\n");
	printf("  -enhance       : Enable graphic enhancement\n");

	printf("\nGles Options:\n");
	printf("  -gles_dithering VALUE : Enable/disable dithering [0/1]\n");
    printf("                          (Default disabled)\n");
	printf("  -gles_mask VALUE      : Enable/disable mask detect [0/1]\n");
    printf("                          (Default disabled)\n");
	printf("  -gles_filtering VALUE : Texture Filtering\n");
	printf("                          0=None (Default)\n");
	printf("                          1=Standard\n");
	printf("                          2=Extended\n");
	printf("                          3=Standard-sprites\n");
	printf("                          4=Extended-sprites\n");
	printf("                          5=Standard+sprites\n");
	printf("                          6=Extended+sprites\n");
	printf("  -gles_fbtex VALUE     : Framebuffer Textures\n");
	printf("                          0=Emulated VRam (Default)\n");
	printf("                          1=Black\n");
	printf("                          2=Card\n");
	printf("                          3=Card+soft\n");
	printf("  -gles_vram VALUE      : Texture RAM size in MB [4-128]\n");
    printf("                          (Default 64)\n");
    printf("  -gles_fastmdec VALUE  : Enable/disable Fast Mdec [0/1]\n");
    printf("                          (Default disabled)\n");
    printf("  -gles_advblend VALUE  : Enable/disable Adv. Blend [0/1]\n");
    printf("                          (Default disabled)\n");
    printf("  -gles_opaque VALUE    : Enable/disable Opaque Pass [0/1]\n");
    printf("                          (Default disabled)\n");
}

int main(int argc, char **argv)
{
	if (argc == 1 || (argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-help") || !strcmp(argv[1], "-h")))) {
		PrintHelp();
		return 0;
	}

	emu_core_preinit();
	ChangeWorkingDirectory("c");
	char file[MAXPATHLEN] = "";
	char path[MAXPATHLEN];
	const char *cdfile = NULL;
	int loadst = 0;
	int i;
	int getst = -1;
	int discNumber = 0;

	g_menuscreen_w = 800;
	g_menuscreen_h = 480;

	strcpy(Config.Gpu, "builtin_gpu");
	strcpy(Config.Spu, "builtin_spu");
	strcpy(Config.BiosDir, "/home/user/MyDocs");
	strcpy(Config.PluginsDir, "/opt/maemo/usr/games/plugins");
	snprintf(Config.PatchesDir, sizeof(Config.PatchesDir), "/opt/maemo/usr/games" PATCHES_DIR);
	Config.PsxAuto = 1;
	pl_rearmed_cbs.frameskip = -1;
	strcpy(Config.Bios, "HLE");
	spu_config.iUseReverb = 1;
	spu_config.iUseInterpolation = 1;
	in_type1 = PSE_PAD_TYPE_STANDARD;
	in_type2 = PSE_PAD_TYPE_STANDARD;

	accelOptions.sens     = 150;
	accelOptions.y_def	  = 500;
	accelOptions.maxValue = 500.0;

	// read command line options
	for (i = 1; i < argc; i++) {
		     if (!strcmp(argv[i], "-psxout")) Config.PsxOut = 1;
		else if (!strcmp(argv[i], "-load")) loadst = atol(argv[++i]);
		else if (!strcmp(argv[i], "-cdfile")) {
			char isofilename[MAXPATHLEN];
			if (i+1 >= argc) break;
			strncpy(isofilename, argv[++i], MAXPATHLEN);
			if (isofilename[0] != '/') {
				getcwd(path, MAXPATHLEN);
				if (strlen(path) + strlen(isofilename) + 1 < MAXPATHLEN) {
					strcat(path, "/");
					strcat(path, isofilename);
					strcpy(isofilename, path);
				} else
					isofilename[0] = 0;
			}
			cdfile = isofilename;
		}
		else if (!strcmp(argv[i],"-frameskip")) {
			int tv_reg = atol(argv[++i]);
			if (tv_reg < -1)
				pl_rearmed_cbs.frameskip = -1;
			else
				pl_rearmed_cbs.frameskip = tv_reg;
		}
		else if (!strcmp(argv[i],"-region")) {
			int psx_reg = atol(argv[++i]);
			if (psx_reg == 0 || psx_reg == 1){
				Config.PsxAuto = 0;
				Config.PsxType = psx_reg;
			}
		}

		else if (!strcmp(argv[i],"-get_sstatename")) getst = atol(argv[++i]);

		else if (!strcmp(argv[i], "-fullscreen"))	        g_maemo_opts |= 2;
		else if (!strcmp(argv[i], "-accel"))				g_maemo_opts |= 4;
		else if (!strcmp(argv[i], "-nosound"))		        strcpy(Config.Spu, "spunull.so");
		else if (!strcmp(argv[i], "-bdir"))			sprintf(Config.BiosDir, "%s", argv[++i]);
		else if (!strcmp(argv[i], "-pdir"))			        sprintf(Config.PluginsDir, "%s", argv[++i]);
		else if (!strcmp(argv[i], "-bios"))			sprintf(Config.Bios, "%s", argv[++i]);
		else if (!strcmp(argv[i], "-gles"))			        { strcpy(Config.Gpu, "gpu_gles.so"); g_maemo_opts |= 8 ;}
		else if (!strcmp(argv[i], "-oldgpu"))		        strcpy(Config.Gpu, "gpu_peops.so");
		else if (!strcmp(argv[i], "-unai"))		            strcpy(Config.Gpu, "gpu_unai.so");
		else if (!strcmp(argv[i], "-cdda"))		Config.Cdda = 1;
		else if (!strcmp(argv[i], "-xa"))		Config.Xa = 1;
		else if (!strcmp(argv[i], "-fps")) 		            g_opts |=OPT_SHOWFPS;
		else if (!strcmp(argv[i], "-cpu")) 		            g_opts |=OPT_SHOWCPU;
		else if (!strcmp(argv[i], "-spu")) 		            g_opts |=OPT_SHOWSPU;
		else if (!strcmp(argv[i], "-nofl")) 		        g_opts |=OPT_NO_FRAMELIM;
		else if (!strcmp(argv[i], "-mcd1")) 	            sprintf(Config.Mcd1, "%s", argv[++i]);
		else if (!strcmp(argv[i], "-mcd2")) 	            sprintf(Config.Mcd2, "%s", argv[++i]);

		else if (!strcmp(argv[i], "-cpuclock")) 	        Config.cycle_multiplier = 10000 / atol(argv[++i]);
		else if (!strcmp(argv[i], "-guncon")) 	            in_type1 = PSE_PAD_TYPE_GUNCON;
		else if (!strcmp(argv[i], "-gunnotrigger")) 		g_opts |= OPT_TSGUN_NOTRIGGER;
		else if (!strcmp(argv[i], "-analog")) 	            in_type1 = PSE_PAD_TYPE_ANALOGPAD;
		else if (!strcmp(argv[i], "-vibration")) 	        { in_type1 = PSE_PAD_TYPE_ANALOGPAD; in_enable_vibration = 1; }
		else if (!strcmp(argv[i], "-sens")) 				accelOptions.sens = atol(argv[++i]);
		else if (!strcmp(argv[i], "-ydef")) 				accelOptions.y_def = atol(argv[++i]);
		else if (!strcmp(argv[i], "-max")) 				    accelOptions.maxValue = atol(argv[++i]);
		else if (!strcmp(argv[i], "-displayon")) 		    bKeepDisplayOn = TRUE;
		else if (!strcmp(argv[i], "-keys")) 				sprintf(keys_config_file, "%s", argv[++i]);
		else if (!strcmp(argv[i], "-autosave")) 		    bAutosaveOnExit = TRUE;
		else if (!strcmp(argv[i], "-disc")) 		        discNumber = atol(argv[++i]);
		else if (!strcmp(argv[i], "-corners")){
			int j = 0;
			i++;
			char num[2];
			for (j=0; j<strlen(argv[i]); j++){
				strncpy(num, argv[i] + j, 1);
				cornerActions[j] = atoi(num);
			}
	}

		else if (!strcmp(argv[i], "-spu_reverb"))		spu_config.iUseReverb = atol(argv[++i]);
		else if (!strcmp(argv[i], "-spu_interpolation")) 	spu_config.iUseInterpolation = atol(argv[++i]);

		else if (!strcmp(argv[i], "-enhance")) 			pl_rearmed_cbs.gpu_neon.enhancement_enable = 1;
		else if (!strcmp(argv[i], "-enhancehack")) 		pl_rearmed_cbs.gpu_neon.enhancement_no_main = 1;

		else if (!strcmp(argv[i], "-gles_dithering")) 	pl_rearmed_cbs.gpu_peopsgl.bDrawDither = atol(argv[++i]);
		else if (!strcmp(argv[i], "-gles_mask")) 	    pl_rearmed_cbs.gpu_peopsgl.iUseMask = atol(argv[++i]);
		else if (!strcmp(argv[i], "-gles_filtering")) 	pl_rearmed_cbs.gpu_peopsgl.iFilterType = atol(argv[++i]);
		else if (!strcmp(argv[i], "-gles_fbtex")) 	    pl_rearmed_cbs.gpu_peopsgl.iFrameTexType = atol(argv[++i]);
		else if (!strcmp(argv[i], "-gles_vram")) 	    pl_rearmed_cbs.gpu_peopsgl.iVRamSize = atol(argv[++i]);
		else if (!strcmp(argv[i], "-gles_fastmdec")) 	pl_rearmed_cbs.gpu_peopsgl.bUseFastMdec = atol(argv[++i]);
        else if (!strcmp(argv[i], "-gles_advblend")) 	pl_rearmed_cbs.gpu_peopsgl.bAdvancedBlend = atol(argv[++i]);
        else if (!strcmp(argv[i], "-gles_opaque")) 	    pl_rearmed_cbs.gpu_peopsgl.bOpaquePass = atol(argv[++i]);

		else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			return 1;
		}
	}
	
	pl_init();
	if (emu_core_init() == -1)
		return 1;
	
	if (cdfile) {
		set_cd_image(cdfile);
		strcpy(file_name, strrchr(cdfile,'/'));
	}

	if (LoadPlugins() == -1) {
		SysMessage("Failed loading plugins!");
		return 1;
	}

	if (discNumber > 0)
		cdrIsoMultidiskSelect = discNumber - 1;

	if (OpenPlugins() == -1) {
		return 1;
	}
	plugin_call_rearmed_cbs();

	CheckCdrom();

	if (getst >= 0){
		char fname[MAXPATHLEN];

		get_state_filename(fname, sizeof(fname), getst);
		printf("SAVESTATE: %s\n", fname);
		if (cdrIsoMultidiskCount > 1){
			int i = 0;
			for (i=1; i<cdrIsoMultidiskCount; i++){
				cdrIsoMultidiskSelect = i;
				CdromId[0] = '\0';
				CdromLabel[0] = '\0';

				CDR_close();
				if (CDR_open() == 0){
					CheckCdrom();
					get_state_filename(fname, sizeof(fname), getst);
					printf("SAVESTATE: %s\n", fname);
				}
			}
		}
		return 0;
	}

	SysReset();

	if (file[0] != '\0') {
		if (Load(file) != -1)
			ready_to_go = 1;
	} else {
		if (cdfile) {
			if (LoadCdrom() == -1) {
				ClosePlugins();
				printf(_("Could not load CD-ROM!\n"));
				return -1;
			}
			emu_on_new_cd(0);
			ready_to_go = 1;
		}
	}

	if (!ready_to_go) {
		printf ("something goes wrong, maybe you forgot -cdfile ? \n");
		return 1;
	}

	if (cdrIsoMultidiskCount > 1)
		printf ("Loaded a multidisc image: %i discs.\n", cdrIsoMultidiskCount);

	// If a state has been specified, then load that
	if (loadst) {
		int ret = emu_load_state(loadst - 1);
		printf("%s state %d\n", ret ? "Failed to load" : "Loaded", loadst);
		state_slot = loadst - 1;
	}

	if (maemo_init(&argc, &argv))
		return 1;

	if (GPU_open != NULL) {
		int ret = GPU_open(&gpuDisp, "PCSX", NULL);
		if (ret){
			fprintf(stderr, "Warning: GPU_open returned %d\n", ret);
			gpuDisp=ret;
		}
	}

	if (Config.HLE)
		printf("Note: running without BIOS, expect compatibility problems\n");

	pl_timing_prepare(Config.PsxType);

	while (1)
	{
		stop = 0;
		emu_action = SACTION_NONE;

		psxCpu->Execute();
		if (emu_action != SACTION_NONE)
			do_emu_action();
	}

	maemo_finish();
	return 0;
}

