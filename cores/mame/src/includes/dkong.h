
#define MASTER_CLOCK            61440000
#define CLOCK_1H                (MASTER_CLOCK / 5 / 4)
#define CLOCK_16H               (CLOCK_1H / 16)
#define CLOCK_1VF               ((CLOCK_16H) / 12 / 2)
#define CLOCK_2VF               ((CLOCK_1VF) / 2)

#define PIXEL_CLOCK             (MASTER_CLOCK/10)
#define HTOTAL                  (384)
#define HBSTART                 (256)
#define HBEND                   (0)
#define VTOTAL                  (264)
#define VBSTART                 (240)
#define VBEND                   (16)

/*need the old  valuse for the flip to work
 * I plan on doing some updates this will make its easier
 * */
 
#define OLD_HTOTAL                  (32*8)
#define OLD_HBSTART                 (32*8)
#define OLD_HBEND                   (0)
#define OLD_VTOTAL                  (32*8)
#define OLD_VBSTART                 (30*8)
#define OLD_VBEND                   (2*8)

#define DK2B_PALETTE_LENGTH     (256+256+8+1) /*  (256) */

enum
{
	HARDWARE_TKG04 = 0,
	HARDWARE_TRS01,
	HARDWARE_TRS02,
	HARDWARE_TKG02
};

enum
{
	DKONG_RADARSCP_CONVERSION = 0,
	DKONG_BOARD = 1
};


extern UINT8           p02_b5_enable;



extern void set_var(void);
static void scanline_callback(int scanline);


extern WRITE_HANDLER( radarscp_grid_enable_w );
extern WRITE_HANDLER( radarscp_grid_color_w );
extern WRITE_HANDLER( dkong_flipscreen_w );
extern WRITE_HANDLER( dkongjr_gfxbank_w );
extern WRITE_HANDLER( dkong3_gfxbank_w );
extern WRITE_HANDLER( dkong_palettebank_w );

extern WRITE_HANDLER( dkong_videoram_w );

extern PALETTE_INIT( dkong );
extern PALETTE_INIT( dkong3 );
extern PALETTE_INIT( radarscp );
extern VIDEO_START( dkong );
extern VIDEO_START( radarscp );
extern VIDEO_UPDATE( radarscp );
extern VIDEO_UPDATE( dkong );
extern VIDEO_UPDATE( pestplce );
extern VIDEO_UPDATE( spclforc );

extern WRITE_HANDLER( dkong_sh_w );
extern WRITE_HANDLER( dkongjr_sh_death_w );
extern WRITE_HANDLER( dkongjr_sh_drop_w );
extern WRITE_HANDLER( dkongjr_sh_roar_w );
extern WRITE_HANDLER( dkongjr_sh_jump_w );
extern WRITE_HANDLER( dkongjr_sh_walk_w );
extern WRITE_HANDLER( dkongjr_sh_climb_w );
extern WRITE_HANDLER( dkongjr_sh_land_w );
extern WRITE_HANDLER( dkongjr_sh_snapjaw_w );

extern WRITE_HANDLER( dkong_sh1_w );
