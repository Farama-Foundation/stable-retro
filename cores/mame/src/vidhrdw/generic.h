#include "driver.h"

#ifdef __cplusplus
extern "C" {
#endif

extern data8_t *videoram;
extern data16_t *videoram16;
extern data32_t *videoram32;
extern size_t videoram_size;
extern data8_t *colorram;
extern data16_t *colorram16;
extern data32_t *colorram32;
extern data8_t *spriteram;
extern data16_t *spriteram16;
extern data32_t *spriteram32;
extern data8_t *spriteram_2;
extern data16_t *spriteram16_2;
extern data32_t *spriteram32_2;
extern data8_t *spriteram_3;
extern data16_t *spriteram16_3;
extern data32_t *spriteram32_3;
extern data8_t *buffered_spriteram;
extern data16_t *buffered_spriteram16;
extern data32_t *buffered_spriteram32;
extern data8_t *buffered_spriteram_2;
extern data16_t *buffered_spriteram16_2;
extern data32_t *buffered_spriteram32_2;
extern size_t spriteram_size;
extern size_t spriteram_2_size;
extern size_t spriteram_3_size;
extern data8_t *dirtybuffer;
extern data16_t *dirtybuffer16;
extern data32_t *dirtybuffer32;
extern struct mame_bitmap *tmpbitmap;


VIDEO_START( generic );
VIDEO_START( generic_bitmapped );
void video_stop_generic(void);
void video_stop_generic_bitmapped(void);
VIDEO_UPDATE( generic_bitmapped );

READ_HANDLER( videoram_r );
READ_HANDLER( colorram_r );
WRITE_HANDLER( videoram_w );
WRITE_HANDLER( colorram_w );
READ_HANDLER( spriteram_r );
WRITE_HANDLER( spriteram_w );
READ16_HANDLER( spriteram16_r );
WRITE16_HANDLER( spriteram16_w );
READ_HANDLER( spriteram_2_r );
WRITE_HANDLER( spriteram_2_w );
WRITE_HANDLER( buffer_spriteram_w );
WRITE16_HANDLER( buffer_spriteram16_w );
WRITE32_HANDLER( buffer_spriteram32_w );
WRITE_HANDLER( buffer_spriteram_2_w );
WRITE16_HANDLER( buffer_spriteram16_2_w );
WRITE32_HANDLER( buffer_spriteram32_2_w );
void buffer_spriteram(unsigned char *ptr,int length);
void buffer_spriteram_2(unsigned char *ptr,int length);

/* screen flipping */
extern int flip_screen_x, flip_screen_y;
void flip_screen_set(int on);
void flip_screen_x_set(int on);
void flip_screen_y_set(int on);
#define flip_screen flip_screen_x

/* sets a variable and schedules a full screen refresh if it changed */
void set_vh_global_attribute(int *addr, int data);
int get_vh_global_attribute_changed(void);


#ifdef __cplusplus
}
#endif
