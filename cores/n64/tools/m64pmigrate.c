/* m64migrate
 * Migrate .srm files from the old layout to the new one.
 *
 * The new layout was introduced in 2015/02/11.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

struct old_layout {
	unsigned char eeprom[0x200];
	unsigned char mempack[4][0x8000];
	unsigned char sram[0x8000];
	unsigned char flashram[0x20000];
	unsigned char eeprom2[0x600];
};

int main(int argc, char *argv[]) {

	int i;
	char backup_path[4096];
	struct old_layout save;
	FILE *fp, *fp2;

	if (argc < 2) {
		printf("usage: %s <file1.srm> [file2.srm] ... [file.srm]", argv[0]);
		exit(EXIT_FAILURE);
	}

	for (i = 1; i < argc; ++i) {
		snprintf(backup_path, sizeof(backup_path), "%s.bak", argv[i]);
		printf("Processing '%s'...\n", argv[i]);
		if (access(argv[i], R_OK|W_OK) != 0) {
			fprintf(stderr, "Unable to read or write '%s': %s\n", argv[i], strerror(errno));
			continue;
		}

		if (access(backup_path, F_OK) == 0) {
			printf("Backup file '%s' found, do you want to overwrite (y/n)?  ", backup_path);
			if (fgetc(stdin) != 'y') {
				puts("  skipping.");
				continue;
			}

			puts("  overwriting.");
		}

		memset(&save, 0, sizeof(save));

		if (!(fp = fopen(argv[i], "r+b"))) {
			fprintf(stderr, "Failed to open '%s', skipping: %s\n", argv[i], strerror(errno));
			continue;
		}

		fread(&save, 1, sizeof(save), fp);

		if (!(fp2 = fopen(backup_path, "wb"))) {
			fprintf(stderr, "Failed to open '%s', skipping: %s\n", backup_path, strerror(errno));
			fclose(fp);
			continue;
		}

		if (fwrite(&save, 1, sizeof(save), fp2) < sizeof(save)) {
			fprintf(stderr, "Failed to backup '%s', skipping: %s\n", argv[i], strerror(errno));
			unlink(backup_path);
			fclose(fp);
			fclose(fp2);
			continue;
		}

		fclose(fp2);

		rewind(fp);

		fwrite(save.eeprom, 1, sizeof(save.eeprom), fp);
		fwrite(save.eeprom2, 1, sizeof(save.eeprom2), fp);
		fwrite(save.mempack, 1, sizeof(save.mempack), fp);
		fwrite(save.sram, 1, sizeof(save.sram), fp);
		fwrite(save.flashram, 1, sizeof(save.flashram), fp);

		fclose(fp);

		continue;
	}

	return 0;
}
