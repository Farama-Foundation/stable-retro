/* Sega Saturn VDP2 */

/*

the dirty marking stuff and tile decoding will probably be removed in the end anyway as we'll need custom
rendering code since mame's drawgfx / tilesytem don't offer everything st-v needs

this system seems far too complex to use Mame's tilemap system

4 'scroll' planes (scroll screens)

the scroll planes have slightly different capabilities

NBG0
NBG1
NBG2
NBG3

2 'rotate' planes

RBG0
RBG1

-- other crap
EXBG (external)

------------------------------------------------------------------------------------------

Notes of Interest:

-the test mode / bios is drawn with layer NBG3

-hanagumi Puts a 'RED' dragon logo in tileram (base 0x64000, 4bpp, 8x8 tiles) but
its not displayed in gurus video.Update:It's actually not drawn because its
priority value is 0.

-scrolling is screen display wise,meaning that a scrolling value is masked with the
screen resolution size values.

-VDP1 "general purpose" priority isn't taken into account yet,for now we fix the priority
value to six...

-AFAIK Suikoenbu & Winter Heat are the only ST-V game that use Mode 2 as the Color RAM
format.

-Bitmaps USES transparency pens,examples are:
elandore's energy bars;
mausuke's foreground(the one used on the playfield)(I guess that this is also
alpha-blended);

for now I've removed black pixels,it isn't 100% right so there MUST BE a better way
for this...

*/

#include "driver.h"

data32_t* stv_vdp2_regs;
data32_t* stv_vdp2_vram;
/* this won't be used in the end .. */
data8_t*  stv_vdp2_vram_dirty_8x8x4;
data8_t*  stv_vdp2_vram_dirty_8x8x8;

data32_t* stv_vdp2_cram;
extern void video_update_vdp1(struct mame_bitmap *bitmap, const struct rectangle *cliprect);
extern int stv_vdp1_start ( void );
static void stv_vdp2_dynamic_res_change(void);

/*

-------------------------------------------------|-----------------------------|------------------------------
|  Function        |  Normal Scroll Screen                                     |  Rotation Scroll Screen     |
|                  |-----------------------------|-----------------------------|------------------------------
|                  | NBG0         | NBG1         | NBG2         | NBG3         | RBG0         | RBG1         |
-------------------------------------------------|-----------------------------|------------------------------
| Character Colour | 16 colours   | 16 colours   | 16 colours   | 16 colours   | 16 colours   | 16 colours   |
| Count            | 256 " "      | 256 " "      | 256 " "      | 256 " "      | 256 " "      | 256 " "      |
|                  | 2048 " "     | 2048 " "     |              |              | 2048 " "     | 2048 " "     |
|                  | 32768 " "    | 32768 " "    |              |              | 32768 " "    | 32768 " "    |
|                  | 16770000 " " |              |              |              | 16770000 " " | 16770000 " " |
-------------------------------------------------|-----------------------------|------------------------------
| Character Size   | 1x1 Cells , 2x2 Cells                                                                   |
-------------------------------------------------|-----------------------------|------------------------------
| Pattern Name     | 1 word , 2 words                                                                        |
| Data Size        |                                                                                         |
-------------------------------------------------|-----------------------------|------------------------------
| Plane Size       | 1 H x 1 V 1 Pages ; 2 H x 1 V 1 Pages ; 2 H x 2 V Pages (I don't understand ... )       |
-------------------------------------------------|-----------------------------|------------------------------
| Plane Count      | 4                                                         | 16                          |
-------------------------------------------------|-----------------------------|------------------------------
| Bitmap Possible  | Yes                         | No                          | Yes          | No           |
-------------------------------------------------|-----------------------------|------------------------------
| Bitmap Size      | 512 x 256                   | N/A                         | 512x256      | N/A          |
|                  | 512 x 512                   |                             | 512x512      |              |
|                  | 1024 x 256                  |                             |              |              |
|                  | 1024 x 512                  |                             |              |              |
-------------------------------------------------|-----------------------------|------------------------------
| Scale            | 0.25 x - 256 x              | None                        | Any ?                       |
-------------------------------------------------|-----------------------------|------------------------------
| Rotation         | No                                                        | Yes                         |
-------------------------------------------------|-----------------------------|-----------------------------|
| Linescroll       | Yes                         | No                                                        |
-------------------------------------------------|-----------------------------|------------------------------
| Column Scroll    | Yes                         | No                                                        |
-------------------------------------------------|-----------------------------|------------------------------
| Mosaic           | Yes                                                       | Horizontal Only             |
-------------------------------------------------|-----------------------------|------------------------------

*/

/* 180000 - r/w - TVMD - TV Screen Mode
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | DISP     |    --    |    --    |    --    |    --    |    --    |    --    | BDCLMD   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | LSMD1    | LSMD0    | VRESO1   | VRESO0   |    --    | HRESO2   | HRESO1   | HRESO0   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_TVMD 	((stv_vdp2_regs[0x000/4] >> 16)&0x0000ffff)

	#define STV_VDP2_BDCLMD	((STV_VDP2_TVMD & 0x0100) >> 8)
	#define STV_VDP2_LSMD 	((STV_VDP2_TVMD & 0x00c0) >> 6)
	#define STV_VDP2_VRES 	((STV_VDP2_TVMD & 0x0030) >> 4)
	#define STV_VDP2_HRES 	((STV_VDP2_TVMD & 0x0007) >> 0)

/* 180002 - r/w - EXTEN - External Signal Enable Register
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    | EXLTEN   | EXSYEN   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    | DASEL    | EXBGEN   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180004 - r/o - TVSTAT - Screen Status
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    | EXLTFG   | EXSYFG   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    | VBLANK   | HBLANK   | ODD      | EVEN     |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180006 - r/w - VRSIZE - VRAM Size
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VRAMSZ   |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    | VER3     | VER2     | VER1     | VER0     |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_VRSIZE ((stv_vdp2_regs[0x004/4] >> 0)&0x0000ffff)

	#define STV_VDP2_VRAMSZ ((STV_VDP2_VRSIZE & 0x8000) >> 15)

/* 180008 - r/o - HCNT - H-Counter
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    | HCT9     | HCT8     |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | HCT7     | HCT6     | HCT5     | HCT4     | HCT3     | HCT2     | HCT1     | HCT0     |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_HCNT ((stv_vdp2_regs[0x008/4] >> 16)&0x000003ff)

/* 18000A - r/o - VCNT - V-Counter
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    | VCT9     | VCT8     |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCT7     | VCT6     | VCT5     | VCT4     | VCT3     | VCT2     | VCT1     | VCT0     |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_VCNT ((stv_vdp2_regs[0x008/4] >> 0)&0x000003ff)

/* 18000C - RESERVED
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18000E - r/w - RAMCTL - RAM Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    | CRMD1    | CRMD0    |    --    |    --    | VRBMD    | VRAMD    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | RDBSB11  | RDBSB10  | RDBSB01  | RDBSB00  | RDBSA11  | RDBSA10  | RDBSA01  | RDBSA00  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_RAMCTL ((stv_vdp2_regs[0x00c/4] >> 0)&0x0000ffff)

	#define STV_VDP2_CRMD ((STV_VDP2_RAMCTL & 0x3000) >> 12)

/* 180010 - r/w - -CYCA0L - VRAM CYCLE PATTERN (BANK A0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP0A03  | VCP0A02  | VCP0A01  | VCP0A00  | VCP1A03  | VCP1A02  | VCP1A01  | VCP1A00  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP2A03  | VCP2A02  | VCP2A01  | VCP2A00  | VCP3A03  | VCP3A02  | VCP3A01  | VCP3A00  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180012 - r/w - -CYCA0U - VRAM CYCLE PATTERN (BANK A0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP4A03  | VCP4A02  | VCP4A01  | VCP4A00  | VCP5A03  | VCP5A02  | VCP5A01  | VCP5A00  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP6A03  | VCP6A02  | VCP6A01  | VCP6A00  | VCP7A03  | VCP7A02  | VCP7A01  | VCP7A00  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180014 - r/w - -CYCA1L - VRAM CYCLE PATTERN (BANK A1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP0A13  | VCP0A12  | VCP0A11  | VCP0A10  | VCP1A13  | VCP1A12  | VCP1A11  | VCP1A10  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP2A13  | VCP2A12  | VCP2A11  | VCP2A10  | VCP3A13  | VCP3A12  | VCP3A11  | VCP3A10  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180016 - r/w - -CYCA1U - VRAM CYCLE PATTERN (BANK A1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP4A13  | VCP4A12  | VCP4A11  | VCP4A10  | VCP5A13  | VCP5A12  | VCP5A11  | VCP5A10  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP6A13  | VCP6A12  | VCP6A11  | VCP6A10  | VCP7A13  | VCP7A12  | VCP7A11  | VCP7A10  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180018 - r/w - -CYCB0L - VRAM CYCLE PATTERN (BANK B0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP0B03  | VCP0B02  | VCP0B01  | VCP0B00  | VCP1B03  | VCP1B02  | VCP1B01  | VCP1B00  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP2B03  | VCP2B02  | VCP2B01  | VCP2B00  | VCP3B03  | VCP3B02  | VCP3B01  | VCP3B00  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18001A - r/w - -CYCB0U - VRAM CYCLE PATTERN (BANK B0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP4B03  | VCP4B02  | VCP4B01  | VCP4B00  | VCP5B03  | VCP5B02  | VCP5B01  | VCP5B00  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP6B03  | VCP6B02  | VCP6B01  | VCP6B00  | VCP7B03  | VCP7B02  | VCP7B01  | VCP7B00  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18001C - r/w - -CYCB1L - VRAM CYCLE PATTERN (BANK B1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP0B13  | VCP0B12  | VCP0B11  | VCP0B10  | VCP1B13  | VCP1B12  | VCP1B11  | VCP1B10  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP2B13  | VCP2B12  | VCP2B11  | VCP2B10  | VCP3B13  | VCP3B12  | VCP3B11  | VCP3B10  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18001E - r/w - -CYCB1U - VRAM CYCLE PATTERN (BANK B1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | VCP4B13  | VCP4B12  | VCP4B11  | VCP4B10  | VCP5B13  | VCP5B12  | VCP5B11  | VCP5B10  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | VCP6B13  | VCP6B12  | VCP6B11  | VCP6B10  | VCP7B13  | VCP7B12  | VCP7B11  | VCP7B10  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180020 - r/w - BGON - SCREEN DISPLAY ENABLE

 this register allows each tilemap to be enabled or disabled and also which layers are solid

 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    | R0TPON   | N3TPON   | N2TPON   | N1TPON   | N0TPON   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    | R1ON     | R0ON     | N3ON     | N2ON     | N1ON     | N0ON     |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_BGON ((stv_vdp2_regs[0x020/4] >> 16)&0x0000ffff)

	/* NxOn - Layer Enable Register*/
	#define STV_VDP2_xxON ((STV_VDP2_BGON & 0x001f) >> 0) /* to see if anything is enabled */

	#define STV_VDP2_N0ON ((STV_VDP2_BGON & 0x0001) >> 0) /* N0On = NBG0 Enable */
	#define STV_VDP2_N1ON ((STV_VDP2_BGON & 0x0002) >> 1) /* N1On = NBG1 Enable */
	#define STV_VDP2_N2ON ((STV_VDP2_BGON & 0x0004) >> 2) /* N2On = NBG2 Enable */
	#define STV_VDP2_N3ON ((STV_VDP2_BGON & 0x0008) >> 3) /* N3On = NBG3 Enable */
	#define STV_VDP2_R0ON ((STV_VDP2_BGON & 0x0010) >> 4) /* R0On = RBG0 Enable */
	#define STV_VDP2_R1ON ((STV_VDP2_BGON & 0x0020) >> 5) /* R1On = RBG1 Enable */

	/* NxTPON - Transparency Pen Enable Registers*/
	#define STV_VDP2_N0TPON ((STV_VDP2_BGON & 0x0100) >> 8) /* 	N0TPON = NBG0 Draw Transparent Pen (as solid) /or/ RBG1 Draw Transparent Pen */
	#define STV_VDP2_N1TPON ((STV_VDP2_BGON & 0x0200) >> 9) /* 	N1TPON = NBG1 Draw Transparent Pen (as solid) /or/ EXBG Draw Transparent Pen */
	#define STV_VDP2_N2TPON ((STV_VDP2_BGON & 0x0400) >> 10)/*  N2TPON = NBG2 Draw Transparent Pen (as solid) */
	#define STV_VDP2_N3TPON ((STV_VDP2_BGON & 0x0800) >> 11)/*  N3TPON = NBG3 Draw Transparent Pen (as solid) */
	#define STV_VDP2_R0TPON ((STV_VDP2_BGON & 0x1000) >> 12)/*  R0TPON = RBG0 Draw Transparent Pen (as solid) */

/*
180022 - MZCTL - Mosaic Control

180024 - Special Function Code Select

180026 - Special Function Code

180028 - CHCTLA - Character Control (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    | N1CHCN1  | N1CHCN0  | N1BMSZ1  | N1BMSZ0  | N1BMEN   | N1CHSZ   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    | N0CHCN2  | N0CHCN1  | N0CHCN0  | N0BMSZ1  | N0BMSZ0  | N0BMEN   | N0CHSZ   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CHCTLA ((stv_vdp2_regs[0x028/4] >> 16)&0x0000ffff)

/* -------------------------- NBG0 Character Control Registers -------------------------- */

/*	N0CHCNx  NBG0 (or RGB1) Colour Depth
	000 - 16 Colours
	001 - 256 Colours
	010 - 2048 Colours
	011 - 32768 Colours (RGB5)
	100 - 16770000 Colours (RGB8)
	101 - invalid
	110 - invalid
	111 - invalid   */
	#define STV_VDP2_N0CHCN ((STV_VDP2_CHCTLA & 0x0070) >> 4)

