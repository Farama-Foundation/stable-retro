//NOTE: You can find the set of original Unai poly routines (disabled now)
// at the bottom end of this file.

//senquack - Original Unai GPU poly routines have been replaced with new
// ones based on DrHell routines. The original routines suffered from
// shifted rows, causing many quads to have their first triangle drawn
// correctly, but the second triangle would randomly have pixels shifted
// either left or right or entire rows not drawn at all. Furthermore,
// some times entire triangles seemed to be either missing or only
// partially drawn (most clearly seen in sky/road textures in NFS3,
// clock tower in beginning of Castlevania SOTN). Pixel gaps were
// prevalent.
//
// Since DrHell GPU didn't seem to exhibit these artifacts at all, I adapted
// its routines to GPU Unai (Unai was probably already originally based on it).
// DrHell uses 22.10 fixed point instead of Unai's 16.16, so gpu_fixedpoint.h
// required modification as well as gpu_inner.h (where gpuPolySpanFn driver
// functions are).
//
// Originally, I tried to patch up original Unai routines and got as far
// as fixing the shifted rows, but still had other problem of triangles rendered
// wrong (black triangular gaps in NFS3 sky, clock tower in Castlevania SOTN).
// I eventually gave up. Even after rewriting/adapting the routines,
// however, I still had some random pixel droupouts, specifically in
// NFS3 sky texture. I discovered that gpu_inner.h gpuPolySpanFn function
// was taking optimizations to an extreme and packing u/v texture coords
// into one 32-bit word, reducing their accuracy. Only once they were
// handled in full-accuracy individual words was that problem fixed.
//
// NOTE: I also added support for doing divisions using the FPU, either
//  with normal division or multiplication-by-reciprocal.
//  To use float division, GPU_UNAI_USE_FLOATMATH should be defined.
//  To use float mult-by-reciprocal, GPU_UNAI_USE_FLOAT_DIV_MULTINV
//   can be specified (GPU_UNAI_USE_FLOATMATH must also be specified)
//  To use inaccurate fixed-point mult-by-reciprocal, define
//   GPU_UNAI_USE_INT_DIV_MULTINV. This is the default on older
//   ARM devices like Wiz/Caanoo that have neither integer division
//   in hardware or an FPU. It results in some pixel dropouts,
//   texture glitches, but less than the original GPU UNAI code.
//
//  If nothing is specified, integer division will be used.
//
// NOTE 2: Even with MIPS32R2 having FPU recip.s instruction, and it is
//  used when this platform is detected, I found it not to give any
//  noticeable speedup over normal float division (in fact seemed a tiny
//  tiny bit slower). I also found float division to not provide any
//  noticeable speedups versus integer division on MISP32R2 platform.
//  Granted, the differences were all around .5 FPS or less.
//
// TODO:
// * See if anything can be done about remaining pixel gaps in Gran
//   Turismo car models, track.
// * Find better way of passing parameters to gpuPolySpanFn functions than
//   through original Unai method of using global variables u4,v4,du4 etc.
// * Come up with some newer way of drawing rows of pixels than by calling
//   gpuPolySpanFn through function pointer. For every row, at least on
//   MIPS platforms, many registers are having to be pushed/popped from stack
//   on each call, which is strange since MIPS has so many registers.
// * MIPS MXU/ASM optimized gpuPolySpanFn ?

//////////////////////////////////////////////////////////////////////////
//senquack - Disabled original Unai poly routines left here for reference:
// ( from gpu_raster_polygon.h )
//////////////////////////////////////////////////////////////////////////
#define GPU_TESTRANGE3() \
{ \
	if(x0<0) { if((x1-x0)>CHKMAX_X) return; if((x2-x0)>CHKMAX_X) return; } \
	if(x1<0) { if((x0-x1)>CHKMAX_X) return; if((x2-x1)>CHKMAX_X) return; } \
	if(x2<0) { if((x0-x2)>CHKMAX_X) return; if((x1-x2)>CHKMAX_X) return; } \
	if(y0<0) { if((y1-y0)>CHKMAX_Y) return; if((y2-y0)>CHKMAX_Y) return; } \
	if(y1<0) { if((y0-y1)>CHKMAX_Y) return; if((y2-y1)>CHKMAX_Y) return; } \
	if(y2<0) { if((y0-y2)>CHKMAX_Y) return; if((y1-y2)>CHKMAX_Y) return; } \
}

