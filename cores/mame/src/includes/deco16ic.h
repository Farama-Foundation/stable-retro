extern data16_t *deco16_pf1_data,*deco16_pf2_data;
extern data16_t *deco16_pf3_data,*deco16_pf4_data;
extern data16_t *deco16_pf1_rowscroll,*deco16_pf2_rowscroll;
extern data16_t *deco16_pf3_rowscroll,*deco16_pf4_rowscroll;

extern data16_t *deco16_pf12_control,*deco16_pf34_control;
extern data16_t deco16_priority;

extern data16_t *deco16_raster_display_list;
extern int deco16_raster_display_position;

int deco16_1_video_init(void);
int deco16_2_video_init(int split);
int deco_allocate_sprite_bitmap(void);

void deco16_pf12_update(const data16_t *rowscroll_1_ptr, const data16_t *rowscroll_2_ptr);
void deco16_pf34_update(const data16_t *rowscroll_1_ptr, const data16_t *rowscroll_2_ptr);

void deco16_pf12_set_gfxbank(int small, int big);
void deco16_pf34_set_gfxbank(int small, int big);

void deco16_set_tilemap_bank_callback(int tilemap, int (*callback)(const int bank));
void deco16_set_tilemap_colour_base(int tilemap, int base);
void deco16_set_tilemap_colour_mask(int tilemap, int mask);
void deco16_set_tilemap_transparency_mask(int tilemap, int mask);

void deco16_tilemap_1_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int flags, UINT32 priority);
void deco16_tilemap_2_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int flags, UINT32 priority);
void deco16_tilemap_3_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int flags, UINT32 priority);
void deco16_tilemap_4_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int flags, UINT32 priority);

void deco16_tilemap_34_combine_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int flags, UINT32 priority);

void deco16_clear_sprite_priority_bitmap(void);
void deco16_pdrawgfx(struct mame_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,UINT32 pri_mask,UINT32 sprite_mask,UINT8 write_pri);

struct tilemap *deco16_get_tilemap(int pf, int size);

WRITE16_HANDLER( deco16_pf1_data_w );
WRITE16_HANDLER( deco16_pf2_data_w );
WRITE16_HANDLER( deco16_pf3_data_w );
WRITE16_HANDLER( deco16_pf4_data_w );

WRITE16_HANDLER( deco16_nonbuffered_palette_w );
WRITE16_HANDLER( deco16_buffered_palette_w );
WRITE16_HANDLER( deco16_palette_dma_w );

WRITE16_HANDLER( deco16_priority_w );

READ16_HANDLER( deco16_71_r );

void deco16_print_debug_info(void);
