#include "driver.h" /* for mame_bitmap */

#define NAMCOS22_SCREEN_WIDTH  640
#define NAMCOS22_SCREEN_HEIGHT 480

extern INT32 *namco_zbuffer;

struct VerTex
{
	double x,y,z;
	double u,v,i;
};

typedef struct
{
	struct rectangle clip;
	double zoom, cx, cy;
	double x,y,z; /* unit vector for light direction */
	double ambient; /* 0.0..1.0 */
	double power;	/* 0.0..1.0 */
} namcos22_camera;

struct RotParam
{
	double thx_sin, thx_cos;
	double thy_sin, thy_cos;
	double thz_sin, thz_cos;
	int rolt; /* rotation type: 0,1,2,3,4,5 */
};

void namcos3d_Rotate( double M[4][4], const struct RotParam *pParam );

int namcos3d_Init( int width, int height, void *pTilemapROM, void *pTextureROM );

void namcos3d_Start( struct mame_bitmap *pBitmap );

void namcos22_BlitTri(
	struct mame_bitmap *pBitmap,
	const struct VerTex v[3],
	unsigned color,
	INT32 bias,
	INT32 flags,
	const namcos22_camera * );

void /*namcos21_*/BlitTriFlat(
	struct mame_bitmap *pBitmap,
	const struct VerTex v[3],
	unsigned color );