/*----------------------------------------------------------------------
F3
----------------------------------------------------------------------*/

void gpuDrawF3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	s32 temp;
	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3=0, x4, dx4=0, dx;
	s32 y0, y1, y2;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2]);
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3]);
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[4]);
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[5]);
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[6]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[7]);

	GPU_TESTRANGE3();
	
	x0 += DrawingOffset[0];   x1 += DrawingOffset[0];   x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];   y1 += DrawingOffset[1];   y2 += DrawingOffset[1];

	xmin = DrawingArea[0];  xmax = DrawingArea[2];
	ymin = DrawingArea[1];  ymax = DrawingArea[3];

	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}
	
	PixelData = GPU_RGB16(PacketBuffer.U4[0]);

	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
		}
	}
	if (y1 >= y2)
	{
		if( y1!=y2 || x1>x2 )
		{
			GPU_SWAP(x1, x2, temp);
			GPU_SWAP(y1, y2, temp);
		}
	}
	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
		}
	}

	ya = y2 - y0;
	yb = y2 - y1;
	dx =(x2 - x1) * ya - (x2 - x0) * yb;

	for (s32 loop0 = 2; loop0; --loop0)
	{
		if (loop0 == 2)
		{
			ya = y0;
			yb = y1;
			x3 = i2x(x0);
			x4 = y0!=y1 ? x3 : i2x(x1);
			if (dx < 0)
			{
				dx3 = xLoDivx((x2 - x0), (y2 - y0));
				dx4 = xLoDivx((x1 - x0), (y1 - y0));
			}
			else
			{
				dx3 = xLoDivx((x1 - x0), (y1 - y0));
				dx4 = xLoDivx((x2 - x0), (y2 - y0));
			}
		}
		else
		{
			ya = y1;
			yb = y2;
			if (dx < 0)
			{
				x4  = i2x(x1);
				x3  = i2x(x0) + (dx3 * (y1 - y0));
				dx4 = xLoDivx((x2 - x1), (y2 - y1));
			}
			else
			{
				x3  = i2x(x1);
				x4  = i2x(x0) + (dx4 * (y1 - y0));
				dx3 = xLoDivx((x2 - x1), (y2 - y1));
			}
		}

		temp = ymin - ya;
		if (temp > 0)
		{
			ya  = ymin;
			x3 += dx3*temp;
			x4 += dx4*temp;
		}
		if (yb > ymax) yb = ymax;
		if (ya>=yb) continue;

		x3+= fixed_HALF;
		x4+= fixed_HALF;

		u16* PixelBase  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];
		
		for(;ya<yb;++ya, PixelBase += FRAME_WIDTH, x3+=dx3, x4+=dx4)
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;
			xa = x2i(x3);
			xb = x2i(x4);
			if( (xa>xmax) || (xb<xmin) ) continue;
			if(xa < xmin) xa = xmin;
			if(xb > xmax) xb = xmax;
			xb-=xa;
			if(xb>0) gpuPolySpanDriver(PixelBase + xa,xb);
		}
	}
}

/*----------------------------------------------------------------------
FT3
----------------------------------------------------------------------*/

