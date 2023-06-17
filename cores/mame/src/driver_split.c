/******************************************************************************

  driver_split.c

  The list of all available drivers. Drivers have to be included here to be
  recognized by the executable.

  To save some typing, we use a hack here. This file is recursively #included
  twice, with different definitions of the DRIVER() macro. The first one
  declares external references to the drivers; the second one builds an array
  storing all the drivers.

******************************************************************************/

#include "driver.h"


#ifndef DRIVER_RECURSIVE

/* The "root" driver, defined so we can have &driver_##NAME in macros. */
struct GameDriver driver_0 =
{
	__FILE__,
	0,
	"",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	NOT_A_DRIVER
};

#endif

#ifndef DRIVER_RECURSIVE

#define DRIVER_RECURSIVE

/* step 1: declare all external references */
#define DRIVER(NAME) extern struct GameDriver driver_##NAME;
#define TESTDRIVER(NAME) extern struct GameDriver driver_##NAME;
#include "driver_split.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#undef TESTDRIVER
#define DRIVER(NAME) &driver_##NAME,
#define TESTDRIVER(NAME)
const struct GameDriver *drivers[] =
{
#include "driver_split.c"
	0	/* end of array */
};

const int total_drivers = sizeof(drivers)/sizeof(drivers[0]);

/* step 3: define the test_drivers[] array */
#undef DRIVER
#undef TESTDRIVER
#define DRIVER(NAME)
#define TESTDRIVER(NAME) &driver_##NAME,

const struct GameDriver *test_drivers[] =
{
#include "driver_split.c"
	0	/* end of array */
};

#else	/* DRIVER_RECURSIVE */

#if defined(drv_1942) || defined(core_none)
	/* 1942.c */
	DRIVER( 1942 )		/* 12/1984 (c) 1984 */
	DRIVER( 1942a )		/* 12/1984 (c) 1984 */
	DRIVER( 1942b )		/* 12/1984 (c) 1984 */
#endif

#if defined(drv_1943) || defined(core_none)
	/* 1943.c */
	DRIVER( 1943 )		/*  6/1987 (c) 1987 (US) */
	DRIVER( 1943j )		/*  6/1987 (c) 1987 (Japan) */
	DRIVER( 1943mii )       /*  6/1987 (c) 1987 (US) */
	DRIVER( 1943kai )	/*  6/1988 (c) 1987 (Japan) */
#endif

#if defined(drv_1945kii) || defined(core_none)
	/* 1945kiii.c */
	DRIVER( 1945kiii )  /* (c) 2000 Oriental */
	DRIVER( flagrall )  /* (c) 1996 Promat? */
#endif

#if defined (drv_40love) || defined(core_none)
	/* 40love.c */
	DRIVER( fieldday )	/* A23 (c) 1984 Taito */
	DRIVER( undoukai )	/* A17 (c) 1984 Taito */
	DRIVER( 40love )	/* A30 (c) 1984 Taito */
#endif

#if defined(drv_4enraya) || defined(core_none)
	/* 4enraya.c */
	DRIVER( 4enraya )	/* (c) 1990 IDSA */
#endif

#if defined(drv_8080bw_drivers) || defined(core_none)
	/* 8080bw_drivers.c */
	/* Midway 8080 b/w games */
	DRIVER( seawolf )	/* 596 [1976] */
	DRIVER( gunfight )	/* 597 [1975] */
	/* 603 - Top Gun [1976] */
	DRIVER( tornbase )	/* 605 [1976] */
	DRIVER( 280zzzap )	/* 610 [1976] */
	DRIVER( maze )		/* 611 [1976] */
	DRIVER( boothill )	/* 612 [1977] */
	DRIVER( checkmat )	/* 615 [1977] */
	DRIVER( desertgu )	/* 618 [1977] */
	DRIVER( dplay )		/* 619 [1977] */
	DRIVER( lagunar )	/* 622 [1977] */
	DRIVER( gmissile )	/* 623 [1977] */
	DRIVER( m4 )		/* 626 [1977] */
	DRIVER( clowns )	/* 630 [1978] */
	DRIVER( clowns1 )	/* 630 [1978] */
	/* 640 - Space Walk [1978] */
	DRIVER( einnings )	/* 642 [1978] Midway */
	DRIVER( shuffle )	/* 643 [1978] */
	DRIVER( dogpatch )	/* 644 [1977] */
	DRIVER( spcenctr )	/* 645 (c) 1980 Midway */
	DRIVER( phantom2 )	/* 652 [1979] */
	DRIVER( bowler )	/* 730 [1978] Midway */
	DRIVER( invaders )	/* 739 [1979] */
	DRIVER( blueshrk )	/* 742 [1978] */
	DRIVER( invad2ct )	/* 851 (c) 1980 Midway */
	DRIVER( invaddlx )	/* 852 [1980] Midway */
	DRIVER( sitv )
	DRIVER( sicv )
	DRIVER( sisv )
	DRIVER( sisv2 )
	DRIVER( galxwars )
	DRIVER( galxwar2 )
	DRIVER( galxwart )
	DRIVER( starw )
	DRIVER( lrescue )	/* LR  (c) 1979 Taito */
	DRIVER( grescue )	/* bootleg? */
	DRIVER( desterth )	/* bootleg */
	DRIVER( invadpt2 )	/* 852 [1980] Taito */
	DRIVER( cosmo )		/* TDS+Mints */
	DRIVER( schaser )	/* RT  Taito */
	DRIVER( schasrcv )	/* RT  Taito */
	DRIVER( sflush ) /* (c)1979 Taito */
	DRIVER( lupin3 )	/* LP  (c) 1980 Taito */
	DRIVER( polaris )	/* PS  (c) 1980 Taito */
	DRIVER( polarisa )	/* PS  (c) 1980 Taito */
	DRIVER( ballbomb )	/* TN  (c) 1980 Taito */
	DRIVER( indianbt )	/* Taito - 1980 */
	DRIVER( steelwkr )	/* (c) 1980 Taito */
	DRIVER( earthinv )
	DRIVER( spaceatt )
	DRIVER( spaceat2 )
	DRIVER( sinvzen )
	DRIVER( sinvemag )
	DRIVER( tst_invd )
	DRIVER( alieninv )
	DRIVER( spceking )
	DRIVER( spcewars )
	DRIVER( spacewr3 )
	DRIVER( invaderl )
	DRIVER( jspecter )
	DRIVER( jspectr2 )
	DRIVER( cosmicmo )	/* Universal */
	DRIVER( cosmicm2 )	/* Universal */
	DRIVER( superinv )
	DRIVER( sstrangr )
	DRIVER( sstrngr2 )
	DRIVER( moonbase )	/* Zeta - Nichibutsu */
	DRIVER( invrvnge )
	DRIVER( invrvnga )
	DRIVER( spclaser )
	DRIVER( laser )
	DRIVER( spcewarl )
	DRIVER( rollingc )	/* Nichibutsu */
	DRIVER( ozmawars )	/* Shin Nihon Kikaku (SNK) */
	DRIVER( ozmawar2 )	/* Shin Nihon Kikaku (SNK) */
	DRIVER( solfight )	/* bootleg */
	DRIVER( spaceph )	/* Zilec Games */
	DRIVER( yosakdon )
	DRIVER( yosakdoa )
	DRIVER( invasion )
	DRIVER( invasioa )
	DRIVER( invasiob )
	DRIVER( astropal )
	DRIVER( galactic )
	DRIVER( spacmiss )
