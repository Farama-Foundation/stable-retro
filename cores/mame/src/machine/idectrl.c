/***************************************************************************

	Generic (PC-style) IDE controller implementation

***************************************************************************/

#include "idectrl.h"
#include "state.h"


/*************************************
 *
 *	Debugging
 *
 *************************************/

#define PRINTF_IDE_COMMANDS			0
#define PRINTF_IDE_PASSWORD			0

#if (VERBOSE && PRINTF_IDE_COMMANDS)
#define LOGPRINT(x)	logerror x; printf x
#elif PRINTF_IDE_COMMANDS
#define LOGPRINT(x)	printf x
#else
#define LOGPRINT(X)
#endif



/*************************************
 *
 *	Constants
 *
 *************************************/

#define IDE_DISK_SECTOR_SIZE			512

#define MINIMUM_COMMAND_TIME			(TIME_IN_USEC(10))

#define TIME_PER_SECTOR					(TIME_IN_USEC(100))
#define TIME_PER_ROTATION				(TIME_IN_HZ(5400/60))
#define TIME_SECURITY_ERROR				(TIME_IN_MSEC(1000))

#define TIME_SEEK_MULTISECTOR			(TIME_IN_MSEC(13))
#define TIME_NO_SEEK_MULTISECTOR		(TIME_IN_USEC(16.3))

#define IDE_STATUS_ERROR				0x01
#define IDE_STATUS_HIT_INDEX			0x02
#define IDE_STATUS_BUFFER_READY			0x08
#define IDE_STATUS_SEEK_COMPLETE		0x10
#define IDE_STATUS_DRIVE_READY			0x40
#define IDE_STATUS_BUSY					0x80

#define IDE_CONFIG_REGISTERS			0x10

#define IDE_ADDR_CONFIG_UNK				0x034
#define IDE_ADDR_CONFIG_REGISTER		0x038
#define IDE_ADDR_CONFIG_DATA			0x03c

#define IDE_ADDR_DATA					0x1f0
#define IDE_ADDR_ERROR					0x1f1
#define IDE_ADDR_SECTOR_COUNT			0x1f2
#define IDE_ADDR_SECTOR_NUMBER			0x1f3
#define IDE_ADDR_CYLINDER_LSB			0x1f4
#define IDE_ADDR_CYLINDER_MSB			0x1f5
#define IDE_ADDR_HEAD_NUMBER			0x1f6
#define IDE_ADDR_STATUS_COMMAND			0x1f7

#define IDE_ADDR_STATUS_CONTROL			0x3f6

#define IDE_COMMAND_READ_MULTIPLE		0x20
#define IDE_COMMAND_READ_MULTIPLE_ONCE	0x21
#define IDE_COMMAND_WRITE_MULTIPLE		0x30
#define IDE_COMMAND_SET_CONFIG			0x91
#define IDE_COMMAND_READ_MULTIPLE_BLOCK	0xc4
#define IDE_COMMAND_WRITE_MULTIPLE_BLOCK 0xc5
#define IDE_COMMAND_SET_BLOCK_COUNT		0xc6
#define IDE_COMMAND_READ_DMA			0xc8
#define IDE_COMMAND_WRITE_DMA			0xca
#define IDE_COMMAND_GET_INFO			0xec
#define IDE_COMMAND_SET_FEATURES		0xef
#define IDE_COMMAND_SECURITY_UNLOCK		0xf2
#define IDE_COMMAND_UNKNOWN_F9			0xf9

#define IDE_ERROR_NONE					0x00
#define IDE_ERROR_DEFAULT				0x01
#define IDE_ERROR_UNKNOWN_COMMAND		0x04
#define IDE_ERROR_BAD_LOCATION			0x10
#define IDE_ERROR_BAD_SECTOR			0x80

#define IDE_BUSMASTER_STATUS_ACTIVE		0x01
#define IDE_BUSMASTER_STATUS_ERROR		0x02
#define IDE_BUSMASTER_STATUS_IRQ		0x04



/*************************************
 *
 *	Type definitions
 *
 *************************************/

struct ide_state
{
	UINT8	adapter_control;
	UINT8	status;
	UINT8	error;
	UINT8	command;
	UINT8	interrupt_pending;
	UINT8	precomp_offset;

	UINT8	buffer[IDE_DISK_SECTOR_SIZE];
	UINT8	features[IDE_DISK_SECTOR_SIZE];
	UINT16	buffer_offset;
	UINT16	sector_count;

	UINT16	block_count;
	UINT16	sectors_until_int;
	
	UINT8	dma_active;
	UINT8	dma_cpu;
	UINT8	dma_address_xor;
	UINT8	dma_last_buffer;
	offs_t	dma_address;
	offs_t	dma_descriptor;
	UINT32	dma_bytes_left;
	
	UINT8	bus_master_command;
	UINT8	bus_master_status;
	UINT32	bus_master_descriptor;

	UINT16	cur_cylinder;
	UINT8	cur_sector;
	UINT8	cur_head;
	UINT8	cur_head_reg;

	UINT32	cur_lba;

	UINT16	num_cylinders;
	UINT8	num_sectors;
	UINT8	num_heads;

	UINT8	config_unknown;
	UINT8	config_register[IDE_CONFIG_REGISTERS];
	UINT8	config_register_num;

	struct ide_interface *intf;
	struct hard_disk_file *	disk;
	void *	last_status_timer;
	void *	reset_timer;

	int	master_password_enable;
	int	user_password_enable;
	UINT8 *	master_password;
	UINT8 *	user_password;
};



/*************************************
 *
 *	Local variables
 *
 *************************************/

static struct ide_state idestate[MAX_IDE_CONTROLLERS];



/*************************************
 *
 *	Prototypes
 *
 *************************************/

static void reset_callback(int param);

static void ide_build_features(struct ide_state *ide);

static void continue_read(struct ide_state *ide);
static void read_sector_done(int which);
static void read_first_sector(struct ide_state *ide);
static void read_next_sector(struct ide_state *ide);

static UINT32 ide_controller_read(struct ide_state *ide, offs_t offset, int size);
static void ide_controller_write(struct ide_state *ide, offs_t offset, int size, UINT32 data);



/*************************************
 *
 *	Interrupts
 *
 *************************************/

static INLINE void signal_interrupt(struct ide_state *ide)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "IDE interrupt assert\n");

	/* signal an interrupt */
	if (ide->intf->interrupt)
		(*ide->intf->interrupt)(ASSERT_LINE);
	ide->interrupt_pending = 1;
	ide->bus_master_status |= IDE_BUSMASTER_STATUS_IRQ;
}


static INLINE void clear_interrupt(struct ide_state *ide)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "IDE interrupt clear\n");

	/* clear an interrupt */
	if (ide->intf->interrupt)
		(*ide->intf->interrupt)(CLEAR_LINE);
	ide->interrupt_pending = 0;
}



/*************************************
 *
 *	Delayed interrupt handling
 *
 *************************************/

static void delayed_interrupt(int which)
{
	struct ide_state *ide = &idestate[which];
	ide->status &= ~IDE_STATUS_BUSY;
	signal_interrupt(ide);
}


