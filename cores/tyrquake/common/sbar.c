/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/* sbar.c -- status bar code */

#include "client.h"
#include "cmd.h"
#include "common.h"
#include "draw.h"
#include "keys.h"
#include "menu.h"
#include "protocol.h"
#include "quakedef.h"
#include "sbar.h"
#include "screen.h"
#include "server.h"
#include "wad.h"

int sb_updates;			/* if >= vid.numpages, no update needed */

#define STAT_MINUS 10		/* num frame for '-' stats digit */
const qpic_t *sb_nums[2][11];
const qpic_t *sb_colon, *sb_slash;
const qpic_t *sb_ibar;
const qpic_t *sb_sbar;
const qpic_t *sb_scorebar;

const qpic_t *sb_weapons[7][8];	/* 0 is active, 1 is owned, 2-5 are flashes */
const qpic_t *sb_ammo[4];
const qpic_t *sb_sigil[4];
const qpic_t *sb_armor[3];
const qpic_t *sb_items[32];

const qpic_t *sb_faces[7][2];	/* 0 is gibbed, 1 is dead, 2-6 are alive */
				/* 0 is static, 1 is temporary animation */
const qpic_t *sb_face_invis;
const qpic_t *sb_face_quad;
const qpic_t *sb_face_invuln;
const qpic_t *sb_face_invis_invuln;

qboolean sb_showscores;

int sb_lines;			/* scan lines to draw */

const qpic_t *rsb_invbar[2];
const qpic_t *rsb_weapons[5];
const qpic_t *rsb_items[2];
const qpic_t *rsb_ammo[3];
const qpic_t *rsb_teambord;	/* PGM 01/19/97 - team color border */

/* MED 01/04/97 added two more weapons + 3 alternates for grenade launcher */
const qpic_t *hsb_weapons[7][5]; /* 0 is active, 1 is owned, 2-5 are flashes */

/* MED 01/04/97 added array to simplify weapon parsing */
int hipweapons[4] = {
    HIT_LASER_CANNON_BIT,
    HIT_MJOLNIR_BIT,
    4,
    HIT_PROXIMITY_GUN_BIT
};

/* MED 01/04/97 added hipnotic items array */
const qpic_t *hsb_items[2];

void Sbar_MiniDeathmatchOverlay(void);
void Sbar_DeathmatchOverlay(void);

/*
===============
Sbar_ShowScores

Tab key down
===============
*/
static void
Sbar_ShowScores(void)
{
    if (sb_showscores)
	return;
    sb_showscores = true;
    sb_updates = 0;
}

/*
===============
Sbar_DontShowScores

Tab key up
===============
*/
static void
Sbar_DontShowScores(void)
{
    sb_showscores = false;
    sb_updates = 0;
}

/*
===============
Sbar_Changed
===============
*/
void
Sbar_Changed(void)
{
    sb_updates = 0;		/* update next frame */
}

