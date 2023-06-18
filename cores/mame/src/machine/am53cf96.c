/*
 * am53cf96.c
 *
 * AMD/NCR/Symbios 53CF96 SCSI-2 controller.
 * Qlogic FAS-236 and Emulex ESP-236 are equivalents
 *
 * References:
 * AMD Am53CF96 manual
 * scsi commands 12, 1a, 42, 43 still needed for GV (Baby Phoenix)
 * 12 = INQUIRY
 * 1A = MODE SENSE (6)
 * 42 = READ SUB-CHANNEL
 * 43 = READ TOC
 */

#include "driver.h"
#include "state.h"
#include "harddisk.h"
#include "am53cf96.h"

data8_t scsi_regs[32], fifo[16], fptr = 0, last_cmd, xfer_state;
int lba, blocks;
struct hard_disk_file *disk;
static struct AM53CF96interface *intf;

/* 53CF96 register set*/
enum
{
	REG_XFERCNTLOW = 0,	/* read = current xfer count lo byte, write = set xfer count lo byte*/
	REG_XFERCNTMID,		/* read = current xfer count mid byte, write = set xfer count mid byte*/
	REG_FIFO,		/* read/write = FIFO*/
	REG_COMMAND,		/* read/write = command*/

	REG_STATUS,		/* read = status, write = destination SCSI ID (4)*/
	REG_IRQSTATE,		/* read = IRQ status, write = timeout	      (5)*/
	REG_INTSTATE,		/* read = internal state, write = sync xfer period (6)*/
	REG_FIFOSTATE,		/* read = FIFO status, write = sync offset*/
	REG_CTRL1,		/* read/write = control 1*/
	REG_CLOCKFCTR,		/* clock factor (write only)*/
	REG_TESTMODE,		/* test mode (write only)*/
	REG_CTRL2,		/* read/write = control 2*/
	REG_CTRL3,		/* read/write = control 3*/
	REG_CTRL4,		/* read/write = control 4*/
	REG_XFERCNTHI,		/* read = current xfer count hi byte, write = set xfer count hi byte*/
	REG_DATAALIGN,		/* data alignment (write only)*/
};

READ32_HANDLER( am53cf96_r )
{
	int reg, shift, rv;
	int states[] = { 0, 0, 1, 1, 2, 3, 4, 5, 6, 7, 0 };

	reg = offset * 2;
	if (mem_mask == 0xffffff00)
	{
		shift = 0;
	}
	else
	{
		reg++;
		shift = 16;
	}

	if (reg == REG_STATUS)
	{
		scsi_regs[REG_STATUS] &= ~0x7;
		scsi_regs[REG_STATUS] |= states[xfer_state];
		if (xfer_state < 10)
		{
			xfer_state++;
		}
	}

	rv = scsi_regs[reg]<<shift;

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "53cf96: read reg %d = %x (PC=%x)\n", reg, rv>>shift, activecpu_get_pc());*/

	if (reg == REG_IRQSTATE)
	{
		scsi_regs[REG_STATUS] &= ~0x80;	/* clear IRQ flag*/
	}

	return rv;
}