static void delayed_interrupt_buffer_ready(int which)
{
	struct ide_state *ide = &idestate[which];
	ide->status &= ~IDE_STATUS_BUSY;
	ide->status |= IDE_STATUS_BUFFER_READY;
	signal_interrupt(ide);
}


static INLINE void signal_delayed_interrupt(struct ide_state *ide, double time, int buffer_ready)
{
	/* clear buffer ready and set the busy flag */
	ide->status &= ~IDE_STATUS_BUFFER_READY;
	ide->status |= IDE_STATUS_BUSY;
	
	/* set a timer */
	if (buffer_ready)
		timer_set(time, ide - idestate, delayed_interrupt_buffer_ready);
	else
		timer_set(time, ide - idestate, delayed_interrupt);
}



/*************************************
 *
 *	Initialization & reset
 *
 *************************************/

int ide_controller_init_custom(int which, struct ide_interface *intf, struct chd_file *diskhandle)
{
	struct ide_state *ide = &idestate[which];
	const struct hard_disk_info *hdinfo;

	/* NULL interface is immediate failure */
	if (!intf)
		return 1;

	/* reset the IDE state */
	memset(ide, 0, sizeof(*ide));
	ide->intf = intf;

	/* set MAME harddisk handle */
	ide->disk = hard_disk_open(diskhandle);

	/* get and copy the geometry */
	if (ide->disk)
	{
		hdinfo = hard_disk_get_info(ide->disk);
		ide->num_cylinders = hdinfo->cylinders;
		ide->num_sectors = hdinfo->sectors;
		ide->num_heads = hdinfo->heads;
		if (hdinfo->sectorbytes != IDE_DISK_SECTOR_SIZE)
			/* wrong sector len */
			return 1;
#if PRINTF_IDE_COMMANDS
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CHS: %d %d %d\n", ide->num_cylinders, ide->num_heads, ide->num_sectors);
#endif
	}

	/* build the features page */
	ide_build_features(ide);

	/* create a timer for timing status */
	ide->last_status_timer = timer_alloc(NULL);
	ide->reset_timer = timer_alloc(reset_callback);

	/* register ide status */
	state_save_register_UINT8 ("ide", which, "adapter_control",        &ide->adapter_control,       1);
	state_save_register_UINT8 ("ide", which, "status",                 &ide->status,                1);
	state_save_register_UINT8 ("ide", which, "error",                  &ide->error,                 1);
	state_save_register_UINT8 ("ide", which, "command",                &ide->command,               1);
	state_save_register_UINT8 ("ide", which, "interrupt_pending",      &ide->interrupt_pending,     1);
	state_save_register_UINT8 ("ide", which, "precomp_offset",         &ide->precomp_offset,        1);

	state_save_register_UINT8 ("ide", which, "buffer",                 ide->buffer,                 IDE_DISK_SECTOR_SIZE);
	state_save_register_UINT8 ("ide", which, "features",               ide->features,               IDE_DISK_SECTOR_SIZE);
	state_save_register_UINT16("ide", which, "buffer_offset",          &ide->buffer_offset,         1);
	state_save_register_UINT16("ide", which, "sector_count",           &ide->sector_count,          1);

	state_save_register_UINT16("ide", which, "block_count",            &ide->block_count,           1);
	state_save_register_UINT16("ide", which, "sectors_until_int",      &ide->sectors_until_int,     1);

	state_save_register_UINT8 ("ide", which, "dma_active",             &ide->dma_active,            1);
	state_save_register_UINT8 ("ide", which, "dma_cpu",                &ide->dma_cpu,               1);
	state_save_register_UINT8 ("ide", which, "dma_address_xor",        &ide->dma_address_xor,       1);
	state_save_register_UINT8 ("ide", which, "dma_last_buffer",        &ide->dma_last_buffer,       1);
	state_save_register_UINT32("ide", which, "dma_address",            &ide->dma_address,           1);
	state_save_register_UINT32("ide", which, "dma_descriptor",         &ide->dma_descriptor,        1);
	state_save_register_UINT32("ide", which, "dma_bytes_left",         &ide->dma_bytes_left,        1);

	state_save_register_UINT8 ("ide", which, "bus_master_command",     &ide->bus_master_command,    1);
	state_save_register_UINT8 ("ide", which, "bus_master_status",      &ide->bus_master_status,     1);
	state_save_register_UINT32("ide", which, "bus_master_descriptor",  &ide->bus_master_descriptor, 1);

	state_save_register_UINT16("ide", which, "cur_cylinder",           &ide->cur_cylinder,          1);
	state_save_register_UINT8 ("ide", which, "cur_sector",             &ide->cur_sector,            1);
	state_save_register_UINT8 ("ide", which, "cur_head",               &ide->cur_head,              1);
	state_save_register_UINT8 ("ide", which, "cur_head_reg",           &ide->cur_head_reg,          1);

	state_save_register_UINT32("ide", which, "cur_lba",                &ide->cur_lba,               1);

	state_save_register_UINT16("ide", which, "num_cylinders",          &ide->num_cylinders,         1);
	state_save_register_UINT8 ("ide", which, "num_sectors",            &ide->num_sectors,           1);
	state_save_register_UINT8 ("ide", which, "num_heads",              &ide->num_heads,             1);

	state_save_register_UINT8 ("ide", which, "config_unknown",         &ide->config_unknown,        1);
	state_save_register_UINT8 ("ide", which, "config_register",        ide->config_register,        IDE_CONFIG_REGISTERS);
	state_save_register_UINT8 ("ide", which, "config_register_num",    &ide->config_register_num,   1);

	state_save_register_int   ("ide", which, "master_password_enable", &ide->master_password_enable);
	state_save_register_int   ("ide", which, "user_password_enable",   &ide->user_password_enable);

	return 0;
}

int ide_controller_init(int which, struct ide_interface *intf)
{
	/* we only support one hard disk right now; get a handle to it */
	return ide_controller_init_custom(which, intf, get_disk_handle(0));
}


void ide_controller_reset(int which)
{
	struct ide_state *ide = &idestate[which];

	log_cb(RETRO_LOG_DEBUG, LOGPRE "IDE controller reset performed\n");

	/* reset the drive state */
	ide->status = IDE_STATUS_DRIVE_READY | IDE_STATUS_SEEK_COMPLETE;
	ide->error = IDE_ERROR_DEFAULT;
	ide->buffer_offset = 0;
	ide->master_password_enable = (ide->master_password != NULL);
	ide->user_password_enable = (ide->user_password != NULL);
	clear_interrupt(ide);
}


UINT8 *ide_get_features(int which)
{
	struct ide_state *ide = &idestate[which];
	return ide->features;
}


void ide_set_master_password(int which, UINT8 *password)
{
	struct ide_state *ide = &idestate[which];

	ide->master_password = password;
	ide->master_password_enable = (ide->master_password != NULL);
}


void ide_set_user_password(int which, UINT8 *password)
{
	struct ide_state *ide = &idestate[which];

	ide->user_password = password;
	ide->user_password_enable = (ide->user_password != NULL);
}


static void reset_callback(int param)
{
	ide_controller_reset(param);
}



/*************************************
 *
 *	Convert offset/mem_mask to offset
 *	and size
 *
 *************************************/