/*
===============
Sbar_Init
===============
*/
void
Sbar_Init(void)
{
    int i;

    for (i = 0; i < 10; i++) {
	sb_nums[0][i] = (qpic_t*)Draw_PicFromWad(va("num_%i", i));
	sb_nums[1][i] = (qpic_t*)Draw_PicFromWad(va("anum_%i", i));
    }

    sb_nums[0][10] = (qpic_t*)Draw_PicFromWad("num_minus");
    sb_nums[1][10] = (qpic_t*)Draw_PicFromWad("anum_minus");

    sb_colon = (qpic_t*)Draw_PicFromWad("num_colon");
    sb_slash = (qpic_t*)Draw_PicFromWad("num_slash");

    sb_weapons[0][0] = (qpic_t*)Draw_PicFromWad("inv_shotgun");
    sb_weapons[0][1] = (qpic_t*)Draw_PicFromWad("inv_sshotgun");
    sb_weapons[0][2] = (qpic_t*)Draw_PicFromWad("inv_nailgun");
    sb_weapons[0][3] = (qpic_t*)Draw_PicFromWad("inv_snailgun");
    sb_weapons[0][4] = (qpic_t*)Draw_PicFromWad("inv_rlaunch");
    sb_weapons[0][5] = (qpic_t*)Draw_PicFromWad("inv_srlaunch");
    sb_weapons[0][6] = (qpic_t*)Draw_PicFromWad("inv_lightng");

    sb_weapons[1][0] = (qpic_t*)Draw_PicFromWad("inv2_shotgun");
    sb_weapons[1][1] = (qpic_t*)Draw_PicFromWad("inv2_sshotgun");
    sb_weapons[1][2] = (qpic_t*)Draw_PicFromWad("inv2_nailgun");
    sb_weapons[1][3] = (qpic_t*)Draw_PicFromWad("inv2_snailgun");
    sb_weapons[1][4] = (qpic_t*)Draw_PicFromWad("inv2_rlaunch");
    sb_weapons[1][5] = (qpic_t*)Draw_PicFromWad("inv2_srlaunch");
    sb_weapons[1][6] = (qpic_t*)Draw_PicFromWad("inv2_lightng");

    for (i = 0; i < 5; i++) {
	sb_weapons[2 + i][0] = (qpic_t*)Draw_PicFromWad(va("inva%i_shotgun", i + 1));
	sb_weapons[2 + i][1] = (qpic_t*)Draw_PicFromWad(va("inva%i_sshotgun", i + 1));
	sb_weapons[2 + i][2] = (qpic_t*)Draw_PicFromWad(va("inva%i_nailgun", i + 1));
	sb_weapons[2 + i][3] = (qpic_t*)Draw_PicFromWad(va("inva%i_snailgun", i + 1));
	sb_weapons[2 + i][4] = (qpic_t*)Draw_PicFromWad(va("inva%i_rlaunch", i + 1));
	sb_weapons[2 + i][5] = (qpic_t*)Draw_PicFromWad(va("inva%i_srlaunch", i + 1));
	sb_weapons[2 + i][6] = (qpic_t*)Draw_PicFromWad(va("inva%i_lightng", i + 1));
    }

    sb_ammo[0] = (qpic_t*)Draw_PicFromWad("sb_shells");
    sb_ammo[1] = (qpic_t*)Draw_PicFromWad("sb_nails");
    sb_ammo[2] = (qpic_t*)Draw_PicFromWad("sb_rocket");
    sb_ammo[3] = (qpic_t*)Draw_PicFromWad("sb_cells");

    sb_armor[0] = (qpic_t*)Draw_PicFromWad("sb_armor1");
    sb_armor[1] = (qpic_t*)Draw_PicFromWad("sb_armor2");
    sb_armor[2] = (qpic_t*)Draw_PicFromWad("sb_armor3");

    sb_items[0] = (qpic_t*)Draw_PicFromWad("sb_key1");
    sb_items[1] = (qpic_t*)Draw_PicFromWad("sb_key2");
    sb_items[2] = (qpic_t*)Draw_PicFromWad("sb_invis");
    sb_items[3] = (qpic_t*)Draw_PicFromWad("sb_invuln");
    sb_items[4] = (qpic_t*)Draw_PicFromWad("sb_suit");
    sb_items[5] = (qpic_t*)Draw_PicFromWad("sb_quad");

    sb_sigil[0] = (qpic_t*)Draw_PicFromWad("sb_sigil1");
    sb_sigil[1] = (qpic_t*)Draw_PicFromWad("sb_sigil2");
    sb_sigil[2] = (qpic_t*)Draw_PicFromWad("sb_sigil3");
    sb_sigil[3] = (qpic_t*)Draw_PicFromWad("sb_sigil4");

    sb_faces[4][0] = (qpic_t*)Draw_PicFromWad("face1");
    sb_faces[4][1] = (qpic_t*)Draw_PicFromWad("face_p1");
    sb_faces[3][0] = (qpic_t*)Draw_PicFromWad("face2");
    sb_faces[3][1] = (qpic_t*)Draw_PicFromWad("face_p2");
    sb_faces[2][0] = (qpic_t*)Draw_PicFromWad("face3");
    sb_faces[2][1] = (qpic_t*)Draw_PicFromWad("face_p3");
    sb_faces[1][0] = (qpic_t*)Draw_PicFromWad("face4");
    sb_faces[1][1] = (qpic_t*)Draw_PicFromWad("face_p4");
    sb_faces[0][0] = (qpic_t*)Draw_PicFromWad("face5");
    sb_faces[0][1] = (qpic_t*)Draw_PicFromWad("face_p5");

    sb_face_invis = (qpic_t*)Draw_PicFromWad("face_invis");
    sb_face_invuln = (qpic_t*)Draw_PicFromWad("face_invul2");
    sb_face_invis_invuln = (qpic_t*)Draw_PicFromWad("face_inv2");
    sb_face_quad = (qpic_t*)Draw_PicFromWad("face_quad");

    Cmd_AddCommand("+showscores", Sbar_ShowScores);
    Cmd_AddCommand("-showscores", Sbar_DontShowScores);

    sb_sbar = (qpic_t*)Draw_PicFromWad("sbar");
    sb_ibar = (qpic_t*)Draw_PicFromWad("ibar");
    sb_scorebar = (qpic_t*)Draw_PicFromWad("scorebar");

/* MED 01/04/97 added new hipnotic weapons */
    if (hipnotic) {
	hsb_weapons[0][0] = (qpic_t*)Draw_PicFromWad("inv_laser");
	hsb_weapons[0][1] = (qpic_t*)Draw_PicFromWad("inv_mjolnir");
	hsb_weapons[0][2] = (qpic_t*)Draw_PicFromWad("inv_gren_prox");
	hsb_weapons[0][3] = (qpic_t*)Draw_PicFromWad("inv_prox_gren");
	hsb_weapons[0][4] = (qpic_t*)Draw_PicFromWad("inv_prox");

	hsb_weapons[1][0] = (qpic_t*)Draw_PicFromWad("inv2_laser");
	hsb_weapons[1][1] = (qpic_t*)Draw_PicFromWad("inv2_mjolnir");
	hsb_weapons[1][2] = (qpic_t*)Draw_PicFromWad("inv2_gren_prox");
	hsb_weapons[1][3] = (qpic_t*)Draw_PicFromWad("inv2_prox_gren");
	hsb_weapons[1][4] = (qpic_t*)Draw_PicFromWad("inv2_prox");

	for (i = 0; i < 5; i++) {
	    hsb_weapons[2 + i][0] =
		(qpic_t*)Draw_PicFromWad(va("inva%i_laser", i + 1));
	    hsb_weapons[2 + i][1] =
		(qpic_t*)Draw_PicFromWad(va("inva%i_mjolnir", i + 1));
	    hsb_weapons[2 + i][2] =
		(qpic_t*)Draw_PicFromWad(va("inva%i_gren_prox", i + 1));
	    hsb_weapons[2 + i][3] =
		(qpic_t*)Draw_PicFromWad(va("inva%i_prox_gren", i + 1));
	    hsb_weapons[2 + i][4] = (qpic_t*)Draw_PicFromWad(va("inva%i_prox", i + 1));
	}

	hsb_items[0] = (qpic_t*)Draw_PicFromWad("sb_wsuit");
	hsb_items[1] = (qpic_t*)Draw_PicFromWad("sb_eshld");
    }

    if (rogue) {
	rsb_invbar[0] = (qpic_t*)Draw_PicFromWad("r_invbar1");
	rsb_invbar[1] = (qpic_t*)Draw_PicFromWad("r_invbar2");

	rsb_weapons[0] = (qpic_t*)Draw_PicFromWad("r_lava");
	rsb_weapons[1] = (qpic_t*)Draw_PicFromWad("r_superlava");
	rsb_weapons[2] = (qpic_t*)Draw_PicFromWad("r_gren");
	rsb_weapons[3] = (qpic_t*)Draw_PicFromWad("r_multirock");
	rsb_weapons[4] = (qpic_t*)Draw_PicFromWad("r_plasma");

	rsb_items[0] = (qpic_t*)Draw_PicFromWad("r_shield1");
	rsb_items[1] = (qpic_t*)Draw_PicFromWad("r_agrav1");

/* PGM 01/19/97 - team color border */
	rsb_teambord = (qpic_t*)Draw_PicFromWad("r_teambord");
/* PGM 01/19/97 - team color border */

	rsb_ammo[0] = (qpic_t*)Draw_PicFromWad("r_ammolava");
	rsb_ammo[1] = (qpic_t*)Draw_PicFromWad("r_ammomulti");
	rsb_ammo[2] = (qpic_t*)Draw_PicFromWad("r_ammoplasma");
    }
}