#endif

#if defined(drv_88games) || defined(core_none)
	/* 88games.c */
	DRIVER( 88games )	/* GX861 (c) 1988 */
	DRIVER( konami88 )	/* GX861 (c) 1988 */
	DRIVER( hypsptsp )	/* GX861 (c) 1988 (Japan) */
#endif

#if defined(drv_aburner) || defined(core_none)
	/* aburner.c */
	DRIVER( aburner )	/* (c) 1987 */
	DRIVER( aburner2 )  /* (c) 1987 */
	DRIVER( loffire )	/* (protected) */
	DRIVER( thndrbld )	/* (protected) */
	DRIVER( thndrbdj )  /* (protected?) */
#endif

#if defined(drv_ace) || defined(core_none)
	/* ace.c */
	DRIVER( ace )		/* [1976 Allied Leisure] */
#endif

#if defined(drv_actfancr) || defined(core_none)
	/* actfancr.c */
	DRIVER( actfancr )	/* (c) 1989 Data East Corporation (World) */
	DRIVER( actfanc1 )	/* (c) 1989 Data East Corporation (World) */
	DRIVER( actfancj )	/* (c) 1989 Data East Corporation (Japan) */
	DRIVER( triothep )	/* (c) 1989 Data East Corporation (Japan) */
#endif

#if defined(drv_aeroboto) || defined(core_none)
	/* aeroboto.c */
	DRIVER( formatz )	/* (c) 1984 Jaleco */
	DRIVER( aeroboto )	/* (c) 1984 Williams */
#endif

#if defined(drv_aerofgt) || defined(core_none)
	/* aerofgt.c */
	DRIVER( spinlbrk )	/* (c) 1990 V-System Co. (World) */
	DRIVER( spinlbru )	/* (c) 1990 V-System Co. (US) */
	DRIVER( spinlbrj )	/* (c) 1990 V-System Co. (Japan) */
	DRIVER( pspikes )	/* (c) 1991 Video System Co. (World) */
	DRIVER( pspikesk )	/* (c) 1991 Video System Co. (Korea) */
	DRIVER( svolly91 )	/* (c) 1991 Video System Co. (Japan) */
	DRIVER( karatblz )	/* (c) 1991 Video System Co. */
	DRIVER( karatblu )	/* (c) 1991 Video System Co. (US) */
	DRIVER( turbofrc )	/* (c) 1991 Video System Co. */
	DRIVER( aerofgt )	/* (c) 1992 Video System Co. */
	DRIVER( aerofgtb )	/* (c) 1992 Video System Co. */
	DRIVER( aerofgtc )	/* (c) 1992 Video System Co. */
	DRIVER( sonicwi )	/* (c) 1992 Video System Co. (Japan) */
#endif

#if defined(drv_afega) || defined(core_none)
	/* afega.c */
	DRIVER( stagger1 )	/* (c) 1998 */
	DRIVER( redhawk )	  /* (c) 1997 */
	DRIVER( grdnstrm )	/* (c) 1998 */
	DRIVER( bubl2000 )	/* (c) 1998 Tuning */
	DRIVER( spec2k )    /* (c) 2000 Yonatech */
	DRIVER( firehawk )  /* (c) 2001 ESD */
#endif

#if defined(drv_airbustr) || defined(core_none)
	/* airbustr.c */
	DRIVER( airbustr )	/* (c) 1990 Kaneko + Namco */
	DRIVER( airbustj )	/* (c) 1990 Kaneko + Namco (Japan) */
#endif

#if defined(drv_ajax) || defined(core_none)
	/* ajax.c */
	DRIVER( ajax )		/* GX770 (c) 1987 */
	DRIVER( typhoon )	/* GX770 (c) 1987 */
	DRIVER( ajaxj )		/* GX770 (c) 1987 (Japan) */
#endif

#if defined(drv_aliens) || defined(core_none)
	/* aliens.c */
	DRIVER( aliens )	/* GX875 (c) 1990 (World) */
	DRIVER( aliens2 )	/* GX875 (c) 1990 (World) */
	DRIVER( aliensu )	/* GX875 (c) 1990 (US) */
	DRIVER( aliensj )	/* GX875 (c) 1990 (Japan) */
#endif