/*	N0BMSZx - NBG0 Bitmap Size *guessed*
	00 - 512 x 256
	01 - 512 x 512
	10 - 1024 x 256
	11 - 1024 x 512   */
	#define STV_VDP2_N0BMSZ ((STV_VDP2_CHCTLA & 0x000c) >> 2)

/*	N0BMEN - NBG0 Bitmap Enable
	0 - use cell mode
	1 - use bitmap mode   */
	#define STV_VDP2_N0BMEN ((STV_VDP2_CHCTLA & 0x0002) >> 1)

/*	N0CHSZ - NBG0 Character (Tile) Size
	0 - 1 cell  x 1 cell  (8x8)
	1 - 2 cells x 2 cells (16x16)  */
	#define STV_VDP2_N0CHSZ ((STV_VDP2_CHCTLA & 0x0001) >> 0)

/* -------------------------- NBG1 Character Control Registers -------------------------- */

/*	N1CHCNx - NBG1 (or EXB1) Colour Depth
	00 - 16 Colours
	01 - 256 Colours
	10 - 2048 Colours
	11 - 32768 Colours (RGB5)  */
	#define STV_VDP2_N1CHCN ((STV_VDP2_CHCTLA & 0x3000) >> 12)

/*	N1BMSZx - NBG1 Bitmap Size *guessed*
	00 - 512 x 256
	01 - 512 x 512
	10 - 1024 x 256
	11 - 1024 x 512   */
	#define STV_VDP2_N1BMSZ ((STV_VDP2_CHCTLA & 0x0c00) >> 10)

/*	N1BMEN - NBG1 Bitmap Enable
	0 - use cell mode
	1 - use bitmap mode   */
	#define STV_VDP2_N1BMEN ((STV_VDP2_CHCTLA & 0x0200) >> 9)

/*	N1CHSZ - NBG1 Character (Tile) Size
	0 - 1 cell  x 1 cell  (8x8)
	1 - 2 cells x 2 cells (16x16)  */
	#define STV_VDP2_N1CHSZ ((STV_VDP2_CHCTLA & 0x0100) >> 8)

/*
18002A - CHCTLB - Character Control (NBG2, NBG1, RBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    | R0CHCN2  | R0CHCN1  | R0CHCN0  |    --    | R0BMSZ   | R0BMEN   | R0CHSZ   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    | N3CHCN   | N3CHSZ   |    --    |    --    | N2CHCN   | N2CHSZ   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CHCTLB ((stv_vdp2_regs[0x028/4] >> 0)&0x0000ffff)

/* -------------------------- RBG0 Character Control Registers -------------------------- */


/*	R0CHCNx  RBG0  Colour Depth
	000 - 16 Colours
	001 - 256 Colours
	010 - 2048 Colours
	011 - 32768 Colours (RGB5)
	100 - 16770000 Colours (RGB8)
	101 - invalid
	110 - invalid
	111 - invalid   */
	#define STV_VDP2_R0CHCN ((STV_VDP2_CHCTLB & 0x7000) >> 12)

/*	R0BMSZx - RBG0 Bitmap Size *guessed*
	00 - 512 x 256
	01 - 512 x 512  */
	#define STV_VDP2_R0BMSZ ((STV_VDP2_CHCTLB & 0x0400) >> 10)

/*	R0BMEN - RBG0 Bitmap Enable
	0 - use cell mode
	1 - use bitmap mode   */
	#define STV_VDP2_R0BMEN ((STV_VDP2_CHCTLB & 0x0200) >> 9)

/*	R0CHSZ - NBG0 Character (Tile) Size
	0 - 1 cell  x 1 cell  (8x8)
	1 - 2 cells x 2 cells (16x16)  */
	#define STV_VDP2_R0CHSZ ((STV_VDP2_CHCTLB & 0x0100) >> 8)

	#define STV_VDP2_N3CHCN ((STV_VDP2_CHCTLB & 0x0020) >> 5)
	#define STV_VDP2_N3CHSZ ((STV_VDP2_CHCTLB & 0x0010) >> 4)
	#define STV_VDP2_N2CHCN ((STV_VDP2_CHCTLB & 0x0002) >> 1)
	#define STV_VDP2_N2CHSZ ((STV_VDP2_CHCTLB & 0x0001) >> 0)


/*
18002C - BMPNA - Bitmap Palette Number (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_BMPNA ((stv_vdp2_regs[0x02c/4] >> 16)&0x0000ffff)

	#define STV_VDP2_N1BMP ((STV_VDP2_BMPNA & 0x0700) >> 8)
	#define STV_VDP2_N0BMP ((STV_VDP2_BMPNA & 0x0007) >> 0)

/* 18002E - Bitmap Palette Number (RBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/


/* 180030 - PNCN0 - Pattern Name Control (NBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | N0PNB    | N0CNSM   |    --    |    --    |    --    |    --    | N0SPR    | N0SCC    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | N0SPLT6  | N0SPLT5  | N0SPLT4  | N0SPCN4  | N0SPCN3  | N0SPCN2  | N0SPCN1  | N0SPCN0  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PNCN0 ((stv_vdp2_regs[0x030/4] >> 16)&0x0000ffff)

/*	Pattern Data Size
	0 = 2 bytes
	1 = 1 byte */
	#define STV_VDP2_N0PNB  ((STV_VDP2_PNCN0 & 0x8000) >> 15)

/*	Character Number Supplement (in 1 byte mode)
	0 = Character Number = 10bits + 2bits for flip
	1 = Character Number = 12 bits, no flip  */
	#define STV_VDP2_N0CNSM ((STV_VDP2_PNCN0 & 0x4000) >> 14)

/*  NBG0 Special Priority Register (in 1 byte mode) */
	#define STV_VDP2_N0SPR ((STV_VDP2_PNCN0 & 0x0200) >> 9)

/*	NBG0 Special Colour Control Register (in 1 byte mode) */
	#define STV_VDP2_N0SCC ((STV_VDP2_PNCN0 & 0x0100) >> 8)

/*	Supplementary Palette Bits (in 1 byte mode) */
	#define STV_VDP2_N0SPLT ((STV_VDP2_PNCN0 & 0x00e0) >> 5)

/*	Supplementary Character Bits (in 1 byte mode) */
	#define STV_VDP2_N0SPCN ((STV_VDP2_PNCN0 & 0x001f) >> 0)

/* 180032 - Pattern Name Control (NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PNCN1 ((stv_vdp2_regs[0x030/4] >> 0)&0x0000ffff)

/*	Pattern Data Size
	0 = 2 bytes
	1 = 1 byte */
	#define STV_VDP2_N1PNB  ((STV_VDP2_PNCN1 & 0x8000) >> 15)

/*	Character Number Supplement (in 1 byte mode)
	0 = Character Number = 10bits + 2bits for flip
	1 = Character Number = 12 bits, no flip  */
	#define STV_VDP2_N1CNSM ((STV_VDP2_PNCN1 & 0x4000) >> 14)

/*  NBG0 Special Priority Register (in 1 byte mode) */
	#define STV_VDP2_N1SPR ((STV_VDP2_PNCN1 & 0x0200) >> 9)

/*	NBG0 Special Colour Control Register (in 1 byte mode) */
	#define STV_VDP2_N1SCC ((STV_VDP2_PNCN1 & 0x0100) >> 8)

/*	Supplementary Palette Bits (in 1 byte mode) */
	#define STV_VDP2_N1SPLT ((STV_VDP2_PNCN1 & 0x00e0) >> 5)

/*	Supplementary Character Bits (in 1 byte mode) */
	#define STV_VDP2_N1SPCN ((STV_VDP2_PNCN1 & 0x001f) >> 0)


/* 180034 - Pattern Name Control (NBG2)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PNCN2 ((stv_vdp2_regs[0x034/4] >> 16)&0x0000ffff)

/*	Pattern Data Size
	0 = 2 bytes
	1 = 1 byte */
	#define STV_VDP2_N2PNB  ((STV_VDP2_PNCN2 & 0x8000) >> 15)

/*	Character Number Supplement (in 1 byte mode)
	0 = Character Number = 10bits + 2bits for flip
	1 = Character Number = 12 bits, no flip  */
	#define STV_VDP2_N2CNSM ((STV_VDP2_PNCN2 & 0x4000) >> 14)

/*  NBG0 Special Priority Register (in 1 byte mode) */
	#define STV_VDP2_N2SPR ((STV_VDP2_PNCN2 & 0x0200) >> 9)

/*	NBG0 Special Colour Control Register (in 1 byte mode) */
	#define STV_VDP2_N2SCC ((STV_VDP2_PNCN2 & 0x0100) >> 8)

/*	Supplementary Palette Bits (in 1 byte mode) */
	#define STV_VDP2_N2SPLT ((STV_VDP2_PNCN2 & 0x00e0) >> 5)

/*	Supplementary Character Bits (in 1 byte mode) */
	#define STV_VDP2_N2SPCN ((STV_VDP2_PNCN2 & 0x001f) >> 0)


/* 180036 - Pattern Name Control (NBG3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       | N3PNB    | N3CNSM   |    --    |    --    |    --    |    --    | N3SPR    | N3SCC    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | N3SPLT6  | N3SPLT5  | N3SPLT4  | N3SPCN4  | N3SPCN3  | N3SPCN2  | N3SPCN1  | N3SPCN0  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PNCN3 ((stv_vdp2_regs[0x034/4] >> 0)&0x0000ffff)

/*	Pattern Data Size
	0 = 2 bytes
	1 = 1 byte */
	#define STV_VDP2_N3PNB  ((STV_VDP2_PNCN3 & 0x8000) >> 15)

/*	Character Number Supplement (in 1 byte mode)
	0 = Character Number = 10bits + 2bits for flip
	1 = Character Number = 12 bits, no flip  */
	#define STV_VDP2_N3CNSM ((STV_VDP2_PNCN3 & 0x4000) >> 14)

/*  NBG0 Special Priority Register (in 1 byte mode) */
	#define STV_VDP2_N3SPR ((STV_VDP2_PNCN3 & 0x0200) >> 9)

/*	NBG0 Special Colour Control Register (in 1 byte mode) */
	#define STV_VDP2_N3SCC ((STV_VDP2_PNCN3 & 0x0100) >> 8)

/*	Supplementary Palette Bits (in 1 byte mode) */
	#define STV_VDP2_N3SPLT ((STV_VDP2_PNCN3 & 0x00e0) >> 5)

/*	Supplementary Character Bits (in 1 byte mode) */
	#define STV_VDP2_N3SPCN ((STV_VDP2_PNCN3 & 0x001f) >> 0)


/* 180038 - Pattern Name Control (RBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18003A - PLSZ - Plane Size (incomplete)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       | N3PLSZ1  | N3PLSZ0  |    --    |    --    | N1PLSZ1  | N1PLSZ0  | N0PLSZ1  | N0PLSZ0  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PLSZ ((stv_vdp2_regs[0x038/4] >> 0)&0x0000ffff)

	/* NBG0 Plane Size
	00 1H Page x 1V Page
	01 2H Pages x 1V Page
	10 invalid
	11 2H Pages x 2V Pages  */
	#define STV_VDP2_N0PLSZ ((STV_VDP2_PLSZ & 0x0003) >> 0)
	#define STV_VDP2_N1PLSZ ((STV_VDP2_PLSZ & 0x000c) >> 2)
	#define STV_VDP2_N2PLSZ ((STV_VDP2_PLSZ & 0x0030) >> 4)
	#define STV_VDP2_N3PLSZ ((STV_VDP2_PLSZ & 0x00c0) >> 6)

/* 18003C - MPOFN - Map Offset (NBG0, NBG1, NBG2, NBG3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    | N3MP8    | N3MP7    | N3MP6    |    --    | N2MP8    | N2MP7    | N2MP6    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    | N1MP8    | N1MP7    | N1MP6    |    --    | N0MP8    | N0MP7    | N0MP6    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPOFN_ ((stv_vdp2_regs[0x03c/4] >> 16)&0x0000ffff)

	/* Higher 3 bits of the map offset for each layer */
	#define STV_VDP2_N0MP_ ((STV_VDP2_MPOFN_ & 0x0007) >> 0)
	#define STV_VDP2_N1MP_ ((STV_VDP2_MPOFN_ & 0x0070) >> 4)
	#define STV_VDP2_N2MP_ ((STV_VDP2_MPOFN_ & 0x0700) >> 8)
	#define STV_VDP2_N3MP_ ((STV_VDP2_MPOFN_ & 0x7000) >> 12)




/* 18003E - Map Offset (Rotation Parameter A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180040 - MPABN0 - Map (NBG0, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    | N0MPB5   | N0MPB4   | N0MPB3   | N0MPB2   | N0MPB1   | N0MPB0   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    | N0MPA5   | N0MPA4   | N0MPA3   | N0MPA2   | N0MPA1   | N0MPA0   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPABN0 ((stv_vdp2_regs[0x040/4] >> 16)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap NBG0 */
	#define STV_VDP2_N0MPB ((STV_VDP2_MPABN0 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap NBG0 */
	#define STV_VDP2_N0MPA ((STV_VDP2_MPABN0 & 0x003f) >> 0)