/* ============================================================================= */

/* drawing routines are relative to the status bar location */

/*
=============
Sbar_DrawPic

(x, y) are logical 320x200-space coordinates, relative to the
top-left of the status bar (which is anchored to the bottom of
the screen). The wrapper applies scr_uiscale and centers the
status bar horizontally on the physical screen.
=============
*/
static void
Sbar_DrawPic(int x, int y, const qpic_t *pic)
{
    int scale = SCR_GetUIScale();
    int yphys = y * scale + (vid.height - SBAR_HEIGHT * scale);
    if (cl.gametype == GAME_DEATHMATCH)
	Draw_PicScaled(x * scale, yphys, pic, scale);
    else
	Draw_PicScaled(x * scale + ((vid.width - 320 * scale) >> 1),
		       yphys, pic, scale);
}

/*
=============
Sbar_DrawTransPic
=============
*/
static void
Sbar_DrawTransPic(int x, int y, const qpic_t *pic)
{
    int scale = SCR_GetUIScale();
    int yphys = y * scale + (vid.height - SBAR_HEIGHT * scale);
    if (cl.gametype == GAME_DEATHMATCH)
	Draw_TransPicScaled(x * scale, yphys, pic, scale);
    else
	Draw_TransPicScaled(x * scale + ((vid.width - 320 * scale) >> 1),
			    yphys, pic, scale);
}

/*
================
Sbar_DrawCharacter

Draws one solid graphics character. Logical coords; the +4 pixel
offset is preserved at scale (becomes +4*scale in physical pixels).
================
*/
static void
Sbar_DrawCharacter(int x, int y, int num)
{
    int scale = SCR_GetUIScale();
    int yphys = y * scale + vid.height - SBAR_HEIGHT * scale;
    if (cl.gametype == GAME_DEATHMATCH)
	Draw_CharacterScaled(x * scale + 4 * scale, yphys, num, scale);
    else
	Draw_CharacterScaled(x * scale + ((vid.width - 320 * scale) >> 1)
			     + 4 * scale, yphys, num, scale);
}

/*
================
Sbar_DrawString
================
*/
static void
Sbar_DrawString(int x, int y, const char *str)
{
    int scale = SCR_GetUIScale();
    int yphys = y * scale + vid.height - SBAR_HEIGHT * scale;
    if (cl.gametype == GAME_DEATHMATCH)
	Draw_StringScaled(x * scale, yphys, (char *)str, scale);
    else
	Draw_StringScaled(x * scale + ((vid.width - 320 * scale) >> 1),
			  yphys, (char *)str, scale);
}

/*
=============
Sbar_itoa
=============
*/
static int
Sbar_itoa(int num, char *buf)
{
    char *str;
    int pow10;
    int dig;

    str = buf;

    if (num < 0) {
	*str++ = '-';
	num = -num;
    }

    for (pow10 = 10; num >= pow10; pow10 *= 10);

    do {
	pow10 /= 10;
	dig = num / pow10;
	*str++ = '0' + dig;
	num -= dig * pow10;
    } while (pow10 != 1);

    *str = 0;

    return str - buf;
}


/*
=============
Sbar_DrawNum
=============
*/
static void
Sbar_DrawNum(int x, int y, int num, int digits, int color)
{
    char str[12];
    char *ptr;
    int l, frame;

    l = Sbar_itoa(num, str);
    ptr = str;
    if (l > digits)
	ptr += (l - digits);
    if (l < digits)
	x += (digits - l) * 24;

    while (*ptr) {
	if (*ptr == '-')
	    frame = STAT_MINUS;
	else
	    frame = *ptr - '0';

	Sbar_DrawTransPic(x, y, sb_nums[color][frame]);
	x += 24;
	ptr++;
    }
}

/* ============================================================================= */

int fragsort[MAX_SCOREBOARD];

char scoreboardtext[MAX_SCOREBOARD][20];
int scoreboardtop[MAX_SCOREBOARD];
int scoreboardbottom[MAX_SCOREBOARD];
int scoreboardcount[MAX_SCOREBOARD];
int scoreboardlines;

