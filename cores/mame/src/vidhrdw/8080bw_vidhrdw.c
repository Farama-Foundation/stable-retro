/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

/**************************************************************************/
/* Change Log                                                             */
/*                                                                        */
/* 7 Sep 2020                                                             */
/*                                                                        */
/* - Updates to all overlay colours as follows:                           */
/*   - Adjusted OVERLAY_BLUE used in overlays so it is viewable on black. */
/*   - Adjusted OVERLAY_LT_BLUE so it actually is blue not purple.        */
/*   - Added OVERLAY_ORANGE and OVERLAY_PURPLE for use in overlays.       */
/* - Updated overlays as follows:                                         */
/*   - invad2ct: added extra overlay and fixed position of overlays.      */
/*   - invaders: repositioned overlays to match actual hardware.          */
/*   - invaders: added a cocktail overlay.                                */
/*   - invrvnge: repositioned overlays to avoid ship partial colouring.   */ 
/* - Added new overlays for the following games (and variants):           */
/*   - 280zzzap                                                           */
/*   - clowns, clowns1                                                    */
/*   - cosmicmo, cosmicm2 (including cocktail variant)                    */
/*   - galxwars, galxwar2, galxwart, starw (including cocktail variant)   */
/*   - invaddlx                                                           */
/*   - maze                                                               */
/*   - ozmawars, ozmawar2, solfight, spaceph (including cocktail variant) */
/*   - spclaser, laser, spcewarl                                          */
/*   - sstranger (including coctail variant)                              */
/*   - yosakdon, yosakdoa                                                 */
/* - Added internal comments on overlays to show source of overlay.       */
/*                                                                        */
/* 21 Mar 2021                                                            */
/*                                                                        */
/* - Added overlay for astropal                                           */
/* - Added overlay for galactic (including cocktail variant)              */
/*                                                                        */
/**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "artwork.h"
#include "8080bw.h"
#include "math.h"

static int screen_red;
static int screen_red_enabled;		/* 1 for games that can turn the screen red */
static int color_map_select;
static int background_color;
static UINT8 cloud_pos;
static data8_t bowler_bonus_display;

static mem_write_handler videoram_w_p;
static void (*video_update_p)(struct mame_bitmap *bitmap,const struct rectangle *cliprect);

static WRITE_HANDLER( bw_videoram_w );
static WRITE_HANDLER( schaser_videoram_w );
static WRITE_HANDLER( lupin3_videoram_w );
static WRITE_HANDLER( polaris_videoram_w );
static WRITE_HANDLER( sstrngr2_videoram_w );
static WRITE_HANDLER( phantom2_videoram_w );
static WRITE_HANDLER( invadpt2_videoram_w );
static WRITE_HANDLER( cosmo_videoram_w );
static WRITE_HANDLER( lrescue_videoram_w );

static VIDEO_UPDATE( 8080bw_common );
static VIDEO_UPDATE( seawolf );
static VIDEO_UPDATE( blueshrk );
static VIDEO_UPDATE( desertgu );
static VIDEO_UPDATE( bowler );

static void plot_pixel_8080(int x, int y, int col);

/* Colors used in overlays. Smoothed out where possible */
/* so that overlays are not so contrasted */
#define OVERLAY_RED		MAKE_ARGB(0x04,0xff,0x20,0x20)
#define OVERLAY_GREEN		MAKE_ARGB(0x04,0x20,0xff,0x20)
/* Original blue is too dark so is replaced with crayola blue */
/* https://en.wikipedia.org/wiki/Shades_of_blue#Blue_(Crayola) */
/*#define OVERLAY_BLUE		MAKE_ARGB(0x04,0x20,0x20,0xff) */
#define OVERLAY_BLUE		MAKE_ARGB(0x04,0x1f,0x75,0xfe)
#define OVERLAY_YELLOW		MAKE_ARGB(0x04,0xff,0xff,0x20)
#define OVERLAY_CYAN		MAKE_ARGB(0x04,0x20,0xff,0xff)
/* Light blue originally used is more purple, so is replaced */
/* https://en.wikipedia.org/wiki/Shades_of_blue#Light_blue */
/*#define OVERLAY_LT_BLUE		MAKE_ARGB(0x04,0xa0,0xa0,0xff) */
#define OVERLAY_LT_BLUE		MAKE_ARGB(0x04,0xad,0xd8,0xe6)
#define OVERLAY_ORANGE		MAKE_ARGB(0x04,0xff,0xa5,0x00)
#define OVERLAY_PURPLE		MAKE_ARGB(0x04,0xff,0x00,0xff)
#define OVERLAY_BROWN		MAKE_ARGB(0x04,0x65,0x43,0x21)

#define COCKTAIL_ONLY		"cocktail"
#define UPRIGHT_ONLY		"upright"