WRITE32_HANDLER( am53cf96_w )
{
	int reg, val, dma;

	reg = offset * 2;
	val = data;
	if (mem_mask == 0xffffff00)
	{
	}
	else
	{
		reg++;
		val >>= 16;
	}
	val &= 0xff;

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "53cf96: w %x to reg %d (ofs %02x data %08x mask %08x PC=%x)\n", val, reg, offset, data, mem_mask, activecpu_get_pc());*/

	if (reg == REG_XFERCNTLOW || reg == REG_XFERCNTMID || reg == REG_XFERCNTHI)
	{
		scsi_regs[REG_STATUS] &= ~0x10;	/* clear CTZ bit*/
	}

	/* FIFO*/
	if (reg == REG_FIFO)
	{
		fifo[fptr++] = val;
		if (fptr > 15)
		{
			fptr = 15;
		}
	}

	/* command*/
	if (reg == REG_COMMAND)
	{
		dma = (val & 0x80) ? 1 : 0;
		fptr = 0;
		switch (val & 0x7f)
		{
			case 0:	/* NOP*/
				scsi_regs[REG_IRQSTATE] = 8;	/* indicate success*/
				xfer_state = 0;
				break;
			case 3:	/* reset SCSI bus*/
				scsi_regs[REG_IRQSTATE] = 8;	/* indicate success*/
				scsi_regs[REG_INTSTATE] = 4;	/* command sent OK*/
				scsi_regs[REG_STATUS] |= 0x80;	/* indicate IRQ*/
				xfer_state = 0;
				intf->irq_callback();
				break;
			case 0x42:    	/* select with ATN steps*/
				scsi_regs[REG_IRQSTATE] = 8;	/* indicate success*/
				scsi_regs[REG_STATUS] |= 0x80;	/* indicate IRQ*/
				intf->irq_callback();
				last_cmd = fifo[1];
/*				log_cb(RETRO_LOG_DEBUG, LOGPRE "53cf96: executing SCSI command %x\n", last_cmd);*/
				if (last_cmd == 0)
				{
					scsi_regs[REG_INTSTATE] = 6;
				}
				else
				{
					scsi_regs[REG_INTSTATE] = 4;
				}

				if (last_cmd == 0x28)	/* READ (10-byte varient)*/
				{
					lba = fifo[3]<<24 | fifo[4]<<16 | fifo[5]<<8 | fifo[6];
					blocks = fifo[8]<<8 | fifo[9];

					log_cb(RETRO_LOG_DEBUG, LOGPRE "53cf96: READ at LBA %x for %x blocks\n", lba, blocks);
				}
				switch (last_cmd)
				{
					case 0:		/* TEST UNIT READY*/
					case 3: 	/* REQUEST SENSE*/
					case 0x28: 	/* READ (10 byte)*/
						break;
					default:
						log_cb(RETRO_LOG_DEBUG, LOGPRE "53cf96: unknown SCSI command %x!\n", last_cmd);
						break;
				}
				xfer_state = 0;
				break;
			case 0x44:	/* enable selection/reselection*/
				xfer_state = 0;
				break;
			case 0x10:	/* information transfer (must not change xfer_state)*/
			case 0x11:	/* second phase of information transfer*/
			case 0x12:	/* message accepted*/
				scsi_regs[REG_IRQSTATE] = 8;	/* indicate success*/
				scsi_regs[REG_INTSTATE] = 6;	/* command sent OK*/
				scsi_regs[REG_STATUS] |= 0x80;	/* indicate IRQ*/
				intf->irq_callback();
				break;
		}
	}

	/* only update the register mirror if it's not a write-only reg*/
	if (reg != REG_STATUS && reg != REG_INTSTATE && reg != REG_IRQSTATE && reg != REG_FIFOSTATE)
	{	
		scsi_regs[reg] = val;
	}
}

void am53cf96_init( struct AM53CF96interface *interface )
{
	const struct hard_disk_info *hdinfo;

	/* save interface pointer for later*/
	intf = interface;

	memset(scsi_regs, 0, sizeof(scsi_regs));

	/* try to open the disk*/
	if (interface->device == AM53CF96_DEVICE_HDD)
	{
		disk = hard_disk_open(get_disk_handle(0));
		if (!disk)
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "53cf96: no disk found!\n");
		}
		else
		{
			hdinfo = hard_disk_get_info(disk);
			if (hdinfo->sectorbytes != 512)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "53cf96: Error!  invalid sector size %d\n", hdinfo->sectorbytes);
			}		
		}
	}
	else if (interface->device == AM53CF96_DEVICE_CDROM)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "53cf96: CDROM not yet supported!\n");
	}
	else
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "53cf96: unknown device type!\n");
	}

	state_save_register_UINT8("53cf96", 0, "registers", scsi_regs, 32);
	state_save_register_UINT8("53cf96", 0, "fifo", fifo, 16);
	state_save_register_UINT8("53cf96", 0, "fifo pointer", &fptr, 1);
	state_save_register_UINT8("53cf96", 0, "last scsi-2 command", &last_cmd, 1);
	state_save_register_UINT8("53cf96", 0, "transfer state", &xfer_state, 1);
	state_save_register_int("53cf96", 0, "current lba", &lba);
	state_save_register_int("53cf96", 0, "blocks to read", &blocks);
}

/* retrieve data from the SCSI controller*/
void am53cf96_read_data(int bytes, data8_t *pData)
{
	int i;

	scsi_regs[REG_STATUS] |= 0x10;	/* indicate DMA finished*/

	switch (last_cmd)
	{
		case 0x03:	/* REQUEST SENSE*/
			pData[0] = 0x80;	/* valid sense*/
			for (i = 1; i < 12; i++)
			{
				pData[i] = 0;
			}
			break;

		case 0x28:	/* READ (10 byte)*/
			if ((disk) && (blocks))
			{
				while (bytes > 0)
				{
					if (!hard_disk_read(disk, lba, 1, pData))
					{
						log_cb(RETRO_LOG_DEBUG, LOGPRE "53cf96: HD read error!\n");
					}
					lba++;
					blocks--;
					bytes -= 512;
					pData += 512;
				}
			}		
			break;

	}
}