void gpuDrawFT3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	s32 temp;
	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3=0, x4, dx4=0, dx;
	s32 y0, y1, y2;
	s32 u0, u1, u2, u3, du3=0;
	s32 v0, v1, v2, v3, dv3=0;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2] );
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3] );
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[6] );
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[7] );
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[10]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[11]);

	GPU_TESTRANGE3();
	
	x0 += DrawingOffset[0];   x1 += DrawingOffset[0];   x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];   y1 += DrawingOffset[1];   y2 += DrawingOffset[1];

	xmin = DrawingArea[0];  xmax = DrawingArea[2];
	ymin = DrawingArea[1];  ymax = DrawingArea[3];

	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}
	
	u0 = PacketBuffer.U1[8];  v0 = PacketBuffer.U1[9];
	u1 = PacketBuffer.U1[16]; v1 = PacketBuffer.U1[17];
	u2 = PacketBuffer.U1[24]; v2 = PacketBuffer.U1[25];

	r4 = s32(PacketBuffer.U1[0]);
	g4 = s32(PacketBuffer.U1[1]);
	b4 = s32(PacketBuffer.U1[2]);
	dr4 = dg4 = db4 = 0;

	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
			GPU_SWAP(u0, u1, temp);
			GPU_SWAP(v0, v1, temp);
		}
	}
	if (y1 >= y2)
	{
		if( y1!=y2 || x1>x2 )
		{
			GPU_SWAP(x1, x2, temp);
			GPU_SWAP(y1, y2, temp);
			GPU_SWAP(u1, u2, temp);
			GPU_SWAP(v1, v2, temp);
		}
	}
	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
			GPU_SWAP(u0, u1, temp);
			GPU_SWAP(v0, v1, temp);
		}
	}

	ya  = y2 - y0;
	yb  = y2 - y1;
	dx  = (x2 - x1) * ya - (x2 - x0) * yb;
	du4 = (u2 - u1) * ya - (u2 - u0) * yb;
	dv4 = (v2 - v1) * ya - (v2 - v0) * yb;

	s32 iF,iS;
	xInv( dx, iF, iS);
	du4 = xInvMulx( du4, iF, iS);
	dv4 = xInvMulx( dv4, iF, iS);
	tInc = ((u32)(du4<<7)&0x7fff0000) | ((u32)(dv4>>9)&0x00007fff);
	tMsk = (TextureWindow[2]<<23) | (TextureWindow[3]<<7) | 0x00ff00ff;

	for (s32 loop0 = 2; loop0; --loop0)
	{
		if (loop0 == 2)
		{
			ya = y0;
			yb = y1;
			u3 = i2x(u0);
			v3 = i2x(v0);
			x3 = i2x(x0);
			x4 = y0!=y1 ? x3 : i2x(x1);
			if (dx < 0)
			{
				xInv( (y2 - y0), iF, iS);
				dx3 = xInvMulx( (x2 - x0), iF, iS);
				du3 = xInvMulx( (u2 - u0), iF, iS);
				dv3 = xInvMulx( (v2 - v0), iF, iS);
				dx4 = xLoDivx ( (x1 - x0), (y1 - y0));
			}
			else
			{
				xInv( (y1 - y0), iF, iS);
				dx3 = xInvMulx( (x1 - x0), iF, iS);
				du3 = xInvMulx( (u1 - u0), iF, iS);
				dv3 = xInvMulx( (v1 - v0), iF, iS);
				dx4 = xLoDivx ( (x2 - x0), (y2 - y0));
			}
		}
		else
		{
			ya = y1;
			yb = y2;
			if (dx < 0)
			{
				temp = y1 - y0;
				u3 = i2x(u0) + (du3 * temp);
				v3 = i2x(v0) + (dv3 * temp);
				x3 = i2x(x0) + (dx3 * temp);
				x4 = i2x(x1);
				dx4 = xLoDivx((x2 - x1), (y2 - y1));
			}
			else
			{
				u3 = i2x(u1);
				v3 = i2x(v1);
				x3 = i2x(x1);
				x4 = i2x(x0) + (dx4 * (y1 - y0));
				xInv( (y2 - y1), iF, iS);
				dx3 = xInvMulx( (x2 - x1), iF, iS);
				du3 = xInvMulx( (u2 - u1), iF, iS);
				dv3 = xInvMulx( (v2 - v1), iF, iS);
			}
		}

		temp = ymin - ya;
		if (temp > 0)
		{
			ya  = ymin;
			x3 += dx3*temp;
			x4 += dx4*temp;
			u3 += du3*temp;
			v3 += dv3*temp;
		}
		if (yb > ymax) yb = ymax;
		if (ya>=yb) continue;

		x3+= fixed_HALF;
		x4+= fixed_HALF;
		u3+= fixed_HALF;
		v4+= fixed_HALF;

		u16* PixelBase  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];

		for(;ya<yb;++ya, PixelBase += FRAME_WIDTH, x3+=dx3, x4+=dx4, u3+=du3, v3+=dv3)
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;
			xa = x2i(x3);
			xb = x2i(x4);
			if( (xa>xmax) || (xb<xmin) ) continue;

			temp = xmin - xa;
			if(temp > 0)
			{
				xa  = xmin;
				u4 = u3 + du4*temp;
				v4 = v3 + dv4*temp;
			}
			else
			{
				u4 = u3;
				v4 = v3;
			}
			if(xb > xmax) xb = xmax;
			xb-=xa;
			if(xb>0) gpuPolySpanDriver(PixelBase + xa,xb);
		}
	}
}