/* Overlay comes from this page: */
/* https://www.proxibid.com/Art-Antiques-Collectibles/Collectibles/COSMIC-MONSTERS-II-UNIVERSAL-ARCADE-GAME-SOLD-AS-IS-TURNS-ON-BUT-SCREEN-DOESN-T-OPERATE-PROPER/lotInformation/43093078 */
/* The overlay is for Cosmic Monsters II, but we assume they */
/* had similar overlays, so apply to both. */
/* Unlike most emulators with an overlay for this game */
/* the actual machine seems to suggest the overlay is */
/* as below (that is, yellow, cyan, purple, green, yellow). */
/* There is no known photos of an overlay for cocktail mode */
/* So I've made a variant to handle this so both players */
/* get the same look from either angle. */
OVERLAY_START( cosmicmo_overlay )
    /* Common to both upright and cocktail */
	OVERLAY_RECT(  0, 0,  40, 224, OVERLAY_YELLOW )
	OVERLAY_RECT( 40, 0,  56, 224, OVERLAY_GREEN )
	OVERLAY_RECT( 56, 0, 184, 224, OVERLAY_PURPLE )

    /* Upright overlay */
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 216, 0, 240, 224, OVERLAY_YELLOW )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 184, 0, 216, 224, OVERLAY_CYAN )

    /* Cocktail overlay */
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 200, 0, 240, 224, OVERLAY_YELLOW )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 184, 0, 200, 224, OVERLAY_GREEN )
OVERLAY_END


/* Overlay based on actual arcade video sources: */
/* https://www.youtube.com/watch?v=0RA-42dPnPk */
/* https://www.youtube.com/watch?v=hmyVEJzgvZ0&feature=emb_logo */
/* Colour is much closer to cyan in videos and photos. */
OVERLAY_START( d280zzzap_overlay )
	OVERLAY_RECT(   0, 180, 256, 224, OVERLAY_CYAN )
OVERLAY_END


/* Overlay based on actual arcade video sources: */
/* https://www.youtube.com/watch?v=NajsSj3nRQI */
/* https://www.arcade-museum.com/game_detail.php?game_id=7356 */
/* Also for clowns1. */
OVERLAY_START( clowns_overlay )
	OVERLAY_RECT(   0,  24, 256,  43, OVERLAY_BLUE )
	OVERLAY_RECT(   0,  43, 256,  64, OVERLAY_GREEN )
	OVERLAY_RECT(   0,  64, 256,  85, OVERLAY_YELLOW )
OVERLAY_END


/* Overlay based on actual arcade video sources. */
/* https://www.youtube.com/watch?v=1uSzmzZP1s8 */
/* The green overlay was originally too high, so has been */
/* adjusted. The red was too low, and should have been */
/* just down to the row for the UFO, so has been adjusted. */
/* Also added an overlay for cocktail mode. This is harder */
/* to based on a "standard" given there appear to be so */
/* many overlays for cocktails. I've gone with this source: */
/* https://i.ebayimg.com/thumbs/images/g/lDMAAOSw2WheBkDx/s-l225.jpg */
/* which ensures the same look no matter which side of the */
/* machine the player is on. */
OVERLAY_START( invaders_overlay )
    /* Upright overlay */
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,  10,  0,  56, 224, OVERLAY_GREEN )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,   0, 18,   8, 134, OVERLAY_GREEN )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 200,  0, 216, 224, OVERLAY_RED )

    /* Cocktail overlay */
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 0,   0,  32, 224, OVERLAY_GREEN )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 32,  0,  56, 224, OVERLAY_ORANGE )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 56,  0,  96, 224, OVERLAY_BLUE )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 96,  0, 144, 224, OVERLAY_YELLOW )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 144, 0, 184, 224, OVERLAY_BLUE )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 184, 0, 208, 224, OVERLAY_ORANGE )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 208, 0, 240, 224, OVERLAY_GREEN )
OVERLAY_END


/* Overlay based on actual arcade video sources. */
/* https://www.youtube.com/watch?v=xnuIEFpZOCM */
/* This is specifically for invaddlx, but also works */
/* for moonbase. */
OVERLAY_START( invaddlx_overlay )
	OVERLAY_RECT(   0,   0,  32, 224, OVERLAY_CYAN )
	OVERLAY_RECT(  32,   0,  80, 224, OVERLAY_GREEN )
	OVERLAY_RECT(  80,   0, 168, 224, OVERLAY_YELLOW )
	OVERLAY_RECT( 168,   0, 200, 224, OVERLAY_ORANGE )
	OVERLAY_RECT( 200,   0, 240, 224, OVERLAY_RED )
OVERLAY_END


/* Cannot find any actual footage of a real arcade machine. */
/* Generally it was just a conversion of Space Invaders. */
/* Have adjusted the red overlay from what is generally used */
/* to ensure your ship doesn't go partly red when near the */
/* top of the screen. */
/* Also works for invrvnga. */
OVERLAY_START( invrvnge_overlay )
	OVERLAY_RECT(   0,   0,  64, 224, OVERLAY_GREEN )
	OVERLAY_RECT( 192,   0, 224, 224, OVERLAY_RED )
OVERLAY_END