#if defined(drv_alpha68k) || defined(core_none)
	/* alpha68k.c */
	DRIVER( sstingry )	/* (c) 1986 Alpha Denshi Co. */
	DRIVER( kyros )		/* (c) 1987 World Games */
	DRIVER( kyrosj )	/* (c) 1986 Alpha Denshi Co. */
	DRIVER( paddlema )	/* Alpha-68K96I  'PM' (c) 1988 SNK */
	DRIVER( timesold )	/* Alpha-68K96II 'BT' (c) 1987 SNK / Romstar */
	DRIVER( timesol1 )  /* Alpha-68K96II 'BT' (c) 1987 */
	DRIVER( btlfield )  /* Alpha-68K96II 'BT' (c) 1987 */
	DRIVER( skysoldr )	/* Alpha-68K96II 'SS' (c) 1988 SNK (Romstar with dip switch) */
	DRIVER( goldmedl )	/* Alpha-68K96II 'GM' (c) 1988 SNK */
	DRIVER( goldmedb )	/* Alpha-68K96II bootleg */
	DRIVER( skyadvnt )	/* Alpha-68K96V  'SA' (c) 1989 Alpha Denshi Co. */
	DRIVER( skyadvnu )	/* Alpha-68K96V  'SA' (c) 1989 SNK of America licensed from Alpha */
	DRIVER( skyadvnj )	/* Alpha-68K96V  'SA' (c) 1989 Alpha Denshi Co. */
	DRIVER( gangwars )	/* Alpha-68K96V       (c) 1989 Alpha Denshi Co. */
	DRIVER( gangwarb )	/* Alpha-68K96V bootleg */
	DRIVER( sbasebal )	/* Alpha-68K96V       (c) 1989 SNK of America licensed from Alpha */
	DRIVER( tnexspce )	/* A8003 'NS' (c) 1989 */
#endif

#if defined(drv_ambush) || defined(core_none)
	/* ambush.c */
	DRIVER( ambush )	/* (c) 1983 Nippon Amuse Co-Ltd */
	DRIVER( ambusht )	/* (c) 1983 Tecfri */
#endif

#if defined(drv_amidar) || defined(core_none)
	/* amidar.c */
	DRIVER( amidar )	/* GX337 (c) 1981 Konami */
	DRIVER( amidaru )	/* GX337 (c) 1982 Konami + Stern license */
	DRIVER( amidaro )	/* GX337 (c) 1982 Konami + Olympia license */
	DRIVER( amigo )		/* bootleg */
	DRIVER( turtles )	/* (c) 1981 Stern */
	DRIVER( turpin )	/* (c) 1981 Sega */
	DRIVER( 600 )		/* GX353 (c) 1981 Konami */
#endif

#if defined(drv_amspdwy) || defined(core_none)
	/* amspdwy.c */
	DRIVER( amspdwy )	/* no copyright notice, but (c) 1987 Enerdyne Technologies, Inc. */
	DRIVER( amspdwya )	/* no copyright notice, but (c) 1987 Enerdyne Technologies, Inc. */
#endif

#if defined(drv_angelkds) || defined(core_none)
	/* angelkds.c */
	DRIVER( angelkds )	/* 833-6599 (c) 1988 Sega / Nasco? */
	DRIVER( spcpostn )	/* (c) 1986 Sega / Nasco" */
#endif

#if defined(drv_appoooh) || defined(core_none)
	/* appoooh.c */
	DRIVER( appoooh )	/* (c) 1984 Sega */	/* made by Sanritsu */
#endif

#if defined(drv_aquarium) || defined(core_none)
	/* aquarium.c */
	DRIVER( aquarium )	/* (c) 1996 Excellent System */
#endif

#if defined(drv_arabian) || defined(core_none)
	/* arabian.c */
	DRIVER( arabian )	/* TVG13 (c) 1983 Sun Electronics */
	DRIVER( arabiana )	/* 136019			(c) 1983 Atari */
#endif

#if defined(drv_arcadecl) || defined(core_none)
	/* arcadecl.c */
	DRIVER( arcadecl )	/* (proto)			(c) 1992 */
	DRIVER( sparkz )	/* (proto)			(c) 1992 */
#endif

#if defined(drv_argus) || defined(core_none)
	/* argus.c */
	DRIVER( argus )		/* (c) 1986 Jaleco */
	DRIVER( valtric )	/* (c) 1986 Jaleco */
	DRIVER( butasan )	/* (c) 1987 Jaleco */
#endif

#if defined(drv_arkanoid) || defined(core_none)
	/* arkanoid.c */
	DRIVER( arkanoid )	/* A75 (c) 1986 Taito Corporation Japan (World) */
	DRIVER( arknoidu )	/* A75 (c) 1986 Taito America Corporation + Romstar license (US) */
	DRIVER( arknoiuo )	/* A75 (c) 1986 Taito America Corporation + Romstar license (US) */
	DRIVER( arknoidj )	/* A75 (c) 1986 Taito Corporation (Japan) */
	DRIVER( arkbl2 )	/* bootleg */
	DRIVER( arkbl3 )	/* bootleg */
	DRIVER( paddle2 )	/* bootleg */
	DRIVER( arkatayt )	/* bootleg */
	DRIVER( arkblock )	/* bootleg */
	DRIVER( arkbloc2 )	/* bootleg */
	DRIVER( arkangc )	/* bootleg */
	DRIVER( arkatour )	/* ??? (c) 1987 Taito America Corporation + Romstar license (US) */
#endif

#if defined(drv_armedf) || defined(core_none)
	/* armedf.c */
	DRIVER( legion )	/* (c) 1986 */
	DRIVER( legiono )	/* (c) 1986 */
	DRIVER( terraf )	/* (c) 1987 */
	DRIVER( terrafu )	/* (c) 1987 Nichibutsu USA */
	DRIVER( kodure )	/* (c) 1987 (Japan) */
	DRIVER( cclimbr2 )	/* (c) 1988 (Japan) */
	DRIVER( armedf )	/* (c) 1988 */
#endif

#if defined(drv_artmagic) || defined(core_none)
	/* artmagic.c */
	DRIVER( ultennis )	/* (c) 1993 */
	DRIVER( cheesech )	/* (c) 1994 */
	DRIVER( stonebal )	/* (c) 1994 */
	DRIVER( stoneba2 )	/* (c) 1994 */