static INLINE int convert_to_offset_and_size32(offs_t *offset, data32_t mem_mask)
{
	int size = 4;

	/* determine which real offset */
	if (mem_mask & 0x000000ff)
	{
		(*offset)++, size = 3;
		if (mem_mask & 0x0000ff00)
		{
			(*offset)++, size = 2;
			if (mem_mask & 0x00ff0000)
				(*offset)++, size = 1;
		}
	}

	/* determine the real size */
	if (!(mem_mask & 0xff000000))
		return size;
	size--;
	if (!(mem_mask & 0x00ff0000))
		return size;
	size--;
	if (!(mem_mask & 0x0000ff00))
		return size;
	size--;
	return size;
}

static INLINE int convert_to_offset_and_size16(offs_t *offset, data32_t mem_mask)
{
	int size = 2;

	/* determine which real offset */
	if (mem_mask & 0x000000ff)
		(*offset)++, size = 1;

	if (!(mem_mask & 0x0000ff00))
		return size;
	size--;
	return size;
}



/*************************************
 *
 *	Compute the LBA address
 *
 *************************************/

static INLINE UINT32 lba_address(struct ide_state *ide)
{
	/* LBA direct? */
	if (ide->cur_head_reg & 0x40)
		return ide->cur_sector + ide->cur_cylinder * 256 + ide->cur_head * 16777216;

	/* standard CHS */
	else
		return (ide->cur_cylinder * ide->num_heads + ide->cur_head) * ide->num_sectors + ide->cur_sector - 1;
}



/*************************************
 *
 *	Advance to the next sector
 *
 *************************************/

static INLINE void next_sector(struct ide_state *ide)
{
	/* LBA direct? */
	if (ide->cur_head_reg & 0x40)
	{
		ide->cur_sector++;
		if (ide->cur_sector == 0)
		{
			ide->cur_cylinder++;
			if (ide->cur_cylinder == 0)
				ide->cur_head++;
		}
	}

	/* standard CHS */
	else
	{
		/* sectors are 1-based */
		ide->cur_sector++;
		if (ide->cur_sector > ide->num_sectors)
		{
			/* heads are 0 based */
			ide->cur_sector = 1;
			ide->cur_head++;
			if (ide->cur_head >= ide->num_heads)
			{
				ide->cur_head = 0;
				ide->cur_cylinder++;
			}
		}
	}

	ide->cur_lba = lba_address(ide);
}



/*************************************
 *
 *	Build a features page
 *
 *************************************/

static void swap_strncpy(UINT8 *dst, const char *src, int field_size_in_words)
{
	int i;

	for (i = 0; i < field_size_in_words * 2 && src[i]; i++)
		dst[i ^ 1] = src[i];
	for ( ; i < field_size_in_words; i++)
		dst[i ^ 1] = ' ';
}


