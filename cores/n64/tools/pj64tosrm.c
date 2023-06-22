/* pj64tosrm
 * Combine Project64's save files (*.eep, *.mpk, *.fla, *.sra) into a
 * libretro-mupen64 save file (*.srm).
 *
 * KNOWN BUGS:
 *     - Some NRage Input Plugin saves may not work regardless of the type.
 *     - Some very old Project64 saves may not work regardless of the type/plugin used.
 */

#if defined(_WIN32) || defined(_WIN32)
# include <windows.h>
# include <io.h>
#else
# include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _MSC_VER
# define strcasecmp _stricmp
# define access _access
#endif

/* from libretro's mupen64+ source (save_memory_data) */
static struct
{
	uint8_t eeprom[0x800];
	uint8_t mempack[4][0x8000];
	uint8_t sram[0x8000];
	uint8_t flashram[0x20000];
} srm;

static struct
{
	char srm[513];
	char eep[513];
	char mpk[513];
	char sra[513];
	char fla[513];
} files;

static uint8_t mempack_init[] = {
	0x81,0x01,0x02,0x03, 0x04,0x05,0x06,0x07, 0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17, 0x18,0x19,0x1a,0x1b, 0x1c,0x1d,0x1e,0x1f,
	0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
	0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x71,0x00,0x03, 0x00,0x03,0x00,0x03, 0x00,0x03,0x00,0x03, 0x00,0x03,0x00,0x03
};

static void die(const char *msg)
{
	puts(msg);
	exit(-1);
}

static int overwrite_confirm(const char *msg)
{
	char buf[1024];
	sprintf(buf, "Do you want to overwrite %s?", msg);

#if defined(_WIN32) || defined(_WIN32)
	int ret = MessageBoxA(NULL, buf, "Confirm action", MB_ICONWARNING | MB_DEFBUTTON2 | MB_YESNO);
	return ret == IDYES;
#else
	printf("%s (y/n):", buf);
	return getc(stdin) == 'y';
#endif
}

static int is_empty_mem(uint8_t *mem, size_t size)
{
	int i;
	if (mem == srm.eeprom || mem == srm.sram) {
		for (i = size-1; i >= 0; --i) {
			if (mem[i] != 0)
				return 0;
		}
	} else if (mem == srm.flashram) {
		for (i = size-1; i >= 0; --i) {
			if (mem[i] != 0xff)
				return 0;
		}
	} else if (mem == (uint8_t*)srm.mempack) {
		for (i = 0; i < 4; i++) {
			if (memcpy(mempack_init, srm.mempack[i], 272) != 0)
				return 0;
		}
	} else
		die("unsupported memory");

	return 1;
}