/* 180042 - MPCDN0 - (NBG0, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    | N0MPD5   | N0MPD4   | N0MPD3   | N0MPD2   | N0MPD1   | N0MPD0   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    | N0MPC5   | N0MPC4   | N0MPC3   | N0MPC2   | N0MPC1   | N0MPC0   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPCDN0 ((stv_vdp2_regs[0x040/4] >> 0)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane D of Tilemap NBG0 */
	#define STV_VDP2_N0MPD ((STV_VDP2_MPCDN0 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane C of Tilemap NBG0 */
	#define STV_VDP2_N0MPC ((STV_VDP2_MPCDN0 & 0x003f) >> 0)


/* 180044 - Map (NBG1, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPABN1 ((stv_vdp2_regs[0x044/4] >> 16)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap NBG1 */
	#define STV_VDP2_N1MPB ((STV_VDP2_MPABN1 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap NBG1 */
	#define STV_VDP2_N1MPA ((STV_VDP2_MPABN1 & 0x003f) >> 0)

/* 180046 - Map (NBG1, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPCDN1 ((stv_vdp2_regs[0x044/4] >> 0)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane D of Tilemap NBG0 */
	#define STV_VDP2_N1MPD ((STV_VDP2_MPCDN1 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane C of Tilemap NBG0 */
	#define STV_VDP2_N1MPC ((STV_VDP2_MPCDN1 & 0x003f) >> 0)


/* 180048 - Map (NBG2, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPABN2 ((stv_vdp2_regs[0x048/4] >> 16)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap NBG2 */
	#define STV_VDP2_N2MPB ((STV_VDP2_MPABN2 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap NBG2 */
	#define STV_VDP2_N2MPA ((STV_VDP2_MPABN2 & 0x003f) >> 0)

/* 18004a - Map (NBG2, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPCDN2 ((stv_vdp2_regs[0x048/4] >> 0)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane D of Tilemap NBG2 */
	#define STV_VDP2_N2MPD ((STV_VDP2_MPCDN2 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane C of Tilemap NBG2 */
	#define STV_VDP2_N2MPC ((STV_VDP2_MPCDN2 & 0x003f) >> 0)

/* 18004c - Map (NBG3, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPABN3 ((stv_vdp2_regs[0x04c/4] >> 16)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap NBG1 */
	#define STV_VDP2_N3MPB ((STV_VDP2_MPABN3 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap NBG1 */
	#define STV_VDP2_N3MPA ((STV_VDP2_MPABN3 & 0x003f) >> 0)


/* 18004e - Map (NBG3, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_MPCDN3 ((stv_vdp2_regs[0x04c/4] >> 0)&0x0000ffff)

	/* N0MPB5 = lower 6 bits of Map Address of Plane B of Tilemap NBG0 */
	#define STV_VDP2_N3MPD ((STV_VDP2_MPCDN3 & 0x3f00) >> 8)

	/* N0MPA5 = lower 6 bits of Map Address of Plane A of Tilemap NBG0 */
	#define STV_VDP2_N3MPC ((STV_VDP2_MPCDN3 & 0x003f) >> 0)

/* 180050 - Map (Rotation Parameter A, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180052 - Map (Rotation Parameter A, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180054 - Map (Rotation Parameter A, Plane E,F)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180056 - Map (Rotation Parameter A, Plane G,H)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180058 - Map (Rotation Parameter A, Plane I,J)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18005a - Map (Rotation Parameter A, Plane K,L)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18005c - Map (Rotation Parameter A, Plane M,N)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18005e - Map (Rotation Parameter A, Plane O,P)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180060 - Map (Rotation Parameter B, Plane A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180062 - Map (Rotation Parameter B, Plane C,D)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180064 - Map (Rotation Parameter B, Plane E,F)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180066 - Map (Rotation Parameter B, Plane G,H)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180068 - Map (Rotation Parameter B, Plane I,J)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18006a - Map (Rotation Parameter B, Plane K,L)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18006c - Map (Rotation Parameter B, Plane M,N)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18006e - Map (Rotation Parameter B, Plane O,P)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180070 - SCXIN0 - Screen Scroll (NBG0, Horizontal Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCXIN0 ((stv_vdp2_regs[0x070/4] >> 16)&0x0000ffff)


/* 180072 - Screen Scroll (NBG0, Horizontal Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180074 - SCYIN0 - Screen Scroll (NBG0, Vertical Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCYIN0 ((stv_vdp2_regs[0x074/4] >> 16)&0x0000ffff)


/* 180076 - Screen Scroll (NBG0, Vertical Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180078 - Coordinate Inc (NBG0, Horizontal Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMXIN0 ((stv_vdp2_regs[0x078/4] >> 16)&0x0000ffff)

	#define STV_VDP2_N0ZMXI ((STV_VDP2_ZMXIN0 & 0x0007) >> 0)

/* 18007a - Coordinate Inc (NBG0, Horizontal Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMXDN0 ((stv_vdp2_regs[0x078/4] >> 0)&0x0000ffff)

	#define STV_VDP2_N0ZMXD ((STV_VDP2_ZMXDN0 >> 8)& 0xff)

/* 18007c - Coordinate Inc (NBG0, Vertical Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMYIN0 ((stv_vdp2_regs[0x07c/4] >> 16)&0x0000ffff)

	#define STV_VDP2_N0ZMYI ((STV_VDP2_ZMYIN0 & 0x0007) >> 0)

/* 18007e - Coordinate Inc (NBG0, Vertical Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMYDN0 ((stv_vdp2_regs[0x07c/4] >> 0)&0x0000ffff)

	#define STV_VDP2_N0ZMYD ((STV_VDP2_ZMYDN0 >> 8)& 0xff)

/* 180080 - SCXIN1 - Screen Scroll (NBG1, Horizontal Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCXIN1 ((stv_vdp2_regs[0x080/4] >> 16)&0x0000ffff)

/* 180082 - Screen Scroll (NBG1, Horizontal Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180084 - SCYIN1 - Screen Scroll (NBG1, Vertical Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCYIN1 ((stv_vdp2_regs[0x084/4] >> 16)&0x0000ffff)

/* 180086 - Screen Scroll (NBG1, Vertical Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180088 - Coordinate Inc (NBG1, Horizontal Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMXIN1 ((stv_vdp2_regs[0x088/4] >> 16)&0x0000ffff)

	#define STV_VDP2_N1ZMXI ((STV_VDP2_ZMXIN1 & 0x0007) >> 0)

/* 18008a - Coordinate Inc (NBG1, Horizontal Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMXDN1 ((stv_vdp2_regs[0x088/4] >> 0)&0x0000ffff)

	#define STV_VDP2_N1ZMXD ((STV_VDP2_ZMXDN1 >> 8)& 0xff)

/* 18008c - Coordinate Inc (NBG1, Vertical Integer Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMYIN1 ((stv_vdp2_regs[0x08c/4] >> 16)&0x0000ffff)

	#define STV_VDP2_N1ZMYI ((STV_VDP2_ZMYIN1 & 0x0007) >> 0)

/* 18008e - Coordinate Inc (NBG1, Vertical Fractional Part)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_ZMYDN1 ((stv_vdp2_regs[0x08c/4] >> 0)&0x0000ffff)

	#define STV_VDP2_N1ZMYD ((STV_VDP2_ZMYDN1 >> 8)& 0xff)

/* 180090 - SCXN2 - Screen Scroll (NBG2, Horizontal)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCXN2 ((stv_vdp2_regs[0x090/4] >> 16)&0x0000ffff)

/* 180092 - SCYN2 - Screen Scroll (NBG2, Vertical)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCYN2 ((stv_vdp2_regs[0x090/4] >> 0)&0x0000ffff)

/* 180094 - SCXN3 - Screen Scroll (NBG3, Horizontal)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCXN3 ((stv_vdp2_regs[0x094/4] >> 16)&0x0000ffff)

/* 180096 - SCYN3 - Screen Scroll (NBG3, Vertical)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_SCYN3 ((stv_vdp2_regs[0x094/4] >> 0)&0x0000ffff)

/* 180098 - Reduction Enable
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18009a - Line and Vertical Cell Scroll Control (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18009c - Vertical Cell Table Address (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18009e - Vertical Cell Table Address (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800a0 - Line Scroll Table Address (NBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800a2 - Line Scroll Table Address (NBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800a4 - Line Scroll Table Address (NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800a6 - Line Scroll Table Address (NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800a8 - Line Colour Screen Table Address
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800aa - Line Colour Screen Table Address
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ac - Back Screen Table Address
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |  BKCLMD  |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |  BKTA18  |  BKTA17  |  BKTA16  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ae - Back Screen Table Address
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |  BKTA15  |  BKTA14  |  BKTA13  |  BKTA12  |  BKTA11  |  BKTA10  |  BKTA9   |  BKTA8   |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |  BKTA7   |  BKTA7   |  BKTA6   |  BKTA5   |  BKTA4   |  BKTA3   |  BKTA2   |  BKTA0   |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_BKTA_UL (stv_vdp2_regs[0x0ac/4])

	#define STV_VDP2_BKCLMD ((STV_VDP2_BKTA_UL & 0x80000000) >> 31)
	#define STV_VDP2_BKTA   ((STV_VDP2_BKTA_UL & 0x0003ffff) >> 0)
	/*MSB of this register is used when the extra RAM cart is used,ignore it for now.*/
	/*	#define STV_VDP2_BKTA   ((STV_VDP2_BKTA_UL & 0x0007ffff) >> 0)*/

/* 1800b0 - Rotation Parameter Mode
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800b2 - Rotation Parameter Read Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800b4 - Coefficient Table Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800b6 - Coefficient Table Address Offset (Rotation Parameter A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800b8 - Screen Over Pattern Name (Rotation Parameter A)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ba - Screen Over Pattern Name (Rotation Parameter B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800bc - Rotation Parameter Table Address (Rotation Parameter A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/


/* 1800be - Rotation Parameter Table Address (Rotation Parameter A,B)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800c0 - Window Position (W0, Horizontal Start Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800c2 - Window Position (W0, Vertical Start Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800c4 - Window Position (W0, Horizontal Emd Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800c6 - Window Position (W0, Vertical End Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800c8 - Window Position (W1, Horizontal Start Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ca - Window Position (W1, Vertical Start Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800cc - Window Position (W1, Horizontal Emd Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ce - Window Position (W1, Vertical End Point)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800d0 - Window Control (NBG0, NBG1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800d2 - Window Control (NBG2, NBG3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800d4 - Window Control (RBG0, Sprite)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800d6 - Window Control (Parameter Window, Colour Calc. Window)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800d8 - Line Window Table Address (W0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800da - Line Window Table Address (W0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800dc - Line Window Table Address (W1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800de - Line Window Table Address (W1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800e0 - Sprite Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800e2 - Shadow Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800e4 - CRAOFA - Colour Ram Address Offset (NBG0 - NBG3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    | N0CAOS2  | N3CAOS1  | N3CAOS0  |    --    | N2CAOS2  | N2CAOS1  | N2CAOS0  |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    | N1CAOS2  | N1CAOS1  | N1CAOS0  |    --    | N0CAOS2  | N0CAOS1  | N0CAOS0  |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_CRAOFA ((stv_vdp2_regs[0x0e4/4] >> 16)&0x0000ffff)

	/* NxCAOS =  */
	#define STV_VDP2_N0CAOS ((STV_VDP2_CRAOFA & 0x0007) >> 0)
	#define STV_VDP2_N1CAOS ((STV_VDP2_CRAOFA & 0x0070) >> 4)
	#define STV_VDP2_N2CAOS ((STV_VDP2_CRAOFA & 0x0700) >> 8)
	#define STV_VDP2_N3CAOS ((STV_VDP2_CRAOFA & 0x7000) >> 12)


/* 1800e6 - Colour Ram Address Offset (RBG0, SPRITE)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800e8 - Line Colour Screen Enable
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ea - Special Priority Mode
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ec - Colour Calculation Control
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800ee - Special Colour Calculation Mode
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800f0 - Priority Number (Sprite 0,1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800f2 - Priority Number (Sprite 2,3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800f4 - Priority Number (Sprite 4,5)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800f6 - Priority Number (Sprite 6,7)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800f8 - PRINA - Priority Number (NBG 0,1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PRINA ((stv_vdp2_regs[0x0f8/4] >> 16)&0x0000ffff)

	#define STV_VDP2_N1PRIN ((STV_VDP2_PRINA & 0x0700) >> 8)
	#define STV_VDP2_N0PRIN ((STV_VDP2_PRINA & 0x0007) >> 0)

/* 1800fa - PRINB - Priority Number (NBG 2,3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

	#define STV_VDP2_PRINB ((stv_vdp2_regs[0x0f8/4] >> 0)&0x0000ffff)

	#define STV_VDP2_N3PRIN ((STV_VDP2_PRINB & 0x0700) >> 8)
	#define STV_VDP2_N2PRIN ((STV_VDP2_PRINB & 0x0007) >> 0)

/* 1800fc - Priority Number (RBG0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 1800fe - Reserved
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180100 - Colour Calculation Ratio (Sprite 0,1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180102 - Colour Calculation Ratio (Sprite 2,3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180104 - Colour Calculation Ratio (Sprite 4,5)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180106 - Colour Calculation Ratio (Sprite 6,7)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180108 - Colour Calculation Ratio (NBG 0,1)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18010a - Colour Calculation Ratio (NBG 2,3)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18010c - Colour Calculation Ratio (RBG 0)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18010e - Colour Calculation Ratio (Line Colour Screen, Back Colour Screen)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180110 - Colour Offset Enable
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180112 - Colour Offset Select
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180114 - Colour Offset A (Red)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180116 - Colour Offset A (Green)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 180118 - Colour Offset A (Blue)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18011a - Colour Offset B (Red)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18011b - Colour Offset B (Green)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* 18011c - Colour Offset B (Blue)
 bit-> /----15----|----14----|----13----|----12----|----11----|----10----|----09----|----08----\
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       |----07----|----06----|----05----|----04----|----03----|----02----|----01----|----00----|
       |    --    |    --    |    --    |    --    |    --    |    --    |    --    |    --    |
       \----------|----------|----------|----------|----------|----------|----------|---------*/