static void ide_build_features(struct ide_state *ide)
{
	int total_sectors = ide->num_cylinders * ide->num_heads * ide->num_sectors;
	int sectors_per_track = ide->num_heads * ide->num_sectors;

	memset(ide->buffer, 0, IDE_DISK_SECTOR_SIZE);

	/* basic geometry */
	ide->features[ 0*2+0] = 0x5a;						/*  0: configuration bits */
	ide->features[ 0*2+1] = 0x04;
	ide->features[ 1*2+0] = ide->num_cylinders & 0xff;	/*  1: logical cylinders */
	ide->features[ 1*2+1] = ide->num_cylinders >> 8;
	ide->features[ 2*2+0] = 0;							/*  2: reserved */
	ide->features[ 2*2+1] = 0;
	ide->features[ 3*2+0] = ide->num_heads & 0xff;		/*  3: logical heads */
	ide->features[ 3*2+1] = ide->num_heads >> 8;
	ide->features[ 4*2+0] = 0;							/*  4: vendor specific (obsolete) */
	ide->features[ 4*2+1] = 0;
	ide->features[ 5*2+0] = 0;							/*  5: vendor specific (obsolete) */
	ide->features[ 5*2+1] = 0;
	ide->features[ 6*2+0] = ide->num_sectors & 0xff;	/*  6: logical sectors per logical track */
	ide->features[ 6*2+1] = ide->num_sectors >> 8;
	ide->features[ 7*2+0] = 0;							/*  7: vendor-specific */
	ide->features[ 7*2+1] = 0;
	ide->features[ 8*2+0] = 0;							/*  8: vendor-specific */
	ide->features[ 8*2+1] = 0;
	ide->features[ 9*2+0] = 0;							/*  9: vendor-specific */
	ide->features[ 9*2+1] = 0;
	swap_strncpy(&ide->features[10*2+0], 				/* 10-19: serial number */
			"00000000000000000000", 10);
	ide->features[20*2+0] = 0;							/* 20: vendor-specific */
	ide->features[20*2+1] = 0;
	ide->features[21*2+0] = 0;							/* 21: vendor-specific */
	ide->features[21*2+1] = 0;
	ide->features[22*2+0] = 4;							/* 22: # of vendor-specific bytes on read/write long commands */
	ide->features[22*2+1] = 0;
	swap_strncpy(&ide->features[23*2+0], 				/* 23-26: firmware revision */
			"1.0", 4);
	swap_strncpy(&ide->features[27*2+0], 				/* 27-46: model number */
			"MAME Compressed Hard Disk", 20);
	ide->features[47*2+0] = 0x01;						/* 47: read/write multiple support */
	ide->features[47*2+1] = 0x80;
	ide->features[48*2+0] = 0;							/* 48: reserved */
	ide->features[48*2+1] = 0;
	ide->features[49*2+0] = 0x03;						/* 49: capabilities */
	ide->features[49*2+1] = 0x0f;
	ide->features[50*2+0] = 0;							/* 50: reserved */
	ide->features[50*2+1] = 0;
	ide->features[51*2+0] = 2;							/* 51: PIO data transfer cycle timing mode */
	ide->features[51*2+1] = 0;
	ide->features[52*2+0] = 2;							/* 52: single word DMA transfer cycle timing mode */
	ide->features[52*2+1] = 0;
	ide->features[53*2+0] = 3;							/* 53: field validity */
	ide->features[53*2+1] = 0;
	ide->features[54*2+0] = ide->num_cylinders & 0xff;	/* 54: number of current logical cylinders */
	ide->features[54*2+1] = ide->num_cylinders >> 8;
	ide->features[55*2+0] = ide->num_heads & 0xff;		/* 55: number of current logical heads */
	ide->features[55*2+1] = ide->num_heads >> 8;
	ide->features[56*2+0] = ide->num_sectors & 0xff;	/* 56: number of current logical sectors per track */
	ide->features[56*2+1] = ide->num_sectors >> 8;
	ide->features[57*2+0] = sectors_per_track & 0xff;	/* 57-58: number of current logical sectors per track */
	ide->features[57*2+1] = sectors_per_track >> 8;
	ide->features[58*2+0] = sectors_per_track >> 16;
	ide->features[58*2+1] = sectors_per_track >> 24;
	ide->features[59*2+0] = 0;							/* 59: multiple sector timing */
	ide->features[59*2+1] = 0;
	ide->features[60*2+0] = total_sectors & 0xff;		/* 60-61: total user addressable sectors */
	ide->features[60*2+1] = total_sectors >> 8;
	ide->features[61*2+0] = total_sectors >> 16;
	ide->features[61*2+1] = total_sectors >> 24;
	ide->features[62*2+0] = 0x07;						/* 62: single word dma transfer */
	ide->features[62*2+1] = 0x00;
	ide->features[63*2+0] = 0x07;						/* 63: multiword DMA transfer */
	ide->features[63*2+1] = 0x04;
	ide->features[64*2+0] = 0x03;						/* 64: flow control PIO transfer modes supported */
	ide->features[64*2+1] = 0x00;
	ide->features[65*2+0] = 0x78;						/* 65: minimum multiword DMA transfer cycle time per word */
	ide->features[65*2+1] = 0x00;
	ide->features[66*2+0] = 0x78;						/* 66: mfr's recommended multiword DMA transfer cycle time */
	ide->features[66*2+1] = 0x00;
	ide->features[67*2+0] = 0x4d;						/* 67: minimum PIO transfer cycle time without flow control */
	ide->features[67*2+1] = 0x01;
	ide->features[68*2+0] = 0x78;						/* 68: minimum PIO transfer cycle time with IORDY */
	ide->features[68*2+1] = 0x00;
	ide->features[69*2+0] = 0x00;						/* 69-70: reserved */
	ide->features[69*2+1] = 0x00;
	ide->features[71*2+0] = 0x00;						/* 71: reserved for IDENTIFY PACKET command */
	ide->features[71*2+1] = 0x00;
	ide->features[72*2+0] = 0x00;						/* 72: reserved for IDENTIFY PACKET command */
	ide->features[72*2+1] = 0x00;
	ide->features[73*2+0] = 0x00;						/* 73: reserved for IDENTIFY PACKET command */
	ide->features[73*2+1] = 0x00;
	ide->features[74*2+0] = 0x00;						/* 74: reserved for IDENTIFY PACKET command */
	ide->features[74*2+1] = 0x00;
	ide->features[75*2+0] = 0x00;						/* 75: queue depth */
	ide->features[75*2+1] = 0x00;
	ide->features[76*2+0] = 0x00;						/* 76-79: reserved */
	ide->features[76*2+1] = 0x00;
	ide->features[80*2+0] = 0x00;						/* 80: major version number */
	ide->features[80*2+1] = 0x00;
	ide->features[81*2+0] = 0x00;						/* 81: minor version number */
	ide->features[81*2+1] = 0x00;
	ide->features[82*2+0] = 0x00;						/* 82: command set supported */
	ide->features[82*2+1] = 0x00;
	ide->features[83*2+0] = 0x00;						/* 83: command sets supported */
	ide->features[83*2+1] = 0x00;
	ide->features[84*2+0] = 0x00;						/* 84: command set/feature supported extension */
	ide->features[84*2+1] = 0x00;
	ide->features[85*2+0] = 0x00;						/* 85: command set/feature enabled */
	ide->features[85*2+1] = 0x00;
	ide->features[86*2+0] = 0x00;						/* 86: command set/feature enabled */
	ide->features[86*2+1] = 0x00;
	ide->features[87*2+0] = 0x00;						/* 87: command set/feature default */
	ide->features[87*2+1] = 0x00;
	ide->features[88*2+0] = 0x00;						/* 88: additional DMA modes */
	ide->features[88*2+1] = 0x00;
	ide->features[89*2+0] = 0x00;						/* 89: time required for security erase unit completion */
	ide->features[89*2+1] = 0x00;
	ide->features[90*2+0] = 0x00;						/* 90: time required for enhanced security erase unit completion */
	ide->features[90*2+1] = 0x00;
	ide->features[91*2+0] = 0x00;						/* 91: current advanced power management value */
	ide->features[91*2+1] = 0x00;
	ide->features[92*2+0] = 0x00;						/* 92: master password revision code */
	ide->features[92*2+1] = 0x00;
	ide->features[93*2+0] = 0x00;						/* 93: hardware reset result */
	ide->features[93*2+1] = 0x00;
	ide->features[94*2+0] = 0x00;						/* 94: acoustic management values */
	ide->features[94*2+1] = 0x00;
	ide->features[95*2+0] = 0x00;						/* 95-99: reserved */
	ide->features[95*2+1] = 0x00;
	ide->features[100*2+0] = total_sectors & 0xff;		/* 100-103: maximum 48-bit LBA */
	ide->features[100*2+1] = total_sectors >> 8;
	ide->features[101*2+0] = total_sectors >> 16;
	ide->features[101*2+1] = total_sectors >> 24;
	ide->features[102*2+0] = 0x00;
	ide->features[102*2+1] = 0x00;
	ide->features[103*2+0] = 0x00;
	ide->features[103*2+1] = 0x00;
	ide->features[104*2+0] = 0x00;						/* 104-126: reserved */
	ide->features[104*2+1] = 0x00;
	ide->features[127*2+0] = 0x00;						/* 127: removable media status notification */
	ide->features[127*2+1] = 0x00;
	ide->features[128*2+0] = 0x00;						/* 128: security status */
	ide->features[128*2+1] = 0x00;
	ide->features[129*2+0] = 0x00;						/* 129-159: vendor specific */
	ide->features[129*2+1] = 0x00;
	ide->features[160*2+0] = 0x00;						/* 160: CFA power mode 1 */
	ide->features[160*2+1] = 0x00;
	ide->features[161*2+0] = 0x00;						/* 161-175: reserved for CompactFlash */
	ide->features[161*2+1] = 0x00;
	ide->features[176*2+0] = 0x00;						/* 176-205: current media serial number */
	ide->features[176*2+1] = 0x00;
	ide->features[206*2+0] = 0x00;						/* 206-254: reserved */
	ide->features[206*2+1] = 0x00;
	ide->features[255*2+0] = 0x00;						/* 255: integrity word */
	ide->features[255*2+1] = 0x00;
}



/*************************************
 *
 *	security error handling
 *
 *************************************/

static void security_error_done(int which)
{
	struct ide_state *ide = &idestate[which];

	/* clear error state */
	ide->status &= ~IDE_STATUS_ERROR;
	ide->status |= IDE_STATUS_DRIVE_READY;
}

static void security_error(struct ide_state *ide)
{
	/* set error state */
	ide->status |= IDE_STATUS_ERROR;
	ide->status &= ~IDE_STATUS_DRIVE_READY;

	/* just set a timer and mark ourselves error */
	timer_set(TIME_SECURITY_ERROR, ide - idestate, security_error_done);
}



/*************************************
 *
 *	Sector reading
 *
 *************************************/

static void continue_read(struct ide_state *ide)
{
	/* reset the totals */
	ide->buffer_offset = 0;

	/* clear the buffer ready flag */
	ide->status &= ~IDE_STATUS_BUFFER_READY;

	if (ide->master_password_enable || ide->user_password_enable)
	{
		security_error(ide);

		ide->sector_count = 0;
		ide->bus_master_status &= ~IDE_BUSMASTER_STATUS_ACTIVE;
		ide->dma_active = 0;

		return;
	}

	/* if there is more data to read, keep going */
	if (ide->sector_count > 0)
		ide->sector_count--;
	if (ide->sector_count > 0)
		read_next_sector(ide);
	else
	{
		ide->bus_master_status &= ~IDE_BUSMASTER_STATUS_ACTIVE;
		ide->dma_active = 0;
	}
}