#endif

#if defined(drv_ashnojoe) || defined(core_none)
	/* ashnojoe.c */
	DRIVER( scessjoe )	/* ??? (c) 1990 Wave / Taito */
	DRIVER( ashnojoe )	/* ??? (c) 1990 Wave / Taito */
#endif

#if defined(drv_asterix) || defined(core_none)
	/* asterix.c */
	DRIVER( asterix )	/* GX068 (c) 1992 (World) */
	DRIVER( astrxeac )	/* GX068 (c) 1992 (World) */
	DRIVER( astrxeaa )	/* GX068 (c) 1992 (World) */
#endif

#if defined(drv_asteroid) || defined(core_none)
	/* asteroid.c */
	DRIVER( asteroid )	/* 035127-035145	(c) 1979 */
	DRIVER( asteroi1 )	/* 035127-035145	no copyright notice */
	DRIVER( asteroib )	/* (bootleg) */
	DRIVER( asterock )	/* Sidam bootleg	(c) 1979 */
	DRIVER( astdelux )	/* 0351xx			(c) 1980 */
	DRIVER( astdelu1 )	/* 0351xx			(c) 1980 */
	DRIVER( llander )	/* 0345xx			no copyright notice */
	DRIVER( llander1 )	/* 0345xx			no copyright notice */
#endif

#if defined(drv_astinvad) || defined(core_none)
	/* astinvad.c */
	DRIVER( astinvad )	/* (c) 1980 Stern */
	DRIVER( kamikaze )	/* Leijac Corporation */
	DRIVER( spcking2 )
	DRIVER( spaceint )	/* [1980] Shoei */
#endif

#if defined(drv_astrocde) || defined(core_none)
	/* astrocde.c */
	DRIVER( seawolf2 )
	DRIVER( spacezap )	/* (c) 1980 */
	DRIVER( ebases )
	DRIVER( wow )		/* (c) 1980 */
	DRIVER( gorf )		/* (c) 1981 */
	DRIVER( gorfpgm1 )	/* (c) 1981 */
	DRIVER( robby )		/* (c) 1981 Bally Midway */
	DRIVER( profpac )	/* (c) 1983 Bally Midway */
#endif

#if defined(drv_astrof) || defined(core_none)
	/* astrof.c */
	DRIVER( astrof )	/* (c) [1980?] */
	DRIVER( astrof2 )	/* (c) [1980?] */
	DRIVER( astrof3 )	/* (c) [1980?] */
	DRIVER( tomahawk )	/* (c) [1980?] */
	DRIVER( tomahaw5 )	/* (c) [1980?] */
#endif

#if defined(drv_asuka) || defined(core_none)
	/* asuka.c */
	DRIVER( bonzeadv )	/* B41 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( bonzeadu )	/* B41 (c) 1988 Taito America Corporation (US) */
	DRIVER( jigkmgri )	/* B41 (c) 1988 Taito Corporation (Japan)*/
	DRIVER( asuka )		/* ??? (c) 1988 Taito Corporation (Japan) */
	DRIVER( mofflott )	/* C17 (c) 1989 Taito Corporation (Japan) */
	DRIVER( cadash )	/* C21 (c) 1989 Taito Corporation Japan */
	DRIVER( cadashj )	/* C21 (c) 1989 Taito Corporation */
	DRIVER( cadashu )	/* C21 (c) 1989 Taito America Corporation */
	DRIVER( cadashi )	/* C21 (c) 1989 Taito Corporation Japan */
	DRIVER( cadashf )	/* C21 (c) 1989 Taito Corporation Japan */
	DRIVER( galmedes )	/* (c) 1992 Visco (Japan) */
	DRIVER( earthjkr )	/* (c) 1993 Visco (Japan) */
	DRIVER( eto )		/* (c) 1994 Visco (Japan) */
#endif

#if defined(drv_atarifb) || defined(core_none)
	/* atarifb.c */
	DRIVER( atarifb )	/* 033xxx			1978/10 [6502] */
	DRIVER( atarifb1 )	/* 033xxx			1978/10 [6502] */
	DRIVER( atarifb4 )	/* 034754			1979/04 [6502] */
	DRIVER( abaseb )	/* 034711-034738	1979/06 [6502] */
	DRIVER( abaseb2 )	/* 034711-034738	1979/06 [6502] */
	DRIVER( soccer )	/* 035222-035260	1980/04 [6502] */
#endif

#if defined(drv_atarig1) || defined(core_none)
	/* atarig1.c */
	DRIVER( hydra )		/* 136079			(c) 1990 */
	DRIVER( hydrap )	/* (proto)			(c) 1990 */
	DRIVER( hydrap2 )	/* (proto)			(c) 1990 */
	DRIVER( pitfight )	/* 136081			(c) 1990 */
	DRIVER( pitfigh3 )	/* 136081			(c) 1990 */
	DRIVER( pitfighj )	/* Japan */
	DRIVER( pitfighb )	/* bootleg */
#endif

#if defined(drv_atarig42) || defined(core_none)
	/* atarig42.c */
	DRIVER( roadriot )	/* 136089			(c) 1991 */
	DRIVER( guardian )	/* 136092			(c) 1992 */
	DRIVER( dangerex )	/* (proto)			(c) 1992 */
#endif

#if defined(drv_atarigt) || defined(core_none)
	/* atarigt.c */
	DRIVER( tmek )		/* 136100			(c) 1994 */
	DRIVER( tmekprot )	/* 136100			(c) 1994 */
	DRIVER( primrage )	/* 136102			(c) 1994 */
	DRIVER( primraga )	/* 136102			(c) 1994 */
#endif

#if defined(drv_atarigx2) || defined(core_none)
	/* atarigx2.c */
	DRIVER( spclords )	/* 136095			(c) 1992 */
	DRIVER( spclorda )	/* 136095			(c) 1992 */
	DRIVER( motofren )	/* 136094			(c) 1992 */
	DRIVER( rrreveng )	/*     ??			(c) 1993 */
	DRIVER( rrrevenp )	/*     ??		    (c) 1993 */
