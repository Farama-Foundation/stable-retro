#include <math.h>
#include "driver.h"
#include "sndhrdw/seibu.h"
#include "includes/denjinmk.h"
#include "machine/seicop.h"
#include "vidhrdw/generic.h"


#define seibu_cop_log logerror
#define LOG_CMDS 1

UINT16 *cop_mcu_ram;

static UINT16 copd2_table[0x100];
static UINT16 copd2_table_2[0x100/8];
static UINT16 copd2_table_3[0x100/8];
static UINT16 copd2_table_4[0x100/8];

static UINT16 cop_438;
static UINT16 cop_43a;
static UINT16 cop_43c;

static UINT16 cop_dma_src[0x200];
static UINT16 cop_dma_size[0x200];
static UINT16 cop_dma_dst[0x200];
static UINT16 cop_dma_fade_table;
static UINT16 cop_dma_trigger = 0;
static UINT16 cop_scale;

static UINT8 cop_rng_max_value;

static UINT16 copd2_offs = 0;

static void copd2_set_tableoffset(UINT16 data)
{
	/*logerror("mcu_offs %04x\n", data);*/
	copd2_offs = data;
	if (copd2_offs>0xff)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "copd2 offs > 0x100\n");
	}

	copd2_table_2[copd2_offs/8] = cop_438;
	copd2_table_3[copd2_offs/8] = cop_43a;
	copd2_table_4[copd2_offs/8] = cop_43c;
#if 0

    {
        FILE *fp;
        char filename[256];
        sprintf(filename,"copdat_%s.table2", Machine->gamedrv->name);
        fp=fopen(filename, "w+b");
        if (fp)
        {
            fwrite(copd2_table_2, 0x200/8, 1, fp);
            fclose(fp);
        }
    }
    {
        FILE *fp;
        char filename[256];
        sprintf(filename,"copdat_%s.table3", Machine->gamedrv->name);
        fp=fopen(filename, "w+b");
        if (fp)
        {
            fwrite(copd2_table_3, 0x200/8, 1, fp);
            fclose(fp);
        }
    }
    {
        FILE *fp;
        char filename[256];
        sprintf(filename,"copdat_%s.table4", Machine->gamedrv->name);
        fp=fopen(filename, "w+b");
        if (fp)
        {
            fwrite(copd2_table_4, 0x200/8, 1, fp);
            fclose(fp);
        }
    }

	{
		int i;

		log_cb(RETRO_LOG_DEBUG, LOGPRE "start\n");

		for (i=0;i<0x20;i++)
		{
			int ii;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x | %01x | %04x | %04x | ", i, copd2_table_2[i], copd2_table_3[i], copd2_table_4[i]);


			for (ii=0;ii<0x8;ii++)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%03x ", copd2_table[i*8 + ii]);

			}
			log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");
		}

	}
#endif

}

static void copd2_set_tabledata(UINT16 data)
{
	copd2_table[copd2_offs] = data;
	/*logerror("mcu_data %04x\n", data);*/
#if 0
    {
        FILE *fp;
        char filename[256];
        sprintf(filename,"copdat_%s.data", Machine->gamedrv->name);
        fp=fopen(filename, "w+b");
        if (fp)
        {
            fwrite(copd2_table, 0x200, 1, fp);
            fclose(fp);
        }
    }
#endif
}

static UINT32 cop_register[8];
static UINT16 seibu_vregs[0x50/2];

static WRITE16_HANDLER( seibu_common_video_regs_w )
{
	COMBINE_DATA(&seibu_vregs[offset]);

	switch(offset)
	{
		case (0x01a/2): { flip_screen_set(seibu_vregs[offset] & 0x01); break; }
		case (0x01c/2): { denjinmk_layer_disable =  seibu_vregs[offset]; break; }
		case (0x020/2): { denjinmk_scrollram16[0] = seibu_vregs[offset]; break; }
		case (0x022/2): { denjinmk_scrollram16[1] = seibu_vregs[offset]; break; }
		case (0x024/2): { denjinmk_scrollram16[2] = seibu_vregs[offset]; break; }
		case (0x026/2): { denjinmk_scrollram16[3] = seibu_vregs[offset]; break; }
		case (0x028/2): { denjinmk_scrollram16[4] = seibu_vregs[offset]; break; }
		case (0x02a/2): { denjinmk_scrollram16[5] = seibu_vregs[offset]; break; }
		default: { log_cb(RETRO_LOG_DEBUG, LOGPRE "seibu_common_video_regs_w unhandled offset %02x %04x\n",offset*2,data); break; }
	}
}