/* Not sure if to use this for the rotating tilemaps as well or just use different draw functions, might add too much bloat */
static struct stv_vdp2_tilemap_capabilities
{
	UINT8  enabled;
	UINT8  trans_enabled;
	UINT8  colour_depth;
	UINT8  tile_size;
	UINT8  bitmap_enable;
	UINT8  bitmap_size;
	UINT8  bitmap_palette_number;
	UINT8  bitmap_map;
	UINT16 map_offset[16];

	UINT8  pattern_data_size;
	UINT8  character_number_supplement;
	UINT8  special_priority_register;
	UINT8  special_colour_control_register;
	UINT8  supplementary_palette_bits;
	UINT8  supplementary_character_bits;

	INT16 scrollx;
	INT16 scrolly;
	UINT8 scalex_i,scaley_i;
	UINT16 scalex_f,scaley_f;

	UINT8  plane_size;
	UINT8  colour_ram_address_offset;

	UINT8  real_map_offset[16];

	int layer_name; /* just to keep track */
} stv2_current_tilemap;



static void stv_vdp2_draw_basic_bitmap(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
/*	logerror ("bitmap enable %02x size %08x depth %08x\n",	stv2_current_tilemap.layer_name, stv2_current_tilemap.bitmap_size, stv2_current_tilemap.colour_depth);*/
/*	usrintf_showmessage ("bitmap enable %02x size %08x depth %08x number %02x",	stv2_current_tilemap.layer_name, stv2_current_tilemap.bitmap_size, stv2_current_tilemap.colour_depth,stv2_current_tilemap.bitmap_palette_number);*/

	int xsize = 0;
	int ysize = 0;
	int xcnt,ycnt;
	data8_t* gfxdata = memory_region(REGION_GFX1);
	static UINT16 *destline;

	if (!stv2_current_tilemap.enabled) return;

	/* size for n0 / n1 */
	switch (stv2_current_tilemap.bitmap_size)
	{
		case 0: xsize=512; ysize=256; break;
		case 1: xsize=512; ysize=512; break;
		case 2: xsize=1024; ysize=256; break;
		case 3: xsize=1024; ysize=512; break;
	}

	gfxdata+=(stv2_current_tilemap.bitmap_map * 0x20000);
	stv2_current_tilemap.bitmap_palette_number+=stv2_current_tilemap.colour_ram_address_offset;
	stv2_current_tilemap.bitmap_palette_number&=7;/*safety check*/

	switch(stv2_current_tilemap.colour_depth)
	{
		/*Palette Format*/
		case 0:/*elandore uses this on the fight,size is wrong.*/
		{
			for (ycnt = 0; ycnt <ysize;ycnt++)
			{
				for (xcnt = 0; xcnt <xsize;xcnt+=2)
				{
					if(Machine->pens[((gfxdata[0] & 0x0f) >> 0) | (stv2_current_tilemap.bitmap_palette_number * 0x100)])
						plot_pixel(bitmap,xcnt+1  ,ycnt,Machine->pens[((gfxdata[0] & 0x0f) >> 0) | (stv2_current_tilemap.bitmap_palette_number * 0x100)]);
					if(Machine->pens[((gfxdata[0] & 0xf0) >> 4) | (stv2_current_tilemap.bitmap_palette_number * 0x100)])
						plot_pixel(bitmap,xcnt,ycnt,Machine->pens[((gfxdata[0] & 0xf0) >> 4) | (stv2_current_tilemap.bitmap_palette_number * 0x100)]);

					gfxdata++;
				}
			}
		}
		break;
		case 1:
		{
			for (ycnt = 0; ycnt <ysize;ycnt++)
			{
				for (xcnt = 0; xcnt <xsize;xcnt++)
				{
					if(Machine->pens[(gfxdata[0] & 0xff) | (stv2_current_tilemap.bitmap_palette_number * 0x100)])
						plot_pixel(bitmap,xcnt,ycnt,Machine->pens[(gfxdata[0] & 0xff) | (stv2_current_tilemap.bitmap_palette_number * 0x100)]);

					gfxdata++;
				}
			}
		}
		break;
		case 2:
		{
			for (ycnt = 0; ycnt <ysize;ycnt++)
			{
				for (xcnt = 0; xcnt <xsize;xcnt++)
				{
					if(Machine->pens[((gfxdata[0] & 0x07) * 0x100) | (gfxdata[1] & 0xff)])
						plot_pixel(bitmap,xcnt,ycnt,Machine->pens[((gfxdata[0] & 0x07) * 0x100) | (gfxdata[1] & 0xff)]);

					gfxdata+=2;
				}
			}
		}
		break;
		/*RGB format*/
		/*
		M                     L
		S                     S
		B                     B
		--------BBBBBGGGGGRRRRR
		*/
		case 3:
		{
			for (ycnt = 0; ycnt <ysize;ycnt++)
			{
				destline = (UINT16 *)(bitmap->line[ycnt]);

				for (xcnt = 0; xcnt <xsize;xcnt++)
				{
					int r,g,b;

					b = ((gfxdata[0] & 0x7c)>>2);
					g = ((gfxdata[0] & 0x03) << 3) | ((gfxdata[1] & 0xe0) >> 5);
					r = ((gfxdata[1] & 0x1f));

					if(r||g||b)
						destline[xcnt] = b | g << 5 | r << 10;

					gfxdata+=2;
				}
			}
		}
		break;
		/*
		M                              L
		S                              S
		B                              B
		--------BBBBBBBBGGGGGGGGRRRRRRRR
		*/
		case 4:
		{
			for (ycnt = 0; ycnt <ysize;ycnt++)
			{
				destline = (UINT16 *)(bitmap->line[ycnt]);

				for (xcnt = 0; xcnt <xsize;xcnt++)
				{
					int r,g,b;

					b = ((gfxdata[1] & 0xff));
					g = ((gfxdata[2] & 0xff));
					r = ((gfxdata[3] & 0xff));

					if(r||g||b)
						destline[xcnt] = b | g << 5 | r << 10;

					gfxdata+=4;
				}
			}
		}
		break;
	}
}

  /*---------------------------------------------------------------------------
   | Plane Size | Pattern Name Data Size | Character Size | Map Bits / Address |
   ----------------------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 6-0 * 0x02000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-0 * 0x00800 |
   | 1 H x 1 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-0 * 0x04000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-0 * 0x01000 |
   -----------------------------------------------------------------------------
   |            |                        | 1 H x 1 V      | bits 6-1 * 0x04000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-1 * 0x01000 |
   | 2 H x 1 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-1 * 0x08000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-1 * 0x02000 |
   -----------------------------------------------------------------------------
   |            |                        | 1 H x 1 V      | bits 6-2 * 0x08000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-2 * 0x02000 |
   | 2 H x 2 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-2 * 0x10000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-2 * 0x04000 |
   --the-highest-bit-is-ignored-if-vram-is-only-4mbits------------------------*/


/*
4.2 Sega's Cell / Character Pattern / Page / Plane / Map system, aka a rather annoying thing that makes optimizations hard
 (this is only for the normal tilemaps at the moment, i haven't even thought about the ROZ ones)

Tiles:

Cells are 8x8 gfx stored in video ram, they can be of various colour depths

Character Patterns can be 8x8 or 16x16 (1 hcell x 1 vcell or 2 hcell x 2 vcell)
  (a 16x16 character pattern is 4 8x8 cells put together)

A page is made up of 64x64 cells, thats 64x64 character patterns in 8x8 mode or 32x32 character patterns in 16x16 mode.
  64 * 8  = 512 (0x200)
  32 * 16 = 512 (0x200)
A page is _always_ 512 (0x200) pixels in each direction

in 1 word mode a 32*16 x 32*16 page is 0x0800 bytes
in 1 word mode a 64*8  x 64*8  page is 0x2000 bytes
in 2 word mode a 32*16 x 32*16 page is 0x1000 bytes
in 2 word mode a 64*8  x 64*8  page is 0x4000 bytes

either 1, 2 or 4 pages make each plane depending on the plane size register (per tilemap)
  therefore each plane is either
  64 * 8 * 1 x 64 * 8 * 1 (512 x 512)
  64 * 8 * 2 x 64 * 8 * 1 (1024 x 512)
  64 * 8 * 2 x 64 * 8 * 2 (1024 x 1024)

  32 * 16 * 1 x 32 * 16 * 1 (512 x 512)
  32 * 16 * 2 x 32 * 16 * 1 (1024 x 512)
  32 * 16 * 2 x 32 * 16 * 2 (1024 x 1024)

map is always enabled?
  map is a 2x2 arrangement of planes, all 4 of the planes can be the same.

*/