/*
===============
Sbar_SortFrags
===============
*/
static void
Sbar_SortFrags(void)
{
    int i, j, k;

/* sort by frags */
    scoreboardlines = 0;
    for (i = 0; i < cl.maxclients; i++) {
	if (cl.players[i].name[0]) {
	    fragsort[scoreboardlines] = i;
	    scoreboardlines++;
	}
    }

    for (i = 0; i < scoreboardlines; i++)
	for (j = 0; j < scoreboardlines - 1 - i; j++)
	    if (cl.players[fragsort[j]].frags <
		cl.players[fragsort[j + 1]].frags) {
		k = fragsort[j];
		fragsort[j] = fragsort[j + 1];
		fragsort[j + 1] = k;
	    }
}

static int
Sbar_ColorForMap(int m)
{
    m = qclamp(m, 0, 13) * 16;

    /* return m < 128 ? m + 8 : m + 8; */
    return m + 8;
}

/*
===============
Sbar_SoloScoreboard
===============
*/
static void
Sbar_SoloScoreboard(void)
{
    char str[80];
    int minutes, seconds, tens, units;
    int l;

    snprintf(str, sizeof(str), "Monsters:%3i /%3i", cl.stats[STAT_MONSTERS],
	    cl.stats[STAT_TOTALMONSTERS]);
    Sbar_DrawString(8, 4, str);

    snprintf(str, sizeof(str), "Secrets :%3i /%3i", cl.stats[STAT_SECRETS],
	    cl.stats[STAT_TOTALSECRETS]);
    Sbar_DrawString(8, 12, str);

/* time */
    minutes = cl.time / 60;
    seconds = cl.time - 60 * minutes;
    tens = seconds / 10;
    units = seconds - 10 * tens;
    snprintf(str, sizeof(str), "Time :%3i:%i%i", minutes, tens, units);
    Sbar_DrawString(184, 4, str);

/* draw level name */
    l = strlen(cl.levelname);
    Sbar_DrawString(232 - l * 4, 12, cl.levelname);
}

/*
===============
Sbar_DrawScoreboard
===============
*/
static void
Sbar_DrawScoreboard(void)
{
    Sbar_SoloScoreboard();
    if (cl.gametype == GAME_DEATHMATCH)
	Sbar_DeathmatchOverlay();
}

/* ============================================================================= */

/*
===============
Sbar_DrawInventory
===============
*/
static void
Sbar_DrawInventory(void)
{
    int i;
    char num[6];
    float time;
    int flashon;

    if (rogue) {
	if (cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN)
	    Sbar_DrawPic(0, -24, rsb_invbar[0]);
	else
	    Sbar_DrawPic(0, -24, rsb_invbar[1]);
    } else {
	Sbar_DrawPic(0, -24, sb_ibar);
    }

/* weapons */
    for (i = 0; i < 7; i++) {
	if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN << i)) {
	    time = cl.item_gettime[i];
	    flashon = qmax((int)(time * 10), 0);
	    if (flashon >= 10) {
		if (cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN << i))
		    flashon = 1;
		else
		    flashon = 0;
	    } else
		flashon = (flashon % 5) + 2;

	    Sbar_DrawPic(i * 24, -16, sb_weapons[flashon][i]);

	    if (flashon > 1)
		sb_updates = 0;	/* force update to remove flash */
	}
    }

/* MED 01/04/97 */
/* hipnotic weapons */
    if (hipnotic) {
	int grenadeflashing = 0;

	for (i = 0; i < 4; i++) {
	    if (cl.stats[STAT_ITEMS] & (1 << hipweapons[i])) {
		time = cl.item_gettime[hipweapons[i]];
		flashon = (int)(time * 10);
		if (flashon >= 10) {
		    if (cl.stats[STAT_ACTIVEWEAPON] == (1 << hipweapons[i]))
			flashon = 1;
		    else
			flashon = 0;
		} else
		    flashon = (flashon % 5) + 2;

		/* check grenade launcher */
		if (i == 2) {
		    if (cl.stats[STAT_ITEMS] & HIT_PROXIMITY_GUN) {
			if (flashon) {
			    grenadeflashing = 1;
			    Sbar_DrawPic(96, -16, hsb_weapons[flashon][2]);
			}
		    }
		} else if (i == 3) {
		    if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN << 4)) {
			if (flashon && !grenadeflashing) {
			    Sbar_DrawPic(96, -16, hsb_weapons[flashon][3]);
			} else if (!grenadeflashing) {
			    Sbar_DrawPic(96, -16, hsb_weapons[0][3]);
			}
		    } else
			Sbar_DrawPic(96, -16, hsb_weapons[flashon][4]);
		} else
		    Sbar_DrawPic(176 + (i * 24), -16,
				 hsb_weapons[flashon][i]);
		if (flashon > 1)
		    sb_updates = 0;	/* force update to remove flash */
	    }
	}
    }

    if (rogue) {
	/* check for powered up weapon. */
	if (cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN) {
	    for (i = 0; i < 5; i++) {
		if (cl.stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i)) {
		    Sbar_DrawPic((i + 2) * 24, -16, rsb_weapons[i]);
		}
	    }
	}
    }