/*----------------------------------------------------------------------
G3
----------------------------------------------------------------------*/

void gpuDrawG3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	s32 temp;
	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3=0, x4, dx4=0, dx;
	s32 y0, y1, y2;
	s32 r0, r1, r2, r3, dr3=0;
	s32 g0, g1, g2, g3, dg3=0;
	s32 b0, b1, b2, b3, db3=0;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2] );
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3] );
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[6] );
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[7] );
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[10]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[11]);

	GPU_TESTRANGE3();
	
	x0 += DrawingOffset[0];   x1 += DrawingOffset[0];   x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];   y1 += DrawingOffset[1];   y2 += DrawingOffset[1];

	xmin = DrawingArea[0];  xmax = DrawingArea[2];
	ymin = DrawingArea[1];  ymax = DrawingArea[3];

	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}
	
	r0 = PacketBuffer.U1[0];	g0 = PacketBuffer.U1[1];	b0 = PacketBuffer.U1[2];
	r1 = PacketBuffer.U1[8];	g1 = PacketBuffer.U1[9];	b1 = PacketBuffer.U1[10];
	r2 = PacketBuffer.U1[16];	g2 = PacketBuffer.U1[17];	b2 = PacketBuffer.U1[18];

	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);		GPU_SWAP(y0, y1, temp);
			GPU_SWAP(r0, r1, temp);		GPU_SWAP(g0, g1, temp);		GPU_SWAP(b0, b1, temp);
		}
	}
	if (y1 >= y2)
	{
		if( y1!=y2 || x1>x2 )
		{
			GPU_SWAP(x1, x2, temp);		GPU_SWAP(y1, y2, temp);
			GPU_SWAP(r1, r2, temp);		GPU_SWAP(g1, g2, temp);   GPU_SWAP(b1, b2, temp);
		}
	}
	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);		GPU_SWAP(y0, y1, temp);
			GPU_SWAP(r0, r1, temp);   GPU_SWAP(g0, g1, temp);		GPU_SWAP(b0, b1, temp);
		}
	}

	ya  = y2 - y0;
	yb  = y2 - y1;
	dx  = (x2 - x1) * ya - (x2 - x0) * yb;
	dr4 = (r2 - r1) * ya - (r2 - r0) * yb;
	dg4 = (g2 - g1) * ya - (g2 - g0) * yb;
	db4 = (b2 - b1) * ya - (b2 - b0) * yb;

	s32 iF,iS;
	xInv(            dx, iF, iS);
	dr4 = xInvMulx( dr4, iF, iS);
	dg4 = xInvMulx( dg4, iF, iS);
	db4 = xInvMulx( db4, iF, iS);
	u32 dr = (u32)(dr4<< 8)&(0xffffffff<<21);   if(dr4<0) dr+= 1<<21;
	u32 dg = (u32)(dg4>> 3)&(0xffffffff<<10);   if(dg4<0) dg+= 1<<10;
	u32 db = (u32)(db4>>14)&(0xffffffff    );   if(db4<0) db+= 1<< 0;
	lInc = db + dg + dr;

	for (s32 loop0 = 2; loop0; --loop0)
	{
		if (loop0 == 2)
		{
			ya = y0;
			yb = y1;
			r3 = i2x(r0);
			g3 = i2x(g0);
			b3 = i2x(b0);
			x3 = i2x(x0);
			x4 = y0!=y1 ? x3 : i2x(x1);
			if (dx < 0)
			{
				xInv(           (y2 - y0), iF, iS);
				dx3 = xInvMulx( (x2 - x0), iF, iS);
				dr3 = xInvMulx( (r2 - r0), iF, iS);
				dg3 = xInvMulx( (g2 - g0), iF, iS);
				db3 = xInvMulx( (b2 - b0), iF, iS);
				dx4 = xLoDivx ( (x1 - x0), (y1 - y0));
			}
			else
			{
				xInv(           (y1 - y0), iF, iS);
				dx3 = xInvMulx( (x1 - x0), iF, iS);
				dr3 = xInvMulx( (r1 - r0), iF, iS);
				dg3 = xInvMulx( (g1 - g0), iF, iS);
				db3 = xInvMulx( (b1 - b0), iF, iS);
				dx4 = xLoDivx ( (x2 - x0), (y2 - y0));
			}
		}
		else
		{
			ya = y1;
			yb = y2;
			if (dx < 0)
			{
				temp = y1 - y0;
				r3  = i2x(r0) + (dr3 * temp);
				g3  = i2x(g0) + (dg3 * temp);
				b3  = i2x(b0) + (db3 * temp);
				x3  = i2x(x0) + (dx3 * temp);
				x4  = i2x(x1);
				dx4 = xLoDivx((x2 - x1), (y2 - y1));
			}
			else
			{
				r3 = i2x(r1);
				g3 = i2x(g1);
				b3 = i2x(b1);
				x3 = i2x(x1);
				x4 = i2x(x0) + (dx4 * (y1 - y0));

				xInv(           (y2 - y1), iF, iS);
				dx3 = xInvMulx( (x2 - x1), iF, iS);
				dr3 = xInvMulx( (r2 - r1), iF, iS);
				dg3 = xInvMulx( (g2 - g1), iF, iS);
				db3 = xInvMulx( (b2 - b1), iF, iS);
			}
		}

		temp = ymin - ya;
		if (temp > 0)
		{
			ya  = ymin;
			x3 += dx3*temp;   x4 += dx4*temp;
			r3 += dr3*temp;   g3 += dg3*temp;   b3 += db3*temp;
		}
		if (yb > ymax) yb = ymax;
		if (ya>=yb) continue;

		x3+= fixed_HALF;  x4+= fixed_HALF;
		r3+= fixed_HALF;  g3+= fixed_HALF;  b3+= fixed_HALF;

		u16* PixelBase  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];
		
		for(;ya<yb;++ya, PixelBase += FRAME_WIDTH, x3+=dx3, x4+=dx4, r3+=dr3, g3+=dg3, b3+=db3)
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;
			xa = x2i(x3);
			xb = x2i(x4);
			if( (xa>xmax) || (xb<xmin) ) continue;

			temp = xmin - xa;
			if(temp > 0)
			{
				xa  = xmin;
				r4 = r3 + dr4*temp;   g4 = g3 + dg4*temp;   b4 = b3 + db4*temp;
			}
			else
			{
				r4 = r3;  g4 = g3;  b4 = b3;
			}
			if(xb > xmax) xb = xmax;
			xb-=xa;
			if(xb>0) gpuPolySpanDriver(PixelBase + xa,xb);
		}
	}
}