static void write_buffer_to_dma(struct ide_state *ide)
{
	int bytesleft = IDE_DISK_SECTOR_SIZE;
	UINT8 *data = ide->buffer;

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "Writing sector to %08X\n", ide->dma_address);*/

	/* loop until we've consumed all bytes */
	while (bytesleft--)
	{
		/* if we're out of space, grab the next descriptor */
		if (ide->dma_bytes_left == 0)
		{
			/* if we're out of buffer space, that's bad */
			if (ide->dma_last_buffer)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "DMA Out of buffer space!\n");
				return;
			}

			/* fetch the address */
			ide->dma_address = cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor);
			ide->dma_address |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 8;
			ide->dma_address |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 16;
			ide->dma_address |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 24;
			ide->dma_address &= 0xfffffffe;

			/* fetch the length */
			ide->dma_bytes_left = cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor);
			ide->dma_bytes_left |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 8;
			ide->dma_bytes_left |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 16;
			ide->dma_bytes_left |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 24;
			ide->dma_last_buffer = (ide->dma_bytes_left >> 31) & 1;
			ide->dma_bytes_left &= 0xfffe;
			if (ide->dma_bytes_left == 0)
				ide->dma_bytes_left = 0x10000;

/*			log_cb(RETRO_LOG_DEBUG, LOGPRE "New DMA descriptor: address = %08X  bytes = %04X  last = %d\n", ide->dma_address, ide->dma_bytes_left, ide->dma_last_buffer);*/
		}

		/* write the next byte */
		cpunum_write_byte(ide->dma_cpu, ide->dma_address++, *data++);
		ide->dma_bytes_left--;
	}
}


static void read_sector_done(int which)
{
	struct ide_state *ide = &idestate[which];
	int lba = lba_address(ide), count = 0;

	/* now do the read */
	if (ide->disk)
		count = hard_disk_read(ide->disk, lba, 1, ide->buffer);

	/* by default, mark the buffer ready and the seek complete */
	ide->status |= IDE_STATUS_BUFFER_READY;
	ide->status |= IDE_STATUS_SEEK_COMPLETE;

	/* and clear the busy adn error flags */
	ide->status &= ~IDE_STATUS_ERROR;
	ide->status &= ~IDE_STATUS_BUSY;

	/* if we succeeded, advance to the next sector and set the nice bits */
	if (count == 1)
	{
		/* advance the pointers, unless this is the last sector */
		/* Gauntlet: Dark Legacy checks to make sure we stop on the last sector */
		if (ide->sector_count != 1)
			next_sector(ide);

		/* clear the error value */
		ide->error = IDE_ERROR_NONE;

		/* signal an interrupt */
		if (--ide->sectors_until_int == 0 || ide->sector_count == 1)
		{
			ide->sectors_until_int = ((ide->command == IDE_COMMAND_READ_MULTIPLE_BLOCK) ? ide->block_count : 1);
			signal_interrupt(ide);
		}

		/* keep going for DMA */
		if (ide->dma_active)
		{
			write_buffer_to_dma(ide);
			continue_read(ide);
		}
	}

	/* if we got an error, we need to report it */
	else
	{
		/* set the error flag and the error */
		ide->status |= IDE_STATUS_ERROR;
		ide->error = IDE_ERROR_BAD_SECTOR;
		ide->bus_master_status |= IDE_BUSMASTER_STATUS_ERROR;
		ide->bus_master_status &= ~IDE_BUSMASTER_STATUS_ACTIVE;

		/* signal an interrupt */
		signal_interrupt(ide);
	}
}


static void read_first_sector(struct ide_state *ide)
{
	/* mark ourselves busy */
	ide->status |= IDE_STATUS_BUSY;

	/* just set a timer */
	if (ide->command == IDE_COMMAND_READ_MULTIPLE_BLOCK)
	{
		double seek_time;

		if (ide->cur_lba == lba_address(ide))
			seek_time = TIME_NO_SEEK_MULTISECTOR;
		else
			seek_time = TIME_SEEK_MULTISECTOR;

		timer_set(seek_time, ide - idestate, read_sector_done);
	}
	else
		timer_set(TIME_PER_SECTOR, ide - idestate, read_sector_done);
}


static void read_next_sector(struct ide_state *ide)
{
	/* mark ourselves busy */
	ide->status |= IDE_STATUS_BUSY;

	if (ide->command == IDE_COMMAND_READ_MULTIPLE_BLOCK)
	{
		if (ide->sectors_until_int != 1)
			/* make ready now */
			read_sector_done(ide - idestate);
		else
			/* just set a timer */
			timer_set(TIME_IN_USEC(1), ide - idestate, read_sector_done);
	}
	else
		/* just set a timer */
		timer_set(TIME_PER_SECTOR, ide - idestate, read_sector_done);
}



/*************************************
 *
 *	Sector writing
 *
 *************************************/

static void write_sector_done(int which);

static void continue_write(struct ide_state *ide)
{
	/* reset the totals */
	ide->buffer_offset = 0;

	/* clear the buffer ready flag */
	ide->status &= ~IDE_STATUS_BUFFER_READY;
	ide->status |= IDE_STATUS_BUSY;

	if (ide->command == IDE_COMMAND_WRITE_MULTIPLE_BLOCK)
	{
		if (ide->sectors_until_int != 1)
		{
			/* ready to write now */
			write_sector_done(ide - idestate);
		}
		else
		{
			/* set a timer to do the write */
			timer_set(TIME_PER_SECTOR, ide - idestate, write_sector_done);
		}
	}
	else
	{
		/* set a timer to do the write */
		timer_set(TIME_PER_SECTOR, ide - idestate, write_sector_done);
	}
}


static void read_buffer_from_dma(struct ide_state *ide)
{
	int bytesleft = IDE_DISK_SECTOR_SIZE;
	UINT8 *data = ide->buffer;

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "Reading sector from %08X\n", ide->dma_address);*/

	/* loop until we've consumed all bytes */
	while (bytesleft--)
	{
		/* if we're out of space, grab the next descriptor */
		if (ide->dma_bytes_left == 0)
		{
			/* if we're out of buffer space, that's bad */
			if (ide->dma_last_buffer)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "DMA Out of buffer space!\n");
				return;
			}

			/* fetch the address */
			ide->dma_address = cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor);
			ide->dma_address |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 8;
			ide->dma_address |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 16;
			ide->dma_address |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 24;
			ide->dma_address &= 0xfffffffe;

			/* fetch the length */
			ide->dma_bytes_left = cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor);
			ide->dma_bytes_left |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 8;
			ide->dma_bytes_left |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 16;
			ide->dma_bytes_left |= cpunum_read_byte(ide->dma_cpu, ide->dma_descriptor++ ^ ide->dma_address_xor) << 24;
			ide->dma_last_buffer = (ide->dma_bytes_left >> 31) & 1;
			ide->dma_bytes_left &= 0xfffe;
			if (ide->dma_bytes_left == 0)
				ide->dma_bytes_left = 0x10000;