/* Real arcade footage exists but actual overlay colours */
/* can't easily be determined. */
/* Updated overlays to ensure invaders don't get two overlays */
/* on same row. Fixed yellow overlay which was misplaced. */
/* Added an extra overlay to ensure player 2 also gets the */
/* same number of overlays. */
OVERLAY_START( invad2ct_overlay )
	OVERLAY_RECT(   0,   0,  48, 224, OVERLAY_YELLOW )
	OVERLAY_RECT(  24,   0,  72, 224, OVERLAY_GREEN )
	OVERLAY_RECT(  48,   0,  96, 224, OVERLAY_CYAN )
	OVERLAY_RECT(  48,   0, 137, 224, OVERLAY_BLUE )
	OVERLAY_RECT( 120,   0, 185, 224, OVERLAY_GREEN )
	OVERLAY_RECT( 160,   0, 232, 224, OVERLAY_YELLOW )
	OVERLAY_RECT( 209,   0, 256, 224, OVERLAY_RED )
OVERLAY_END


/* Overlay colours are based on the upright machine overlays shown */
/* in https://www.rotheblog.com/2019/07/classic-arcade-games/space-stranger-arcade-upright-yachiyo-electronics/ */
/* and https://www.arcade-museum.com/game_detail.php?game_id=18021 */
/* The overlay on the shields appears red in the first, and orange */
/* in the second; it is quite possible it was a red overlay on top */
/* of the larger yellow overlay, making orange, so orange is used */
/* in the overlay. */
/* There is also, shown in the second URL, an overlay for the */
/* cocktail varient; just blue and yellow. This is used in cocktail */
/* mode, though cocktail mode implementation is broken currently. */
OVERLAY_START( sstrangr_overlay )
    /* Upright overlay */
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,  0,   0,  16, 224, OVERLAY_BLUE )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 16,   0,  40, 224, OVERLAY_YELLOW )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 40,   0,  56, 224, OVERLAY_ORANGE )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 56,   0, 200, 224, OVERLAY_YELLOW )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,200,   0, 216, 224, OVERLAY_RED )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,216,   0, 240,  68, OVERLAY_BLUE )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,216,  68, 240, 148, OVERLAY_GREEN )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,216, 148, 240, 224, OVERLAY_BLUE )

    /* Cocktail overlay */
	OVERLAY_RECT_TAG( COCKTAIL_ONLY,  0,   0,  40, 224, OVERLAY_BLUE )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 40,   0, 200, 224, OVERLAY_YELLOW )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY,200,   0, 240, 224, OVERLAY_BLUE )
OVERLAY_END


/* Overlay based on flyer: */
/* https://flyers.arcade-museum.com/?page=flyer&db=videodb&id=5784&image=2 */
/* Can't find any flyers or videos of cocktail version, so have used */
/* Same overlay with different top and bottom to work in */
/* upside down mode. */
/* Also works for ozmawar2, solfight and spaceph. */
OVERLAY_START( ozmawars_overlay )
    /* Common to both upright and cocktail */
	OVERLAY_RECT( 200, 0, 224, 224, OVERLAY_PURPLE )
	OVERLAY_RECT( 175, 0, 200, 224, OVERLAY_BLUE )
	OVERLAY_RECT( 148, 0, 175, 224, OVERLAY_YELLOW )
	OVERLAY_RECT( 120, 0, 148, 224, OVERLAY_CYAN )
	OVERLAY_RECT(  88, 0, 120, 224, OVERLAY_PURPLE )
	OVERLAY_RECT(  56, 0,  88, 224, OVERLAY_YELLOW )
	OVERLAY_RECT(  32, 0,  56, 224, OVERLAY_ORANGE )
	OVERLAY_RECT(  16, 0,  32, 224, OVERLAY_CYAN )

    /* Upright overlay */
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 224,  74, 240, 148, OVERLAY_BLUE )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 224, 148, 240, 224, OVERLAY_YELLOW )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 224,   0, 240,  74, OVERLAY_CYAN )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,   0,   0,  16, 144, OVERLAY_CYAN )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,   0, 192,  16, 224, OVERLAY_CYAN )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,   0, 144,  16, 192, OVERLAY_PURPLE )

    /* Cocktail overlay */
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 224, 0, 240, 224, OVERLAY_CYAN )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY,   0, 0,  16, 224, OVERLAY_PURPLE )
OVERLAY_END