/*
"The area of ASM snippets"

player-1 priorities list:
1086d8: show this sprite (bit 15)
1086dc: lives (BCD,bits 3,2,1,0)
1086de: energy bar (upper byte)
1086e0: walk animation (lower byte)
1086ec: "death" status (bit 15)
1086f4: sprite y axis
1086f0: sprite x axis

Sprite DMA TODO:
- various bits not yet understood in the sprite src tables and in the 0x400/0x402 sprite param;

spriteram DMA [1]
001DE4: 3086                     move.w  D6, (A0) ;$100400,color + other stuff
001DE6: 2440                     movea.l D0, A2
001DE8: 0269 0004 0002           andi.w  #$4, ($2,A1)
001DEE: 3152 000C                move.w  (A2), ($c,A0) ;DMA size
001DF2: 3145 0002                move.w  D5, ($2,A0)
001DF6: 0245 0040                andi.w  #$40, D5
001DFA: 2009                     move.l  A1, D0
001DFC: 3140 00C0                move.w  D0, ($c0,A0) ;RAM -> $1004c0 (work ram index?)
001E00: 4840                     swap    D0
001E02: 3140 00A0                move.w  D0, ($a0,A0) ;RAM -> $1004a0
001E06: 200A                     move.l  A2, D0
001E08: 3140 0014                move.w  D0, ($14,A0) ;$ROM lo -> $100414 src
001E0C: 4840                     swap    D0
001E0E: 3140 0012                move.w  D0, ($12,A0) ;$ROM hi -> $100412
001E12: 2679 0010 8116           movea.l $108116.l, A3 ;points to dst spriteram
001E18: 3839 0010 810A           move.w  $10810a.l, D4 ;spriteram index
001E1E: 260B                     move.l  A3, D3
001E20: 3143 00C8                move.w  D3, ($c8,A0) ;sets the dst spriteram
001E24: 4843                     swap    D3
001E26: 3143 00A8                move.w  D3, ($a8,A0)
001E2A: 45EA 0004                lea     ($4,A2), A2
//at this point we're ready for DMAing
001E2E: 317C A180 0100           move.w  #$a180, ($100,A0) ;<-get x/y from sprite
001E34: 317C 6980 0102           move.w  #$6980, ($102,A0) ;<-adjust sprite x/y
001E3A: 317C C480 0102           move.w  #$c480, ($102,A0) ;<-load sprite offset
001E40: 317C 0000 0010           move.w  #$0, ($10,A0)     ;<-do the job?
001E46: 302A 0002                move.w  ($2,A2), D0
001E4A: 816B 0006                or.w    D0, ($6,A3)
001E4E: 45EA 0006                lea     ($6,A2), A2
001E52: 302B 0008                move.w  ($8,A3), D0
001E56: B079 0010 8112           cmp.w   $108112.l, D0
001E5C: 6E00 0054                bgt     $1eb2
001E60: B079 0010 8110           cmp.w   $108110.l, D0
001E66: 6D00 004A                blt     $1eb2
001E6A: 026B 7FFF 000A           andi.w  #$7fff, ($a,A3)
001E70: 8B6B 0004                or.w    D5, ($4,A3)
001E74: 47EB 0008                lea     ($8,A3), A3
001E78: 260B                     move.l  A3, D3
001E7A: 3143 00C8                move.w  D3, ($c8,A0)
001E7E: 4843                     swap    D3
001E80: 3143 00A8                move.w  D3, ($a8,A0)
001E84: 5244                     addq.w  #1, D4
001E86: B879 0010 8114           cmp.w   $108114.l, D4
001E8C: 6500 000C                bcs     $1e9a
001E90: 0069 0002 0002           ori.w   #$2, ($2,A1)
001E96: 6000 000C                bra     $1ea4
001E9A: 3028 01B0                move.w  ($1b0,A0), D0 ;bit 1 = DMA job finished
001E9E: 0240 0002                andi.w  #$2, D0
001EA2: 6790                     beq     $1e34
001EA4: 33C4 0010 810A           move.w  D4, $10810a.l
001EAA: 23CB 0010 8116           move.l  A3, $108116.l
001EB0: 4E75                     rts

x/y check [2]
002030: E58D                     lsl.l   #2, D5
002032: 0685 0003 0000           addi.l  #$30000, D5
002038: 33C5 0010 04C4           move.w  D5, $1004c4.l
00203E: 4845                     swap    D5
002040: 33C5 0010 04A4           move.w  D5, $1004a4.l
002046: E58E                     lsl.l   #2, D6
002048: 0686 0003 0000           addi.l  #$30000, D6
00204E: 33C6 0010 04C6           move.w  D6, $1004c6.l
002054: 4846                     swap    D6
002056: 33C6 0010 04A6           move.w  D6, $1004a6.l
00205C: 33FC A180 0010 0500      move.w  #$a180, $100500.l
002064: 33FC B100 0010 0500      move.w  #$b100, $100500.l
00206C: 33FC A980 0010 0500      move.w  #$a980, $100500.l
002074: 33FC B900 0010 0500      move.w  #$b900, $100500.l
00207C: 4E75                     rts
[...]
//then reads at $580

sine cosine has a weird math problem, it needs that the amp is multiplied by two when the direction is TOTALLY left or TOTALLY up.
No known explaination to this so far ...

003306: move.w  #$8100, ($100,A0)
00330C: move.w  #$8900, ($100,A0)
003312: cmpi.w  #$80, ($36,A1) ;checks if angle is equal to 0x80 (left direction of objects)
003318: bne     $332a
00331C: move.l  ($14,A1), D0 ;divide by two if so
003320: asr.l   #1, D0
003322: move.l  D0, ($14,A1)
003326: bra     $333e
00332A: cmpi.w  #$c0, ($36,A1) ;checks if angle is equal to 0xc0 (up direction of objects)
003330: bne     $333e
003334: move.l  ($10,A1), D0 ;divide by two if so
003338: asr.l   #1, D0
00333A: move.l  D0, ($10,A1)
00333E: movem.l (A7)+, D0/A0-A1
003342: rts

*/


/* Generic COP functions
  -- the game specific handlers fall through to these if there
     isn't a specific case for them.  these implement behavior
     which seems common to all the agmes
*/

static UINT16 cop_status,cop_dist,cop_angle;
static UINT16 cop_hit_status;
static INT16 cop_hit_val_x,cop_hit_val_y,cop_hit_val_z,cop_hit_val_unk;
static UINT32 cop_sort_lookup,cop_sort_ram_addr,cop_sort_param;
static INT8 cop_angle_compare;
static UINT8 cop_angle_mod_val;
static struct
{
	int x,y;
	INT16 min_x,min_y,max_x,max_y;
	UINT16 hitbox;
	UINT16 hitbox_x,hitbox_y;
}cop_collision_info[2];
static int r0, r1;

/* RE from Seibu Cup Soccer bootleg */
static const UINT8 fade_table(int v)
{
    int low  = v & 0x001f;
    int high = v & 0x03e0;

    return (low * (high | (high >> 5)) + 0x210) >> 10;
}

static UINT16 u1,u2;

#define COP_CMD(_1_,_2_,_3_,_4_,_5_,_6_,_7_,_8_,_u1_,_u2_) \
	(copd2_table[command+0] == _1_ && copd2_table[command+1] == _2_ && copd2_table[command+2] == _3_ && copd2_table[command+3] == _4_ && \
	copd2_table[command+4] == _5_ && copd2_table[command+5] == _6_ && copd2_table[command+6] == _7_ && copd2_table[command+7] == _8_ && \
	u1 == _u1_ && u2 == _u2_) \

static void cop_take_hit_box_params(UINT8 offs)
{
	INT16 start_x,start_y,end_x,end_y;

	end_y = (cop_collision_info[offs].hitbox_y >> 8);
	start_y = (cop_collision_info[offs].hitbox_y);
	end_x = (cop_collision_info[offs].hitbox_x >> 8);
	start_x = (cop_collision_info[offs].hitbox_x);

	cop_collision_info[offs].min_x = start_x + (cop_collision_info[offs].x >> 16);
	cop_collision_info[offs].min_y = start_y + (cop_collision_info[offs].y >> 16);
	cop_collision_info[offs].max_x = end_x + (cop_collision_info[offs].x >> 16);
	cop_collision_info[offs].max_y = end_y + (cop_collision_info[offs].y >> 16);
}

static UINT8 cop_calculate_collsion_detection(void)
{
	static UINT8 res;

	res = 3;

	/* outbound X check */
	if(cop_collision_info[0].max_x >= cop_collision_info[1].min_x && cop_collision_info[0].min_x <= cop_collision_info[1].max_x)
		res &= ~2;

	/* outbound Y check */
	if(cop_collision_info[0].max_y >= cop_collision_info[1].min_y && cop_collision_info[0].min_y <= cop_collision_info[1].max_y)
		res &= ~1;

	cop_hit_val_x = (cop_collision_info[0].x - cop_collision_info[1].x) >> 16;
	cop_hit_val_y = (cop_collision_info[0].y - cop_collision_info[1].y) >> 16;
	cop_hit_val_z = 1;
	cop_hit_val_unk = res; /* TODO: there's also bit 2 and 3 triggered in the tests, no known meaning*/


	/*popmessage("%d %d %04x %04x %04x %04x",cop_hit_val_x,cop_hit_val_y,cop_collision_info[0].hitbox_x,cop_collision_info[0].hitbox_y,cop_collision_info[1].hitbox_x,cop_collision_info[1].hitbox_y);*/

	/*if(res == 0)*/
	/*popmessage("0:%08x %08x %08x 1:%08x %08x %08x\n",cop_collision_info[0].x,cop_collision_info[0].y,cop_collision_info[0].hitbox,cop_collision_info[1].x,cop_collision_info[1].y,cop_collision_info[1].hitbox);*/

	return res;
}

