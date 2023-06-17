#ifndef EEPROM_H
#define EEPROM_H

struct EEPROM_interface
{
	int address_bits;	/* EEPROM has 2^address_bits cells */
	int data_bits;		/* every cell has this many bits (8 or 16) */
	const char *cmd_read;	/*   read command string, e.g. "0110" */
	const char *cmd_write;	/*  write command string, e.g. "0111" */
	const char *cmd_erase;	/*  erase command string, or 0 if n/a */
	const char *cmd_lock;	/*   lock command string, or 0 if n/a */
	const char *cmd_unlock;	/* unlock command string, or 0 if n/a */
	int enable_multi_read;/* set to 1 to enable multiple values to be read from one read command */
	int reset_delay;	/* number of times EEPROM_read_bit() should return 0 after a reset, */
						/* before starting to return 1. */
};


void EEPROM_init(struct EEPROM_interface *interface);

void EEPROM_write_bit(int bit);
int EEPROM_read_bit(void);
void EEPROM_set_cs_line(int state);
void EEPROM_set_clock_line(int state);

void EEPROM_load(mame_file *file);
void EEPROM_save(mame_file *file);

void EEPROM_set_data(UINT8 *data, int length);
UINT8 * EEPROM_get_data_pointer(int * length);

/* 93C46 */
extern struct EEPROM_interface eeprom_interface_93C46;
void nvram_handler_93C46(mame_file *file,int read_or_write);

#endif