/* No known overlay exists, so I have just created one here.  */
/* Also works for yosakdoa. */
OVERLAY_START( yosakdon_overlay )
	OVERLAY_RECT( 216,  74, 240, 148, OVERLAY_PURPLE )
	OVERLAY_RECT( 216, 148, 240, 224, OVERLAY_GREEN )
	OVERLAY_RECT( 216,   0, 240,  74, OVERLAY_CYAN )
	OVERLAY_RECT( 177, 97, 184, 127, OVERLAY_GREEN )
	OVERLAY_RECT( 169, 127, 177, 143, OVERLAY_GREEN )
	OVERLAY_RECT( 169, 81, 177, 97, OVERLAY_GREEN )
	OVERLAY_RECT( 160, 143, 169, 160, OVERLAY_GREEN )
	OVERLAY_RECT( 160, 64, 169, 81, OVERLAY_GREEN )
	OVERLAY_RECT( 144, 159, 160, 176, OVERLAY_GREEN )
	OVERLAY_RECT( 144, 48, 160, 64, OVERLAY_GREEN )
	OVERLAY_RECT( 128, 176, 144, 200, OVERLAY_GREEN )
	OVERLAY_RECT( 128, 24, 144, 48, OVERLAY_GREEN )
	OVERLAY_RECT( 113, 200, 128, 224, OVERLAY_GREEN )
	OVERLAY_RECT( 113, 0, 128, 24, OVERLAY_GREEN )
	OVERLAY_RECT( 88, 208, 113, 224, OVERLAY_GREEN )
	OVERLAY_RECT( 88, 0, 113, 16, OVERLAY_GREEN )
	OVERLAY_RECT( 72, 118, 88, 119, OVERLAY_BROWN )
	OVERLAY_RECT( 72, 106, 88, 107, OVERLAY_BROWN )
	OVERLAY_RECT( 56, 119, 72, 120, OVERLAY_BROWN )
	OVERLAY_RECT( 56, 105, 72, 106, OVERLAY_BROWN )
	OVERLAY_RECT( 40, 120, 56, 121, OVERLAY_BROWN )
	OVERLAY_RECT( 40, 104, 56, 105, OVERLAY_BROWN )
	OVERLAY_RECT( 32, 121, 40, 122, OVERLAY_BROWN )
	OVERLAY_RECT( 32, 103, 40, 104, OVERLAY_BROWN )
	OVERLAY_RECT( 24, 103, 32, 122, OVERLAY_BROWN )
	OVERLAY_RECT( 9, 102, 24, 123, OVERLAY_BROWN )
	OVERLAY_RECT( 8, 0, 9, 224, OVERLAY_BROWN )
	OVERLAY_RECT( 0, 182, 8, 224, OVERLAY_CYAN )
	OVERLAY_RECT( 0, 137, 8, 182, OVERLAY_YELLOW )
	OVERLAY_RECT( 0, 0, 8, 137, OVERLAY_CYAN )
OVERLAY_END


/* Cannot find any actual arcade footage to show there is an */
/* overlay, but a light blue colour seems to be the standard */
/* used in emulators. */
OVERLAY_START( phantom2_overlay )
	OVERLAY_RECT(   0,   0, 240, 224, OVERLAY_LT_BLUE )
OVERLAY_END


/* Overlay based on actual arcade video sources. */
/* https://www.youtube.com/watch?v=u5vSZyDhFLo */
/* Video of actual arcade machine shows yellow screen. */
OVERLAY_START( gunfight_overlay )
	OVERLAY_RECT(   0,   0, 256, 224, OVERLAY_YELLOW )
OVERLAY_END

/* Overlay based on actual arcade video sources. */
/* https://youtu.be/KITGPAKABlY */
/* Video of actual arcade machine shows green screen. */
OVERLAY_START( maze_overlay )
	OVERLAY_RECT(   0,   0, 256, 224, OVERLAY_GREEN )
OVERLAY_END


/* The only actual screen shots I could find were from: */
/* https://flyers.arcade-museum.com/?page=flyer&db=videodb&id=3236&image=1 */
/* The overlay is on a cocktail version and clearly doesn't */
/* actually fit correctly with poor overlaps on the scoring */
/* area. So this overlay has attempted to mirror the colours */
/* and positions of overlays, but fix the scoring area while */
/* maintaining the look and feel of the screenshots. */
/* This also works with laser and spcewarl. */
OVERLAY_START( spclaser_overlay )
	OVERLAY_RECT( 232,   0, 240, 126, OVERLAY_BLUE )
	OVERLAY_RECT( 232, 126, 240, 224, OVERLAY_YELLOW )
	OVERLAY_RECT( 224,   0, 232, 224, OVERLAY_GREEN )
	OVERLAY_RECT( 216,   0, 224, 224, OVERLAY_RED )
	OVERLAY_RECT( 184,   0, 216, 224, OVERLAY_PURPLE )
	OVERLAY_RECT( 152,   0, 184, 224, OVERLAY_GREEN )
	OVERLAY_RECT( 120,   0, 152, 224, OVERLAY_BLUE )
	OVERLAY_RECT(  88,   0, 120, 224, OVERLAY_PURPLE )
	OVERLAY_RECT(  56,   0,  88, 224, OVERLAY_YELLOW )
	OVERLAY_RECT(  40,   0,  56, 224, OVERLAY_RED )
	OVERLAY_RECT(  32,   0,  40, 224, OVERLAY_YELLOW )
	OVERLAY_RECT(  24,   0,  32, 224, OVERLAY_PURPLE )
	OVERLAY_RECT(  16,   0,  24, 224, OVERLAY_RED )
	OVERLAY_RECT(   0,   0,  16, 224, OVERLAY_GREEN )
OVERLAY_END