#endif

#if defined(drv_atarisy1) || defined(core_none)
	/* atarisy1.c */
	DRIVER( marble )	/* 136033			(c) 1984 */
	DRIVER( marble2 )	/* 136033			(c) 1984 */
	DRIVER( marble3 )	/* 136033			(c) 1984 */
	DRIVER( marble4 )	/* 136033			(c) 1984 */
	DRIVER( indytemp )	/* 136036			(c) 1985 */
	DRIVER( indytem2 )	/* 136036			(c) 1985 */
	DRIVER( indytem3 )	/* 136036			(c) 1985 */
	DRIVER( indytem4 )	/* 136036			(c) 1985 */
	DRIVER( indytemd )	/* 136036			(c) 1985 */
	DRIVER( roadrunn )	/* 136040			(c) 1985 */
	DRIVER( roadrun2 )	/* 136040			(c) 1985 */
	DRIVER( roadrun1 )	/* 136040			(c) 1985 */
	DRIVER( roadblst )	/* 136048			(c) 1986, 1987 */
	DRIVER( roadblsg )	/* 136048			(c) 1986, 1987 */
	DRIVER( roadbls3 )	/* 136048			(c) 1986, 1987 */
	DRIVER( roadblg2 )	/* 136048			(c) 1986, 1987 */
	DRIVER( roadbls2 )	/* 136048			(c) 1986, 1987 */
	DRIVER( roadblg1 )	/* 136048			(c) 1986, 1987 */
	DRIVER( roadbls1 )	/* 136048			(c) 1986, 1987 */
	DRIVER( roadblsc )	/* 136048			(c) 1986, 1987 */
	DRIVER( roadblcg )	/* 136048			(c) 1986, 1987 */
	DRIVER( roadblc1 )	/* 136048			(c) 1986, 1987 */
#endif

#if defined(drv_atarisy2) || defined(core_none)
	/* atarisy2.c */
	DRIVER( paperboy )	/* 136034			(c) 1984 */
	DRIVER( paperbr2 )	/* 136034			(c) 1984 */
	DRIVER( paperbr1 )	/* 136034			(c) 1984 */
	DRIVER( 720 )		/* 136047			(c) 1986 */
	DRIVER( 720r3 )		/* 136047			(c) 1986 */
	DRIVER( 720r2 )		/* 136047			(c) 1986 */
	DRIVER( 720r1 )		/* 136047			(c) 1986 */
	DRIVER( 720g )		/* 136047			(c) 1986 */
	DRIVER( 720gr1 )		/* 136047			(c) 1986 */
	DRIVER( ssprint )	/* 136042			(c) 1986 */
	DRIVER( ssprint3 )	/* 136042			(c) 1986 */
	DRIVER( ssprint1 )	/* 136042			(c) 1986 */
	DRIVER( ssprintg )	/* 136042			(c) 1986 */
	DRIVER( sspring1 )	/* 136042			(c) 1986 */
	DRIVER( ssprintf )	/* 136042			(c) 1986 */
	DRIVER( ssprints )	/* 136042			(c) 1986 */
	DRIVER( csprint )	/* 136045			(c) 1986 */
	DRIVER( csprint2 )	/* 136045			(c) 1986 */
	DRIVER( csprint1 )	/* 136045			(c) 1986 */
	DRIVER( csprintg )	/* 136045			(c) 1986 */
	DRIVER( cspring1 )	/* 136045			(c) 1986 */
	DRIVER( csprintf )	/* 136045			(c) 1986 */
	DRIVER( csprints )	/* 136045			(c) 1986 */
	DRIVER( csprins1 )	/* 136045			(c) 1986 */
	DRIVER( apb )		/* 136051			(c) 1987 */
	DRIVER( apb6 )		/* 136051			(c) 1987 */
	DRIVER( apb5 )		/* 136051			(c) 1987 */
	DRIVER( apb4 )		/* 136051			(c) 1987 */
	DRIVER( apb3 )		/* 136051			(c) 1987 */
	DRIVER( apb2 )		/* 136051			(c) 1987 */
	DRIVER( apb1 )		/* 136051			(c) 1987 */
	DRIVER( apbg )		/* 136051			(c) 1987 */
	DRIVER( apbf )		/* 136051			(c) 1987 */
#endif

#if defined(drv_ataxx) || defined(core_none)
	/* ataxx.c */
	DRIVER( ataxx )		/* (c) 1990 Leland */
	DRIVER( ataxxa )	/* (c) 1990 Leland */
	DRIVER( ataxxj )	/* (c) 1990 Leland */
	DRIVER( wsf )		/* (c) 1990 Leland */
	DRIVER( indyheat )	/* (c) 1991 Leland */
	DRIVER( brutforc )	/* (c) 1991 Leland */
	DRIVER( asylum )	/* (c) 1991 Leland */
#endif

#if defined(drv_atetris) || defined(core_none)
	/* atetris.c */
	DRIVER( atetris )	/* 136066			(c) 1988 */
	DRIVER( atetrisa )	/* 136066			(c) 1988 */
	DRIVER( atetrisb )	/* (bootleg) */
	DRIVER( atetcktl )	/* 136066			(c) 1989 */
	DRIVER( atetckt2 )	/* 136066			(c) 1989 */
#endif

#if defined(drv_avalnche) || defined(core_none)
	/* avalnche.c */
	DRIVER( avalnche )	/* 030574			1978/04 [6502] */
#endif

#if defined(drv_aztarac) || defined(core_none)
	/* aztarac.c */
	DRIVER( aztarac )	/* (c) 1983 Centuri (vector game) */
#endif

#if defined(drv_backfire) || defined(core_none)
	/* backfire.c */
	DRIVER( backfire )      /* MBZ (c) 1995 Data East Corporation */
#endif

#if defined(drv_badlands) || defined(core_none)
	/* badlands.c */
	DRIVER( badlands )	/* 136074			(c) 1989 */