/*			log_cb(RETRO_LOG_DEBUG, LOGPRE "New DMA descriptor: address = %08X  bytes = %04X  last = %d\n", ide->dma_address, ide->dma_bytes_left, ide->dma_last_buffer);*/
		}

		/* read the next byte */
		*data++ = cpunum_read_byte(ide->dma_cpu, ide->dma_address++);
		ide->dma_bytes_left--;
	}
}


static void write_sector_done(int which)
{
	struct ide_state *ide = &idestate[which];
	int lba = lba_address(ide), count = 0;

	/* now do the write */
	if (ide->disk)
		count = hard_disk_write(ide->disk, lba, 1, ide->buffer);

	/* by default, mark the buffer ready and the seek complete */
	ide->status |= IDE_STATUS_BUFFER_READY;
	ide->status |= IDE_STATUS_SEEK_COMPLETE;

	/* and clear the busy adn error flags */
	ide->status &= ~IDE_STATUS_ERROR;
	ide->status &= ~IDE_STATUS_BUSY;

	/* if we succeeded, advance to the next sector and set the nice bits */
	if (count == 1)
	{
		/* advance the pointers, unless this is the last sector */
		/* Gauntlet: Dark Legacy checks to make sure we stop on the last sector */
		if (ide->sector_count != 1)
			next_sector(ide);

		/* clear the error value */
		ide->error = IDE_ERROR_NONE;

		/* signal an interrupt */
		if (--ide->sectors_until_int == 0 || ide->sector_count == 1)
		{
			ide->sectors_until_int = ((ide->command == IDE_COMMAND_WRITE_MULTIPLE_BLOCK) ? ide->block_count : 1);
			signal_interrupt(ide);
		}

		/* signal an interrupt if there's more data needed */
		if (ide->sector_count > 0)
			ide->sector_count--;
		if (ide->sector_count == 0)
			ide->status &= ~IDE_STATUS_BUFFER_READY;

		/* keep going for DMA */
		if (ide->dma_active && ide->sector_count != 0)
		{
			read_buffer_from_dma(ide);
			continue_write(ide);
		}
		else
			ide->dma_active = 0;
	}

	/* if we got an error, we need to report it */
	else
	{
		/* set the error flag and the error */
		ide->status |= IDE_STATUS_ERROR;
		ide->error = IDE_ERROR_BAD_SECTOR;
		ide->bus_master_status |= IDE_BUSMASTER_STATUS_ERROR;
		ide->bus_master_status &= ~IDE_BUSMASTER_STATUS_ACTIVE;

		/* signal an interrupt */
		signal_interrupt(ide);
	}
}



/*************************************
 *
 *	Handle IDE commands
 *
 *************************************/

void handle_command(struct ide_state *ide, UINT8 command)
{
	/* implicitly clear interrupts here */
	clear_interrupt(ide);

	ide->command = command;
	switch (command)
	{
		case IDE_COMMAND_READ_MULTIPLE:
		case IDE_COMMAND_READ_MULTIPLE_ONCE:
			LOGPRINT(("IDE Read multiple: C=%d H=%d S=%d LBA=%d count=%d\n",
				ide->cur_cylinder, ide->cur_head, ide->cur_sector, lba_address(ide), ide->sector_count));

			/* reset the buffer */
			ide->buffer_offset = 0;
			ide->sectors_until_int = 1;
			ide->dma_active = 0;

			/* start the read going */
			read_first_sector(ide);
			break;

		case IDE_COMMAND_READ_MULTIPLE_BLOCK:
			LOGPRINT(("IDE Read multiple block: C=%d H=%d S=%d LBA=%d count=%d\n",
				ide->cur_cylinder, ide->cur_head, ide->cur_sector, lba_address(ide), ide->sector_count));

			/* reset the buffer */
			ide->buffer_offset = 0;
			ide->sectors_until_int = 1;
			ide->dma_active = 0;

			/* start the read going */
			read_first_sector(ide);
			break;

		case IDE_COMMAND_READ_DMA:
			LOGPRINT(("IDE Read multiple DMA: C=%d H=%d S=%d LBA=%d count=%d\n",
				ide->cur_cylinder, ide->cur_head, ide->cur_sector, lba_address(ide), ide->sector_count));

			/* reset the buffer */
			ide->buffer_offset = 0;
			ide->sectors_until_int = ide->sector_count;
			ide->dma_active = 1;

			/* start the read going */
			if (ide->bus_master_command & 1)
				read_first_sector(ide);
			break;

		case IDE_COMMAND_WRITE_MULTIPLE:
			LOGPRINT(("IDE Write multiple: C=%d H=%d S=%d LBA=%d count=%d\n",
				ide->cur_cylinder, ide->cur_head, ide->cur_sector, lba_address(ide), ide->sector_count));

			/* reset the buffer */
			ide->buffer_offset = 0;
			ide->sectors_until_int = 1;
			ide->dma_active = 0;

			/* mark the buffer ready */
			ide->status |= IDE_STATUS_BUFFER_READY;
			break;

		case IDE_COMMAND_WRITE_MULTIPLE_BLOCK:
			LOGPRINT(("IDE Write multiple block: C=%d H=%d S=%d LBA=%d count=%d\n",
				ide->cur_cylinder, ide->cur_head, ide->cur_sector, lba_address(ide), ide->sector_count));

			/* reset the buffer */
			ide->buffer_offset = 0;
			ide->sectors_until_int = 1;
			ide->dma_active = 0;

			/* mark the buffer ready */
			ide->status |= IDE_STATUS_BUFFER_READY;
			break;

		case IDE_COMMAND_WRITE_DMA:
			LOGPRINT(("IDE Write multiple DMA: C=%d H=%d S=%d LBA=%d count=%d\n",
				ide->cur_cylinder, ide->cur_head, ide->cur_sector, lba_address(ide), ide->sector_count));

			/* reset the buffer */
			ide->buffer_offset = 0;
			ide->sectors_until_int = ide->sector_count;
			ide->dma_active = 1;

			/* start the read going */
			if (ide->bus_master_command & 1)
			{
				read_buffer_from_dma(ide);
				continue_write(ide);
			}
			break;

		case IDE_COMMAND_SECURITY_UNLOCK:
			LOGPRINT(("IDE Security Unlock\n"));

			/* reset the buffer */
			ide->buffer_offset = 0;
			ide->sectors_until_int = 0;
			ide->dma_active = 0;

			/* mark the buffer ready */
			ide->status |= IDE_STATUS_BUFFER_READY;
			signal_interrupt(ide);
			break;

		case IDE_COMMAND_GET_INFO:
			LOGPRINT(("IDE Read features\n"));

			/* reset the buffer */
			ide->buffer_offset = 0;
			ide->sector_count = 1;

			/* build the features page */
			memcpy(ide->buffer, ide->features, sizeof(ide->buffer));

			/* indicate everything is ready */
			ide->status |= IDE_STATUS_BUFFER_READY;
			ide->status |= IDE_STATUS_SEEK_COMPLETE;

			/* and clear the busy adn error flags */
			ide->status &= ~IDE_STATUS_ERROR;
			ide->status &= ~IDE_STATUS_BUSY;

			/* clear the error too */
			ide->error = IDE_ERROR_NONE;

			/* signal an interrupt */
			signal_delayed_interrupt(ide, MINIMUM_COMMAND_TIME, 1);
			break;

		case IDE_COMMAND_SET_CONFIG:
			LOGPRINT(("IDE Set configuration (%d heads, %d sectors)\n", ide->cur_head + 1, ide->sector_count));

			ide->num_sectors = ide->sector_count;
			ide->num_heads = ide->cur_head + 1;

			/* signal an interrupt */
			signal_interrupt(ide);
			break;

		case IDE_COMMAND_UNKNOWN_F9:
			/* only used by Killer Instinct AFAICT */
			LOGPRINT(("IDE unknown command (F9)\n"));

			/* signal an interrupt */
			signal_interrupt(ide);
			break;

		case IDE_COMMAND_SET_FEATURES:
			LOGPRINT(("IDE Set features (%02X %02X %02X %02X %02X)\n", ide->precomp_offset, ide->sector_count & 0xff, ide->cur_sector, ide->cur_cylinder & 0xff, ide->cur_cylinder >> 8));

			/* signal an interrupt */
			signal_delayed_interrupt(ide, MINIMUM_COMMAND_TIME, 0);
			break;
		
		case IDE_COMMAND_SET_BLOCK_COUNT:
			LOGPRINT(("IDE Set block count (%02X)\n", ide->sector_count));

			ide->block_count = ide->sector_count;

			/* signal an interrupt */
			signal_interrupt(ide);
			break;

		default:
			LOGPRINT(("IDE unknown command (%02X)\n", command));
#ifdef MAME_DEBUG
{
	extern int debug_key_pressed;
	debug_key_pressed = 1;
}
#endif
			break;
	}
}