static void read_mem(const char *filename, void *dest, size_t size)
{
	FILE *fp = fopen(filename, "rb");

	if (!fp) {
		perror(filename);
		exit(EXIT_FAILURE);
	}

	fread(dest, size, 1, fp);

	if (ferror(fp)) {
		perror(filename);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	fclose(fp);
}

static void write_mem(const char *filename, void *source, size_t size)
{
	FILE *fp = fopen(filename, "wb");

	if (!fp) {
		perror(filename);
		exit(EXIT_FAILURE);
	}

	fwrite(source, size, 1, fp);

	if (ferror(fp)) {
		perror(filename);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	fclose(fp);
}

/* from libretro mupen64+ format_saved_memory()*/
static void format_srm(void)
{
   int i,j;

   memset(srm.sram, 0, sizeof(srm.sram));
   memset(srm.eeprom, 0, sizeof(srm.eeprom));
   memset(srm.flashram, 0xff, sizeof(srm.flashram));

   for (i=0; i<4; i++) {
      for (j=0; j<0x8000; j+=2) {
         srm.mempack[i][j] = 0;
         srm.mempack[i][j+1] = 0x03;
      }
      memcpy(srm.mempack[i], mempack_init, 272);
   }
}

static int ext_matches(const char *ext, const char *filename)
{
	const char *suffix = strrchr(filename, '.');

	return suffix ? strcasecmp(ext, suffix+1) == 0 : 0;
}

static void change_extension(char *dst, const char *src, const char *newext)
{
	const char *ext = strrchr(src, '.');

	if (ext)
		strncpy(dst, src, ext - src);
	else
		strcpy(dst, src);

	strcat(dst, newext);
}

int main(int argc, char *argv[])
{
	FILE *fp;
	int i;

	format_srm();
	memset(&files, 0, sizeof(files));

	if (argc < 2) {
Usage:
		printf("Usage:\n");
		printf("  To create a srm file: %s [file.eep] [file.mpk] [file.sra] [file.fla]\n", argv[0]);
		printf("  To extract a srm file: %s <file.srm>\n\n", argv[0]);
		printf("Output files will be placed in the same directory of the input files.\n");

		exit(EXIT_FAILURE);
	}

	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") || !strcmp(argv[i], "/?"))
			goto Usage;

		if (ext_matches("srm", argv[i]))
			strncpy(files.srm, argv[i], 512);
		if (ext_matches("eep", argv[i]))
			strncpy(files.eep, argv[i], 512);
		if (ext_matches("mpk", argv[i]))
			strncpy(files.mpk, argv[i], 512);
		if (ext_matches("sra", argv[i]))
			strncpy(files.sra, argv[i], 512);
		if (ext_matches("fla", argv[i]))
			strncpy(files.fla, argv[i], 512);
	}

	if (*files.srm) {
		puts("Running in srm extraction mode");

		read_mem(files.srm, &srm, sizeof(srm));

		if (!is_empty_mem(srm.eeprom, sizeof(srm.eeprom)))
			change_extension(files.eep, files.srm, ".eep");

		if (!is_empty_mem((uint8_t*)srm.mempack, sizeof(srm.mempack)))
			change_extension(files.mpk, files.srm, ".mpk");

		if (!is_empty_mem(srm.sram, sizeof(srm.sram)))
			change_extension(files.sra, files.srm, ".sra");

		if (!is_empty_mem(srm.flashram, sizeof(srm.flashram)))
			change_extension(files.fla, files.srm, ".fla");

		int over = ((*files.eep && access(files.eep, 0) != -1) || (*files.mpk && access(files.mpk, 0) != -1) ||
				(*files.sra && access(files.sra, 0) != -1) || (*files.fla && access(files.fla, 0) != -1));

		if (over && !overwrite_confirm("existing saves in the source directory")) {
			puts("existing saves unmodified.");
			exit(EXIT_SUCCESS);
		}

		if (*files.eep)
			write_mem(files.eep, srm.eeprom, sizeof(srm.eeprom));

		if (*files.mpk)
			write_mem(files.mpk, srm.mempack, sizeof(srm.mempack));

		if (*files.sra)
			write_mem(files.sra, srm.sram, sizeof(srm.sram));

		if (*files.fla)
			write_mem(files.fla, srm.flashram, sizeof(srm.flashram));

	} else {
		puts("srm file unspecified, running in srm creation mode");

		if (!files.eep && !files.mpk && !files.sra && !files.fla)
			die("no input files.");

		/* pick the first filename */
		if (!*files.srm) {
			if (*files.eep)
				change_extension(files.srm, files.eep, ".srm");
			else if (*files.mpk)
				change_extension(files.srm, files.mpk, ".srm");
			else if (*files.sra)
				change_extension(files.srm, files.sra, ".srm");
			else if (*files.fla)
				change_extension(files.srm, files.fla, ".srm");

			if (strlen(files.srm) + 3 > 512)
				die("path too long");
		}

		if (*files.eep)
			read_mem(files.eep, srm.eeprom, sizeof(srm.eeprom));
		if (*files.mpk)
			read_mem(files.mpk, srm.mempack, sizeof(srm.mempack));
		if (*files.sra)
			read_mem(files.sra, srm.sram, sizeof(srm.sram));
		if (*files.fla)
			read_mem(files.fla, srm.flashram, sizeof(srm.flashram));

		int over = access(files.srm, 0 /*F_OK*/) != -1;

		if (over && !overwrite_confirm(files.srm)) {
			printf("%s unmodified.\n", files.srm);
			exit(EXIT_SUCCESS);
		}

		printf("Writting srm data to %s...\n", files.srm);

		fp = fopen(files.srm, "wb");

		if (!fp) {
			perror(files.srm);
			exit(EXIT_FAILURE);
		}

		fwrite(srm.eeprom, sizeof(srm.eeprom), 1, fp);
		fwrite(srm.mempack, sizeof(srm.mempack), 1, fp);
		fwrite(srm.sram, sizeof(srm.sram), 1, fp);
		fwrite(srm.flashram, sizeof(srm.flashram), 1, fp);
		fclose(fp);
	}

	return EXIT_SUCCESS;
}