static READ16_HANDLER( generic_cop_r )
{
	UINT16 retvalue;
	retvalue = cop_mcu_ram[offset];


	switch (offset)
	{
		/* RNG max value readback, trusted */
		case 0x02c/2:
			return retvalue;

		/* DMA mode register readback, trusted */
		case 0x07e/2:
			return retvalue;

		case 0x180/2:
			return cop_hit_status;

		/* these two controls facing direction in Godzilla opponents (only vs.) - x value compare? */
		case 0x182/2:
			return (cop_hit_val_y);

		case 0x184/2:
			return (cop_hit_val_x);

		/* Legionnaire only - z value compare? */
		case 0x186/2:
			return (cop_hit_val_z);

		case 0x188/2:
			return cop_hit_val_unk;

		/* BCD */
		case 0x190/2:
		case 0x192/2:
		case 0x194/2:
		case 0x196/2:
		case 0x198/2:
			return retvalue;

		/* RNG, trusted */
		case 0x1a0/2:
		case 0x1a2/2:
		case 0x1a4/2:
		case 0x1a6/2:
			return (activecpu_gettotalcycles() % (cop_rng_max_value+1));

		case 0x1b0/2:
			return cop_status;

		case 0x1b2/2:
			return cop_dist;

		case 0x1b4/2:
			return cop_angle;

		default:
			seibu_cop_log("%06x: COPX unhandled read returning %04x from offset %04x\n", activecpu_get_pc(), retvalue, offset*2);
			return retvalue;
	}
}

static UINT32 fill_val;
static UINT8 pal_brightness_val,pal_brightness_mode;
static UINT32 cop_sprite_dma_src;
static int cop_sprite_dma_abs_x,cop_sprite_dma_abs_y,cop_sprite_dma_size;
static UINT32 cop_sprite_dma_param;