#endif

#if defined(drv_bagman) || defined(core_none)
	/* bagman.c */
	DRIVER( bagman )	/* (c) 1982 */
	DRIVER( bagnard )	/* (c) 1982 */
	DRIVER( bagmans )	/* (c) 1982 + Stern license */
	DRIVER( bagmans2 )	/* (c) 1982 + Stern license */
	DRIVER( sbagman )	/* (c) 1984 */
	DRIVER( sbagmans )	/* (c) 1984 + Stern license */
	DRIVER( pickin )	/* (c) 1983 */
	DRIVER( botanic )	/* (c) 1983 */
	DRIVER( botanicf )	/* (c) 1984 */
#endif

#if defined(drv_balsente) || defined(core_none)
	/* balsente.c */
	DRIVER( sentetst )
	DRIVER( cshift )	/* (c) 1984 */
	DRIVER( gghost )	/* (c) 1984 */
	DRIVER( hattrick )	/* (c) 1984 */
	DRIVER( otwalls )	/* (c) 1984 */
	DRIVER( snakepit )	/* (c) 1984 */
	DRIVER( snakjack )	/* (c) 1984 */
	DRIVER( stocker )	/* (c) 1984 */
	DRIVER( triviag1 )	/* (c) 1984 */
	DRIVER( triviag2 )	/* (c) 1984 */
	DRIVER( triviasp )	/* (c) 1984 */
	DRIVER( triviayp )	/* (c) 1984 */
	DRIVER( triviabb )	/* (c) 1984 */
	DRIVER( gimeabrk )	/* (c) 1985 */
	DRIVER( minigolf )	/* (c) 1985 */
	DRIVER( minigol2 )	/* (c) 1985 */
	DRIVER( toggle )	/* (c) 1985 */
	DRIVER( nametune )	/* (c) 1986 */
	DRIVER( nstocker )	/* (c) 1986 */
	DRIVER( sfootbal )	/* (c) 1986 */
	DRIVER( spiker )	/* (c) 1986 */
	DRIVER( stompin )	/* (c) 1986 */
	DRIVER( rescraid )	/* (c) 1987 */
	DRIVER( rescrdsa )	/* (c) 1987 */
	DRIVER( grudge )
	DRIVER( shrike )	/* (c) 1987 */
#endif

#if defined(drv_bankp) || defined(core_none)
	/* bankp.c */
	DRIVER( bankp )		/* (c) 1984 Sega */	/* made by Sanritsu */
	DRIVER( combh )		/* (c) 1987 Sega */	/* made by Sanritsu */
#endif

#if defined(drv_baraduke) || defined(core_none)
	/* baraduke.c */
	DRIVER( baraduke )	/* (c) 1985 */
	DRIVER( baraduka )	/* (c) 1985 */
	DRIVER( metrocrs )	/* (c) 1985 */
	DRIVER( metrocra )	/* (c) 1985 */
#endif

#if defined(drv_batman) || defined(core_none)
	/* batman.c */
	DRIVER( batman )	/* 136085			(c) 1991 */
	DRIVER( marblmd2 )
#endif

#if defined(drv_battlane) || defined(core_none)
	/* battlane.c */
	DRIVER( battlane )	/* -0215, -0216 (Data East part number) (c) 1986 + Taito license */
	DRIVER( battlan2 )	/* -0215, -0216 (Data East part number) (c) 1986 + Taito license */
	DRIVER( battlan3 )	/* -0215, -0216 (Data East part number) (c) 1986 + Taito license */
#endif

#if defined(drv_battlera) || defined(core_none)
	/* battlera.c */
	DRIVER( battlera )	/* (c) 1988 Data East Corporation (World) */
	DRIVER( bldwolf )	/* (c) 1988 Data East USA (US) */
#endif

#if defined(drv_battlex) || defined(core_none)
	/* battlex.c */
	DRIVER( battlex )	/* (c) 1982 Omori Electric Co., Ltd. */
	DRIVER( dodgeman )	/* (c) 1983 Omori Electric Co., Ltd. */
#endif

#if defined(drv_battlnts) || defined(core_none)
	/* battlnts.c */
	DRIVER( battlnts )	/* GX777 (c) 1987 */
	DRIVER( battlntj )	/* GX777 (c) 1987 (Japan) */
	DRIVER( thehustl )	/* GX765 (c) 1987 (Japan) */
	DRIVER( thehustj )	/* GX765 (c) 1987 (Japan) */
	DRIVER( rackemup )	/* GX765 (c) 1987 */
#endif

#if defined(drv_bbusters) || defined(core_none)
	/* bbusters.c */
	DRIVER( bbusters )	/* A9003 'BB' (c) 1989 */
	DRIVER( mechatt )	/* A8002 'MA' (c) 1989 */
#endif

#if defined(drv_beaminv) || defined(core_none)
	/* beaminv.c */
	DRIVER( beaminv )	/* Tekunon Kougyou */
#endif

#if defined(drv_beathead) || defined(core_none)
	/* beathead.c */
	DRIVER( beathead )	/* (proto)			(c) 1993 */
#endif

#if defined(drv_beezer) || defined(core_none)
	/* beezer.c */
	DRIVER( beezer )	/* (c) 1982 */
	DRIVER( beezer1 )	/* (c) 1982 */
#endif

#if defined(drv_berzerk) || defined(core_none)
	/* berzerk.c */
	DRIVER( berzerk )	/* (c) 1980 */
	DRIVER( berzerk1 )	/* (c) 1980 */
	DRIVER( frenzy )	/* (c) 1982 */
#endif

#if defined(drv_bigevglf) || defined(core_none)
	/* bigevglf.c */
	DRIVER( bigevglf )	/* A67 (c) 1986 Taito America Corporation (US) */
#endif

#if defined(drv_bigstrkb) || defined(core_none)
	/* bigstrkb.c */
	DRIVER( bigstrkb )	/* bootleg on different hardware */
#endif