/* Overlay based on images from flyer: */
/* https://flyers.arcade-museum.com/?page=flyer&db=videodb&id=431&image=1 */
/* Overlays are carefully placed to avoid meteors and */
/* titles being cut by two overlay colours. */
/* This overlay also works for galxwar2, galxwart, starw. */
/* A slight variant is used in cocktail mode (less colourful */
/* scores. */
OVERLAY_START( galxwars_overlay )
    /* Common to both upright and cocktail */
	OVERLAY_RECT( 184, 0, 224, 224, OVERLAY_CYAN )
	OVERLAY_RECT( 144, 0, 184, 224, OVERLAY_PURPLE )
	OVERLAY_RECT(  96, 0, 144, 224, OVERLAY_RED )
	OVERLAY_RECT(  56, 0,  96, 224, OVERLAY_GREEN )
	OVERLAY_RECT(  13, 0,  56, 224, OVERLAY_CYAN )

    /* Upright overlay */
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 224,   0, 240,  80, OVERLAY_CYAN )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 224,  80, 240, 144, OVERLAY_BLUE )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 224, 144, 240, 224, OVERLAY_YELLOW )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,  10,   0,  13, 224, OVERLAY_RED )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,   0,   0,  10, 224, OVERLAY_CYAN )

    /* Cocktail overlay */
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 224, 0, 240, 224, OVERLAY_CYAN )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY,   0, 0,  13, 224, OVERLAY_CYAN )
OVERLAY_END


/* No known flyers or images of game play are available. */
/* Added a simple overlay to give the game some colour. */
OVERLAY_START( astropal_overlay )
	OVERLAY_RECT(   0,   0,   85,  17, OVERLAY_CYAN )
	OVERLAY_RECT(  85,   0,  155,  17, OVERLAY_BLUE )
	OVERLAY_RECT( 155,   0,  256,  17, OVERLAY_YELLOW )
	OVERLAY_RECT(   0,  17,  256, 216, OVERLAY_GREEN )
	OVERLAY_RECT(   0, 216,  160, 224, OVERLAY_CYAN )
	OVERLAY_RECT( 160, 216,  256, 224, OVERLAY_PURPLE )
OVERLAY_END

/* Based on overlay shown on original machine here: */
/* https://youtu.be/BGcsreq6NEA?t=27 */
/* Can't find any sources for an overlay for cocktail */
/* mode, so a simple one has been added. */
OVERLAY_START( galactic_overlay )
    /* Common */
	OVERLAY_RECT(240,   0, 256, 224, OVERLAY_BLUE )

    /* Upright */
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 200,   0, 240, 224, OVERLAY_RED )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY, 136,   0, 200, 224, OVERLAY_ORANGE )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,  48,   0, 136, 224, OVERLAY_YELLOW )
	OVERLAY_RECT_TAG( UPRIGHT_ONLY,   0,   0,  48, 224, OVERLAY_GREEN )

    /* Cocktail */
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 208,   0, 240, 224, OVERLAY_GREEN )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY,  48,  0, 208, 224, OVERLAY_ORANGE )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY,  16,  0, 48, 224, OVERLAY_GREEN )
	OVERLAY_RECT_TAG( COCKTAIL_ONLY, 0,   0, 16, 224, OVERLAY_BLUE )
OVERLAY_END


DRIVER_INIT( 8080bw )
{
	videoram_w_p = bw_videoram_w;
	video_update_p = video_update_8080bw_common;
	screen_red = 0;
	screen_red_enabled = 0;
	color_map_select = 0;
	flip_screen_set(0);
}

DRIVER_INIT( cosmicmo )
{
	init_8080bw();
	artwork_set_overlay(cosmicmo_overlay);
}

DRIVER_INIT( yosakdon )
{
	init_8080bw();
	artwork_set_overlay(yosakdon_overlay);
}

DRIVER_INIT( galactic )
{
	init_8080bw();
	artwork_set_overlay(galactic_overlay);
}

DRIVER_INIT( astropal )
{
	init_8080bw();
	artwork_set_overlay(astropal_overlay);
}


DRIVER_INIT( 280zzzap )
{
	init_8080bw();
	artwork_set_overlay(d280zzzap_overlay);
}

DRIVER_INIT( clowns )
{
	init_8080bw();
	artwork_set_overlay(clowns_overlay);
}

DRIVER_INIT( invaders )
{
	init_8080bw();
	artwork_set_overlay(invaders_overlay);
}

DRIVER_INIT( invaddlx )
{
	init_8080bw();
	artwork_set_overlay(invaddlx_overlay);
}

DRIVER_INIT( invrvnge )
{
	init_8080bw();
	artwork_set_overlay(invrvnge_overlay);
}

DRIVER_INIT( invad2ct )
{
	init_8080bw();
	artwork_set_overlay(invad2ct_overlay);
}

DRIVER_INIT( sstrangr )
{
	init_8080bw();
	artwork_set_overlay(sstrangr_overlay);
}

DRIVER_INIT( sstrngr2 )
{
	init_8080bw();
	videoram_w_p = sstrngr2_videoram_w;
	screen_red_enabled = 1;
}

DRIVER_INIT( schaser )
{
	init_8080bw();
	videoram_w_p = schaser_videoram_w;
	background_color = 2;	/* blue */
}

DRIVER_INIT( rollingc )
{
	init_8080bw();
	videoram_w_p = schaser_videoram_w;
	background_color = 0;	/* black */
}

DRIVER_INIT( ozmawars )
{
	init_8080bw();
	artwork_set_overlay(ozmawars_overlay);
}