/*************************************
 *
 *	IDE controller read
 *
 *************************************/

static UINT32 ide_controller_read(struct ide_state *ide, offs_t offset, int size)
{
	UINT32 result = 0;

	/* logit */
	if (offset != IDE_ADDR_DATA && offset != IDE_ADDR_STATUS_COMMAND && offset != IDE_ADDR_STATUS_CONTROL)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X:IDE read at %03X, size=%d\n", activecpu_get_previouspc(), offset, size);

	switch (offset)
	{
		/* unknown config register */
		case IDE_ADDR_CONFIG_UNK:
			return ide->config_unknown;

		/* active config register */
		case IDE_ADDR_CONFIG_REGISTER:
			return ide->config_register_num;

		/* data from active config register */
		case IDE_ADDR_CONFIG_DATA:
			if (ide->config_register_num < IDE_CONFIG_REGISTERS)
				return ide->config_register[ide->config_register_num];
			return 0;

		/* read data if there's data to be read */
		case IDE_ADDR_DATA:
			if (ide->status & IDE_STATUS_BUFFER_READY)
			{
				/* fetch the correct amount of data */
				result = ide->buffer[ide->buffer_offset++];
				if (size > 1)
					result |= ide->buffer[ide->buffer_offset++] << 8;
				if (size > 2)
				{
					result |= ide->buffer[ide->buffer_offset++] << 16;
					result |= ide->buffer[ide->buffer_offset++] << 24;
				}

				/* if we're at the end of the buffer, handle it */
				if (ide->buffer_offset >= IDE_DISK_SECTOR_SIZE)
					continue_read(ide);
			}
			break;

		/* return the current error */
		case IDE_ADDR_ERROR:
			return ide->error;

		/* return the current sector count */
		case IDE_ADDR_SECTOR_COUNT:
			return ide->sector_count;

		/* return the current sector */
		case IDE_ADDR_SECTOR_NUMBER:
			return ide->cur_sector;

		/* return the current cylinder LSB */
		case IDE_ADDR_CYLINDER_LSB:
			return ide->cur_cylinder & 0xff;

		/* return the current cylinder MSB */
		case IDE_ADDR_CYLINDER_MSB:
			return ide->cur_cylinder >> 8;

		/* return the current head */
		case IDE_ADDR_HEAD_NUMBER:
			return ide->cur_head_reg;

		/* return the current status and clear any pending interrupts */
		case IDE_ADDR_STATUS_COMMAND:
		/* return the current status but don't clear interrupts */
		case IDE_ADDR_STATUS_CONTROL:
			result = ide->status;
			if (timer_timeelapsed(ide->last_status_timer) > TIME_PER_ROTATION)
			{
				result |= IDE_STATUS_HIT_INDEX;
				timer_adjust(ide->last_status_timer, TIME_NEVER, 0, 0);
			}

			/* clear interrutps only when reading the real status */
			if (offset == IDE_ADDR_STATUS_COMMAND)
			{
				if (ide->interrupt_pending)
					clear_interrupt(ide);
			}
			
			/* take a bit of time to speed up people who poll hard */
			activecpu_adjust_icount(-100);
			break;

		/* log anything else */
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X:unknown IDE read at %03X, size=%d\n", activecpu_get_previouspc(), offset, size);
			break;
	}

	/* return the result */
	return result;
}



/*************************************
 *
 *	IDE controller write
 *
 *************************************/