static void stv_vdp2_draw_basic_tilemap(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/* hopefully this is easier to follow than it is efficient .. */

	/* I call character patterns tiles .. even if they represent up to 4 tiles */

	/* Page variables */
	int pgtiles_x, pgpixels_x;
	int pgtiles_y, pgpixels_y;
	int pgsize_bytes, pgsize_dwords;

	/* Plane Variables */
	int pltiles_x, plpixels_x;
	int pltiles_y, plpixels_y;
	int plsize_bytes, plsize_dwords;

	/* Map Variables */
	int mptiles_x, mppixels_x;
	int mptiles_y, mppixels_y;
	int mpsize_bytes, mpsize_dwords;

	/* work Variables */
	int i, x, y;
	int base[4];

	int scalex,scaley;

	/* Calculate the Number of tiles for x / y directions of each page (actually these will be the same */
	/* (2-stv2_current_tilemap.tile_size) << 5) */
	pgtiles_x = ((2-stv2_current_tilemap.tile_size) << 5); /* 64 (8x8 mode) or 32 (16x16 mode)*/
	pgtiles_y = ((2-stv2_current_tilemap.tile_size) << 5); /* 64 (8x8 mode) or 32 (16x16 mode)*/

	/* Calculate the Page Size in BYTES */
	/* 64 * 64 * (1 * 2) = 0x2000 bytes
	   32 * 32 * (1 * 2) = 0x0800 bytes
	   64 * 64 * (2 * 2) = 0x4000 bytes
	   32 * 32 * (2 * 2) = 0x1000 bytes */

	pgsize_bytes = (pgtiles_x * pgtiles_y) * ((2-stv2_current_tilemap.pattern_data_size)*2);

   /*---------------------------------------------------------------------------
   | Plane Size | Pattern Name Data Size | Character Size | Map Bits / Address |
   ----------------------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 6-0 * 0x02000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-0 * 0x00800 |
   | 1 H x 1 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-0 * 0x04000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-0 * 0x01000 |
   ---------------------------------------------------------------------------*/


	/* Page Dimensions are always 0x200 pixes (512x512) */
	pgpixels_x = 0x200;
	pgpixels_y = 0x200;

	/* Work out the Plane Size in tiles and Plane Dimensions (pixels) */
	switch (stv2_current_tilemap.plane_size & 3)
	{
		case 0: /* 1 page * 1 page*/
			pltiles_x  = pgtiles_x;
			plpixels_x = pgpixels_x;
			pltiles_y  = pgtiles_y;
			plpixels_y = pgpixels_y;
			break;

		case 1: /* 2 pages * 1 page*/
			pltiles_x  = pgtiles_x * 2;
			plpixels_x = pgpixels_x * 2;
			pltiles_y  = pgtiles_y;
			plpixels_y = pgpixels_y;
			break;

		case 3: /* 2 pages * 2 pages*/
			pltiles_x  = pgtiles_x * 2;
			plpixels_x = pgpixels_x * 2;
			pltiles_y  = pgtiles_y * 2;
			plpixels_y = pgpixels_y * 2;
			break;

		default:
			/* illegal*/
			pltiles_x  = pgtiles_x;
			plpixels_x = pgpixels_x;
			pltiles_y  = pgtiles_y * 2;
			plpixels_y = pgpixels_y * 2;
		break;
	}

	/* Plane Size in BYTES */
	/* still the same as before
	   (64 * 1) * (64 * 1) * (1 * 2) = 0x02000 bytes
	   (32 * 1) * (32 * 1) * (1 * 2) = 0x00800 bytes
	   (64 * 1) * (64 * 1) * (2 * 2) = 0x04000 bytes
	   (32 * 1) * (32 * 1) * (2 * 2) = 0x01000 bytes
	   changed
	   (64 * 2) * (64 * 1) * (1 * 2) = 0x04000 bytes
	   (32 * 2) * (32 * 1) * (1 * 2) = 0x01000 bytes
	   (64 * 2) * (64 * 1) * (2 * 2) = 0x08000 bytes
	   (32 * 2) * (32 * 1) * (2 * 2) = 0x02000 bytes
	   changed
	   (64 * 2) * (64 * 1) * (1 * 2) = 0x08000 bytes
	   (32 * 2) * (32 * 1) * (1 * 2) = 0x02000 bytes
	   (64 * 2) * (64 * 1) * (2 * 2) = 0x10000 bytes
	   (32 * 2) * (32 * 1) * (2 * 2) = 0x04000 bytes
	*/

	plsize_bytes = (pltiles_x * pltiles_y) * ((2-stv2_current_tilemap.pattern_data_size)*2);

   /*---------------------------------------------------------------------------
   | Plane Size | Pattern Name Data Size | Character Size | Map Bits / Address |
   -----------------------------------------------------------------------------
   | 1 H x 1 V   see above, nothing has changed                                |
   -----------------------------------------------------------------------------
   |            |                        | 1 H x 1 V      | bits 6-1 * 0x04000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-1 * 0x01000 |
   | 2 H x 1 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-1 * 0x08000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-1 * 0x02000 |
   -----------------------------------------------------------------------------
   |            |                        | 1 H x 1 V      | bits 6-2 * 0x08000 |
   |            | 1 word                 |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-2 * 0x02000 |
   | 2 H x 2 V  ---------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-2 * 0x10000 |
   |            | 2 words                |-------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-2 * 0x04000 |
   --the-highest-bit-is-ignored-if-vram-is-only-4mbits------------------------*/


	/* Work out the Map Sizes in tiles, Map Dimensions */
	/* maps are always enabled? */
	mptiles_x = pltiles_x * 2;
	mptiles_y = pltiles_y * 2;
	mppixels_x = plpixels_x * 2;
	mppixels_y = plpixels_y * 2;

	/* Map Size in BYTES */
	mpsize_bytes = (mptiles_x * mptiles_y) * ((2-stv2_current_tilemap.pattern_data_size)*2);


   /*-----------------------------------------------------------------------------------------------------------
   |            |                        | 1 H x 1 V      | bits 6-1 (upper mask 0x07f) (0x1ff >> 2) * 0x04000 |
   |            | 1 word                 |---------------------------------------------------------------------|
   |            |                        | 2 H x 2 V      | bits 8-1 (upper mask 0x1ff) (0x1ff >> 0) * 0x01000 |
   | 2 H x 1 V  -----------------------------------------------------------------------------------------------|
   |            |                        | 1 H x 1 V      | bits 5-1 (upper mask 0x03f) (0x1ff >> 3) * 0x08000 |
   |            | 2 words                |---------------------------------------------------------------------|
   |            |                        | 2 H x 2 V      | bits 7-1 (upper mask 0x0ff) (0x1ff >> 1) * 0x02000 |
   -------------------------------------------------------------------------------------------------------------
    lower mask = ~stv2_current_tilemap.plane_size
   -----------------------------------------------------------------------------------------------------------*/

	/* Precalculate bases from MAP registers */
	for (i = 0; i < 4; i++)
	{
		int shifttable[4] = {0,1,2,2};

		int uppermask, uppermaskshift;

		uppermaskshift = (1-stv2_current_tilemap.pattern_data_size) | ((1-stv2_current_tilemap.tile_size)<<1);
		uppermask = 0x1ff >> uppermaskshift;

		base[i] = ((stv2_current_tilemap.map_offset[i] & uppermask) >> shifttable[stv2_current_tilemap.plane_size]) * plsize_bytes;

		base[i] &= 0x7ffff; /* shienryu needs this for the text layer, is there a problem elsewhere or is it just right without the ram cart */

		base[i] = base[i] / 4; /* convert bytes to DWORDS*/
	}

	/* other bits */
	stv2_current_tilemap.trans_enabled = stv2_current_tilemap.trans_enabled ? TRANSPARENCY_NONE : TRANSPARENCY_PEN;
	stv2_current_tilemap.scrollx &= mppixels_x-1;
	stv2_current_tilemap.scrolly &= mppixels_y-1;

	pgsize_dwords = pgsize_bytes /4;
	plsize_dwords = plsize_bytes /4;
	mpsize_dwords = mpsize_bytes /4;

/*	if (stv2_current_tilemap.layer_name==3) usrintf_showmessage ("well this is a bit  %08x", stv2_current_tilemap.map_offset[0]);*/
/*	if (stv2_current_tilemap.layer_name==3) usrintf_showmessage ("well this is a bit  %08x %08x %08x %08x", stv2_current_tilemap.plane_size, pgtiles_x, pltiles_x, mptiles_x);*/

	if (!stv2_current_tilemap.enabled) return; /* stop right now if its disabled ...*/

	/* most things we need (or don't need) to work out are now worked out */

	for (y = 0; y<mptiles_y; y++) {
		int drawypos, ypageoffs, yplaneoffs;
		int page, map, newbase, offs, data;
		int tilecode, flipyx, pal, gfx;

		map = 0 ; page = 0 ;
		if(stv2_current_tilemap.scaley_i != 1)
			drawypos = ((stv2_current_tilemap.tile_size ? 16 : 8) * 2) * y;
		else
			drawypos = (stv2_current_tilemap.tile_size ? 16 : 8) * y;
		drawypos -= stv2_current_tilemap.scrolly;

		if (drawypos > Machine->visible_area.max_y) continue;

		ypageoffs = y & (pgtiles_y-1);
		yplaneoffs = y & (pltiles_y-1);

		if (yplaneoffs > ypageoffs) page |= 2;
		if (y > yplaneoffs) map |= 2;

		for (x = 0; x<mptiles_x; x++) {
			int drawxpos, xpageoffs, xplaneoffs;
			if(stv2_current_tilemap.scalex_i != 1)
				drawxpos = ((stv2_current_tilemap.tile_size ? 16 : 8) * 2) * x;
			else
				drawxpos = (stv2_current_tilemap.tile_size ? 16 : 8) * x;
			drawxpos -= stv2_current_tilemap.scrollx;

			if (drawxpos > Machine->visible_area.max_x) continue;

			xpageoffs = x & (pgtiles_x-1);
			xplaneoffs = x & (pltiles_x-1);

			if (xplaneoffs > xpageoffs) page |= 1;
			if (x > xplaneoffs) map |= 1;

			newbase = base[map] + page * pgsize_dwords;
			offs = (ypageoffs * pgtiles_x) + xpageoffs;

/* GET THE TILE INFO ... */
			/* 1 word per tile mode with supplement bits */
			if (stv2_current_tilemap.pattern_data_size ==1)
			{

				data = stv_vdp2_vram[newbase + offs/2];
				data = (offs&1) ? (data & 0x0000ffff) : ((data & 0xffff0000) >> 16);

				/* Supplement Mode 12 bits, no flip */
				if (stv2_current_tilemap.character_number_supplement == 1)
				{
/* no flip */		flipyx   = 0;
/* 8x8 */			if (stv2_current_tilemap.tile_size==0) tilecode = (data & 0x0fff) + ( (stv2_current_tilemap.supplementary_character_bits&0x1c) << 10);
/* 16x16 */ 		else tilecode = ((data & 0x0fff) << 2) + (stv2_current_tilemap.supplementary_character_bits&0x03) + ((stv2_current_tilemap.supplementary_character_bits&0x10) << 10);
				}
				/* Supplement Mode 10 bits, with flip */
				else
				{
/* flip bits */ 	flipyx   = (data & 0x0c00) >> 10;
/* 8x8 */			if (stv2_current_tilemap.tile_size==0) tilecode = (data & 0x03ff) +  ( (stv2_current_tilemap.supplementary_character_bits) << 10);
/* 16x16 */			else tilecode = ((data & 0x03ff) <<2) +  (stv2_current_tilemap.supplementary_character_bits&0x03) + ((stv2_current_tilemap.supplementary_character_bits&0x1c) << 10);
				}

/*>16cols*/		if (stv2_current_tilemap.colour_depth != 0) pal = ((data & 0x7000)>>8);
/*16 cols*/		else pal = ((data & 0xf000)>>12) +( (stv2_current_tilemap.supplementary_palette_bits) << 4);

			}
			/* 2 words per tile, no supplement bits */
			else
			{

				data = stv_vdp2_vram[newbase + offs];
				tilecode = (data & 0x00007fff);
				pal   = (data &    0x007f0000)>>16;
	/*			specialc = (data & 0x10000000)>>28;;*/
				flipyx   = (data & 0xc0000000)>>30;
			}
/* WE'VE GOT THE TILE INFO ... */

/* DECODE ANY TILES WE NEED TO DECODE */

			pal += stv2_current_tilemap.colour_ram_address_offset<< 4; /* bios uses this ..*/

			if (stv2_current_tilemap.colour_depth != 0)
			{
				tilecode = tilecode >> 1;
				gfx = 2;
				pal = pal >>4;

				/* do a bit of tile decoding */
				tilecode &=0x3fff;
				if (stv2_current_tilemap.tile_size==1)
				{ /* we're treating 16x16 tiles as 4 8x8's atm */
					if (stv_vdp2_vram_dirty_8x8x8[tilecode] == 1) { stv_vdp2_vram_dirty_8x8x8[tilecode] = 0; decodechar(Machine->gfx[2], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x8[tilecode+1] == 1) { stv_vdp2_vram_dirty_8x8x8[tilecode+1] = 0; decodechar(Machine->gfx[2], tilecode+1,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x8[tilecode+2] == 1) { stv_vdp2_vram_dirty_8x8x8[tilecode+2] = 0; decodechar(Machine->gfx[2], tilecode+2,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x8[tilecode+3] == 1) { stv_vdp2_vram_dirty_8x8x8[tilecode+3] = 0; decodechar(Machine->gfx[2], tilecode+3,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout); };
				}
				else
				{
					if (stv_vdp2_vram_dirty_8x8x8[tilecode] == 1) { stv_vdp2_vram_dirty_8x8x8[tilecode] = 0; decodechar(Machine->gfx[2], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout); };
				}

			}
			else
			{
				gfx = 0;

				/* do a bit of tile decoding */
				tilecode &=0x7fff;
				if (stv2_current_tilemap.tile_size==1)
				{ /* we're treating 16x16 tiles as 4 8x8's atm */
					if (stv_vdp2_vram_dirty_8x8x4[tilecode] == 1) { stv_vdp2_vram_dirty_8x8x4[tilecode] = 0; decodechar(Machine->gfx[0], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x4[tilecode+1] == 1) { stv_vdp2_vram_dirty_8x8x4[tilecode+1] = 0; decodechar(Machine->gfx[0], tilecode+1,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x4[tilecode+2] == 1) { stv_vdp2_vram_dirty_8x8x4[tilecode+2] = 0; decodechar(Machine->gfx[0], tilecode+2,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout); };
					if (stv_vdp2_vram_dirty_8x8x4[tilecode+3] == 1) { stv_vdp2_vram_dirty_8x8x4[tilecode+3] = 0; decodechar(Machine->gfx[0], tilecode+3,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout); };
				}
				else
				{
					if (stv_vdp2_vram_dirty_8x8x4[tilecode] == 1) { stv_vdp2_vram_dirty_8x8x4[tilecode] = 0; decodechar(Machine->gfx[0], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout); };
				}
			}
/* TILES ARE NOW DECODED */

/* DRAW! */
			if(stv2_current_tilemap.scalex_i != 1 || stv2_current_tilemap.scaley_i != 1)
			{

				if(stv2_current_tilemap.scalex_i == 0)
					scalex = stv2_current_tilemap.scalex_f /** STV_VDP2_HCNT*/ * 0x2c0*2;
				else
					scalex = 0xffff;

				if(stv2_current_tilemap.scaley_i == 0)
					scaley = stv2_current_tilemap.scaley_f * STV_VDP2_VCNT * 0x100;
				else
					scaley = 0xffff;

				usrintf_showmessage("%04x %04x",STV_VDP2_HCNT,stv2_current_tilemap.scalex_f);

				if (stv2_current_tilemap.tile_size==1)
				{
					/* normal */
					drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos, drawypos,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
					drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
					drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos,drawypos+8,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
					drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos+8,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);

					/* this isn't very efficient .. we could probably improve it */
					if (stv2_current_tilemap.scrollx) /* wraparound x */
					{
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x, drawypos,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x,drawypos+8,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos+8,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
					}
					if (stv2_current_tilemap.scrolly) /* wraparound y */
					{
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos, drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
					}
					if (stv2_current_tilemap.scrollx && stv2_current_tilemap.scrolly) /* wraparound x & y */
					{
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x, drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
					}

				}
				else
				{
					drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos, drawypos,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
					/* this isn't very efficient .. we could probably improve it */
					if (stv2_current_tilemap.scrollx) /* wraparound x */
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos+mppixels_x, drawypos,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
					if (stv2_current_tilemap.scrolly) /* wraparound y */
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos, drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
					if (stv2_current_tilemap.scrollx && stv2_current_tilemap.scrolly) /* wraparound x & y */
						drawgfxzoom(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos+mppixels_x, drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0,scalex,scaley);
				}
			}
			else
			{
				if (stv2_current_tilemap.tile_size==1)
				{
					/* normal */
					drawgfx(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos, drawypos,cliprect,stv2_current_tilemap.trans_enabled,0);
					drawgfx(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos,cliprect,stv2_current_tilemap.trans_enabled,0);
					drawgfx(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos,drawypos+8,cliprect,stv2_current_tilemap.trans_enabled,0);
					drawgfx(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos+8,cliprect,stv2_current_tilemap.trans_enabled,0);

					/* this isn't very efficient .. we could probably improve it */
					if (stv2_current_tilemap.scrollx) /* wraparound x */
					{
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x, drawypos,cliprect,stv2_current_tilemap.trans_enabled,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos,cliprect,stv2_current_tilemap.trans_enabled,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x,drawypos+8,cliprect,stv2_current_tilemap.trans_enabled,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos+8,cliprect,stv2_current_tilemap.trans_enabled,0);
					}
					if (stv2_current_tilemap.scrolly) /* wraparound y */
					{
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos, drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0);
					}
					if (stv2_current_tilemap.scrollx && stv2_current_tilemap.scrolly) /* wraparound x & y */
					{
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+0+(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x, drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+1-(flipyx&1)+(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+2+(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+mppixels_x,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0);
						drawgfx(bitmap,Machine->gfx[gfx],tilecode+3-(flipyx&1)-(flipyx&2),pal,flipyx&1,flipyx&2,drawxpos+8+mppixels_x,drawypos+8+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0);
					}
				}
				else
				{
					drawgfx(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos, drawypos,cliprect,stv2_current_tilemap.trans_enabled,0);

					/* this isn't very efficient .. we could probably improve it */
					if (stv2_current_tilemap.scrollx) /* wraparound x */
						drawgfx(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos+mppixels_x, drawypos,cliprect,stv2_current_tilemap.trans_enabled,0);
					if (stv2_current_tilemap.scrolly) /* wraparound y */
						drawgfx(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos, drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0);
					if (stv2_current_tilemap.scrollx && stv2_current_tilemap.scrolly) /* wraparound x & y */
						drawgfx(bitmap,Machine->gfx[gfx],tilecode,pal,flipyx&1,flipyx&2, drawxpos+mppixels_x, drawypos+mppixels_y,cliprect,stv2_current_tilemap.trans_enabled,0);
				}
			}
/* DRAWN?! */

		}
	}
}

static void stv_vdp2_check_tilemap(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/* the idea is here we check the tilemap capabilities / whats enabled and call an appropriate tilemap drawing routine, or
	  at the very list throw up a few errors if the tilemaps want to do something we don't support yet */

	if (stv2_current_tilemap.bitmap_enable) /* this layer is a bitmap*/
	{
		stv_vdp2_draw_basic_bitmap(bitmap, cliprect);
	}
	else
	{
		stv_vdp2_draw_basic_tilemap(bitmap, cliprect);
	}
}


static void stv_vdp2_draw_NBG0(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/*
	   Colours           : 16, 256, 2048, 32768, 16770000
	   Char Size         : 1x1 cells, 2x2 cells
	   Pattern Data Size : 1 word, 2 words
	   Plane Layouts     : 1 x 1, 2 x 1, 2 x 2
	   Planes            : 4
	   Bitmap            : Possible
	   Bitmap Sizes      : 512 x 256, 512 x 512, 1024 x 256, 1024 x 512
	   Scale             : 0.25 x - 256 x
	   Rotation          : No
	   Linescroll        : Yes
	   Column Scroll     : Yes
	   Mosaic            : Yes
	*/
	stv2_current_tilemap.enabled = STV_VDP2_N0ON;

/*	if (!stv2_current_tilemap.enabled) return; // stop right now if its disabled ...*/

	stv2_current_tilemap.trans_enabled = STV_VDP2_N0TPON;
	stv2_current_tilemap.colour_depth = STV_VDP2_N0CHCN;
	stv2_current_tilemap.tile_size = STV_VDP2_N0CHSZ;
	stv2_current_tilemap.bitmap_enable = STV_VDP2_N0BMEN;
	stv2_current_tilemap.bitmap_size = STV_VDP2_N0BMSZ;
	stv2_current_tilemap.bitmap_palette_number = STV_VDP2_N0BMP;
	stv2_current_tilemap.bitmap_map = STV_VDP2_N0MP_;
	stv2_current_tilemap.map_offset[0] = STV_VDP2_N0MPA | (STV_VDP2_N0MP_ << 6);
	stv2_current_tilemap.map_offset[1] = STV_VDP2_N0MPB | (STV_VDP2_N0MP_ << 6);
	stv2_current_tilemap.map_offset[2] = STV_VDP2_N0MPC | (STV_VDP2_N0MP_ << 6);
	stv2_current_tilemap.map_offset[3] = STV_VDP2_N0MPD | (STV_VDP2_N0MP_ << 6);

	stv2_current_tilemap.pattern_data_size = STV_VDP2_N0PNB;
	stv2_current_tilemap.character_number_supplement = STV_VDP2_N0CNSM;
	stv2_current_tilemap.special_priority_register = STV_VDP2_N0SPR;
	stv2_current_tilemap.special_colour_control_register = STV_VDP2_PNCN0;
	stv2_current_tilemap.supplementary_palette_bits = STV_VDP2_N0SPLT;
	stv2_current_tilemap.supplementary_character_bits = STV_VDP2_N0SPCN;

	stv2_current_tilemap.scrollx = STV_VDP2_SCXIN0;
	stv2_current_tilemap.scrolly = STV_VDP2_SCYIN0;
	stv2_current_tilemap.scalex_i = STV_VDP2_N0ZMXI;
	stv2_current_tilemap.scalex_f = STV_VDP2_N0ZMXD;
	stv2_current_tilemap.scaley_i = STV_VDP2_N0ZMYI;
	stv2_current_tilemap.scaley_f = STV_VDP2_N0ZMYD;

	stv2_current_tilemap.plane_size = STV_VDP2_N0PLSZ;
	stv2_current_tilemap.colour_ram_address_offset = STV_VDP2_N0CAOS;

	stv2_current_tilemap.layer_name=0;

	stv_vdp2_check_tilemap(bitmap, cliprect);
}

static void stv_vdp2_draw_NBG1(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/*
	   Colours           : 16, 256, 2048, 32768
	   Char Size         : 1x1 cells, 2x2 cells
	   Pattern Data Size : 1 word, 2 words
	   Plane Layouts     : 1 x 1, 2 x 1, 2 x 2
	   Planes            : 4
	   Bitmap            : Possible
	   Bitmap Sizes      : 512 x 256, 512 x 512, 1024 x 256, 1024 x 512
	   Scale             : 0.25 x - 256 x
	   Rotation          : No
	   Linescroll        : Yes
	   Column Scroll     : Yes
	   Mosaic            : Yes
	*/
	stv2_current_tilemap.enabled = STV_VDP2_N1ON;

/*	if (!stv2_current_tilemap.enabled) return; // stop right now if its disabled ...*/

	stv2_current_tilemap.trans_enabled = STV_VDP2_N1TPON;
	stv2_current_tilemap.colour_depth = STV_VDP2_N1CHCN;
	stv2_current_tilemap.tile_size = STV_VDP2_N1CHSZ;
	stv2_current_tilemap.bitmap_enable = STV_VDP2_N1BMEN;
	stv2_current_tilemap.bitmap_size = STV_VDP2_N1BMSZ;
	stv2_current_tilemap.bitmap_palette_number = STV_VDP2_N1BMP;
	stv2_current_tilemap.bitmap_map = STV_VDP2_N1MP_;
	stv2_current_tilemap.map_offset[0] = STV_VDP2_N1MPA | (STV_VDP2_N1MP_ << 6);
	stv2_current_tilemap.map_offset[1] = STV_VDP2_N1MPB | (STV_VDP2_N1MP_ << 6);
	stv2_current_tilemap.map_offset[2] = STV_VDP2_N1MPC | (STV_VDP2_N1MP_ << 6);
	stv2_current_tilemap.map_offset[3] = STV_VDP2_N1MPD | (STV_VDP2_N1MP_ << 6);

	stv2_current_tilemap.pattern_data_size = STV_VDP2_N1PNB;
	stv2_current_tilemap.character_number_supplement = STV_VDP2_N1CNSM;
	stv2_current_tilemap.special_priority_register = STV_VDP2_N1SPR;
	stv2_current_tilemap.special_colour_control_register = STV_VDP2_PNCN1;
	stv2_current_tilemap.supplementary_palette_bits = STV_VDP2_N1SPLT;
	stv2_current_tilemap.supplementary_character_bits = STV_VDP2_N1SPCN;

	stv2_current_tilemap.scrollx = STV_VDP2_SCXIN1;
	stv2_current_tilemap.scrolly = STV_VDP2_SCYIN1;
	stv2_current_tilemap.scalex_i = STV_VDP2_N1ZMXI;
	stv2_current_tilemap.scalex_f = STV_VDP2_N1ZMXD;
	stv2_current_tilemap.scaley_i = STV_VDP2_N1ZMYI;
	stv2_current_tilemap.scaley_f = STV_VDP2_N1ZMYD;

	stv2_current_tilemap.plane_size = STV_VDP2_N1PLSZ;
	stv2_current_tilemap.colour_ram_address_offset = STV_VDP2_N1CAOS;

	stv2_current_tilemap.layer_name=1;

	stv_vdp2_check_tilemap(bitmap, cliprect);
}

static void stv_vdp2_draw_NBG2(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/*
	   NBG2 is the first of the 2 more basic tilemaps, it has exactly the same capabilities as NBG3

	   Colours           : 16, 256
	   Char Size         : 1x1 cells, 2x2 cells
	   Pattern Data Size : 1 word, 2 words
	   Plane Layouts     : 1 x 1, 2 x 1, 2 x 2
	   Planes            : 4
	   Bitmap            : No
	   Bitmap Sizes      : N/A
	   Scale             : No
	   Rotation          : No
	   Linescroll        : No
	   Column Scroll     : No
	   Mosaic            : Yes
	*/

	stv2_current_tilemap.enabled = STV_VDP2_N2ON;

	/* these modes for N0 disable this layer */
	if (STV_VDP2_N0CHCN == 0x03) stv2_current_tilemap.enabled = 0;
	if (STV_VDP2_N0CHCN == 0x04) stv2_current_tilemap.enabled = 0;

/*	if (!stv2_current_tilemap.enabled) return; // stop right now if its disabled ...*/

	stv2_current_tilemap.trans_enabled = STV_VDP2_N2TPON;
	stv2_current_tilemap.trans_enabled = STV_VDP2_N2TPON;
	stv2_current_tilemap.colour_depth = STV_VDP2_N2CHCN;
	stv2_current_tilemap.tile_size = STV_VDP2_N2CHSZ;
	/* this layer can't be a bitmap,so ignore these registers*/
	stv2_current_tilemap.bitmap_enable = 0;
	stv2_current_tilemap.bitmap_size = 0;
	stv2_current_tilemap.bitmap_palette_number = 0;
	stv2_current_tilemap.bitmap_map = 0;
	stv2_current_tilemap.map_offset[0] = STV_VDP2_N2MPA | (STV_VDP2_N2MP_ << 6);
	stv2_current_tilemap.map_offset[1] = STV_VDP2_N2MPB | (STV_VDP2_N2MP_ << 6);
	stv2_current_tilemap.map_offset[2] = STV_VDP2_N2MPC | (STV_VDP2_N2MP_ << 6);
	stv2_current_tilemap.map_offset[3] = STV_VDP2_N2MPD | (STV_VDP2_N2MP_ << 6);

	stv2_current_tilemap.pattern_data_size = STV_VDP2_N2PNB;
	stv2_current_tilemap.character_number_supplement = STV_VDP2_N2CNSM;
	stv2_current_tilemap.special_priority_register = STV_VDP2_N2SPR;
	stv2_current_tilemap.special_colour_control_register = STV_VDP2_PNCN2;
	stv2_current_tilemap.supplementary_palette_bits = STV_VDP2_N2SPLT;
	stv2_current_tilemap.supplementary_character_bits = STV_VDP2_N2SPCN;

	stv2_current_tilemap.scrollx = STV_VDP2_SCXN2;
	stv2_current_tilemap.scrolly = STV_VDP2_SCYN2;
	/*This layer can't be scaled*/
	stv2_current_tilemap.scalex_i = 1;
	stv2_current_tilemap.scalex_f = 0;
	stv2_current_tilemap.scaley_i = 1;
	stv2_current_tilemap.scaley_f = 0;

	stv2_current_tilemap.colour_ram_address_offset = STV_VDP2_N2CAOS;

	stv2_current_tilemap.layer_name=2;

	stv2_current_tilemap.plane_size = STV_VDP2_N2PLSZ;
	stv_vdp2_check_tilemap(bitmap, cliprect);
}

static void stv_vdp2_draw_NBG3(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/*
	   NBG3 is the second of the 2 more basic tilemaps, it has exactly the same capabilities as NBG2

	   Colours           : 16, 256
	   Char Size         : 1x1 cells, 2x2 cells
	   Pattern Data Size : 1 word, 2 words
	   Plane Layouts     : 1 x 1, 2 x 1, 2 x 2
	   Planes            : 4
	   Bitmap            : No
	   Bitmap Sizes      : N/A
	   Scale             : No
	   Rotation          : No
	   Linescroll        : No
	   Column Scroll     : No
	   Mosaic            : Yes
	*/

	stv2_current_tilemap.enabled = STV_VDP2_N3ON;

/*	if (!stv2_current_tilemap.enabled) return; // stop right now if its disabled ...*/

	stv2_current_tilemap.trans_enabled = STV_VDP2_N3TPON;
	stv2_current_tilemap.trans_enabled = STV_VDP2_N3TPON;
	stv2_current_tilemap.colour_depth = STV_VDP2_N3CHCN;
	stv2_current_tilemap.tile_size = STV_VDP2_N3CHSZ;
	/* this layer can't be a bitmap,so ignore these registers*/
	stv2_current_tilemap.bitmap_enable = 0;
	stv2_current_tilemap.bitmap_size = 0;
	stv2_current_tilemap.bitmap_palette_number = 0;
	stv2_current_tilemap.bitmap_map = 0;
	stv2_current_tilemap.map_offset[0] = STV_VDP2_N3MPA | (STV_VDP2_N3MP_ << 6);
	stv2_current_tilemap.map_offset[1] = STV_VDP2_N3MPB | (STV_VDP2_N3MP_ << 6);
	stv2_current_tilemap.map_offset[2] = STV_VDP2_N3MPC | (STV_VDP2_N3MP_ << 6);
	stv2_current_tilemap.map_offset[3] = STV_VDP2_N3MPD | (STV_VDP2_N3MP_ << 6);

	stv2_current_tilemap.pattern_data_size = STV_VDP2_N3PNB;
	stv2_current_tilemap.character_number_supplement = STV_VDP2_N3CNSM;
	stv2_current_tilemap.special_priority_register = STV_VDP2_N3SPR;
	stv2_current_tilemap.special_colour_control_register = STV_VDP2_PNCN3;
	stv2_current_tilemap.supplementary_palette_bits = STV_VDP2_N3SPLT;
	stv2_current_tilemap.supplementary_character_bits = STV_VDP2_N3SPCN;

	stv2_current_tilemap.scrollx = STV_VDP2_SCXN3;
	stv2_current_tilemap.scrolly = STV_VDP2_SCYN3;
	/*This layer can't be scaled*/
	stv2_current_tilemap.scalex_i = 1;
	stv2_current_tilemap.scalex_f = 0;
	stv2_current_tilemap.scaley_i = 1;
	stv2_current_tilemap.scaley_f = 0;

	stv2_current_tilemap.colour_ram_address_offset = STV_VDP2_N3CAOS;

	stv2_current_tilemap.layer_name=3;

	stv2_current_tilemap.plane_size = STV_VDP2_N3PLSZ;
	stv_vdp2_check_tilemap(bitmap, cliprect);
}

static void stv_vdp2_draw_back(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int xcnt,ycnt;
	data8_t* gfxdata = memory_region(REGION_GFX1);
	static UINT16 *destline;

	if(!(STV_VDP2_BDCLMD & 1))
		fillbitmap(bitmap, get_black_pen(), cliprect);
	else
	{
		#ifdef MAME_DEBUG
		/*usrintf_showmessage("Back screen enabled %08x",STV_VDP2_BKTA);*/
		#endif
		gfxdata+=(STV_VDP2_BKTA);

		for (ycnt = 0; ycnt <1024;ycnt++)
		{
			destline = (UINT16 *)(bitmap->line[ycnt]);

			for (xcnt = 0; xcnt <1024;xcnt++)
			{
				int r,g,b;

				b = ((gfxdata[0] & 0x7c) >> 2);
				g = ((gfxdata[0] & 0x03) << 3) | ((gfxdata[1] & 0xe0) >> 5);
				r = ((gfxdata[1] & 0x1f));

				destline[xcnt] = b | g << 5 | r << 10;
			}
			if(STV_VDP2_BKCLMD)
				gfxdata+=2;
		}
	}
}


WRITE32_HANDLER ( stv_vdp2_vram_w )
{
	data8_t *stv_vdp2_vram_decode = memory_region(REGION_GFX1);

	COMBINE_DATA(&stv_vdp2_vram[offset]);

	data = stv_vdp2_vram[offset];
	/* put in gfx region for easy decoding */
	stv_vdp2_vram_decode[offset*4+0] = (data & 0xff000000) >> 24;
	stv_vdp2_vram_decode[offset*4+1] = (data & 0x00ff0000) >> 16;
	stv_vdp2_vram_decode[offset*4+2] = (data & 0x0000ff00) >> 8;
	stv_vdp2_vram_decode[offset*4+3] = (data & 0x000000ff) >> 0;

	stv_vdp2_vram_dirty_8x8x4[offset/8] = 1;
	stv_vdp2_vram_dirty_8x8x8[offset/16] = 1;
}

READ32_HANDLER ( stv_vdp2_vram_r )
{
	return stv_vdp2_vram[offset];
}


WRITE32_HANDLER ( stv_vdp2_cram_w )
{
	int r,g,b;
	COMBINE_DATA(&stv_vdp2_cram[offset]);

/*	usrintf_showmessage("%01x",STV_VDP2_CRMD);*/

	switch( STV_VDP2_CRMD )
	{
		/*Mode 2/3*/
		case 2:
		case 3:
		{
			b = ((stv_vdp2_cram[offset] & 0x00ff0000) >> 16);
			g = ((stv_vdp2_cram[offset] & 0x0000ff00) >> 8);
			r = ((stv_vdp2_cram[offset] & 0x000000ff) >> 0);
			palette_set_color(offset,r,g,b);
		}
		break;
		/*Mode 0*/
		case 0:
		{
			offset &= 0x3ff;

			b = ((stv_vdp2_cram[offset] & 0x00007c00) >> 10);
			g = ((stv_vdp2_cram[offset] & 0x000003e0) >> 5);
			r = ((stv_vdp2_cram[offset] & 0x0000001f) >> 0);
			b*=0x8;
			g*=0x8;
			r*=0x8;
			palette_set_color((offset*2)+1,r,g,b);
			b = ((stv_vdp2_cram[offset] & 0x7c000000) >> 26);
			g = ((stv_vdp2_cram[offset] & 0x03e00000) >> 21);
			r = ((stv_vdp2_cram[offset] & 0x001f0000) >> 16);
			b*=0x8;
			g*=0x8;
			r*=0x8;
			palette_set_color(offset*2,r,g,b);
		}
		break;
		/*Mode 1*/
		case 1:
		{
			offset &= 0x7ff;

			b = ((stv_vdp2_cram[offset] & 0x00007c00) >> 10);
			g = ((stv_vdp2_cram[offset] & 0x000003e0) >> 5);
			r = ((stv_vdp2_cram[offset] & 0x0000001f) >> 0);
			b*=0x8;
			g*=0x8;
			r*=0x8;
			palette_set_color((offset*2)+1,r,g,b);
			b = ((stv_vdp2_cram[offset] & 0x7c000000) >> 26);
			g = ((stv_vdp2_cram[offset] & 0x03e00000) >> 21);
			r = ((stv_vdp2_cram[offset] & 0x001f0000) >> 16);
			b*=0x8;
			g*=0x8;
			r*=0x8;
			palette_set_color(offset*2,r,g,b);
		}
		break;
	}
}

READ32_HANDLER ( stv_vdp2_cram_r )
{
	return stv_vdp2_cram[offset];
}

WRITE32_HANDLER ( stv_vdp2_regs_w )
{
	COMBINE_DATA(&stv_vdp2_regs[offset]);
}

extern int stv_vblank;
READ32_HANDLER ( stv_vdp2_regs_r )
{
/*	if (offset!=1) logerror ("VDP2: Read from Registers, Offset %04x\n",offset);*/

	switch(offset)
	{
		case 0x4/4:
		/*Screen Status Register*/
								   /*VBLANK              HBLANK            ODD         PAL    */
			stv_vdp2_regs[offset] = (stv_vblank<<19) | ((rand()&1)<<18) | (1 << 17) | (0 << 16);
		break;
		case 0x8/4:
		/*H/V Counter Register*/
								     /*H-Counter                               V-Counter                                         */
			stv_vdp2_regs[offset] = (((Machine->visible_area.max_x - 1)<<16)&0x3ff)|(((Machine->visible_area.max_y - 1)<<0)&0x3ff);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #%d PC(%08x) = VDP2: H/V counter read : %08x\n",cpu_getactivecpu(),activecpu_get_pc(),stv_vdp2_regs[offset]);
		break;
	}
	return stv_vdp2_regs[offset];
}

int stv_vdp2_start ( void )
{
	stv_vdp2_regs = auto_malloc ( 0x040000 );
	stv_vdp2_vram = auto_malloc ( 0x100000 ); /* actually we only need half of it since we don't emulate extra 4mbit ram cart.*/
	stv_vdp2_cram = auto_malloc ( 0x080000 );
	stv_vdp2_vram_dirty_8x8x4 = auto_malloc ( 0x100000 );
	stv_vdp2_vram_dirty_8x8x8 = auto_malloc ( 0x100000 );

	memset(stv_vdp2_regs, 0, 0x040000);
	memset(stv_vdp2_vram, 0, 0x100000);
	memset(stv_vdp2_cram, 0, 0x080000);

/*	Machine->gfx[0]->color_granularity=4;*/
/*	Machine->gfx[1]->color_granularity=4;*/

	return 0;
}

/* maybe we should move this to vidhrdw/stv.c */
VIDEO_START( stv_vdp2 )
{
	stv_vdp2_start();
	stv_vdp1_start();

	return 0;
}

static void stv_vdp2_dynamic_res_change()
{
	static UINT16 horz,vert;

	switch( STV_VDP2_VRES & 3 )
	{
		case 0: vert = 224; break;
		case 1: vert = 240; break;
		case 2: vert = 256; break;
		case 3:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "WARNING: V Res setting (3) not allowed!\n");
			vert = 256;
			break;
	}

	/*Double-density interlace mode,doubles the vertical res*/
	if((STV_VDP2_LSMD & 3) == 3) { vert*=2;  }

	switch( STV_VDP2_HRES & 7 )
	{
		case 0: horz = 320; break;
		case 1: horz = 352; break;
		case 2: horz = 640; break;
		case 3: horz = 704; break;
		/*Exclusive modes,they sets the Vertical Resolution without considering the
			VRES register.*/
		case 4: horz = 320; vert = 480; break;
		case 5: horz = 352; vert = 480; break;
		case 6: horz = 640; vert = 480; break;
		case 7: horz = 704; vert = 480; break;
	}

	set_visible_area(0*8, horz-1,0*8, vert-1);
}

extern data32_t *stv_vdp1_vram;

VIDEO_UPDATE( stv_vdp2 )
{
	static UINT8 pri;

/*#ifndef MAME_DEBUG*/
	stv_vdp2_dynamic_res_change();
/*#endif*/

	stv_vdp2_draw_back(bitmap,cliprect);

	/*If a plane has a priority value of zero it isn't shown at all.*/
	for(pri=1;pri<8;pri++)
	{
		if (!(keyboard_pressed(KEYCODE_T))) {if(pri==STV_VDP2_N3PRIN) stv_vdp2_draw_NBG3(bitmap,cliprect);}
		if (!(keyboard_pressed(KEYCODE_Y))) {if(pri==STV_VDP2_N2PRIN) stv_vdp2_draw_NBG2(bitmap,cliprect);}
		if (!(keyboard_pressed(KEYCODE_U))) {if(pri==STV_VDP2_N1PRIN) stv_vdp2_draw_NBG1(bitmap,cliprect);}
		if (!(keyboard_pressed(KEYCODE_I))) {if(pri==STV_VDP2_N0PRIN) stv_vdp2_draw_NBG0(bitmap,cliprect);}
		if (!(keyboard_pressed(KEYCODE_O))) {if(pri==6)               video_update_vdp1(bitmap,cliprect);}
	}

#ifdef MAME_DEBUG
	if(STV_VDP2_VRAMSZ)
		usrintf_showmessage("Warning: VRAM Size = 8 MBit!");

	/*usrintf_showmessage("N0 %02x %04x %02x %04x N1 %02x %04x %02x %04x"
	,STV_VDP2_N0ZMXI,STV_VDP2_N0ZMXD
	,STV_VDP2_N0ZMYI,STV_VDP2_N0ZMYD
	,STV_VDP2_N1ZMXI,STV_VDP2_N1ZMXD
	,STV_VDP2_N1ZMYI,STV_VDP2_N1ZMYD);*/

	if ( keyboard_pressed_memory(KEYCODE_W) )
	{
		int tilecode;

		for (tilecode = 0;tilecode<0x8000;tilecode++)
		{
			decodechar(Machine->gfx[0], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[0].gfxlayout); ;
		}

		for (tilecode = 0;tilecode<0x2000;tilecode++)
		{
			decodechar(Machine->gfx[1], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[1].gfxlayout); ;
		}

		for (tilecode = 0;tilecode<0x4000;tilecode++)
		{
			decodechar(Machine->gfx[2], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[2].gfxlayout); ;
		}

		for (tilecode = 0;tilecode<0x1000;tilecode++)
		{
			decodechar(Machine->gfx[3], tilecode,  (data8_t*)memory_region(REGION_GFX1), Machine->drv->gfxdecodeinfo[3].gfxlayout); ;
		}

		/* vdp 1 ... doesn't have to be tile based */

		for (tilecode = 0;tilecode<0x8000;tilecode++)
		{
			decodechar(Machine->gfx[4], tilecode,  (data8_t*)memory_region(REGION_GFX2), Machine->drv->gfxdecodeinfo[4].gfxlayout); ;
		}
		for (tilecode = 0;tilecode<0x2000;tilecode++)
		{
			decodechar(Machine->gfx[5], tilecode,  (data8_t*)memory_region(REGION_GFX2), Machine->drv->gfxdecodeinfo[5].gfxlayout); ;
		}
		for (tilecode = 0;tilecode<0x4000;tilecode++)
		{
			decodechar(Machine->gfx[6], tilecode,  (data8_t*)memory_region(REGION_GFX2), Machine->drv->gfxdecodeinfo[6].gfxlayout); ;
		}
		for (tilecode = 0;tilecode<0x1000;tilecode++)
		{
			decodechar(Machine->gfx[7], tilecode,  (data8_t*)memory_region(REGION_GFX2), Machine->drv->gfxdecodeinfo[7].gfxlayout); ;
		}
	}
#endif

#ifdef MAME_DEBUG

	if ( keyboard_pressed_memory(KEYCODE_K) )
	{
		FILE *fp;

		fp=fopen("mamevdp1", "w+b");
		if (fp)
		{
			fwrite(stv_vdp1_vram, 0x80000, 1, fp);
			fclose(fp);
		}
	}

	if ( keyboard_pressed_memory(KEYCODE_L) )
	{
		FILE *fp;

		fp=fopen("vdp1_vram.bin", "r+b");
		if (fp)
		{
			fread(stv_vdp1_vram, 0x80000, 1, fp);
			fclose(fp);
		}
	}

#endif



}

/* below is some old code we might use .. */

#if 0
/*
One of the features of Sega ST-V is that the Color RAM could use two+one
different formats of Paletteram:
(1)Mode 0:RGB up to 5 bits per color,for 1024 possible combinations.16-bit format.
(2)Mode 1:Same as mode 0 but with 2048 possible combinations.
(3)Mode 2:RGB up to 8 bits per color,for 1024 possible combinations.32-bit format.
Notice that if it's currently using the mode 0/1,the first three bits (aka bits 0,1 and 2)
aren't used in output data(they are filled with 0).
The MSB in any mode is known to be used as "Color Calculation"(transparency).

TODO: we have to refresh the entire palette when it change color mode.
*/

/**********************************************************************************
VDP2 Registers Table
(registers are in 16-bit format,h=high word,l=low word):
===================================================================================
0h  	00 		TV Screen Mode
0l  	02		External Signal Enable Register
1h  	04		Screen Status(VBlank)
1l  	06		Vram Size

2h  	08		H Counter
2l		0a		V Counter
3h		0c			< Reserved >
3l		0e		Ram Control
-----------------------------------------------------------------------------------
4h 	 	10		Vram Cycle Pattern (Bank A0) [1/2]
4l		12		Vram Cycle Pattern (Bank A0) [2/2]
5h		14		Vram Cycle Pattern (Bank A1) [1/2]
5l		16		Vram Cycle Pattern (Bank A1) [2/2]

6h		18		Vram Cycle Pattern (Bank B0) [1/2]
6l		1a		Vram Cycle Pattern (Bank B0) [2/2]
7h		1c		Vram Cycle Pattern (Bank B1) [1/2]
7l		1e		Vram Cycle Pattern (Bank B1) [2/2]
-----------------------------------------------------------------------------------
8h		20		Screen Display Enable
8l		22		Mosaic Control
9h		24		Special Function Code Select
9l		26		Special Function Code

10h		28		Character Control (NBG0,NBG1)
10l		2a		Character Control (NBG2,NBG3,RBG0)
11h		2c		Bitmap Palette Number (NBG0, NBG1)
11l		2e		Bitmap Palette Number (RBG0)
-----------------------------------------------------------------------------------
12h		30		Pattern Name Control (NBG0)
12l		32		Pattern Name Control (NBG1)
13h		34		Pattern Name Control (NBG2)
13l		36		Pattern Name Control (NBG3)

14h		38		Pattern Name Control (RBG0)
14l		3a		Plane Size
15h		3c		Map Offset (NBG0-NBG3)
15l		3e		Map Offset (Rotation Parameter A,B)
-----------------------------------------------------------------------------------
16h		40		Map
16l		42
17h		44
17l		46

18h		48
18l		4a
19h		4c
19l		4e
-----------------------------------------------------------------------------------
20h		50		Map (Rotation Parameter A)
20l		52
21h		54
21l		56

22h		58
22l		5a
23h		5c
23l		5e
-----------------------------------------------------------------------------------
24h		60		Map (Rotation Parameter B)
24l		62
25h		64
25l		66

26h		68
26l		6a
27h		6c
27l		6e
-----------------------------------------------------------------------------------
28h		70		Screen Scroll Value (NBG0,H integer)
28l		72		Screen Scroll Value (NBG0,H fractional)
29h		74		Screen Scroll Value (NBG0,V integer)
29l		76		Screen Scroll Value (NBG0,V fractional)

30h		78		Coordinate Increment (NBG0,H integer)
30l		7a		Coordinate Increment (NBG0,H fractional)
31h		7c		Coordinate Increment (NBG0,V integer)
31l		7e		Coordinate Increment (NBG0,V fractional)
-----------------------------------------------------------------------------------
32h		80		Screen Scroll Value (NBG1,H integer)
32l		82		Screen Scroll Value (NBG1,H fractional)
33h		84		Screen Scroll Value (NBG1,V integer)
33l		86		Screen Scroll Value (NBG1,V fractional)

34h		88		Coordinate Increment (NBG1,H integer)
34l		8a		Coordinate Increment (NBG1,H fractional)
35h		8c		Coordinate Increment (NBG1,V integer)
35l		8e		Coordinate Increment (NBG1,V fractional)
-----------------------------------------------------------------------------------
36h		90		Screen Scroll Value (NBG2,H)
36l		92		Screen Scroll Value (NBG2,V)
37h		94		Screen Scroll Value (NBG3,H)
37l		96		Screen Scroll Value (NBG3,V)

38h		98		Reduction Enable
38l		9a		Line and Vertical Cell Scroll Control (NBG0,NBG1)
39h		9c		Vertical Cell Scroll Table Address (NBG0,NBG1) [1/2]
39l		9e		Vertical Cell Scroll Table Address (NBG0,NBG1) [2/2]
-----------------------------------------------------------------------------------
40h		a0		Line Scroll Table Address (NBG0) [1/2]
40l		a2		Line Scroll Table Address (NBG0) [2/2]
41h		a4		Line Scroll Table Address (NBG1) [1/2]
41l		a6		Line Scroll Table Address (NBG1) [2/2]

42h		a8		Line Color Screen Table Address [1/2]
42l		aa		Line Color Screen Table Address [2/2]
43h		ac		Back Screen Table Address [1/2]
43l		ae		Back Screen Table Address [2/2]
-----------------------------------------------------------------------------------
44h		b0		Rotation Parameter Mode
44l		b2		Rotation Parameter Read Control
45h		b4		Coefficient Table Control
45l		b6		Coefficient Table Address Offset

46h		b8		Screen Over Pattern Name (Rotation Parameter A)
46l		ba		Screen Over Pattern Name (Rotation Parameter B)
47h		bc		Rotation Parameter Table Address [1/2]
47l		be		Rotation Parameter Table Address [2/2]
-----------------------------------------------------------------------------------
48h		c0		Window Position (W0 H start point)
48l		c2		Window Position (W0 H end point)
49h		c4		Window Position (W0 V start point)
49l		c6		Window Position (W0 V end point)

50h		c8		Window Position (W1 H start point)
50l		ca		Window Position (W1 H end point)
51h		cc		Window Position (W1 V start point)
51l		ce		Window Position (W1 V end point)
-----------------------------------------------------------------------------------
52h		d0		Window Control (NBG0,NBG1)
52l		d2		Window Control (NBG2,NBG3)
53h		d4		Window Control (RBG0,SPRITE)
53l		d6		Window Control (Misc.)

54h		d8		Line Window Table Address (W0) [1/2]
54l		da		Line Window Table Address (W0) [2/2]
55h		dc		Line Window Table Address (W1) [1/2]
55l		de		Line Window Table Address (W1) [2/2]
-----------------------------------------------------------------------------------
56h		e0		Sprite Control
56l		e2		Shadow Control
57h		e4		Color RAM Address Offset (NBG0-NBG3)
57l		e6		Color RAM Address Offset (RBG0-SPRITE)

58h		e8		Line Color Screen Enable
58l		ea		Special Priority Mode
59h		ec		Color Calculation Control
59l		ee		Special Color Calculation Mode
-----------------------------------------------------------------------------------
60h		f0		Priority Number (SPRITE)
60l		f2
61h		f4
61l		f6

62h		f8		Priority Number (NBG0,NBG1)
62l		fa		Priority Number (NBG2,NBG3)
63h		fc		Priority Number (RBG0)
63l		fe			< Reserved >
-----------------------------------------------------------------------------------
64h	   	100		Color Calculation Ratio (SPRITE)
64l		102
65h		104
65l		106

66h		108		Color Calculation Ratio (NBG0,NBG1)
66l		10a		Color Calculation Ratio (NBG2,NBG3)
67h		10c		Color Calculation Ratio (RBG0)
67l		10e		Color Calculation Ratio (LINE,BACK)
-----------------------------------------------------------------------------------
68h		110		Color Offset Enable
68l		112		Color Offset Select
69h		114		Color Offset A R
69l		116		Color Offset A G

70h		118		Color Offset A B
70l		11a		Color Offset B R
71h		11c		Color Offset B G
71l		11e		Color Offset B B
-----------------------------------------------------------------------------------
**********************************************************************************/

/*An empty comment at the start of a line means             *
 *that the corrispetive bit isn't used by the driver ATM.   */
static READ32_HANDLER ( stv_vdp2_regs_r32 )
{
/*	if (offset!=1) logerror ("VDP2: Read from Registers, Offset %04x\n",offset);*/
	switch(offset)
	{
		case 1:
		/*Screen Status Register*/
		/*VBLANK & HBLANK(bit 3 & 2 of high word),fake for now*/
		/*VBLANK is always one when the DISP is 0*/
			stv_vdp2_regs[offset] ^= 0x000c0000;
		break;
		case 3:
		/*(V)RAM Control Register*/
		/*Color RAM Mode (bit 13 & 12) (CRMD1 & CRMD0) */
			CRMD = ((stv_vdp2_regs[offset] & 0x00003000) >> 12);
		break;
	}
	return stv_vdp2_regs[offset];
}

static WRITE32_HANDLER ( stv_vdp2_regs_w32 )
{
	COMBINE_DATA(&stv_vdp2_regs[offset]);

	switch(offset)
	{
		/*r/w*/
		case 0:
			/*TV screen display bit*/
/**/		DISP = ((stv_vdp2_regs[offset] & 0x80000000) >> 31);
			/*Border Color mode bit*/
			BDCLMD = ((stv_vdp2_regs[offset] & 0x01000000) >> 24);
			/*Interlace mode bit*/
/**/		LSMD = ((stv_vdp2_regs[offset] & 0x00c00000) >> 22);
			/*Vertical/Horizontal Resolution bits*/
			VRES = ((stv_vdp2_regs[offset] & 0x00300000) >> 20);
			HRES = ((stv_vdp2_regs[offset] & 0x00070000) >> 16);
			res_change();
		break;
		case 3:
			/*Vram mode bits (partitioning)*/
			VRAMD =((stv_vdp2_regs[offset] & 0x00000300) >> 8);
			/*Color RAM mode*/
			CRMD = ((stv_vdp2_regs[offset] & 0x00003000) >> 12);
		break;
		case 4: VCPA0 = (stv_vdp2_regs[4]); break;
		case 5: VCPA1 = (stv_vdp2_regs[5]); break;
		case 6: VCPB0 = (stv_vdp2_regs[6]); break;
		case 7: VCPB1 = (stv_vdp2_regs[7]); break;
		case 8:
		/*Screen Display enable bit*/
/**/		TPON = ((stv_vdp2_regs[offset] & 0x1f000000) >> 24);
/**/	 	SDON = ((stv_vdp2_regs[offset] & 0x003f0000) >> 16);
		break;
		case 10:
		/*Character Control Register*/
		/*These needs to be added into a struct*/
			CHCTLN0 = ((stv_vdp2_regs[offset] & 0x007f0000) >> 16);
			CHCTLN1 = ((stv_vdp2_regs[offset] & 0x3f000000) >> 24);
/**/		CHCTLN2 = ((stv_vdp2_regs[offset] & 0x00000003) >> 0);
/**/		CHCTLN3 = ((stv_vdp2_regs[offset] & 0x00000030) >> 4);
		break;
		case 12:
		/*Pattern Name Control Register*/
/**/		PNCN0 = ((stv_vdp2_regs[offset] & 0xffff0000) >> 16);
/**/		PNCN1 = ((stv_vdp2_regs[offset] & 0x0000ffff) >> 0);
		break;
		case 13:
/**/		PNCN2 = ((stv_vdp2_regs[offset] & 0xffff0000) >> 16);
/**/		PNCN3 = ((stv_vdp2_regs[offset] & 0x0000ffff) >> 0);
		break;
		case 14:
/**/		PLSZ = ((stv_vdp2_regs[offset] & 0x0000ffff) >> 0);
		break;
		case 43:
/**/		BKCLMD = ((stv_vdp2_regs[offset] & 0x80000000) >> 31);
/**/		BKTA   = ((stv_vdp2_regs[offset] & 0x0007ffff) >> 0);
		break;
		/* Color Offset Enable/Select Register */
		case 68:
			CLOFEN = ((stv_vdp2_regs[offset] & 0x007f0000) >> 16);
			CLOFSL = ((stv_vdp2_regs[offset] & 0x0000007f) >> 0);
		break;
		case 69:
		/* Color Offset Registers*/
			COAR = ((stv_vdp2_regs[offset] & 0x01ff0000) >> 16);
			COAG = ((stv_vdp2_regs[offset] & 0x000001ff) >> 0);
		break;
		case 70:
			COAB = ((stv_vdp2_regs[offset] & 0x01ff0000) >> 16);
			stv_bright();
		break;
		case 71:
		break;
		}
}

/*
**
** Functions to emulate some aspects of the VDP-2.
**
*/

/*This is WRONG,the actual brightness control is much more complex than this...*/
static void stv_bright()
{
	double brt = (COAR & 0x100) ? (COAR & 0xff) / 256.0 : (COAR + 0x100) / 256.0;

	/*apply only if NBG3 needs it...*/
	if(CLOFEN & 8 && (!(CLOFSL & 8)))
		palette_set_global_brightness(brt);
	else
		palette_set_global_brightness(1.00);
}

static void stv_dump_ram()
{
	FILE *fp;

	fp=fopen("workraml.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_workram_l, 0x00100000, 1, fp);
		fclose(fp);
	}
	fp=fopen("workramh.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_workram_h, 0x00100000, 1, fp);
		fclose(fp);
	}
	fp=fopen("scu.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_scu, 0xd0, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_a0_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_a0_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_a1_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_a1_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_b0_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_b0_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("stv_b1_vram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_b1_vram, 0x00020000, 1, fp);
		fclose(fp);
	}
	fp=fopen("cram.dmp", "w+b");
	if (fp)
	{
		fwrite(stv_cram, 0x00080000, 1, fp);
		fclose(fp);
	}
	fp=fopen("68k.dmp", "w+b");
	if (fp)
	{
		fwrite(memory_region(REGION_CPU3), 0x100000, 1, fp);
		fclose(fp);
	}
}


#endif
