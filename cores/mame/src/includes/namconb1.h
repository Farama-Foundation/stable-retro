/* namconb1.h */

#define NAMCONB1_COLS		36
#define NAMCONB1_ROWS		28
#define NAMCONB1_FG1BASE	(0x8010/2)
#define NAMCONB1_FG2BASE	(0x8810/2)

#define NAMCONB1_TILEMASKREGION		REGION_GFX1
#define NAMCONB1_TILEGFXREGION		REGION_GFX2
#define NAMCONB1_SPRITEGFXREGION	REGION_GFX3
#define NAMCONB1_ROTMASKREGION		REGION_GFX4
#define NAMCONB1_ROTGFXREGION		REGION_GFX5

#define NAMCONB1_TILEGFX		0
#define NAMCONB1_SPRITEGFX		1
#define NAMCONB1_ROTGFX			2

extern data32_t *namconb1_workram32;
extern data32_t *namconb1_spritebank32;
extern data32_t *namconb1_scrollram32;
extern data32_t *namconb1_spritepos32;

WRITE32_HANDLER( namconb1_videoram_w );

VIDEO_UPDATE( namconb1 );
VIDEO_START( namconb1 );

VIDEO_UPDATE( namconb2 );
VIDEO_START( namconb2 );