/* ammo counts.  Skip the loop when the console covers
 * the inventory ammo-digit row in vid.buffer (the row
 * lives at logical y = -24, i.e., physical y = vid.height
 * - 48*scale for the digit top, spanning 8*scale pixels).
 * On the SW path the digits would land in vid.buffer
 * and then be overwritten by the conback that
 * SCR_DrawConsole queues right after Sbar_Draw -- a pure
 * CPU savings here.  On the Vulkan path it's the actual
 * fix: Sbar_DrawCharacter routes through Draw_Character
 * Scaled, which Phase 4o intercepts to the overlay
 * queue at every scale.  The conback in vid.buffer
 * (Draw_ConsoleBackground writes vid.buffer directly,
 * no intercept) can't reach into the overlay queue to
 * mask out the digit quads, so the '25 0 0 0' text
 * renders on top of what the SW renderer shows as a
 * clean console cover.  Match SW visually by source-
 * suppressing.
 *
 * Threshold: skip the moment any part of the digit row
 * enters the console-covered area.  scr_con_current
 * holds the on-screen height of the console (top is at
 * y = 0, bottom at scr_con_current).  Sbar_DrawCharacter
 * computes physical y as `y * scale + vid.height -
 * SBAR_HEIGHT * scale`, so at logical y = -24 the digit
 * row top sits at vid.height - (24 + SBAR_HEIGHT) *
 * scale.  The row enters the covered area when
 * scr_con_current exceeds that top.  At smaller
 * scr_con_current the digits sit below the console and
 * must draw normally; at the threshold or above SW
 * shows them fully covered and we skip to match. */
{
    int scale    = SCR_GetUIScale();
    int ammo_top = (int)vid.height - (24 + SBAR_HEIGHT) * scale;
    if ((int)scr_con_current <= ammo_top) {
	for (i = 0; i < 4; i++) {
	    snprintf(num, sizeof(num), "%3i", cl.stats[STAT_SHELLS + i]);
	    if (num[0] != ' ')
		Sbar_DrawCharacter((6 * i + 1) * 8 - 2, -24, 18 + num[0] - '0');
	    if (num[1] != ' ')
		Sbar_DrawCharacter((6 * i + 2) * 8 - 2, -24, 18 + num[1] - '0');
	    if (num[2] != ' ')
		Sbar_DrawCharacter((6 * i + 3) * 8 - 2, -24, 18 + num[2] - '0');
	}
    }
}

    flashon = 0;
    /* items */
    for (i = 0; i < 6; i++)
	if (cl.stats[STAT_ITEMS] & (1 << (17 + i))) {
	    time = cl.item_gettime[17 + i];
	    if (time < 2 && flashon) {	/* flash frame */
		sb_updates = 0;
	    } else {
		/* MED 01/04/97 changed keys */
		if (!hipnotic || (i > 1)) {
		    Sbar_DrawPic(192 + i * 16, -16, sb_items[i]);
		}
	    }
	    if (time < 2)
		sb_updates = 0;
	}
    /* MED 01/04/97 added hipnotic items */
    /* hipnotic items */
    if (hipnotic) {
	for (i = 0; i < 2; i++)
	    if (cl.stats[STAT_ITEMS] & (1 << (24 + i))) {
		time = cl.item_gettime[24 + i];
		if (time < 2 && flashon) {	/* flash frame */
		    sb_updates = 0;
		} else {
		    Sbar_DrawPic(288 + i * 16, -16, hsb_items[i]);
		}
		if (time < 2)
		    sb_updates = 0;
	    }
    }

    if (rogue) {
	/* new rogue items */
	for (i = 0; i < 2; i++) {
	    if (cl.stats[STAT_ITEMS] & (1 << (29 + i))) {
		time = cl.item_gettime[29 + i];

		if (time < 2 && flashon) {	/* flash frame */
		    sb_updates = 0;
		} else {
		    Sbar_DrawPic(288 + i * 16, -16, rsb_items[i]);
		}

		if (time < 2)
		    sb_updates = 0;
	    }
	}
    } else {
	/* sigils */
	for (i = 0; i < 4; i++) {
	    if (cl.stats[STAT_ITEMS] & (1 << (28 + i))) {
		time = cl.item_gettime[28 + i];
		if (time < 2 && flashon) {	/* flash frame */
		    sb_updates = 0;
		} else
		    Sbar_DrawPic(320 - 32 + i * 8, -16, sb_sigil[i]);
		if (time < 2)
		    sb_updates = 0;
	    }
	}
    }
}

/* ============================================================================= */

/*
===============
Sbar_DrawFrags
===============
*/
static void
Sbar_DrawFrags(void)
{
    int i, k, l;
    int top, bottom;
    int x, y, f;
    int xofs;
    char num[12];
    player_info_t *p;
    int scale = SCR_GetUIScale();

    Sbar_SortFrags();

/* draw the text */
    l = scoreboardlines <= 4 ? scoreboardlines : 4;

    x = 23;
    if (cl.gametype == GAME_DEATHMATCH)
	xofs = 0;
    else
	xofs = (vid.width - 320 * scale) >> 1;
    y = vid.height - SBAR_HEIGHT * scale - 23 * scale;

    for (i = 0; i < l; i++) {
	k = fragsort[i];
	p = &cl.players[k];
	if (!p->name[0])
	    continue;

	/* draw background (physical coords, scaled width) */
	top = Sbar_ColorForMap(p->topcolor);
	bottom = Sbar_ColorForMap(p->bottomcolor);

	Draw_Fill(xofs + (x * 8 + 10) * scale, y,
		  28 * scale, 4 * scale, top);
	Draw_Fill(xofs + (x * 8 + 10) * scale, y + 4 * scale,
		  28 * scale, 3 * scale, bottom);

	/* draw number (logical coords, wrapper handles scale) */
	f = p->frags;
	snprintf(num, sizeof(num), "%3i", f);

	Sbar_DrawCharacter((x + 1) * 8, -24, num[0]);
	Sbar_DrawCharacter((x + 2) * 8, -24, num[1]);
	Sbar_DrawCharacter((x + 3) * 8, -24, num[2]);

	if (k == cl.viewentity - 1) {
	    Sbar_DrawCharacter(x * 8 + 2, -24, 16);
	    Sbar_DrawCharacter((x + 4) * 8 - 4, -24, 17);
	}
	x += 4;
    }
}

