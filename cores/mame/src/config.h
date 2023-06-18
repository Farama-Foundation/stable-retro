/***************************************************************************

	config.h

	Wrappers for handling MAME configuration files

***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include "inptport.h"
#include "fileio.h"
#include "sound/mixer.h"



/***************************************************************************

	Constants

***************************************************************************/

#define CONFIG_ERROR_SUCCESS			0	/* success */
#define CONFIG_ERROR_CORRUPT			-1	/* config file is corrupt */
#define CONFIG_ERROR_BADMODE			-2	/* bad open mode */
#define CONFIG_ERROR_BADPOSITION		-3	/* functions called in improper order */



/***************************************************************************

	Type definitions

***************************************************************************/

typedef struct _config_file config_file;



/***************************************************************************

	Prototypes

***************************************************************************/

/* opens a config file for reading */
config_file *config_open(const char *name);

/* opens a config file for writing */
config_file *config_create(const char *name);

/* closes a config file */
void config_close(config_file *file);

/* reads inputports out of a configuration file */
int config_read_ports(config_file *file, struct InputPort *input_ports_default, struct InputPort *input_ports);

/* reads default inputports out of a configuration file */
int config_read_default_ports(config_file *cfg, struct ipd *input_ports_default);

/* reads coin and ticket counters (arrays of length COIN_COUNTERS, except for dispensed_tickets) */
int config_read_coin_and_ticket_counters(config_file *file, unsigned int *coins, unsigned int *lastcoin,
	unsigned int *coinlockedout, unsigned int *dispensed_tickets);

/* reads mixer configuration */
int config_read_mixer_config(config_file *file, struct mixer_config *mixercfg);

/* writes inputports out of a configuration file */
int config_write_ports(config_file *file, const struct InputPort *input_ports_default, const struct InputPort *input_ports);

/* writes default inputports out of a configuration file */
int config_write_default_ports(config_file *cfg, const struct ipd *input_ports_default_backup, const struct ipd *input_ports_default);

/* writes coin and ticket counters (arrays of length COIN_COUNTERS) */
int config_write_coin_and_ticket_counters(config_file *file, const unsigned int *coins, const unsigned int *lastcoin,
	const unsigned int *coinlockedout, unsigned int dispensed_tickets);

/* writes mixer configuration */
int config_write_mixer_config(config_file *file, const struct mixer_config *mixercfg);

#endif /* CONFIG_H */