#if defined(drv_bionicc) || defined(core_none)
	/* bionicc.c */
	DRIVER( bionicc )	/*  3/1987 (c) 1987 (US) */
	DRIVER( bionicc2 )	/*  3/1987 (c) 1987 (US) */
	DRIVER( topsecrt )	/*  3/1987 (c) 1987 (Japan) */
#endif

#if defined(drv_bishi) || defined(core_none)
	/* bishi.c */
	DRIVER( bishi )     /* GX575 (c) 1996 (Japan) */
	DRIVER( sbishi )    /* GX575 (c) 1998 (Japan) */
	DRIVER( sbishik )   /* GX575 (c) 1998 (Japan) */
#endif

#if defined(drv_bking2) || defined(core_none)
	/* bking2.c */
	DRIVER( bking )		/* DM  (c) 1982 Taito Corporation */
	DRIVER( bking2 )	/* AD6 (c) 1983 Taito Corporation */
	DRIVER( bking3 )	/* A24 (c) 1984 Taito Corporation */
#endif

#if defined(drv_bladestl) || defined(core_none)
	/* bladestl.c */
	DRIVER( bladestl )	/* GX797 (c) 1987 */
	DRIVER( bladstle )	/* GX797 (c) 1987 */
#endif

#if defined(drv_blktiger) || defined(core_none)
	/* blktiger.c */
	DRIVER( blktiger )	/*  8/1987 (c) 1987 (US) */
	DRIVER( bktigerb )	/* bootleg */
	DRIVER( blkdrgon )	/*  8/1987 (c) 1987 (Japan) */
	DRIVER( blkdrgonb )	/* bootleg, hacked to say Black Tiger */
#endif

#if defined(drv_blmbycar) || defined(core_none)
	/* blmbycar.c */
	DRIVER( blmbycar )	/* (c) 1994 ABM & Gecas - uses same gfx chip as powerins? */
	DRIVER( blmbycau )	/* (c) 1994 ABM & Gecas - uses same gfx chip as powerins? */
#endif

#if defined(drv_blockade) || defined(core_none)
	/* blockade.c */
	DRIVER( blockade )	/* 1-4 [1977 Gremlin] */
	DRIVER( comotion )	/* 5-7 [1977 Gremlin] */
	DRIVER( hustle )	/* 16-21 [1977 Gremlin] */
	DRIVER( blasto )	/* [1978 Gremlin] */
	DRIVER( mineswpr )	/* [1977 Amutech] */
#endif

#if defined(drv_blockhl) || defined(core_none)
	/* blockhl.c */
	DRIVER( blockhl )	/* GX973 (c) 1989 */
	DRIVER( quarth )	/* GX973 (c) 1989 (Japan) */
#endif

#if defined(drv_blockout) || defined(core_none)
	/* blockout.c */
	DRIVER( blockout )	/* TA-0029 (c) 1989 + California Dreams */
	DRIVER( blckout2 )	/* TA-0029 (c) 1989 + California Dreams */
	DRIVER( blckoutj )	/* TA-0029 (c) 1989 + California Dreams (Japan) */
#endif

#if defined(drv_bloodbro) || defined(core_none)
	/* bloodbro.c */
	DRIVER( bloodbro )	/* (c) 1990 Tad */
	DRIVER( weststry )	/* bootleg */
	DRIVER( skysmash )	/* (c) 1990 Nihon System Inc. */
#endif

#if defined(drv_blstroid) || defined(core_none)
	/* blstroid.c */
	DRIVER( blstroid )	/* 136057			(c) 1987 */
	DRIVER( blstroi3 )	/* 136057			(c) 1987 */
	DRIVER( blstroi2 )	/* 136057			(c) 1987 */
	DRIVER( blstroig )	/* 136057			(c) 1987 */
	DRIVER( blsthead )	/* (proto)			(c) 1987 */
#endif

#if defined(drv_blueprnt) || defined(core_none)
	/* blueprnt.c */
	DRIVER( blueprnt )	/* (c) 1982 Bally Midway (Zilec in ROM 3U, and the programmer names) */
	DRIVER( blueprnj )	/* (c) 1982 Jaleco (Zilec in ROM 3U, and the programmer names) */
	DRIVER( saturn )	/* (c) 1983 Jaleco (Zilec in ROM R6, and the programmer names) */
#endif

#if defined(drv_bogeyman) || defined(core_none)
	/* bogeyman.c */
	DRIVER( bogeyman )	/* -0204-0 (Data East part number) (c) [1985?] */
#endif

#if defined(drv_bombjack) || defined(core_none)
	/* bombjack.c */
	DRIVER( bombjack )	/* (c) 1984 Tehkan */
	DRIVER( bombjac2 )	/* (c) 1984 Tehkan */
#endif

#if defined(drv_boogwing) || defined(core_none)
	/* boogwing.c */
	DRIVER( boogwing )	/* MBD (c) 1993 Data East Corporation (Euro) */
	DRIVER( boogwinga )	/* MBD (c) 1993 Data East Corporation (Asia) */
	DRIVER( ragtime )	/* MBD (c) 1993 Data East Corporation (Japan)*/
	DRIVER( ragtimea )	/* MBD (c) 1993 Data East Corporation (Japan)*/
#endif

#if defined(drv_bottom9) || defined(core_none)
	/* bottom9.c */
	DRIVER( bottom9 )	/* GX891 (c) 1989 */
	DRIVER( bottom9n )	/* GX891 (c) 1989 */
	DRIVER( mstadium )	/* GX891 (c) 1989 (Japan) */
#endif

#if defined(drv_boxer) || defined(core_none)
	/* boxer.c */
	DRIVER( boxer )		/* ?????? prototype 1978    [6502] */
#endif

#if defined(drv_brkthru) || defined(core_none)
	/* brkthru.c */
	DRIVER( brkthru )	/* (c) 1986 Data East USA (US) */
	DRIVER( brkthruj )	/* (c) 1986 Data East Corporation (Japan) */
	DRIVER( darwin )	/* (c) 1986 Data East Corporation (Japan) */