static void ide_controller_write(struct ide_state *ide, offs_t offset, int size, UINT32 data)
{
	/* logit */
	if (offset != IDE_ADDR_DATA)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X:IDE write to %03X = %08X, size=%d\n", activecpu_get_previouspc(), offset, data, size);

	switch (offset)
	{
		/* unknown config register */
		case IDE_ADDR_CONFIG_UNK:
			ide->config_unknown = data;
			break;

		/* active config register */
		case IDE_ADDR_CONFIG_REGISTER:
			ide->config_register_num = data;
			break;

		/* data from active config register */
		case IDE_ADDR_CONFIG_DATA:
			if (ide->config_register_num < IDE_CONFIG_REGISTERS)
				ide->config_register[ide->config_register_num] = data;
			break;

		/* write data */
		case IDE_ADDR_DATA:
			if (ide->status & IDE_STATUS_BUFFER_READY)
			{
				/* store the correct amount of data */
				ide->buffer[ide->buffer_offset++] = data;
				if (size > 1)
					ide->buffer[ide->buffer_offset++] = data >> 8;
				if (size > 2)
				{
					ide->buffer[ide->buffer_offset++] = data >> 16;
					ide->buffer[ide->buffer_offset++] = data >> 24;
				}

				/* if we're at the end of the buffer, handle it */
				if (ide->buffer_offset >= IDE_DISK_SECTOR_SIZE)
				{
					if (ide->command != IDE_COMMAND_SECURITY_UNLOCK)
						continue_write(ide);
					else
					{
						if (ide->user_password_enable && memcmp(ide->buffer, ide->user_password, 2 + 32) == 0)
						{
							LOGPRINT(("IDE Unlocked user password\n"));
							ide->user_password_enable = 0;
						}
						if (ide->master_password_enable && memcmp(ide->buffer, ide->master_password, 2 + 32) == 0)
						{
							LOGPRINT(("IDE Unlocked master password\n"));
							ide->master_password_enable = 0;
						}
#if PRINTF_IDE_PASSWORD
						{
							int i;

							for (i = 0; i < 34; i += 2)
							{
								if (i % 8 == 2)
									log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");

								log_cb(RETRO_LOG_DEBUG, LOGPRE "0x%02x, 0x%02x, ", ide->buffer[i], ide->buffer[i + 1]);
								/*printf("0x%02x%02x, ", ide->buffer[i], ide->buffer[i + 1]);*/
							}
							log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");
						}
#endif

						/* clear the busy adn error flags */
						ide->status &= ~IDE_STATUS_ERROR;
						ide->status &= ~IDE_STATUS_BUSY;
						ide->status &= ~IDE_STATUS_BUFFER_READY;

						if (ide->master_password_enable || ide->user_password_enable)
							security_error(ide);
						else
							ide->status |= IDE_STATUS_DRIVE_READY;
					}
				}
			}
			break;

		/* precompensation offset?? */
		case IDE_ADDR_ERROR:
			ide->precomp_offset = data;
			break;

		/* sector count */
		case IDE_ADDR_SECTOR_COUNT:
			ide->sector_count = data ? data : 256;
			break;

		/* current sector */
		case IDE_ADDR_SECTOR_NUMBER:
			ide->cur_sector = data;
			break;

		/* current cylinder LSB */
		case IDE_ADDR_CYLINDER_LSB:
			ide->cur_cylinder = (ide->cur_cylinder & 0xff00) | (data & 0xff);
			break;

		/* current cylinder MSB */
		case IDE_ADDR_CYLINDER_MSB:
			ide->cur_cylinder = (ide->cur_cylinder & 0x00ff) | ((data & 0xff) << 8);
			break;

		/* current head */
		case IDE_ADDR_HEAD_NUMBER:
			ide->cur_head = data & 0x0f;
			ide->cur_head_reg = data;
			/* drive index = data & 0x10*/
			/* LBA mode = data & 0x40*/
			break;

		/* command */
		case IDE_ADDR_STATUS_COMMAND:
			handle_command(ide, data);
			break;

		/* adapter control */
		case IDE_ADDR_STATUS_CONTROL:
			ide->adapter_control = data;

			/* handle controller reset */
			/*if (data == 0x04)*/
			if (data & 0x04)
			{
				ide->status |= IDE_STATUS_BUSY;
				ide->status &= ~IDE_STATUS_DRIVE_READY;
				timer_adjust(ide->reset_timer, TIME_IN_MSEC(5), ide - idestate, 0);
			}
			break;
	}
}



/*************************************
 *
 *	Bus master read
 *
 *************************************/

static UINT32 ide_bus_master_read(struct ide_state *ide, offs_t offset, int size)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X:ide_bus_master_read(%d, %d)\n", activecpu_get_previouspc(), offset, size);

	/* command register */
	if (offset == 0)
		return ide->bus_master_command | (ide->bus_master_status << 16);
	
	/* status register */
	if (offset == 2)
		return ide->bus_master_status;
	
	/* descriptor table register */
	if (offset == 4)
		return ide->bus_master_descriptor;
	
	return 0xffffffff;
}



/*************************************
 *
 *	Bus master write
 *
 *************************************/

static void ide_bus_master_write(struct ide_state *ide, offs_t offset, int size, UINT32 data)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X:ide_bus_master_write(%d, %d, %08X)\n", activecpu_get_previouspc(), offset, size, data);

	/* command register */
	if (offset == 0)
	{
		UINT8 old = ide->bus_master_command;
		UINT8 val = data & 0xff;
		
		/* save the read/write bit and the start/stop bit */
		ide->bus_master_command = (old & 0xf6) | (val & 0x09);
		ide->bus_master_status = (ide->bus_master_status & ~IDE_BUSMASTER_STATUS_ACTIVE) | (val & 0x01);
		
		/* handle starting a transfer */
		if (!(old & 1) && (val & 1))
		{
			/* reset all the DMA data */
			ide->dma_bytes_left = 0;
			ide->dma_last_buffer = 0;
			ide->dma_descriptor = ide->bus_master_descriptor;
			ide->dma_cpu = cpu_getactivecpu();
			ide->dma_address_xor = (activecpu_endianess() == CPU_IS_LE) ? 0 : 3;
			
			/* if we're going live, start the pending read/write */
			if (ide->dma_active)
			{
				if (ide->bus_master_command & 8)
					read_next_sector(ide);
				else
				{
					read_buffer_from_dma(ide);
					continue_write(ide);
				}
			}
		}
	}
	
	/* status register */
	if (offset <= 2 && offset + size > 2)
	{
		UINT8 old = ide->bus_master_status;
		UINT8 val = data >> (8 * (2 - offset));
		
		/* save the DMA capable bits */
		ide->bus_master_status = (old & 0x9f) | (val & 0x60);
		
		/* clear interrupt and error bits */
		if (val & IDE_BUSMASTER_STATUS_IRQ)
			ide->bus_master_status &= ~IDE_BUSMASTER_STATUS_IRQ;
		if (val & IDE_BUSMASTER_STATUS_ERROR)
			ide->bus_master_status &= ~IDE_BUSMASTER_STATUS_ERROR;
	}
	
	/* descriptor table register */
	if (offset == 4)
		ide->bus_master_descriptor = data & 0xfffffffc;
}



/*************************************
 *
 *	32-bit IDE handlers
 *
 *************************************/

READ32_HANDLER( ide_controller32_0_r )
{
	int size;

	offset *= 4;
	size = convert_to_offset_and_size32(&offset, mem_mask);

	return ide_controller_read(&idestate[0], offset, size) << ((offset & 3) * 8);
}


WRITE32_HANDLER( ide_controller32_0_w )
{
	int size;

	offset *= 4;
	size = convert_to_offset_and_size32(&offset, mem_mask);

	ide_controller_write(&idestate[0], offset, size, data >> ((offset & 3) * 8));
}


READ32_HANDLER( ide_bus_master32_0_r )
{
	int size;

	offset *= 4;
	size = convert_to_offset_and_size32(&offset, mem_mask);

	return ide_bus_master_read(&idestate[0], offset, size) << ((offset & 3) * 8);
}


WRITE32_HANDLER( ide_bus_master32_0_w )
{
	int size;

	offset *= 4;
	size = convert_to_offset_and_size32(&offset, mem_mask);

	ide_bus_master_write(&idestate[0], offset, size, data >> ((offset & 3) * 8));
}



/*************************************
 *
 *	16-bit IDE handlers
 *
 *************************************/

READ16_HANDLER( ide_controller16_0_r )
{
	int size;

	offset *= 2;
	size = convert_to_offset_and_size16(&offset, mem_mask);

	return ide_controller_read(&idestate[0], offset, size) << ((offset & 1) * 8);
}


WRITE16_HANDLER( ide_controller16_0_w )
{
	int size;

	offset *= 2;
	size = convert_to_offset_and_size16(&offset, mem_mask);

	ide_controller_write(&idestate[0], offset, size, data >> ((offset & 1) * 8));
}