/* ============================================================================= */


/*
===============
Sbar_DrawFace
===============
*/
static void
Sbar_DrawFace(void)
{
    int f, anim;

/* PGM 01/19/97 - team color drawing */
/* PGM 03/02/97 - fixed so color swatch only appears in CTF modes */
    if (rogue &&
	(cl.maxclients != 1) &&
	(teamplay.value > 3) && (teamplay.value < 7)) {
	int top, bottom;
	int xofs;
	char num[12];
	player_info_t *p;
	int scale = SCR_GetUIScale();

	p = &cl.players[cl.viewentity - 1];
	/* draw background */
	top = Sbar_ColorForMap(p->topcolor);
	bottom = Sbar_ColorForMap(p->bottomcolor);

	if (cl.gametype == GAME_DEATHMATCH)
	    xofs = 113 * scale;
	else
	    xofs = ((vid.width - 320 * scale) >> 1) + 113 * scale;

	Sbar_DrawPic(112, 0, rsb_teambord);
	Draw_Fill(xofs, vid.height - SBAR_HEIGHT * scale + 3 * scale,
		  22 * scale, 9 * scale, top);
	Draw_Fill(xofs, vid.height - SBAR_HEIGHT * scale + 12 * scale,
		  22 * scale, 9 * scale, bottom);

	/* draw number */
	f = p->frags;
	snprintf(num, sizeof(num), "%3i", f);

	if (top == 8) {
	    if (num[0] != ' ')
		Sbar_DrawCharacter(109, 3, 18 + num[0] - '0');
	    if (num[1] != ' ')
		Sbar_DrawCharacter(116, 3, 18 + num[1] - '0');
	    if (num[2] != ' ')
		Sbar_DrawCharacter(123, 3, 18 + num[2] - '0');
	} else {
	    Sbar_DrawCharacter(109, 3, num[0]);
	    Sbar_DrawCharacter(116, 3, num[1]);
	    Sbar_DrawCharacter(123, 3, num[2]);
	}

	return;
    }
/* PGM 01/19/97 - team color drawing */

    if ((cl.stats[STAT_ITEMS] & (IT_INVISIBILITY | IT_INVULNERABILITY))
	== (IT_INVISIBILITY | IT_INVULNERABILITY)) {
	Sbar_DrawPic(112, 0, sb_face_invis_invuln);
	return;
    }
    if (cl.stats[STAT_ITEMS] & IT_QUAD) {
	Sbar_DrawPic(112, 0, sb_face_quad);
	return;
    }
    if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY) {
	Sbar_DrawPic(112, 0, sb_face_invis);
	return;
    }
    if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY) {
	Sbar_DrawPic(112, 0, sb_face_invuln);
	return;
    }

    if (cl.stats[STAT_HEALTH] >= 100)
	f = 4;
    else
	f = cl.stats[STAT_HEALTH] / 20;

    if (cl.faceanimtime > 0) {
	anim = 1;
	sb_updates = 0;		/* make sure the anim gets drawn over */
    } else
	anim = 0;
    Sbar_DrawPic(112, 0, sb_faces[f][anim]);
}