/*----------------------------------------------------------------------
GT3
----------------------------------------------------------------------*/

void gpuDrawGT3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	s32 temp;
	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3=0, x4, dx4=0, dx;
	s32 y0, y1, y2;
	s32 u0, u1, u2, u3, du3=0;
	s32 v0, v1, v2, v3, dv3=0;
	s32 r0, r1, r2, r3, dr3=0;
	s32 g0, g1, g2, g3, dg3=0;
	s32 b0, b1, b2, b3, db3=0;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2] );
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3] );
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[8] );
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[9] );
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[14]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[15]);

	GPU_TESTRANGE3();
	
	x0 += DrawingOffset[0];   x1 += DrawingOffset[0];   x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];   y1 += DrawingOffset[1];   y2 += DrawingOffset[1];

	xmin = DrawingArea[0];	xmax = DrawingArea[2];
	ymin = DrawingArea[1];	ymax = DrawingArea[3];

	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}

	r0 = PacketBuffer.U1[0];	g0 = PacketBuffer.U1[1];	b0 = PacketBuffer.U1[2];
	u0 = PacketBuffer.U1[8];	v0 = PacketBuffer.U1[9];
	r1 = PacketBuffer.U1[12];	g1 = PacketBuffer.U1[13];	b1 = PacketBuffer.U1[14];
	u1 = PacketBuffer.U1[20];	v1 = PacketBuffer.U1[21];
	r2 = PacketBuffer.U1[24];	g2 = PacketBuffer.U1[25];	b2 = PacketBuffer.U1[26];
	u2 = PacketBuffer.U1[32];	v2 = PacketBuffer.U1[33];

	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);		GPU_SWAP(y0, y1, temp);
			GPU_SWAP(u0, u1, temp);		GPU_SWAP(v0, v1, temp);
			GPU_SWAP(r0, r1, temp);		GPU_SWAP(g0, g1, temp);   GPU_SWAP(b0, b1, temp);
		}
	}
	if (y1 >= y2)
	{
		if( y1!=y2 || x1>x2 )
		{
			GPU_SWAP(x1, x2, temp);		GPU_SWAP(y1, y2, temp);
			GPU_SWAP(u1, u2, temp);		GPU_SWAP(v1, v2, temp);
			GPU_SWAP(r1, r2, temp);   GPU_SWAP(g1, g2, temp);		GPU_SWAP(b1, b2, temp);
		}
	}
	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);		GPU_SWAP(y0, y1, temp);
			GPU_SWAP(u0, u1, temp);		GPU_SWAP(v0, v1, temp);
			GPU_SWAP(r0, r1, temp);		GPU_SWAP(g0, g1, temp);		GPU_SWAP(b0, b1, temp);
		}
	}

	ya  = y2 - y0;
	yb  = y2 - y1;
	dx  = (x2 - x1) * ya - (x2 - x0) * yb;
	du4 = (u2 - u1) * ya - (u2 - u0) * yb;
	dv4 = (v2 - v1) * ya - (v2 - v0) * yb;
	dr4 = (r2 - r1) * ya - (r2 - r0) * yb;
	dg4 = (g2 - g1) * ya - (g2 - g0) * yb;
	db4 = (b2 - b1) * ya - (b2 - b0) * yb;

	s32 iF,iS;

	xInv(            dx, iF, iS);
	du4 = xInvMulx( du4, iF, iS);
	dv4 = xInvMulx( dv4, iF, iS);
	dr4 = xInvMulx( dr4, iF, iS);
	dg4 = xInvMulx( dg4, iF, iS);
	db4 = xInvMulx( db4, iF, iS);
	u32 dr = (u32)(dr4<< 8)&(0xffffffff<<21);   if(dr4<0) dr+= 1<<21;
	u32 dg = (u32)(dg4>> 3)&(0xffffffff<<10);   if(dg4<0) dg+= 1<<10;
	u32 db = (u32)(db4>>14)&(0xffffffff    );   if(db4<0) db+= 1<< 0;
	lInc = db + dg + dr;
	tInc = ((u32)(du4<<7)&0x7fff0000) | ((u32)(dv4>>9)&0x00007fff);
	tMsk = (TextureWindow[2]<<23) | (TextureWindow[3]<<7) | 0x00ff00ff;

	for (s32 loop0 = 2; loop0; --loop0)
	{
		if (loop0 == 2)
		{
			ya = y0;
			yb = y1;
			u3 = i2x(u0);
			v3 = i2x(v0);
			r3 = i2x(r0);
			g3 = i2x(g0);
			b3 = i2x(b0);
			x3 = i2x(x0);
			x4 = y0!=y1 ? x3 : i2x(x1);
			if (dx < 0)
			{
				xInv(           (y2 - y0), iF, iS);
				dx3 = xInvMulx( (x2 - x0), iF, iS);
				du3 = xInvMulx( (u2 - u0), iF, iS);
				dv3 = xInvMulx( (v2 - v0), iF, iS);
				dr3 = xInvMulx( (r2 - r0), iF, iS);
				dg3 = xInvMulx( (g2 - g0), iF, iS);
				db3 = xInvMulx( (b2 - b0), iF, iS);
				dx4 = xLoDivx ( (x1 - x0), (y1 - y0));
			}
			else
			{
				xInv(           (y1 - y0), iF, iS);
				dx3 = xInvMulx( (x1 - x0), iF, iS);
				du3 = xInvMulx( (u1 - u0), iF, iS);
				dv3 = xInvMulx( (v1 - v0), iF, iS);
				dr3 = xInvMulx( (r1 - r0), iF, iS);
				dg3 = xInvMulx( (g1 - g0), iF, iS);
				db3 = xInvMulx( (b1 - b0), iF, iS);
				dx4 = xLoDivx ( (x2 - x0), (y2 - y0));
			}
		}
		else
		{
			ya = y1;
			yb = y2;
			if (dx < 0)
			{
				temp = y1 - y0;
				u3  = i2x(u0) + (du3 * temp);
				v3  = i2x(v0) + (dv3 * temp);
				r3  = i2x(r0) + (dr3 * temp);
				g3  = i2x(g0) + (dg3 * temp);
				b3  = i2x(b0) + (db3 * temp);
				x3  = i2x(x0) + (dx3 * temp);
				x4  = i2x(x1);
				dx4 = xLoDivx((x2 - x1), (y2 - y1));
			}
			else
			{
				u3 = i2x(u1);
				v3 = i2x(v1);
				r3 = i2x(r1);
				g3 = i2x(g1);
				b3 = i2x(b1);
				x3 = i2x(x1);
				x4 = i2x(x0) + (dx4 * (y1 - y0));

				xInv(           (y2 - y1), iF, iS);
				dx3 = xInvMulx( (x2 - x1), iF, iS);
				du3 = xInvMulx( (u2 - u1), iF, iS);
				dv3 = xInvMulx( (v2 - v1), iF, iS);
				dr3 = xInvMulx( (r2 - r1), iF, iS);
				dg3 = xInvMulx( (g2 - g1), iF, iS);
				db3 = xInvMulx( (b2 - b1), iF, iS);
			}
		}

		temp = ymin - ya;
		if (temp > 0)
		{
			ya  = ymin;
			x3 += dx3*temp;   x4 += dx4*temp;
			u3 += du3*temp;   v3 += dv3*temp;
			r3 += dr3*temp;   g3 += dg3*temp;   b3 += db3*temp;
		}
		if (yb > ymax) yb = ymax;
		if (ya>=yb) continue;

		x3+= fixed_HALF;  x4+= fixed_HALF;
		u3+= fixed_HALF;  v4+= fixed_HALF;
		r3+= fixed_HALF;  g3+= fixed_HALF;  b3+= fixed_HALF;
		u16* PixelBase  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];
		
		for(;ya<yb;++ya, PixelBase += FRAME_WIDTH, x3+=dx3, x4+=dx4, u3+=du3, v3+=dv3, r3+=dr3, g3+=dg3,	b3+=db3)
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;
			xa = x2i(x3);
			xb = x2i(x4);
			if( (xa>xmax) || (xb<xmin))	continue;

			temp = xmin - xa;
			if(temp > 0)
			{
				xa  = xmin;
				u4 = u3 + du4*temp;   v4 = v3 + dv4*temp;
				r4 = r3 + dr4*temp;   g4 = g3 + dg4*temp;   b4 = b3 + db4*temp;
			}
			else
			{
				u4 = u3;  v4 = v3;
				r4 = r3;  g4 = g3;  b4 = b3;
			}
			if(xb > xmax) xb = xmax;
			xb-=xa;
			if(xb>0) gpuPolySpanDriver(PixelBase + xa,xb);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//senquack - Original Unai poly routines left here for reference:
// ( from gpu_inner.h ) NOTE: this uses 16.16, not 22.10 fixed point
//////////////////////////////////////////////////////////////////////////
template<const int CF>
INLINE void  gpuPolySpanFn(u16 *pDst, u32 count)
{
	if (!TM)
	{	
		// NO TEXTURE
		if (!G)
		{
			// NO GOURAUD
			u16 data;
			if (L) { u32 lCol=((u32)(b4<< 2)&(0x03ff)) | ((u32)(g4<<13)&(0x07ff<<10)) | ((u32)(r4<<24)&(0x07ff<<21)); gpuLightingRGB(data,lCol); }
			else data=PixelData;
			if ((!M)&&(!B))
			{
				if (MB) { data = data | 0x8000; }
				do { *pDst++ = data; } while (--count);
			}
			else if ((M)&&(!B))
			{
				if (MB) { data = data | 0x8000; }
				do { if (!(*pDst&0x8000)) { *pDst = data; } pDst++; } while (--count);
			}
			else
			{
				u16 uSrc;
				u16 uDst;
				u32 uMsk; if (BM==0) uMsk=0x7BDE;
				u32 bMsk; if (BI) bMsk=blit_mask;
				do
				{
					// blit-mask
					if (BI) { if((bMsk>>((((u32)pDst)>>1)&7))&1) goto endtile; }
					//  masking
					uDst = *pDst;
					if(M) { if (uDst&0x8000) goto endtile;  }
					uSrc = data;
					//  blend
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);
					if (MB) { *pDst = uSrc | 0x8000; }
					else    { *pDst = uSrc; }
					endtile: pDst++;
				}
				while (--count);
			}
		}
		else
		{
			// GOURAUD
			u16 uDst;
			u16 uSrc;
			u32 linc=lInc;
			u32 lCol=((u32)(b4>>14)&(0x03ff)) | ((u32)(g4>>3)&(0x07ff<<10)) | ((u32)(r4<<8)&(0x07ff<<21));
			u32 uMsk; if ((B)&&(BM==0)) uMsk=0x7BDE;
			u32 bMsk; if (BI) bMsk=blit_mask;
			do
			{
				// blit-mask
				if (BI) { if((bMsk>>((((u32)pDst)>>1)&7))&1) goto endgou; }
				//  masking
				if(M) { uDst = *pDst;  if (uDst&0x8000) goto endgou;  }
				//  blend
				if(B)
				{
					//  light
					gpuLightingRGB(uSrc,lCol);
					if(!M)    { uDst = *pDst; }
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);
				}
				else
				{
					//  light
					gpuLightingRGB(uSrc,lCol);
				}
				if (MB) { *pDst = uSrc | 0x8000; }
				else    { *pDst = uSrc; }
				endgou: pDst++; lCol=(lCol+linc);
			}
			while (--count);
		}
	}
	else
	{
		// TEXTURE
		u16 uDst;
		u16 uSrc;
		u32 linc; if (L&&G) linc=lInc;
		u32 tinc=tInc;
		u32 tmsk=tMsk;
		u32 tCor = ((u32)( u4<<7)&0x7fff0000) | ((u32)( v4>>9)&0x00007fff); tCor&= tmsk;
		const u16* _TBA=TBA;
		const u16* _CBA; if (TM!=3) _CBA=CBA;
		u32 lCol;
		if(L && !G) { lCol = ((u32)(b4<< 2)&(0x03ff)) | ((u32)(g4<<13)&(0x07ff<<10)) | ((u32)(r4<<24)&(0x07ff<<21)); }
		else if(L && G) { lCol = ((u32)(b4>>14)&(0x03ff)) | ((u32)(g4>>3)&(0x07ff<<10)) | ((u32)(r4<<8)&(0x07ff<<21)); 	}
		u32 uMsk; if ((B)&&(BM==0)) uMsk=0x7BDE;
		u32 bMsk; if (BI) bMsk=blit_mask;
		do
		{
			// blit-mask
			if (BI) { if((bMsk>>((((u32)pDst)>>1)&7))&1) goto endpoly; }
			//  masking
			if(M) { uDst = *pDst;  if (uDst&0x8000) goto endpoly;  }
			//  texture
			if (TM==1) { u32 tu=(tCor>>23); u32 tv=(tCor<<4)&(0xff<<11); u8 rgb=((u8*)_TBA)[tv+(tu>>1)]; uSrc=_CBA[(rgb>>((tu&1)<<2))&0xf]; if(!uSrc) goto endpoly; }
			if (TM==2) { uSrc = _CBA[(((u8*)_TBA)[(tCor>>23)+((tCor<<4)&(0xff<<11))])]; if(!uSrc)  goto endpoly; }
			if (TM==3) { uSrc = _TBA[(tCor>>23)+((tCor<<3)&(0xff<<10))]; if(!uSrc)  goto endpoly; }
			//  blend
			if(B)
			{
				if (uSrc&0x8000)
				{
					//  light
					if(L) gpuLightingTXT(uSrc, lCol);
					if(!M)    { uDst = *pDst; }
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);
				}
				else
				{
					// light
					if(L) gpuLightingTXT(uSrc, lCol);
				}
			}
			else
			{
				//  light
				if(L)  { gpuLightingTXT(uSrc, lCol); } else if(!MB) { uSrc&= 0x7fff; }
			}
			if (MB) { *pDst = uSrc | 0x8000; }
			else    { *pDst = uSrc; }
			endpoly: pDst++;
			tCor=(tCor+tinc)&tmsk;
			if (L&&G) lCol=(lCol+linc);
		}
		while (--count);
	}
}