DRIVER_INIT( polaris )
{
	init_8080bw();
	videoram_w_p = polaris_videoram_w;
}

DRIVER_INIT( lupin3 )
{
	init_8080bw();
	videoram_w_p = lupin3_videoram_w;
}

DRIVER_INIT( invadpt2 )
{
	init_8080bw();
	videoram_w_p = invadpt2_videoram_w;
	screen_red_enabled = 1;
}

DRIVER_INIT( lrescue )
{
	init_8080bw();
	videoram_w_p = lrescue_videoram_w;
	screen_red_enabled = 1;
}

DRIVER_INIT( cosmo )
{
	init_8080bw();
	videoram_w_p = cosmo_videoram_w;
}

DRIVER_INIT( seawolf )
{
	init_8080bw();
	video_update_p = video_update_seawolf;
}

DRIVER_INIT( blueshrk )
{
	init_8080bw();
	video_update_p = video_update_blueshrk;
}

DRIVER_INIT( desertgu )
{
	init_8080bw();
	video_update_p = video_update_desertgu;
}

DRIVER_INIT( bowler )
{
	init_8080bw();
	video_update_p = video_update_bowler;
}

DRIVER_INIT( phantom2 )
{
	init_8080bw();
	videoram_w_p = phantom2_videoram_w;
	artwork_set_overlay(phantom2_overlay);
}

DRIVER_INIT( gunfight )
{
	init_8080bw();
	artwork_set_overlay(gunfight_overlay);
}

DRIVER_INIT( indianbt )
{
        init_8080bw();
        videoram_w_p = invadpt2_videoram_w;
}

DRIVER_INIT( maze )
{
	init_8080bw();
	artwork_set_overlay(maze_overlay);
}

DRIVER_INIT( spclaser )
{
	init_8080bw();
	artwork_set_overlay(spclaser_overlay);
}

DRIVER_INIT( galxwars )
{
	init_8080bw();
	artwork_set_overlay(galxwars_overlay);
}



void c8080bw_flip_screen_w(int data)
{
	set_vh_global_attribute(&color_map_select, data);

	if (input_port_3_r(0) & 0x01)
	{
		flip_screen_set(data);
	}
}


void c8080bw_screen_red_w(int data)
{
	if (screen_red_enabled)
	{
		set_vh_global_attribute(&screen_red, data);
	}
}


INTERRUPT_GEN( polaris_interrupt )
{
	static int cloud_speed;

	cloud_speed++;

	if (cloud_speed >= 8)	/* every 4 frames - this was verified against real machine */
	{
		cloud_speed = 0;

		cloud_pos--;

		if (cloud_pos >= 0xe0)
		{
			cloud_pos = 0xdf;	/* no delay for invisible region */
		}

		set_vh_global_attribute(NULL,0);
	}

	c8080bw_interrupt();
}


INTERRUPT_GEN( phantom2_interrupt )
{
	static int cloud_speed;

	cloud_speed++;

	if (cloud_speed >= 2)	/* every 2 frames - no idea of correct */
	{
		cloud_speed = 0;

		cloud_pos++;
		set_vh_global_attribute(NULL,0);
	}

	c8080bw_interrupt();
}


static void plot_pixel_8080(int x, int y, int col)
{
	if (flip_screen)
	{
		x = 255-x;
		y = 255-y;
	}

	plot_pixel(tmpbitmap,x,y,Machine->pens[col]);
}

static INLINE void plot_byte(int x, int y, int data, int fore_color, int back_color)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		plot_pixel_8080(x, y, (data & 0x01) ? fore_color : back_color);

		x++;
		data >>= 1;
	}
}


WRITE_HANDLER( c8080bw_videoram_w )
{
	videoram_w_p(offset, data);
}


static WRITE_HANDLER( bw_videoram_w )
{
	int x,y;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	plot_byte(x, y, data, 1, 0);
}

static WRITE_HANDLER( schaser_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	col = colorram[offset & 0x1f1f] & 0x07;

	plot_byte(x, y, data, col, background_color);
}

static WRITE_HANDLER( lupin3_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	col = ~colorram[offset & 0x1f1f] & 0x07;

	plot_byte(x, y, data, col, 0);
}

static WRITE_HANDLER( polaris_videoram_w )
{
	int x,i,col,back_color,fore_color,color_map;
	UINT8 y, cloud_y;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* for the background color, bit 0 of the map PROM is connected to green gun.
	   red is 0 and blue is 1, giving cyan and blue for the background.  This
	   is different from what the schematics shows, but it's supported
	   by screenshots. */

	color_map = memory_region(REGION_PROMS)[(y >> 3 << 5) | (x >> 3)];
	back_color = (color_map & 1) ? 6 : 2;
	fore_color = ~colorram[offset & 0x1f1f] & 0x07;

	/* bit 3 is connected to the cloud enable. bits 1 and 2 are marked 'not use' (sic)
	   on the schematics */

	if (y < cloud_pos)
	{
		cloud_y = y - cloud_pos - 0x20;
	}
	else
	{
		cloud_y = y - cloud_pos;
	}

	if ((color_map & 0x08) || (cloud_y > 64))
	{
		plot_byte(x, y, data, fore_color, back_color);
	}
	else
	{
		/* cloud appears in this part of the screen */
		for (i = 0; i < 8; i++)
		{
			if (data & 0x01)
			{
				col = fore_color;
			}
			else
			{
				int bit;
				offs_t offs;

				col = back_color;

				bit = 1 << (~x & 0x03);
				offs = ((x >> 2) & 0x03) | ((~cloud_y & 0x3f) << 2);

				col = (memory_region(REGION_USER1)[offs] & bit) ? 7 : back_color;
			}

			plot_pixel_8080(x, y, col);

			x++;
			data >>= 1;
		}
	}
}