/*
===============
Sbar_Draw
===============
*/
void
Sbar_Draw(void)
{
    int scale = SCR_GetUIScale();

    if (scr_con_current == vid.height)
	return;			/* console is full screen */

    /* Same architectural hazard as Con_DrawConsole at console.c::
     * Con_DrawConsole (Phase 4o-fixup): when the menu is up, M_Draw
     * is about to cover the screen with Draw_ConsoleBackground(vid
     * .height) or Draw_FadeScreen.  On the SW path those writes
     * land in vid.buffer and cover the HUD pics / digits Sbar_Draw
     * would have laid down there; the result is the menu-only
     * picture the user expects.  On the Vulkan path the Phase 4k /
     * 4o intercepts route every Sbar_DrawPic / Sbar_Number /
     * Draw_CharacterScaled into the per-frame overlay queue, but
     * Draw_ConsoleBackground writes vid.buffer directly (no
     * intercept) and so can't reach into the queue to mask out
     * what we just pushed.  The HUD pics and ammo digits then
     * render on top of the menu pics' transparent regions (gfx/
     * mainmenu.lmp is mostly transparent) and bleed through.
     *
     * Suppress at the source, same as Con_DrawConsole.  Saves the
     * per-frame Sbar_Draw cost while the menu is up too, on both
     * backends. */
    if (key_dest == key_menu)
	return;

    if (sb_updates >= vid.numpages)
	return;

    scr_copyeverything = 1;

    sb_updates++;

    /* Tile-clear residual area outside the (scaled) status bar with
     * the wood-grain backdrop. The backdrop itself is intentionally
     * left at native 1:1 to preserve the original look. */
    if (sb_lines && vid.width > 320 * scale)
	Draw_TileClear(0, vid.height - sb_lines, vid.width, sb_lines);

    if (sb_lines > 24 * scale) {
	Sbar_DrawInventory();
	if (cl.maxclients != 1)
	    Sbar_DrawFrags();
    }

    if (sb_showscores || cl.stats[STAT_HEALTH] <= 0) {
	Sbar_DrawPic(0, 0, sb_scorebar);
	Sbar_DrawScoreboard();
	sb_updates = 0;
    } else if (sb_lines) {
	Sbar_DrawPic(0, 0, sb_sbar);

	/* keys (hipnotic only) */
	/* MED 01/04/97 moved keys here so they would not be overwritten */
	if (hipnotic) {
	    if (cl.stats[STAT_ITEMS] & IT_KEY1)
		Sbar_DrawPic(209, 3, sb_items[0]);
	    if (cl.stats[STAT_ITEMS] & IT_KEY2)
		Sbar_DrawPic(209, 12, sb_items[1]);
	}
	/* armor */
	if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY) {
	    Sbar_DrawNum(24, 0, 666, 3, 1);
	    Sbar_DrawPic(0, 0, draw_disc);
	} else {
	    if (rogue) {
		Sbar_DrawNum(24, 0, cl.stats[STAT_ARMOR], 3,
			     cl.stats[STAT_ARMOR] <= 25);
		if (cl.stats[STAT_ITEMS] & RIT_ARMOR3)
		    Sbar_DrawPic(0, 0, sb_armor[2]);
		else if (cl.stats[STAT_ITEMS] & RIT_ARMOR2)
		    Sbar_DrawPic(0, 0, sb_armor[1]);
		else if (cl.stats[STAT_ITEMS] & RIT_ARMOR1)
		    Sbar_DrawPic(0, 0, sb_armor[0]);
	    } else {
		Sbar_DrawNum(24, 0, cl.stats[STAT_ARMOR], 3,
			     cl.stats[STAT_ARMOR] <= 25);
		if (cl.stats[STAT_ITEMS] & IT_ARMOR3)
		    Sbar_DrawPic(0, 0, sb_armor[2]);
		else if (cl.stats[STAT_ITEMS] & IT_ARMOR2)
		    Sbar_DrawPic(0, 0, sb_armor[1]);
		else if (cl.stats[STAT_ITEMS] & IT_ARMOR1)
		    Sbar_DrawPic(0, 0, sb_armor[0]);
	    }
	}

	/* face */
	Sbar_DrawFace();

	/* health */
	Sbar_DrawNum(136, 0, cl.stats[STAT_HEALTH], 3,
		     cl.stats[STAT_HEALTH] <= 25);

	/* ammo icon */
	if (rogue) {
	    if (cl.stats[STAT_ITEMS] & RIT_SHELLS)
		Sbar_DrawPic(224, 0, sb_ammo[0]);
	    else if (cl.stats[STAT_ITEMS] & RIT_NAILS)
		Sbar_DrawPic(224, 0, sb_ammo[1]);
	    else if (cl.stats[STAT_ITEMS] & RIT_ROCKETS)
		Sbar_DrawPic(224, 0, sb_ammo[2]);
	    else if (cl.stats[STAT_ITEMS] & RIT_CELLS)
		Sbar_DrawPic(224, 0, sb_ammo[3]);
	    else if (cl.stats[STAT_ITEMS] & RIT_LAVA_NAILS)
		Sbar_DrawPic(224, 0, rsb_ammo[0]);
	    else if (cl.stats[STAT_ITEMS] & RIT_PLASMA_AMMO)
		Sbar_DrawPic(224, 0, rsb_ammo[1]);
	    else if (cl.stats[STAT_ITEMS] & RIT_MULTI_ROCKETS)
		Sbar_DrawPic(224, 0, rsb_ammo[2]);
	} else {
	    if (cl.stats[STAT_ITEMS] & IT_SHELLS)
		Sbar_DrawPic(224, 0, sb_ammo[0]);
	    else if (cl.stats[STAT_ITEMS] & IT_NAILS)
		Sbar_DrawPic(224, 0, sb_ammo[1]);
	    else if (cl.stats[STAT_ITEMS] & IT_ROCKETS)
		Sbar_DrawPic(224, 0, sb_ammo[2]);
	    else if (cl.stats[STAT_ITEMS] & IT_CELLS)
		Sbar_DrawPic(224, 0, sb_ammo[3]);
	}

	Sbar_DrawNum(248, 0, cl.stats[STAT_AMMO], 3,
		     cl.stats[STAT_AMMO] <= 10);
    }

    if (vid.width > 320) {
	if (cl.gametype == GAME_DEATHMATCH)
	    Sbar_MiniDeathmatchOverlay();
    }
}

/* ============================================================================= */

/*
==================
Sbar_IntermissionNumber

==================
*/
static void
Sbar_IntermissionNumber(int x, int y, int num, int digits, int color)
{
    char str[12];
    char *ptr;
    int l, frame;
    int scale = SCR_GetUIScale();
    int xofs = (vid.width - 320 * scale) >> 1;

    l = Sbar_itoa(num, str);
    ptr = str;
    if (l > digits)
	ptr += (l - digits);
    if (l < digits)
	x += (digits - l) * 24;

    while (*ptr) {
	if (*ptr == '-')
	    frame = STAT_MINUS;
	else
	    frame = *ptr - '0';

	Draw_TransPicScaled(xofs + x * scale, y * scale,
			    sb_nums[color][frame], scale);
	x += 24;
	ptr++;
    }
}

/*
==================
Sbar_DeathmatchOverlay

==================
*/
void
Sbar_DeathmatchOverlay(void)
{
    const qpic_t *pic;
    int i, k, l;
    int top, bottom;
    int x, y, f;
    char num[12];
    player_info_t *p;
    int scale = SCR_GetUIScale();

    scr_copyeverything = 1;
    scr_fullupdate = 0;

    pic = Draw_CachePic("gfx/ranking.lmp");
    M_DrawPic((320 - pic->width) / 2, 8, pic);

/* scores */
    Sbar_SortFrags();

/* draw the text */
    l = scoreboardlines;

    x = 80 * scale + ((vid.width - 320 * scale) >> 1);
    y = 40 * scale;
    for (i = 0; i < l; i++) {
	k = fragsort[i];
	p = &cl.players[k];
	if (!p->name[0])
	    continue;

	/* draw background */
	top = Sbar_ColorForMap(p->topcolor);
	bottom = Sbar_ColorForMap(p->bottomcolor);

	Draw_Fill(x, y, 40 * scale, 4 * scale, top);
	Draw_Fill(x, y + 4 * scale, 40 * scale, 4 * scale, bottom);

	/* draw number */
	f = p->frags;
	snprintf(num, sizeof(num), "%3i", f);

	Draw_CharacterScaled(x + 8 * scale, y, num[0], scale);
	Draw_CharacterScaled(x + 16 * scale, y, num[1], scale);
	Draw_CharacterScaled(x + 24 * scale, y, num[2], scale);

	if (k == cl.viewentity - 1)
	    Draw_CharacterScaled(x - 8 * scale, y, 12, scale);


	/* draw name */
	Draw_StringScaled(x + 64 * scale, y, p->name, scale);

	y += 10 * scale;
    }
}