static WRITE16_HANDLER( generic_cop_w )
{
	UINT32 temp32;

	switch (offset)
	{
		default:
			seibu_cop_log("%06x: COPX unhandled write data %04x at offset %04x\n", activecpu_get_pc(), data, offset*2);
			break;

		/* Sprite DMA */
		case (0x000/2):
		case (0x002/2):
			cop_sprite_dma_param = (cop_mcu_ram[0x000/2]) | (cop_mcu_ram[0x002/2] << 16);
			/*popmessage("%08x",cop_sprite_dma_param & 0xffffffc0);*/
			break;

		case (0x00c/2): { cop_sprite_dma_size = cop_mcu_ram[offset]; break; }
		case (0x010/2):
		{
			if(data)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Warning: COP RAM 0x410 used with %04x\n",data);
			else
			{
				/* guess */
				cop_register[4]+=8;
				cop_sprite_dma_src+=6;

				cop_sprite_dma_size--;

				if(cop_sprite_dma_size > 0)
					cop_status &= ~2;
				else
					cop_status |= 2;
			}
			break;
		}

		case (0x012/2):
		case (0x014/2):
			cop_sprite_dma_src = (cop_mcu_ram[0x014/2]) | (cop_mcu_ram[0x012/2] << 16);
			break;

		/* triggered before 0x6200 in Seibu Cup, looks like an angle value ... */
		case (0x01c/2): cop_angle_compare = cop_mcu_ram[0x1c/2] & 0xff;	break;
		case (0x01e/2): cop_angle_mod_val = cop_mcu_ram[0x1e/2] & 0xff; break;

		case (0x08c/2): cop_sprite_dma_abs_y = (cop_mcu_ram[0x08c/2]); break;
		case (0x08e/2): cop_sprite_dma_abs_x = (cop_mcu_ram[0x08e/2]); break;

		/* BCD Protection */
		case (0x020/2):
		case (0x022/2):
			temp32 = (cop_mcu_ram[0x020/2]) | (cop_mcu_ram[0x022/2] << 16);
			cop_mcu_ram[0x190/2] = (((temp32 / 1) % 10) + (((temp32 / 10) % 10) << 8) + 0x3030);
			cop_mcu_ram[0x192/2] = (((temp32 / 100) % 10) + (((temp32 / 1000) % 10) << 8) + 0x3030);
			cop_mcu_ram[0x194/2] = (((temp32 / 10000) % 10) + (((temp32 / 100000) % 10) << 8) + 0x3030);
			cop_mcu_ram[0x196/2] = (((temp32 / 1000000) % 10) + (((temp32 / 10000000) % 10) << 8) + 0x3030);
			cop_mcu_ram[0x198/2] = (((temp32 / 100000000) % 10) + (((temp32 / 1000000000) % 10) << 8) + 0x3030);
			break;
		case (0x024/2):
			/*
            This looks like a register for the BCD...
            Godzilla and Heated Barrel sets 3
            Denjin Makai sets 3 at start-up and toggles between 2 and 3 during gameplay on the BCD subroutine
            SD Gundam sets 0
            */
			break;

		case (0x028/2):
		case (0x02a/2):
			fill_val = (cop_mcu_ram[0x028/2]) | (cop_mcu_ram[0x02a/2] << 16);
			break;

		/* Command tables for 0x500 / 0x502 commands */
		case (0x032/2): { copd2_set_tabledata(data); break; }
		case (0x034/2): { copd2_set_tableoffset(data); break; }
		case (0x038/2):	{ cop_438 = data; break; }
		case (0x03a/2):	{ cop_43a = data; break; }
		case (0x03c/2): { cop_43c = data; break; }
		case (0x03e/2):
			/*
            0 in all 68k based games
            0xffff in raiden2 / raidendx
            0x2000 in zeroteam / xsedae
            it's always setted up just before the 0x474 register
            */
			break;

		/* brightness control */
		case (0x05a/2): pal_brightness_val = data & 0xff; break;
		case (0x05c/2): pal_brightness_mode = data & 0xff; break;

		/* DMA / layer clearing section */
		case (0x074/2):
			/*
            This sets up a DMA mode of some sort
                0x0e00: grainbow, cupsoc
                0x0a00: legionna, godzilla, denjinmk
                0x0600: heatbrl
                0x1e00: zeroteam, xsedae
            raiden2 and raidendx doesn't set this up, this could indicate that this is related to the non-private buffer DMAs
            (both only uses 0x14 and 0x15 as DMAs)
            */
			break;

		/* used in palette DMAs, for fading effects */
		case (0x076/2):
			cop_dma_fade_table = data;
			break;

		case (0x078/2): /* DMA source address */
		{
			cop_dma_src[cop_dma_trigger] = data; /* << 6 to get actual address*/
			/*seibu_cop_log("%06x: COPX set layer clear address to %04x (actual %08x)\n", cpu_get_pc(space->cpu), data, data<<6);*/
			break;
		}

		case (0x07a/2): /* DMA length */
		{
			cop_dma_size[cop_dma_trigger] = data;
			/*seibu_cop_log("%06x: COPX set layer clear length to %04x (actual %08x)\n", cpu_get_pc(space->cpu), data, data<<5);*/
			break;
		}

		case (0x07c/2): /* DMA destination */
		{
			cop_dma_dst[cop_dma_trigger] = data;
			/*seibu_cop_log("%06x: COPX set layer clear value to %04x (actual %08x)\n", cpu_get_pc(space->cpu), data, data<<6);*/
			break;
		}

		case (0x07e/2): /* DMA parameter */
		{
			cop_dma_trigger = data;
			/*seibu_cop_log("%06x: COPX set layer clear trigger? to %04x\n", cpu_get_pc(space->cpu), data);*/
			if (data>=0x1ff)
			{
				seibu_cop_log("invalid DMA trigger!, >0x1ff\n");
				cop_dma_trigger = 0;
			}

			break;
		}

		/* max possible value returned by the RNG at 0x5a*, trusted */
		case (0x02c/2): cop_rng_max_value = cop_mcu_ram[0x2c/2] & 0xff; break;

		case (0x044/2):
		{
			cop_scale = data & 3;
			break;
		}

		/* Registers */
		case (0x0a0/2): { cop_register[0] = (cop_register[0]&0x0000ffff)|(cop_mcu_ram[offset]<<16); break; }
		case (0x0c0/2): { cop_register[0] = (cop_register[0]&0xffff0000)|(cop_mcu_ram[offset]<<0);  break; }

		case (0x0a2/2): { cop_register[1] = (cop_register[1]&0x0000ffff)|(cop_mcu_ram[offset]<<16); break; }
		case (0x0c2/2): { cop_register[1] = (cop_register[1]&0xffff0000)|(cop_mcu_ram[offset]<<0);  break; }

		case (0x0a4/2): { cop_register[2] = (cop_register[2]&0x0000ffff)|(cop_mcu_ram[offset]<<16); break; }
		case (0x0c4/2): { cop_register[2] = (cop_register[2]&0xffff0000)|(cop_mcu_ram[offset]<<0);  break; }

		case (0x0a6/2): { cop_register[3] = (cop_register[3]&0x0000ffff)|(cop_mcu_ram[offset]<<16); break; }
		case (0x0c6/2): { cop_register[3] = (cop_register[3]&0xffff0000)|(cop_mcu_ram[offset]<<0);  break; }

		case (0x0a8/2): { cop_register[4] = (cop_register[4]&0x0000ffff)|(cop_mcu_ram[offset]<<16); break; }
		case (0x0c8/2): { cop_register[4] = (cop_register[4]&0xffff0000)|(cop_mcu_ram[offset]<<0);  break; }

		case (0x0aa/2): { cop_register[5] = (cop_register[5]&0x0000ffff)|(cop_mcu_ram[offset]<<16); break; }
		case (0x0ca/2): { cop_register[5] = (cop_register[5]&0xffff0000)|(cop_mcu_ram[offset]<<0);  break; }

		case (0x0ac/2): { cop_register[6] = (cop_register[6]&0x0000ffff)|(cop_mcu_ram[offset]<<16); break; }
		case (0x0cc/2): { cop_register[6] = (cop_register[6]&0xffff0000)|(cop_mcu_ram[offset]<<0);  break; }


		case (0x100/2):
		case (0x102/2):
		case (0x104/2):
		{
			int i;
			int command;

			#if LOG_CMDS
			seibu_cop_log("%06x: COPX execute table macro command %04x %04x | regs %08x %08x %08x %08x %08x\n", activecpu_get_pc(), data, cop_mcu_ram[offset], cop_register[0], cop_register[1], cop_register[2], cop_register[3], cop_register[4]);
			#endif

			command = -1;
			/* search the uploaded 'trigger' table for a matching trigger*/
			/* note, I don't know what the 'mask' or 'value' tables are... probably important, might determine what actually gets executed! */
			/* note: Zero Team triggers macro 0x904 instead of 0x905, Seibu Cup Soccer triggers 0xe30e instead of 0xe38e. I highly doubt that AT LEAST
               it isn't supposed to do anything, especially in the former case (it definitely NEED that sprites have an ark movement when they are knocked down). */
			for (i=0;i<32;i++)
			{
				if ((cop_mcu_ram[offset] & 0xff00) == (copd2_table_4[i] & 0xff00))
				{
					#if LOG_CMDS
					seibu_cop_log("    Cop Command %04x found in slot %02x with other params %04x %04x\n", cop_mcu_ram[offset], i, copd2_table_2[i], copd2_table_3[i]);
					#endif

					u1 = copd2_table_2[i] & 0x000f;
					u2 = copd2_table_3[i] & 0xffff;
					command = i;
				}
			}

			if (command==-1)
			{
				seibu_cop_log("    Cop Command %04x NOT IN TABLE!\n", cop_mcu_ram[offset]);
				break;
			}
			else
			{
				command*=0x8;
				#if LOG_CMDS
				{
					int j;
					seibu_cop_log("     Sequence: ");
					for (j=0;j<0x8;j++)
					{
						seibu_cop_log("%04x ", copd2_table[command+j]);
					}
					seibu_cop_log("\n");
				}
				#endif
			}

			/*printf("%04x %04x %04x\n",cop_mcu_ram[offset],u1,u2);*/

			/*
            Macro notes:
            - endianess changes from/to Raiden 2:
              dword ^= 0
              word ^= 2
              byte ^= 3
            - some macro commands here have a commented algorythm, it's how Seibu Cup Bootleg version handles maths inside the 14/15 roms.
              The ROMs map tables in the following arrangement:
              0x00000 - 0x1ffff Sine math results
              0x20000 - 0x3ffff Cosine math results
              0x40000 - 0x7ffff Division math results
              0x80000 - 0xfffff Pythagorean theorem, hypotenuse length math results
              Surprisingly atan maths are nowhere to be found from the roms.
            */

			/* "automatic" movement */
			if(COP_CMD(0x188,0x282,0x082,0xb8e,0x98e,0x000,0x000,0x000,6,0xffeb))
			{
				UINT8 offs;

				offs = (offset & 3) * 4;

				/* TODO: 0x1c operation? */

				cpu_writemem32bedw_dword(cop_register[0] + 0x04 + offs, cpu_readmem32bedw_dword(cop_register[0] + 0x04 + offs) + cpu_readmem32bedw_dword(cop_register[0] + 0x10 + offs));
				return;
			}

			/* "automatic" movement, for arks in Legionnaire / Zero Team (expression adjustment) */
			if(COP_CMD(0x194,0x288,0x088,0x000,0x000,0x000,0x000,0x000,6,0xfbfb))
			{
				UINT8 offs;

				offs = (offset & 3) * 4;

				/* read 0x28 + offs */
				/* add 0x10 + offs */
				/* write 0x10 + offs */

				cpu_writemem32bedw_dword(cop_register[0] + 0x10 + offs, cpu_readmem32bedw_dword(cop_register[0] + 0x10 + offs) + cpu_readmem32bedw_dword(cop_register[0] + 0x28 + offs));
				return;
			}

			/* SINE math - 0x8100 */
			/*
                 00000-0ffff:
                   amp = x/256
                   ang = x & 255
                   s = sin(ang*2*pi/256)
                   val = trunc(s*amp)
                   if(s<0)
                     val--
                   if(s == 192)
                     val = -2*amp
            */
			if(COP_CMD(0xb9a,0xb88,0x888,0x000,0x000,0x000,0x000,0x000,7,0xfdfb))
			{
				int raw_angle = (cpu_readmem24bew_word(cop_register[0]+(0x34^2)) & 0xff);
				double angle = raw_angle * 128.0 / 3.1415926535897932384626433832795;
				double amp = (65536 >> 5)*(cpu_readmem24bew_word(cop_register[0]+(0x36^2)) & 0xff);
                int res;
                /* TODO: left direction, why? */
                if(raw_angle == 0xc0)
                amp*=2;
                res = ((int)(amp*sin(angle)) << cop_scale);
                cpu_writemem32bedw_dword(cop_register[0] + 10, res);
                return;

            return;
			}

			/* COSINE math - 0x8900 */
			/*
             10000-1ffff:
               amp = x/256
               ang = x & 255
               s = cos(ang*2*pi/256)
               val = trunc(s*amp)
               if(s<0)
                 val--
               if(s == 128)
                 val = -2*amp
            */
			if(COP_CMD(0xb9a,0xb8a,0x88a,0x000,0x000,0x000,0x000,0x000,7,0xfdfb))
			{ 
				int raw_angle = (cpu_readmem24bew_word(cop_register[0]+(0x34^2)) & 0xff);
				double angle = raw_angle * 128.0 / 3.1415926535897932384626433832795;
			    double amp = (65536 >> 5)*(cpu_readmem24bew_word(cop_register[0]+(0x36^2)) & 0xff);
                int res;

                /* TODO: left direction, why? */
                if(raw_angle == 0x80)
                amp*=2;
				res = ((int)(amp*cos(angle)) << cop_scale);
                cpu_writemem32bedw_dword(cop_register[0] + 20, res);
                return;
			}

			/* 0x130e / 0x138e */
			if(COP_CMD(0x984,0xaa4,0xd82,0xaa2,0x39b,0xb9a,0xb9a,0xa9a,5,0xbf7f))
			{
				int dy = cpu_readmem32bedw_dword(cop_register[1]+4) - cpu_readmem32bedw_dword(cop_register[0]+4);
				int dx = cpu_readmem32bedw_dword(cop_register[1]+8) - cpu_readmem32bedw_dword(cop_register[0]+8);

				cop_status = 7;
				if(!dx) {
					cop_status |= 0x8000;
					cop_angle = 0;
				} else {
                     cop_angle = (atan((double)dy / ((double)dx) * 128.0)) / 3.1415926535897932384626433832795;
					if(dx<0)
						cop_angle += 0x80;
				}

				/*printf("%d %d %f %04x\n",dx,dy,atan(double(dy)/double(dx)) * 128 / M_PI,cop_angle);*/

				if(cop_mcu_ram[offset] & 0x80)
					cpu_writemem24bew_word(cop_register[0]+(0x34^2), cop_angle);
				return;
			}

			/* Pythagorean theorem, hypotenuse direction - 130e / 138e */
			/*(heatbrl)  | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a b9a*/
			if(COP_CMD(0x984,0xaa4,0xd82,0xaa2,0x39b,0xb9a,0xb9a,0xb9a,5,0xbf7f))
			{
				int dy = cpu_readmem32bedw_dword(cop_register[1]+4) - cpu_readmem32bedw_dword(cop_register[0]+4);
				int dx = cpu_readmem32bedw_dword(cop_register[1]+8) - cpu_readmem32bedw_dword(cop_register[0]+8);

				cop_status = 7;
				if(!dx) {
					cop_status |= 0x8000;
					cop_angle = 0;
				} else {
                    cop_angle = (atan((double)dy / ((double)dx) * 128.0)) / 3.1415926535897932384626433832795;
					if(dx<0)
						cop_angle += 0x80;
				}

				r0 = dy;
				r1 = dx;

				if(cop_mcu_ram[offset] & 0x80)
					cpu_writemem24bew_word(cop_register[0]+(0x34^2), cop_angle);
				return;
			}

			/* Pythagorean theorem, hypotenuse length - 0x3bb0 */
			/*(grainbow) | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c*/
			/*
             40000-7ffff:
               v1 = (x / 32768)*64
               v2 = (x & 255)*32767/255
               val = sqrt(v1*v1+v2*v2) (unsigned)
            */
			if(COP_CMD(0xf9c,0xb9c,0xb9c,0xb9c,0xb9c,0xb9c,0xb9c,0x99c,4,0x007f))
			{
				int dy = r0;
				int dx = r1;

				dx = dx >> 16;
				dy = dy >> 16;
				cop_dist = sqrt((double)(dx*dx+dy*dy));

				if(cop_mcu_ram[offset] & 0x80)
					cpu_writemem24bew_word(cop_register[0]+(0x38^2), cop_dist);
				return;
			}

			/* Division - 0x42c2 */
			/*
             20000-2ffff:
               v1 = x / 1024
               v2 = x & 1023
               val = !v1 ? 32767 : trunc(v2/v1+0.5)
             30000-3ffff:
               v1 = x / 1024
               v2 = (x & 1023)*32
               val = !v1 ? 32767 : trunc(v2/v1+0.5)
            */
            /* TODO: this is WRONG! */
			if(COP_CMD(0xf9a,0xb9a,0xb9c,0xb9c,0xb9c,0x29c,0x000,0x000,5,0xfcdd))
			{
				int div = cpu_readmem24bew_word(cop_register[0]+(0x36^2));
				int res;

				if(!div)
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "divide by zero?\n");
					div = 1;
				}

				res = cpu_readmem24bew_word(cop_register[0]+(0x38^2)) / div;
				res <<= cop_scale + 2; /* TODO: check this */

				cpu_writemem24bew_word(cop_register[0]+(0x38^2), res);
				return;
			}

			/*
                collision detection:

                int dy_0 = cpu_readmem24bew_dword(cop_register[0]+4);
                int dx_0 = cpu_readmem24bew_dword(cop_register[0]+8);
                int dy_1 = cpu_readmem24bew_dword(cop_register[1]+4);
                int dx_1 = cpu_readmem24bew_dword(cop_register[1]+8);
                int hitbox_param1 = cpu_readmem24bew_dword(cop_register[2]);
                int hitbox_param2 = cpu_readmem24bew_dword(cop_register[3]);

                TODO: we are ignoring the u1 / u2 params for now
            */

			if(COP_CMD(0xb80,0xb82,0xb84,0xb86,0x000,0x000,0x000,0x000,u1,u2))
			{
				cop_collision_info[0].y = (cpu_readmem32bedw_dword(cop_register[0]+4));
				cop_collision_info[0].x = (cpu_readmem32bedw_dword(cop_register[0]+8));
				return;
			}

			/*(heatbrl)  | 9 | ffff | b080 | b40 bc0 bc2*/
			if(COP_CMD(0xb40,0xbc0,0xbc2,0x000,0x000,0x000,0x000,0x000,u1,u2))
			{
				cop_collision_info[0].hitbox = cpu_readmem24bew_word(cop_register[2]);
				cop_collision_info[0].hitbox_y = cpu_readmem24bew_word((cop_register[2]&0xffff0000)|(cop_collision_info[0].hitbox));
				cop_collision_info[0].hitbox_x = cpu_readmem24bew_word(((cop_register[2]&0xffff0000)|(cop_collision_info[0].hitbox))+2);

				/* do the math */
				cop_take_hit_box_params(0);
				cop_hit_status = cop_calculate_collsion_detection();

				return;
			}

			if(COP_CMD(0xba0,0xba2,0xba4,0xba6,0x000,0x000,0x000,0x000,u1,u2))
			{
				cop_collision_info[1].y = (cpu_readmem32bedw_dword(cop_register[1]+4));
				cop_collision_info[1].x = (cpu_readmem32bedw_dword(cop_register[1]+8));
				return;
			}

			/*(heatbrl)  | 6 | ffff | b880 | b60 be0 be2*/
			if(COP_CMD(0xb60,0xbe0,0xbe2,0x000,0x000,0x000,0x000,0x000,u1,u2))
			{
				cop_collision_info[1].hitbox = cpu_readmem24bew_word(cop_register[3]);
				cop_collision_info[1].hitbox_y = cpu_readmem24bew_word((cop_register[3]&0xffff0000)|(cop_collision_info[1].hitbox));
				cop_collision_info[1].hitbox_x = cpu_readmem24bew_word(((cop_register[3]&0xffff0000)|(cop_collision_info[1].hitbox))+2);

				/* do the math */
				cop_take_hit_box_params(1);
				cop_hit_status = cop_calculate_collsion_detection();
				return;
			}

			/* grainbow 0d | a | fff3 | 6980 | b80 ba0*/
			if(COP_CMD(0xb80,0xba0,0x000,0x000,0x000,0x000,0x000,0x000,10,0xfff3))
			{
				UINT8 offs;
				int abs_x,abs_y,rel_xy;

				offs = (offset & 3) * 4;

				/* TODO: I really suspect that following two are actually taken from the 0xa180 macro command then internally loaded */
				abs_x = cpu_readmem24bew_word(cop_register[0] + 8) - cop_sprite_dma_abs_x;
				abs_y = cpu_readmem24bew_word(cop_register[0] + 4) - cop_sprite_dma_abs_y;
				rel_xy = cpu_readmem24bew_word(cop_sprite_dma_src + 4 + offs);

				/*if(rel_xy & 0x0706)*/
				/*  log_cb(RETRO_LOG_DEBUG, LOGPRE "sprite rel_xy = %04x\n",rel_xy);*/

				if(rel_xy & 1)
					cpu_writemem24bew_word(cop_register[4] + offs + 4,0xc0 + abs_x - (rel_xy & 0xf8));
				else
					cpu_writemem24bew_word(cop_register[4] + offs + 4,(((rel_xy & 0x78) + (abs_x) - ((rel_xy & 0x80) ? 0x80 : 0))));

				cpu_writemem24bew_word(cop_register[4] + offs + 6,(((rel_xy & 0x7800) >> 8) + (abs_y) - ((rel_xy & 0x8000) ? 0x80 : 0)));
				return;
			}

			/* grainbow 18 | a | ff00 | c480 | 080 882*/
			if(COP_CMD(0x080,0x882,0x000,0x000,0x000,0x000,0x000,0x000,10,0xff00))
			{
				UINT8 offs;

				offs = (offset & 3) * 4;

				cpu_writemem24bew_word(cop_register[4] + offs + 0,cpu_readmem24bew_word(cop_sprite_dma_src + offs) + (cop_sprite_dma_param & 0x3f));
				/*cpu_writemem24bew_word(cop_register[4] + offs + 2,cpu_readmem24bew_word(cop_sprite_dma_src+2 + offs));*/
				return;
			}

			/* cupsoc 1b | 5 | 7ff7 | dde5 | f80 aa2 984 0c2*/
			/* radar x/y positions */
			/* FIXME: x/ys are offsetted */
			/* FIXME: uses 0x10044a for something */
			if(COP_CMD(0xf80,0xaa2,0x984,0x0c2,0x000,0x000,0x000,0x000,5,0x7ff7))
			{
				UINT8 offs;
				int div;
/*              INT16 offs_val;*/

				/* TODO: [4-7] could be mirrors of [0-3] (this is the only command so far that uses 4-7 actually)*/
				/* 0 + [4] */
				/* 4 + [5] */
				/* 8 + [4] */
				/* 4 + [6] */

				/*printf("%08x %08x %08x %08x %08x %08x %08x\n",cop_register[0],cop_register[1],cop_register[2],cop_register[3],cop_register[4],cop_register[5],cop_register[6]);*/

				offs = (offset & 3) * 4;

				div = cpu_readmem24bew_word(cop_register[4] + offs) + 1;
/*              offs_val = cpu_readmem24bew_word(cop_register[3] + offs);*/
				/*420 / 180 = 500 : 400 = 30 / 50 = 98 / 18*/

				if(div == 0) { div = 1; }

				cpu_writemem24bew_word((cop_register[6] + offs + 4), ((cpu_readmem24bew_word(cop_register[5] + offs + 4)) / div));
				return;
			}

			/*(cupsoc)   | 8 | f3e7 | 6200 | 3a0 3a6 380 aa0 2a6*/
			if(COP_CMD(0x3a0,0x3a6,0x380,0xaa0,0x2a6,0x000,0x000,0x000,8,0xf3e7))
			{
				INT8 cur_angle;

				/* 0 [1] */
				/* 0xc [1] */
				/* 0 [0] */
				/* 0 [1] */
				/* 0xc [1] */

				cur_angle = cpu_readmem24bew(cop_register[1] + (0xc ^ 3));
				cpu_writemem24bew(cop_register[1] + (0^3),cpu_readmem24bew(cop_register[1] + (0^3)) & 0xfb); /*correct?*/

				if(cur_angle >= cop_angle_compare)
				{
					cur_angle -= cop_angle_mod_val;
					if(cur_angle <= cop_angle_compare)
					{
						cur_angle = cop_angle_compare;
						cpu_writemem24bew(cop_register[1] + (0^3),cpu_readmem24bew(cop_register[1] + (0^3)) | 2);
					}
				}
				else if(cur_angle <= cop_angle_compare)
				{
					cur_angle += cop_angle_mod_val;
					if(cur_angle >= cop_angle_compare)
					{
						cur_angle = cop_angle_compare;
						cpu_writemem24bew(cop_register[1] + (0^3),cpu_readmem24bew(cop_register[1] + (0^3)) | 2);
					}
				}

				cpu_writemem24bew(cop_register[1] + (0xc ^ 3),cur_angle);
				return;
			}

			/*(grainbow) | 8 | f3e7 | 6200 | 380 39a 380 a80 29a*/
			/* search direction, used on SD Gundam homing weapon */
			/* FIXME: still doesn't work ... */
			if(COP_CMD(0x380,0x39a,0x380,0xa80,0x29a,0x000,0x000,0x000,8,0xf3e7))
			{
				INT8 cur_angle;

				cur_angle = cpu_readmem24bew(cop_register[0] + (0x34 ^ 3));
				/*cpu_writemem24bew(cop_register[0] + (0^3),cpu_readmem24bew(cop_register[0] + (0^3)) & 0xfb); */ /*correct?*/

				if(cur_angle >= cop_angle_compare)
				{
					cur_angle -= cop_angle_mod_val;

					if(cur_angle <= cop_angle_compare)
					{
						cur_angle = cop_angle_compare;
						/*cpu_writemem24bew(cop_register[0] + (0^3),cpu_readmem24bew(cop_register[0] + (0^3)) | 2);*/
					}
				}
				else if(cur_angle <= cop_angle_compare)
				{
					cur_angle += cop_angle_mod_val;

					if(cur_angle >= cop_angle_compare)
					{
						cur_angle = cop_angle_compare;
						/*cpu_writemem24bew(cop_register[0] + (0^3),cpu_readmem24bew(cop_register[0] + (0^3)) | 2);*/
					}
				}

				cpu_writemem24bew(cop_register[0] + (0x34 ^ 3),cur_angle);
				return;
			}

			log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x\n",cop_mcu_ram[offset]);
			break;
		}

		/* DMA go register */
		case (0x2fc/2):
		{
			/*seibu_cop_log("%06x: COPX execute current layer clear??? %04x\n", space->device().safe_pc(), data);*/

			if (cop_dma_trigger >= 0x80 && cop_dma_trigger <= 0x87)
			{
				UINT32 src,dst,size,i;

				/*
                Apparently all of those are just different DMA channels, brightness effects are done through a RAM table and the pal_brightness_val / mode
                0x80 is used by Legionnaire
                0x81 is used by SD Gundam and Godzilla
                0x82 is used by Zero Team and X Se Dae
                0x86 is used by Seibu Cup Soccer
                0x87 is used by Denjin Makai

                TODO:
                - Denjin Makai mode 4 is totally guessworked.
                - SD Gundam doesn't fade colors correctly, it should have the text layer / sprites with normal gradient and the rest dimmed in most cases,
                  presumably bad RAM table or bad algorithm
                */

				/*if(dma_trigger != 0x87)*/
				/*printf("SRC: %08x %08x DST:%08x SIZE:%08x TRIGGER: %08x %02x %02x\n",cop_dma_src[cop_dma_trigger] << 6,cop_dma_fade_table * 0x400,cop_dma_dst[cop_dma_trigger] << 6,cop_dma_size[cop_dma_trigger] << 5,cop_dma_trigger,pal_brightness_val,pal_brightness_mode);*/

				src = (cop_dma_src[cop_dma_trigger] << 6);
				dst = (cop_dma_dst[cop_dma_trigger] << 6);
				size = ((cop_dma_size[cop_dma_trigger] << 5) - (cop_dma_dst[cop_dma_trigger] << 6) + 0x20)/2;

				for(i = 0;i < size;i++)
				{
					UINT16 pal_val;
					int r,g,b;
					int rt,gt,bt;

					if(pal_brightness_mode == 5)
					{
						bt = ((cpu_readmem24bew_word(src + (cop_dma_fade_table * 0x400))) & 0x7c00) >> 5;
						bt = fade_table(bt|(pal_brightness_val ^ 0));
						b = ((cpu_readmem24bew_word(src)) & 0x7c00) >> 5;
						b = fade_table(b|(pal_brightness_val ^ 0x1f));
						pal_val = ((b + bt) & 0x1f) << 10;
						gt = ((cpu_readmem24bew_word(src + (cop_dma_fade_table * 0x400))) & 0x03e0);
						gt = fade_table(gt|(pal_brightness_val ^ 0));
						g = ((cpu_readmem24bew_word(src)) & 0x03e0);
						g = fade_table(g|(pal_brightness_val ^ 0x1f));
						pal_val |= ((g + gt) & 0x1f) << 5;
						rt = ((cpu_readmem24bew_word(src + (cop_dma_fade_table * 0x400))) & 0x001f) << 5;
						rt = fade_table(rt|(pal_brightness_val ^ 0));
						r = ((cpu_readmem24bew_word(src)) & 0x001f) << 5;
						r = fade_table(r|(pal_brightness_val ^ 0x1f));
						pal_val |= ((r + rt) & 0x1f);
					}
					else if(pal_brightness_mode == 4) /*Denjin Makai*/
					{
						bt =(cpu_readmem24bew_word(src + (cop_dma_fade_table * 0x400)) & 0x7c00) >> 10;
						b = (cpu_readmem24bew_word(src) & 0x7c00) >> 10;
						gt =(cpu_readmem24bew_word(src + (cop_dma_fade_table * 0x400)) & 0x03e0) >> 5;
						g = (cpu_readmem24bew_word(src) & 0x03e0) >> 5;
						rt =(cpu_readmem24bew_word(src + (cop_dma_fade_table * 0x400)) & 0x001f) >> 0;
						r = (cpu_readmem24bew_word(src) & 0x001f) >> 0;

						if(pal_brightness_val == 0x10)
							pal_val = bt << 10 | gt << 5 | rt << 0;
						else if (pal_brightness_val == 0xffff) /* level transitions*/
				            pal_val = bt << 10 | gt << 5 | rt << 0;
						else
						{
							bt = fade_table(bt<<5|((pal_brightness_val*2) ^ 0));
							b =  fade_table(b<<5|((pal_brightness_val*2) ^ 0x1f));
							pal_val = ((b + bt) & 0x1f) << 10;
							gt = fade_table(gt<<5|((pal_brightness_val*2) ^ 0));
							g =  fade_table(g<<5|((pal_brightness_val*2) ^ 0x1f));
							pal_val |= ((g + gt) & 0x1f) << 5;
							rt = fade_table(rt<<5|((pal_brightness_val*2) ^ 0));
							r =  fade_table(r<<5|((pal_brightness_val*2) ^ 0x1f));
							pal_val |= ((r + rt) & 0x1f);
						}
					}
					else
					{
						log_cb(RETRO_LOG_DEBUG, LOGPRE "Warning: palette DMA used with mode %02x!\n",pal_brightness_mode);
						pal_val = cpu_readmem24bew_word(src);
					}

					cpu_writemem24bew_word(dst, pal_val);
					src+=2;
					dst+=2;
				}

				return;
			}

			/* Seibu Cup Soccer trigger this*/
			if (cop_dma_trigger == 0x0e)
			{
				UINT32 src,dst,size,i;

				src = (cop_dma_src[cop_dma_trigger] << 6);
				dst = (cop_dma_dst[cop_dma_trigger] << 6);
				size = ((cop_dma_size[cop_dma_trigger] << 5) - (cop_dma_dst[cop_dma_trigger] << 6) + 0x20)/2;

				for(i = 0;i < size;i++)
				{
					cpu_writemem24bew_word(dst, cpu_readmem24bew_word(src));
					src+=2;
					dst+=2;
				}

				return;
			}

			/* do the fill  */
			if (cop_dma_trigger >= 0x118 && cop_dma_trigger <= 0x11f)
			{
				UINT32 length, address;
				int i;
				if(cop_dma_dst[cop_dma_trigger] != 0x0000) /* Invalid?*/
					return;

				address = (cop_dma_src[cop_dma_trigger] << 6);
				length = (cop_dma_size[cop_dma_trigger]+1) << 5;

				/*printf("%08x %08x\n",address,length);*/

				for (i=address;i<address+length;i+=4)
				{
					cpu_writemem32bedw_dword(i, fill_val);
				}

				return;
			}

			/* Godzilla specific */
			if (cop_dma_trigger == 0x116)
			{
				UINT32 length, address;
				int i;

				/*if(cop_dma_dst[cop_dma_trigger] != 0x0000) */ /* Invalid?*/
				/*  return;*/

				address = (cop_dma_src[cop_dma_trigger] << 6);
				length = ((cop_dma_size[cop_dma_trigger]+1) << 4);

				for (i=address;i<address+length;i+=4)
				{
					cpu_writemem32bedw_dword(i, fill_val);
				}

				return;
			}

			/* private buffer copies */
			if ((cop_dma_trigger==0x14) || (cop_dma_trigger==0x15)) return;

			log_cb(RETRO_LOG_DEBUG, LOGPRE "SRC: %08x %08x DST:%08x SIZE:%08x TRIGGER: %08x\n",cop_dma_src[cop_dma_trigger] << 6,cop_dma_fade_table,cop_dma_dst[cop_dma_trigger] << 6,cop_dma_size[cop_dma_trigger] << 5,cop_dma_trigger);

			break;
		}

		/* sort-DMA, oh my ... */
		case (0x054/2): { cop_sort_lookup = (cop_sort_lookup&0x0000ffff)|(cop_mcu_ram[offset]<<16); break; }
		case (0x056/2): { cop_sort_lookup = (cop_sort_lookup&0xffff0000)|(cop_mcu_ram[offset]<<0);  break; }
		case (0x050/2): { cop_sort_ram_addr = (cop_sort_ram_addr&0x0000ffff)|(cop_mcu_ram[offset]<<16); break; }
		case (0x052/2): { cop_sort_ram_addr = (cop_sort_ram_addr&0xffff0000)|(cop_mcu_ram[offset]<<0);  break; }
		case (0x058/2): { cop_sort_param = cop_mcu_ram[offset]; break; }

		case (0x2fe/2):
		{
			UINT16 sort_size;

			sort_size = cop_mcu_ram[offset];

			{
				int i,j;
				UINT8 xchg_flag;
				UINT32 addri,addrj;
				UINT16 vali,valj;

				/* TODO: use a better algorithm than bubble sort! */
				for(i=2;i<sort_size;i+=2)
				{
					for(j=i-2;j<sort_size;j+=2)
					{
						addri = cop_sort_ram_addr+cpu_readmem24bew_word(cop_sort_lookup+i);
						addrj = cop_sort_ram_addr+cpu_readmem24bew_word(cop_sort_lookup+j);

						vali = cpu_readmem24bew_word(addri);
						valj = cpu_readmem24bew_word(addrj);

						/*printf("%08x %08x %04x %04x\n",addri,addrj,vali,valj);*/

						switch(cop_sort_param)
						{
							case 2:	xchg_flag = (vali > valj); break;
							case 1: xchg_flag = (vali < valj); break;
							default: xchg_flag = 0; log_cb(RETRO_LOG_DEBUG, LOGPRE "Warning: sort-DMA used with param %02x\n",cop_sort_param); break;
						}

						if(xchg_flag)
						{
							UINT16 xch_val;

							xch_val = cpu_readmem24bew_word(cop_sort_lookup+i);
							cpu_writemem24bew_word(cop_sort_lookup+i,cpu_readmem24bew_word(cop_sort_lookup+j));
							cpu_writemem24bew_word(cop_sort_lookup+j,xch_val);
						}
					}
				}
			}
			/*else*/

			break;
		}
	}
}