WRITE_HANDLER( schaser_colorram_w )
{
	int i;


	offset &= 0x1f1f;

	colorram[offset] = data;

	/* redraw region with (possibly) changed color */
	for (i = 0; i < 8; i++, offset += 0x20)
	{
		videoram_w_p(offset, videoram[offset]);
	}
}

READ_HANDLER( schaser_colorram_r )
{
	return colorram[offset & 0x1f1f];
}


static WRITE_HANDLER( phantom2_videoram_w )
{
	static int CLOUD_SHIFT[] = { 0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08,
	                             0x10, 0x10, 0x20, 0x20, 0x40, 0x40, 0x80, 0x80 };

	int i,col;
	UINT8 x,y,cloud_x;
	UINT8 *cloud_region;
	offs_t cloud_offs;


	videoram[offset] = data;

	y = offset / 32;
	x = (offset % 32) * 8;


	cloud_region = memory_region(REGION_PROMS);
	cloud_offs = ((y - cloud_pos) & 0xff) >> 1 << 4;
	cloud_x = x - 12;  /* based on screen shots */


	for (i = 0; i < 8; i++)
	{
		if (data & 0x01)
		{
			col = 1;	/* white foreground */
		}
		else
		{
			UINT8 cloud_data;


			cloud_offs = (cloud_offs & 0xfff0) | (cloud_x >> 4);
			cloud_data = cloud_region[cloud_offs];

			if (cloud_data & (CLOUD_SHIFT[cloud_x & 0x0f]))
			{
				col = 2;	/* grey cloud */
			}
			else
			{
				col = 0;	/* black background */
			}
		}

		plot_pixel_8080(x, y, col);

		x++;
		cloud_x++;
		data >>= 1;
	}
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( 8080bw )
{
	video_update_p(bitmap, cliprect);
}


static VIDEO_UPDATE( 8080bw_common )
{
	if (get_vh_global_attribute_changed())
	{
		int offs;

		for (offs = 0;offs < videoram_size;offs++)
			videoram_w_p(offs, videoram[offs]);
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);
}


static void draw_sight(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int x_center, int y_center)
{
	int x,y;
	int sight_xs;
	int sight_xc;
	int sight_xe;
	int sight_ys;
	int sight_yc;
	int sight_ye;


	sight_xc = x_center;
	if( sight_xc < 2 )
	{
		sight_xc = 2;
	}
	else if( sight_xc > 253 )
	{
		sight_xc = 253;
	}

	sight_yc = y_center;
	if( sight_yc < 2 )
	{
		sight_yc = 2;
	}
	else if( sight_yc > 221 )
	{
		sight_yc = 221;
	}

	sight_xs = sight_xc - 20;
	if( sight_xs < 0 )
	{
		sight_xs = 0;
	}
	sight_xe = sight_xc + 20;
	if( sight_xe > 255 )
	{
		sight_xe = 255;
	}

	sight_ys = sight_yc - 20;
	if( sight_ys < 0 )
	{
		sight_ys = 0;
	}
	sight_ye = sight_yc + 20;
	if( sight_ye > 223 )
	{
		sight_ye = 223;
	}

	x = sight_xc;
	y = sight_yc;
	if (flip_screen)
	{
		x = 255-x;
		y = 255-y;
	}


	draw_crosshair(1, bitmap,x,y,cliprect);
}


static VIDEO_UPDATE( seawolf )
{
	/* update the bitmap (and erase old cross) */
	video_update_8080bw_common(bitmap, cliprect);

    draw_sight(bitmap,cliprect,((input_port_0_r(0) & 0x1f) * 8) + 4, 63);
}

static VIDEO_UPDATE( blueshrk )
{
	/* update the bitmap (and erase old cross) */
	video_update_8080bw_common(bitmap, cliprect);

    draw_sight(bitmap,cliprect,((input_port_0_r(0) & 0x7f) * 2) - 12, 63);
}

static VIDEO_UPDATE( desertgu )
{
	/* update the bitmap (and erase old cross) */
	video_update_8080bw_common(bitmap, cliprect);

	draw_sight(bitmap,cliprect,
			   ((input_port_0_r(0) & 0x7f) * 2) - 30,
			   ((input_port_2_r(0) & 0x7f) * 2) + 2);
}


WRITE_HANDLER( bowler_bonus_display_w )
{
	/* Bits 0-6 control which score is lit.
	   Bit 7 appears to be a global enable, but the exact
	   effect is not known. */

	bowler_bonus_display = data;
}


static VIDEO_UPDATE( bowler )
{
	int x,y,i;

	char score_line_1[] = "Bonus 200 400 500 700 500 400 200";
	char score_line_2[] = "      110 220 330 550 330 220 110";


	/* update the bitmap */
	video_update_8080bw_common(bitmap, cliprect);


	/* draw the current bonus value - on the original game this
	   was done using lamps that lit score displays on the bezel. */

	x = 33 * 8;
	y = 31 * 8;

	for (i = 0; i < 33; i++)
	{
		int col;


		col = UI_COLOR_NORMAL;

		if ((i >= 6) && ((i % 4) != 1))
		{
			int bit = (i - 6) / 4;

			if (bowler_bonus_display & (1 << bit))
			{
				col = UI_COLOR_INVERSE;
			}
		}


		drawgfx(bitmap,Machine->uifont,
				score_line_1[i],col,
				0,1,
				x,y,
				cliprect,TRANSPARENCY_NONE,0);

		drawgfx(bitmap,Machine->uifont,
				score_line_2[i],col,
				0,1,
				x+8,y,
				cliprect,TRANSPARENCY_NONE,0);

		y -= Machine->uifontwidth;
	}
}


PALETTE_INIT( invadpt2 )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		/* this bit arrangment is a little unusual but are confirmed by screen shots */
		int r = 0xff * ((i >> 0) & 1);
		int g = 0xff * ((i >> 2) & 1);
		int b = 0xff * ((i >> 1) & 1);
		palette_set_color(i,r,g,b);
	}
}