/*
==================
Sbar_DeathmatchOverlay

==================
*/
void
Sbar_MiniDeathmatchOverlay(void)
{
    int i, k;
    int top, bottom;
    int x, y, f;
    char num[12];
    player_info_t *p;
    int numlines;
    int scale = SCR_GetUIScale();

    /* Mini overlay only fits when the screen has room beyond the
     * (scaled) status bar plus a useful chunk of side-column space. */
    if (vid.width < 512 * scale || !sb_lines)
	return;

    scr_copyeverything = 1;
    scr_fullupdate = 0;

/* scores */
    Sbar_SortFrags();

/* draw the text */
    y = vid.height - sb_lines;
    numlines = sb_lines / (8 * scale);
    if (numlines < 3)
	return;

    /* find us */
    for (i = 0; i < scoreboardlines; i++)
	if (fragsort[i] == cl.viewentity - 1)
	    break;

    if (i == scoreboardlines)	/* we're not there */
	i = 0;
    else			/* figure out start */
	i = i - numlines / 2;

    if (i > scoreboardlines - numlines)
	i = scoreboardlines - numlines;
    if (i < 0)
	i = 0;

    x = 324 * scale;
    for ( /* */ ; i < scoreboardlines && y < vid.height - 8 * scale; i++) {
	k = fragsort[i];
	p = &cl.players[k];
	if (!p->name[0])
	    continue;

	/* draw background */
	top = Sbar_ColorForMap(p->topcolor);
	bottom = Sbar_ColorForMap(p->bottomcolor);

	Draw_Fill(x, y + 1 * scale, 40 * scale, 3 * scale, top);
	Draw_Fill(x, y + 4 * scale, 40 * scale, 4 * scale, bottom);

	/* draw number */
	f = p->frags;
	snprintf(num, sizeof(num), "%3i", f);

	Draw_CharacterScaled(x + 8 * scale, y, num[0], scale);
	Draw_CharacterScaled(x + 16 * scale, y, num[1], scale);
	Draw_CharacterScaled(x + 24 * scale, y, num[2], scale);

	if (k == cl.viewentity - 1) {
	    Draw_CharacterScaled(x, y, 16, scale);
	    Draw_CharacterScaled(x + 32 * scale, y, 17, scale);
	}

	/* draw name */
	Draw_StringScaled(x + 48 * scale, y, p->name, scale);

	y += 8 * scale;
    }
}

/*
==================
Sbar_IntermissionOverlay

==================
*/
void
Sbar_IntermissionOverlay(void)
{
    const qpic_t *pic;
    int dig;
    int num;
    int scale = SCR_GetUIScale();
    int xofs = (vid.width - 320 * scale) >> 1;

    scr_copyeverything = 1;
    scr_fullupdate = 0;

    if (cl.gametype == GAME_DEATHMATCH) {
	Sbar_DeathmatchOverlay();
	return;
    }

    pic = Draw_CachePic("gfx/complete.lmp");
    Draw_PicScaled(xofs + 64 * scale, 24 * scale, pic, scale);

    pic = Draw_CachePic("gfx/inter.lmp");
    Draw_TransPicScaled(xofs + 0, 56 * scale, pic, scale);

/* time */
    dig = cl.completed_time / 60;
    Sbar_IntermissionNumber(160, 64, dig, 3, 0);
    num = cl.completed_time - dig * 60;
    Draw_TransPicScaled(xofs + 234 * scale, 64 * scale, sb_colon, scale);
    Draw_TransPicScaled(xofs + 246 * scale, 64 * scale,
			sb_nums[0][num / 10], scale);
    Draw_TransPicScaled(xofs + 266 * scale, 64 * scale,
			sb_nums[0][num % 10], scale);

    Sbar_IntermissionNumber(160, 104, cl.stats[STAT_SECRETS], 3, 0);
    Draw_TransPicScaled(xofs + 232 * scale, 104 * scale, sb_slash, scale);
    Sbar_IntermissionNumber(240, 104, cl.stats[STAT_TOTALSECRETS], 3, 0);

    Sbar_IntermissionNumber(160, 144, cl.stats[STAT_MONSTERS], 3, 0);
    Draw_TransPicScaled(xofs + 232 * scale, 144 * scale, sb_slash, scale);
    Sbar_IntermissionNumber(240, 144, cl.stats[STAT_TOTALMONSTERS], 3, 0);

}


/*
==================
Sbar_FinaleOverlay

==================
*/
void
Sbar_FinaleOverlay(void)
{
    const qpic_t *pic;
    int scale = SCR_GetUIScale();

    scr_copyeverything = 1;

    pic = Draw_CachePic("gfx/finale.lmp");
    Draw_TransPicScaled((vid.width - pic->width * scale) / 2,
			16 * scale, pic, scale);
}