/**********************************************************************************************
  Denjin Makai
**********************************************************************************************/

READ16_HANDLER( denjinmk_mcu_r )
{

	if(offset >= 0x300/2 && offset <= 0x31f/2)
		return seibu_main_word_r((offset >> 1) & 7,0xffff);

	switch (offset)
	{
		default:
			return generic_cop_r(offset, mem_mask);

		/* Inputs */
		case (0x340/2): return input_port_1_word_r(0,0);
		case (0x344/2):	return input_port_2_word_r(0,0);
		case (0x348/2): return input_port_4_word_r(0,0);
		case (0x34c/2): return input_port_3_word_r(0,0);
		case (0x35c/2): return input_port_5_word_r(0,0);
		
	}
}

WRITE16_HANDLER( denjinmk_mcu_w )
{
	COMBINE_DATA(&cop_mcu_ram[offset]);
	
	if(offset == 0x280/2) /*irq ack / sprite buffering?*/
		return;


	if(offset == 0x070/2)
	{
		denjinmk_setgfxbank(cop_mcu_ram[offset]);
		return;
	}

	if(offset >= 0x200/2 && offset <= 0x24f/2)
	{
		seibu_common_video_regs_w(offset-0x200/2,cop_mcu_ram[offset],mem_mask);
		return;
	}

	if(offset >= 0x300/2 && offset <= 0x31f/2)
	{
		seibu_main_word_w((offset >> 1) & 7,cop_mcu_ram[offset],0xff00);
		return;
	}

	generic_cop_w(offset, data, mem_mask);
}