PALETTE_INIT( indianbt )
{
        int i;

        for (i = 0;i < Machine->drv->total_colors;i++)
        {
                int r = 0xff * ((i >> 0) & 1);
                int b = 0xff * ((i >> 2) & 1);
                int g = 0xff * ((i >> 1) & 1);
		palette_set_color(i,r,g,b);
        }
}

PALETTE_INIT( lrescue )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		/* clone of invadpt2 */
		int r = 0xff * ((i >> 0) & 1);
		int g = 0xff * ((i >> 2) & 1);
		int b = 0xff * ((i >> 1) & 1);
		palette_set_color(i,r,g,b);
	}
}

PALETTE_INIT( sflush )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		/* this bit arrangment is a little unusual but are confirmed by screen shots */
		int r = 0xff * ((i >> 0) & 1);
		int g = 0xff * ((i >> 2) & 1);
		int b = 0xff * ((i >> 1) & 1);
		palette_set_color(i,r,g,b);
	}

	/* Original colour used was way too purple. The colour should be a */
	/* medium blue, as confirmed from screen shots here: */
	/* https://flyers.arcade-museum.com/?page=flyer&db=videodb&id=1088&image=1 */
	/* Other colours seem to be more accurate. */
	/*palette_set_color(0,0x80,0x80,0xff); // Too purple */
	palette_set_color(0,0x1f,0x75,0xfe); /* Blue matching OVERLAY_BLUE */
}


static WRITE_HANDLER( invadpt2_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* 32 x 32 colormap */
	if (!screen_red)
	{
		UINT16 colbase;

		colbase = color_map_select ? 0x0400 : 0;
		col = memory_region(REGION_PROMS)[colbase | (y >> 3 << 5) | (x >> 3)] & 0x07;
	}
	else
		col = 1;	/* red */

	plot_byte(x, y, data, col, 0);
}

static WRITE_HANDLER( lrescue_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* 32 x 32 colormap */
	if (!screen_red)
	{
		UINT16 colbase;

		colbase = color_map_select ? 0x0400 : 0;
		col = memory_region(REGION_PROMS)[colbase | (y >> 3 << 5) | (x >> 3)] & 0x07;
	}
	else
		col = 1;	/* red */

	plot_byte(x, y, data, col, 0);
}

PALETTE_INIT( cosmo )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int r = 0xff * ((i >> 0) & 1);
		int g = 0xff * ((i >> 1) & 1);
		int b = 0xff * ((i >> 2) & 1);
		palette_set_color(i,r,g,b);
	}
}

WRITE_HANDLER( cosmo_colorram_w )
{
	int i;
	int offs = ((offset>>5)<<8) | (offset&0x1f);

	colorram[offset] = data;

	/* redraw region with (possibly) changed color */
	for (i=0; i<8; i++)
	{
		videoram_w_p(offs, videoram[offs]);
		offs+= 0x20;
	}		
}

static WRITE_HANDLER( cosmo_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = offset % 32;

	/* 32 x 32 colormap */
	col = colorram[(y >> 3 << 5) | x ] & 0x07;

	plot_byte(8*x, y, data, col, 0);
}

static WRITE_HANDLER( sstrngr2_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* 16 x 32 colormap */
	if (!screen_red)
	{
		UINT16 colbase;

		colbase = color_map_select ? 0 : 0x0200;
		col = memory_region(REGION_PROMS)[colbase | (y >> 4 << 5) | (x >> 3)] & 0x0f;
	}
	else
		col = 1;	/* red */

	if (color_map_select)
	{
		x = 240 - x;
		y = 31 - y;
	}

	plot_byte(x, y, data, col, 0);
}