#endif

#if defined(drv_climber) || defined(core_none)
	/* cclimber.c */
	DRIVER( cclimber )	/* (c) 1980 Nichibutsu */
	DRIVER( cclimbrj )	/* (c) 1980 Nichibutsu */
	DRIVER( ccboot )	/* bootleg */
	DRIVER( ccboot2 )	/* bootleg */
	DRIVER( ckong )		/* (c) 1981 Falcon */
	DRIVER( ckonga )	/* (c) 1981 Falcon */
	DRIVER( ckongjeu )	/* bootleg */
	DRIVER( ckongo )	/* bootleg */
	DRIVER( ckongalc )	/* bootleg */
	DRIVER( monkeyd )	/* bootleg */
	DRIVER( rpatrolb )	/* bootleg */
	DRIVER( silvland )	/* Falcon */
	DRIVER( swimmer )	/* (c) 1982 Tehkan */
	DRIVER( swimmera )	/* (c) 1982 Tehkan */
	DRIVER( swimmerb )	/* (c) 1982 Tehkan */
	DRIVER( guzzler )	/* (c) 1983 Tehkan */
	DRIVER( cannonb )	/* (c) 1985 Soft */
#endif

#if defined(drv_namcos2) || defined(core_none) /* reduction needed */
	/* namcos2.c */
	DRIVER( finallap )	/* 87.12 Final Lap */
	DRIVER( finalapd )	/* 87.12 Final Lap */
	DRIVER( finalapc )	/* 87.12 Final Lap */
	DRIVER( finlapjc )	/* 87.12 Final Lap */
	DRIVER( finlapjb )	/* 87.12 Final Lap */
	DRIVER( assault )	/* (c) 1988 */
	DRIVER( assaultj )	/* (c) 1988 (Japan) */
	DRIVER( assaultp )	/* (c) 1988 (Japan) */
	DRIVER( metlhawk )	/* (c) 1988 */
	DRIVER( ordyne )	/* (c) 1988 */
	DRIVER( mirninja )	/* (c) 1988 (Japan) */
	DRIVER( phelios )	/* (c) 1988 (Japan) */
	DRIVER( dirtfoxj )	/* (c) 1989 (Japan) */
	DRIVER( fourtrax )	/* 89.11 */
	DRIVER( valkyrie )	/* (c) 1989 (Japan) */
	DRIVER( finehour )	/* (c) 1989 (Japan) */
	DRIVER( burnforc )	/* (c) 1989 (Japan) */
	DRIVER( marvland )	/* (c) 1989 (US) */
	DRIVER( marvlanj )	/* (c) 1989 (Japan) */
	DRIVER( kyukaidk )	/* (c) 1990 (Japan) */
	DRIVER( kyukaido )	/* (c) 1990 (Japan) */
	DRIVER( dsaber )	/* (c) 1990 */
	DRIVER( dsaberj )	/* (c) 1990 (Japan) */
	DRIVER( finalap2 )	/* 90.8  Final Lap 2 */
	DRIVER( finalp2j )	/* 90.8  Final Lap 2 (Japan) */
	DRIVER( gollygho )	/* 91.7  Golly Ghost */
	DRIVER( rthun2 )	/* (c) 1990 */
	DRIVER( rthun2j )	/* (c) 1990 (Japan) */
	DRIVER( sgunner )	/* (c) 1990 */
	/* 91.9  Super World Stadium */
	DRIVER( sgunner2 )	/* (c) 1991 (US) */
	DRIVER( sgunnr2j )	/* (c) 1991 (Japan) */
	DRIVER( cosmogng )	/* (c) 1991 (US) */
	DRIVER( cosmognj )	/* (c) 1991 (Japan) */
	DRIVER( finalap3 )	/* 92.9  Final Lap 3 */
	DRIVER( finalp3a )	/* 92.9  Final Lap 3 */
	DRIVER( luckywld )	/* (c) 1992 */
	DRIVER( suzuka8h )
	DRIVER( suzuk8hj )
	/* 92.8  Bubble Trouble */
	DRIVER( sws92 )		/* (c) 1992 (Japan) */
	DRIVER( sws92g )	/* (c) 1992 (Japan) */
	DRIVER( suzuk8h2 )
	DRIVER( sws93 )		/* (c) 1993 (Japan) */
#endif

#if defined(drv_system32) || defined(core_none)
	/* system32.c */
	DRIVER( holo )		/* (c) 1992 (US) */
	DRIVER( arescue )	/* (c) 1992 */
	DRIVER( radm )
	DRIVER( radr )		/* (c) 1991 */
	DRIVER( spidey )	/* (c) 1991 */
	DRIVER( spideyj )	/* (c) 1991 (Japan) */
	DRIVER( f1en )
	DRIVER( arabfgt )	/* (c) 1991 */
	DRIVER( ga2 )		/* (c) 1992 */
	DRIVER( ga2j )		/* (c) 1992 */
	DRIVER( brival )	/* (c) 1992 (Japan) */
	DRIVER( sonic )		/* (c) 1992 (Japan) */
	DRIVER( sonicp )	/* (c) 1992 (Japan) */
	DRIVER( alien3 )	/* (c) 1993 */
	DRIVER( jpark )		/* (c) 1994 */
	DRIVER( svf )		/* (c) 1994 */
	DRIVER( svs )		/* (c) 1994 */
	DRIVER( jleague )	/* (c) 1994 (Japan) */
	DRIVER( f1lap )     /* (c) 1993 (World) */
	DRIVER( f1lapj )    /* (c) 1993 (Japan) */
	DRIVER( darkedge )
	DRIVER( dbzvrvs )
	DRIVER( slipstrm )	/* Capcom */
#endif

#if defined(drv_yamato) || defined(core_none)
	/* yamato.c */
	DRIVER( yamato )	/* (c) 1983 Sega */
	DRIVER( yamato2 )	/* (c) 1983 Sega */
#endif

#endif	/* DRIVER_RECURSIVE */
