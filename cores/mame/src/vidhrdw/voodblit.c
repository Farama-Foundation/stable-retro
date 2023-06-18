#ifndef NUM_TMUS
#error need to define the number of TMUs
#endif


#ifndef BITFIELD
#define BITFIELD(fix,fixmask,var,shift,mask) \
	(((((fixmask) >> (shift)) & (mask)) == (mask)) ? (((fix) >> (shift)) & (mask)) : (((var) >> (shift)) & (mask)))
#define FBZCOLORPATH_BITS(start,len) \
	BITFIELD(FBZCOLORPATH, FBZCOLORPATH_MASK, voodoo_regs[fbzColorPath], start, (1 << (len)) - 1)
#define FOGMODE_BITS(start,len) \
	((voodoo_regs[fogMode] >> (start)) & ((1 << (len)) - 1))
#define ALPHAMODE_BITS(start,len) \
	BITFIELD(ALPHAMODE, ALPHAMODE_MASK, voodoo_regs[alphaMode], start, (1 << (len)) - 1)
#define FBZMODE_BITS(start,len) \
	BITFIELD(FBZMODE, FBZMODE_MASK, voodoo_regs[fbzMode], start, (1 << (len)) - 1)
#define TEXTUREMODE0_BITS(start,len) \
	BITFIELD(TEXTUREMODE0, TEXTUREMODE0_MASK, voodoo_regs[0x100 + textureMode], start, (1 << (len)) - 1)
#define TEXTUREMODE1_BITS(start,len) \
	BITFIELD(TEXTUREMODE1, TEXTUREMODE1_MASK, voodoo_regs[0x200 + textureMode], start, (1 << (len)) - 1)

#define NEEDS_TEX1		(NUM_TMUS > 1 && (TEXTUREMODE0_BITS(12,1) == 0 || TEXTUREMODE0_BITS(21,1) == 0))
#endif


void RENDERFUNC(void)
{
#if (PER_PIXEL_LOD)
	float sscale0 = (float)(trex_width[0] * trex_width[0]) * (1. / 65536.);
	float tscale0 = (float)(trex_height[0] * trex_height[0]) * (1. / 65536.);
	float tex0x = (float) sqrt(tri_ds0dx * tri_ds0dx * sscale0 + tri_dt0dx * tri_dt0dx * tscale0);
	float tex0y = (float) sqrt(tri_ds0dy * tri_ds0dy * sscale0 + tri_dt0dy * tri_dt0dy * tscale0);
	float lodbase0 = ((tex0x > tex0y) ? tex0x : tex0y) * 256.0f;
#if (NUM_TMUS > 1)
	float sscale1 = (float)(trex_width[1] * trex_width[1]) * (1. / 65536.);
	float tscale1 = (float)(trex_height[1] * trex_height[1]) * (1. / 65536.);
	float tex1x = (float) sqrt(tri_ds1dx * tri_ds1dx * sscale1 + tri_dt1dx * tri_dt1dx * tscale1);
	float tex1y = (float) sqrt(tri_ds1dy * tri_ds1dy * sscale1 + tri_dt1dy * tri_dt1dy * tscale1);
	float lodbase1 = ((tex1x > tex1y) ? tex1x : tex1y) * 256.0f;
#endif
#endif

	UINT16 *buffer = *fbz_draw_buffer;
	const UINT32 *lookup0 = NULL;
#if (NUM_TMUS > 1)
	const UINT32 *lookup1 = NULL;
#endif
	int x, y;
	struct tri_vertex *vmin, *vmid, *vmax;
	float dxdy_minmid, dxdy_minmax, dxdy_midmax;
	int starty, stopy;
	float fptemp;

#if (0)
	if (FBZMODE_BITS(4,1) || FBZMODE_BITS(10,1))
	{
		static const char *funcs[] = { "never", "lt", "eq", "le", "gt", "ne", "ge", "always" };
		if (!FBZMODE_BITS(20,1))
		{
			if (!FBZMODE_BITS(3,1))
				logerror("Depth Z: %c%c %s %08X,%08X,%08X -> %04X,%04X,%04X", FBZMODE_BITS(4,1) ? 'T' : ' ', FBZMODE_BITS(10,1) ? 'W' : ' ', funcs[FBZMODE_BITS(5,3)],
					tri_startz,
					tri_startz + (INT32)((tri_vb.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vb.x - tri_va.x) * (float)tri_dzdx),
					tri_startz + (INT32)((tri_vc.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vc.x - tri_va.x) * (float)tri_dzdx),
					(UINT16)(tri_startz >> 12),
					(UINT16)(tri_startz + (INT32)((tri_vb.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vb.x - tri_va.x) * (float)tri_dzdx)) >> 12,
					(UINT16)(tri_startz + (INT32)((tri_vc.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vc.x - tri_va.x) * (float)tri_dzdx)) >> 12);
			else if (!FBZMODE_BITS(21,1))
				logerror("Depth Wf: %c%c %s %f,%f,%f -> %04X,%04X,%04X", FBZMODE_BITS(4,1) ? 'T' : ' ', FBZMODE_BITS(10,1) ? 'W' : ' ', funcs[FBZMODE_BITS(5,3)],
					tri_startw,
					tri_startw + (INT32)((tri_vb.y - tri_va.y) * tri_dwdy) + (INT32)((tri_vb.x - tri_va.x) * tri_dwdx),
					tri_startw + (INT32)((tri_vc.y - tri_va.y) * tri_dwdy) + (INT32)((tri_vc.x - tri_va.x) * tri_dwdx),
					float_to_depth(tri_startw),
					float_to_depth(tri_startw + (INT32)((tri_vb.y - tri_va.y) * tri_dwdy) + (INT32)((tri_vb.x - tri_va.x) * tri_dwdx)),
					float_to_depth(tri_startw + (INT32)((tri_vc.y - tri_va.y) * tri_dwdy) + (INT32)((tri_vc.x - tri_va.x) * tri_dwdx)));
			else
				logerror("Depth Wz: %c%c %s %08X,%08X,%08X -> %04X,%04X,%04X", FBZMODE_BITS(4,1) ? 'T' : ' ', FBZMODE_BITS(10,1) ? 'W' : ' ', funcs[FBZMODE_BITS(5,3)],
					tri_startz,
					tri_startz + (INT32)((tri_vb.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vb.x - tri_va.x) * (float)tri_dzdx),
					tri_startz + (INT32)((tri_vc.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vc.x - tri_va.x) * (float)tri_dzdx),
					float_to_depth((float)(tri_startz) * (1.0 / 4096.0)),
					float_to_depth((float)(tri_startz + (INT32)((tri_vb.y - tri_va.y) * tri_dzdy) + (INT32)((tri_vb.x - tri_va.x) * tri_dzdx)) * (1.0 / 4096.0)),
					float_to_depth((float)(tri_startz + (INT32)((tri_vc.y - tri_va.y) * tri_dzdy) + (INT32)((tri_vc.x - tri_va.x) * tri_dzdx)) * (1.0 / 4096.0)));
			
			if (FBZMODE_BITS(16,1))
				log_cb(RETRO_LOG_DEBUG, LOGPRE " + %04X\n", (UINT16)voodoo_regs[zaColor]);
		}
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Depth const: %04X\n", (UINT16)voodoo_regs[zaColor]);
	}
#endif

#if (TRACK_LOD)
	int lodbin[9];
	if (loglod)
	{
		int tlod;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "-----\n");
		log_cb(RETRO_LOG_DEBUG, LOGPRE "LOD: (%f,%f)-(%f,%f)-(%f,%f)\n", tri_va.x, tri_va.y, tri_vb.x, tri_vb.y, tri_vc.x, tri_vc.y);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "LOD: startw0 = %f, dwdx = %f, dwdy = %f\n", tri_startw0, tri_dw0dx, tri_dw0dy);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "LOD: dsdx=%f dtdx=%f tex0x=%f tex0x/startw0=%f, twidth=%d\n", tri_ds0dx, tri_dt0dx, tex0x, tex0x/tri_startw0, trex_width[0]);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "LOD: dsdy=%f dtdy=%f tex0y=%f tex0y/startw0=%f, theight=%d\n", tri_ds0dy, tri_dt0dy, tex0y, tex0y/tri_startw0, trex_height[0]);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "LOD: lodbase0 = %f (%f)\n", lodbase0, lodbase0 / 256.0f);
		tlod = TRUNC_TO_INT((1.0f / tri_startw0) * lodbase0);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "LOD: lodbase0 * startw0^2 = %f (%d)\n", (1.0f / tri_startw0) * lodbase0 / 256.0f, tlod);
		if (tlod < 0)
			tlod = 0;
		else if (tlod < 65536)
			tlod = lod_lookup[tlod];
		else
			tlod = 8 << 2;
		memset(lodbin, 0, sizeof(lodbin));
		log_cb(RETRO_LOG_DEBUG, LOGPRE "LOD: final lod=%d, bias=%d, min=%d, max=%d\n", tlod, trex_lodbias[0], trex_lodmin[0], trex_lodmax[0]);
	}
#endif

	/* check for unhandled stuff */
	if ((voodoo_regs[tLOD] >> 24) & 1)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "tmultibaseaddr\n");
	
	/* sort the verticies */
	vmin = &tri_va;
	vmid = &tri_vb;
	vmax = &tri_vc;
	if (vmid->y < vmin->y) { struct tri_vertex *temp = vmid; vmid = vmin; vmin = temp; }
	if (vmax->y < vmin->y) { struct tri_vertex *temp = vmax; vmax = vmin; vmin = temp; }
	if (vmax->y < vmid->y) { struct tri_vertex *temp = vmax; vmax = vmid; vmid = temp; }

	/* compute the clipped start/end y */
	starty = TRUNC_TO_INT(vmin->y + 0.5f);
	stopy = TRUNC_TO_INT(vmax->y + 0.5f);
	if (starty < fbz_cliprect->min_y)
		starty = fbz_cliprect->min_y;
	if (stopy > fbz_cliprect->max_y)
		starty = fbz_cliprect->max_y;
	if (starty >= stopy)
		return;
	
	/* compute the slopes */
	fptemp = vmid->y - vmin->y;
	if (fptemp == 0.0f) fptemp = 1.0f;
	dxdy_minmid = (vmid->x - vmin->x) / fptemp;
	fptemp = vmax->y - vmin->y;
	if (fptemp == 0.0f) fptemp = 1.0f;
	dxdy_minmax = (vmax->x - vmin->x) / fptemp;
	fptemp = vmax->y - vmid->y;
	if (fptemp == 0.0f) fptemp = 1.0f;
	dxdy_midmax = (vmax->x - vmid->x) / fptemp;

	/* setup texture */
	if (FBZCOLORPATH_BITS(27,1))
	{
		int t;

		/* determine the lookup */
		t = TEXTUREMODE0_BITS(8,4);
		if ((t & 7) == 1 && TEXTUREMODE0_BITS(5,1))
			t += 6;
		
		/* handle dirty tables */
		if (texel_lookup_dirty[0][t])
		{
			(*update_texel_lookup[t])(0);
			texel_lookup_dirty[0][t] = 0;
		}
		lookup0 = &texel_lookup[0][t][0];
		
#if (NUM_TMUS > 1)
		/* determine the lookup */
		if (NEEDS_TEX1 && tmus > 1)
		{
			t = TEXTUREMODE1_BITS(8,4);
			if ((t & 7) == 1 && TEXTUREMODE1_BITS(5,1))
				t += 6;
			
			/* handle dirty tables */
			if (texel_lookup_dirty[1][t])
			{
				(*update_texel_lookup[t])(1);
				texel_lookup_dirty[1][t] = 0;
			}
			lookup1 = &texel_lookup[1][t][0];
		}
#endif
	}

	/* do the render */
	for (y = starty; y < stopy; y++)
	{
		int effy = FBZMODE_BITS(17,1) ? (inverted_yorigin - y) : y;
		if (effy >= 0 && effy < FRAMEBUF_HEIGHT)
		{
			UINT16 *dest = &buffer[effy * FRAMEBUF_WIDTH];
			UINT16 *depth = &depthbuf[effy * FRAMEBUF_WIDTH];
			INT32 dx, dy;
			float fdx, fdy;
			INT32 curr, curg, curb, cura, curz;
			float curw;
			float curs0, curt0, curw0, invw0;
#if (NUM_TMUS > 1)
			float curs1, curt1, curw1, invw1;
#endif
			int startx, stopx;
			float fpy;
			
			/* compute X endpoints */
			fpy = (float)y + 0.5f;
			startx = TRUNC_TO_INT((fpy - vmin->y) * dxdy_minmax + vmin->x + 0.5f);
			if (fpy < vmid->y)
				stopx = TRUNC_TO_INT((fpy - vmin->y) * dxdy_minmid + vmin->x + 0.5f);
			else
				stopx = TRUNC_TO_INT((fpy - vmid->y) * dxdy_midmax + vmid->x + 0.5f);
			if (startx > stopx) { int temp = startx; startx = stopx; stopx = temp; }
			if (startx < fbz_cliprect->min_x) startx = fbz_cliprect->min_x;
			if (stopx > fbz_cliprect->max_x) stopx = fbz_cliprect->max_x;
			if (startx >= stopx)
				continue;

			/* compute parameters */
			fdx = (float)startx + 0.5f - tri_va.x;
			fdy = (float)y + 0.5f - tri_va.y;
			dx = TRUNC_TO_INT(fdx * 16.0f + 0.5f);
			dy = TRUNC_TO_INT(fdy * 16.0f + 0.5f);
			curr = tri_startr + ((dy * tri_drdy + dx * tri_drdx) >> 4);
			curg = tri_startg + ((dy * tri_dgdy + dx * tri_dgdx) >> 4);
			curb = tri_startb + ((dy * tri_dbdy + dx * tri_dbdx) >> 4);
			cura = tri_starta + ((dy * tri_dady + dx * tri_dadx) >> 4);
			curz = tri_startz + ((dy * tri_dzdy + dx * tri_dzdx) >> 4);
			curw = tri_startw + fdy * tri_dwdy + fdx * tri_dwdx;
			curs0 = tri_starts0 + fdy * tri_ds0dy + fdx * tri_ds0dx;
			curt0 = tri_startt0 + fdy * tri_dt0dy + fdx * tri_dt0dx;
			curw0 = tri_startw0 + fdy * tri_dw0dy + fdx * tri_dw0dx;
#if (NUM_TMUS > 1)
			curs1 = tri_starts1 + fdy * tri_ds1dy + fdx * tri_ds1dx;
			curt1 = tri_startt1 + fdy * tri_dt1dy + fdx * tri_dt1dx;
			curw1 = tri_startw1 + fdy * tri_dw1dy + fdx * tri_dw1dx;
#endif

			/* loop over X */
			voodoo_regs[fbiPixelsIn] += stopx - startx;
			ADD_TO_PIXEL_COUNT(stopx - startx);
			for (x = startx; x < stopx; x++)
			{
				INT32 r = 0, g = 0, b = 0, a = 0, depthval;
				UINT32 texel = 0, c_local = 0;
				
				/* rotate stipple pattern */
				if (!FBZMODE_BITS(12,1))
					voodoo_regs[stipple] = (voodoo_regs[stipple] << 1) | (voodoo_regs[stipple] >> 31);
				
				/* handle stippling */
				if (FBZMODE_BITS(2,1))
				{
					/* rotate mode */
					if (!FBZMODE_BITS(12,1))
					{
						if ((voodoo_regs[stipple] & 0x80000000) == 0)
							goto skipdrawdepth;
					}
					
					/* pattern mode */
					else
					{
						int stipple_index = ((y & 3) << 3) | (~x & 7);
						if ((voodoo_regs[stipple] & (1 << stipple_index)) == 0)
							goto skipdrawdepth;
					}
				}

				/* compute depth value (W or Z) for this pixel */
				if (!FBZMODE_BITS(3,1))
				{
					depthval = curz >> 12;
					if (depthval >= 0xffff)
						depthval = 0xffff;
					else if (depthval < 0)
						depthval = 0;
				}
				else if (!FBZMODE_BITS(21,1))
					depthval = float_to_depth(curw);
				else
					depthval = float_to_depth((float)curz * (1.0 / 4096.0));
					
				/* handle depth buffer testing */
				if (FBZMODE_BITS(4,1))
				{
					INT32 depthsource;
				
					/* the source depth is either the iterated W/Z+bias or a constant value */
					if (!FBZMODE_BITS(20,1))
					{
						depthsource = depthval;

						/* add the bias */
						if (FBZMODE_BITS(16,1))
						{
							depthsource += (INT16)voodoo_regs[zaColor];
						
							if (depthsource >= 0xffff)
								depthsource = 0xffff;
							else if (depthsource < 0)
								depthsource = 0;
						}
					}
					else
						depthsource = voodoo_regs[zaColor] & 0xffff;

					/* test against the depth buffer */
					switch (FBZMODE_BITS(5,3))
					{
						case 0:
							goto skipdrawdepth;
						case 1:
							if (depthsource >= depth[x])
								goto skipdrawdepth;
							break;
						case 2:
							if (depthsource != depth[x])
								goto skipdrawdepth;
							break;
						case 3:
							if (depthsource > depth[x])
								goto skipdrawdepth;
							break;
						case 4:
							if (depthsource <= depth[x])
								goto skipdrawdepth;
							break;
						case 5:
							if (depthsource == depth[x])
								goto skipdrawdepth;
							break;
						case 6: 
							if (depthsource < depth[x])
								goto skipdrawdepth;
							break;
						case 7:
							break;
					}
				}
				
				/* load the texel if necessary */
				if (FBZCOLORPATH_BITS(0,2) == 1 || FBZCOLORPATH_BITS(2,2) == 1)
				{
					UINT32 c_other = 0;
					INT32 tr, tg, tb, ta;
					UINT8 *texturebase;
					float fs, ft;
					UINT8 lodshift;
					int lod;
					
#if (NUM_TMUS > 1)
					/* import from TMU 1 if needed */
					if (NEEDS_TEX1 && tmus > 1)
					{
						/* perspective correct */
						fs = curs1;
						ft = curt1;
						if (TEXTUREMODE1_BITS(0,1))
						{
							invw1 = 1.0f / curw1;
							fs *= invw1;
							ft *= invw1;
#if (PER_PIXEL_LOD)
							lod = TRUNC_TO_INT(invw1 * lodbase1);
#endif
						}
#if (PER_PIXEL_LOD)
						else
							lod = TRUNC_TO_INT(lodbase1);
							
						/* compute LOD */
						if (lod < 0)
							lod = 0;
						else if (lod < 65536)
							lod = lod_lookup[lod];
						else
							lod = 8 << 2;
						
						/* clamp LOD */
						lod += trex_lodbias[1];
						if (TEXTUREMODE1_BITS(4,1))
							lod += lod_dither_matrix[(x & 3) | ((y & 3) << 2)];
						if (lod < trex_lodmin[1]) lod = trex_lodmin[1];
						if (lod > trex_lodmax[1]) lod = trex_lodmax[1];
#else
						lod = trex_lodmin[1];
#endif
						/* compute texture base */
						if (!TEXTUREMODE1_BITS(11,1))
							texturebase = textureram[1] + ((voodoo_regs[0x200 + texBaseAddr] * 8 + trex_lod_offset[1][lod >> 2]) & texram_mask);
						else
							texturebase = textureram[1] + ((voodoo_regs[0x200 + texBaseAddr] * 8 + 2 * trex_lod_offset[1][lod >> 2]) & texram_mask);
						lodshift = trex_lod_width_shift[1][lod >> 2];

						/* point-sampled filter */
#if (BILINEAR_FILTER)
						if ((TEXTUREMODE1_BITS(1,2) == 0) || 
							(lod == trex_lodmin[1] && !TEXTUREMODE1_BITS(2,1)) || 
							(lod != trex_lodmin[1] && !TEXTUREMODE1_BITS(1,1)))
#endif
						{
							/* convert to int */
							INT32 s = TRUNC_TO_INT(fs);
							INT32 t = TRUNC_TO_INT(ft);
							lod >>= 2;

							/* clamp W */
							if (TEXTUREMODE1_BITS(3,1) && curw1 < 0.0f)
								s = t = 0;
								
							/* clamp S */
							if (TEXTUREMODE1_BITS(6,1))
							{
								if (s < 0) s = 0;
								else if (s >= trex_width[1]) s = trex_width[1] - 1;
							}
							else
								s &= trex_width[1] - 1;
							s >>= lod;
								
							/* clamp T */
							if (TEXTUREMODE1_BITS(7,1))
							{
								if (t < 0) t = 0;
								else if (t >= trex_height[1]) t = trex_height[1] - 1;
							}
							else
								t &= trex_height[1] - 1;
							t >>= lod;
							
							/* fetch raw texel data */
							if (!TEXTUREMODE1_BITS(11,1))
								texel = *((UINT8 *)texturebase + (t << lodshift) + s);
							else
								texel = *((UINT16 *)texturebase + (t << lodshift) + s);
							
							/* convert to ARGB */
							c_local = lookup1[texel];
						}
						
#if (BILINEAR_FILTER)
						/* bilinear filter */
						else
						{
							INT32 ts0, tt0, ts1, tt1;
							UINT32 factor, factorsum, ag, rb;
						
							/* convert to int */
							INT32 s, t;
							lod >>= 2;
							s = TRUNC_TO_INT(fs * 256.0f) - (128 << lod);
							t = TRUNC_TO_INT(ft * 256.0f) - (128 << lod);

							/* clamp W */
							if (TEXTUREMODE1_BITS(3,1) && curw1 < 0.0f)
								s = t = 0;

							/* clamp S0 */
							ts0 = s >> 8;
							if (TEXTUREMODE1_BITS(6,1))
							{
								if (ts0 < 0) ts0 = 0;
								else if (ts0 >= trex_width[1]) ts0 = trex_width[1] - 1;
							}
							else
								ts0 &= trex_width[1] - 1;
							ts0 >>= lod;
								
							/* clamp S1 */
							ts1 = (s >> 8) + (1 << lod);
							if (TEXTUREMODE1_BITS(6,1))
							{
								if (ts1 < 0) ts1 = 0;
								else if (ts1 >= trex_width[1]) ts1 = trex_width[1] - 1;
							}
							else
								ts1 &= trex_width[1] - 1;
							ts1 >>= lod;
								
							/* clamp T0 */
							tt0 = t >> 8;
							if (TEXTUREMODE1_BITS(7,1))
							{
								if (tt0 < 0) tt0 = 0;
								else if (tt0 >= trex_height[1]) tt0 = trex_height[1] - 1;
							}
							else
								tt0 &= trex_height[1] - 1;
							tt0 >>= lod;
								
							/* clamp T1 */
							tt1 = (t >> 8) + (1 << lod);
							if (TEXTUREMODE1_BITS(7,1))
							{
								if (tt1 < 0) tt1 = 0;
								else if (tt1 >= trex_height[1]) tt1 = trex_height[1] - 1;
							}
							else
								tt1 &= trex_height[1] - 1;
							tt1 >>= lod;
							
							s >>= lod;
							t >>= lod;
								
							/* texel 0 */
							factorsum = factor = ((0x100 - (s & 0xff)) * (0x100 - (t & 0xff))) >> 8;
							
							/* fetch raw texel data */
							if (!TEXTUREMODE1_BITS(11,1))
								texel = *((UINT8 *)texturebase + (tt0 << lodshift) + ts0);
							else
								texel = *((UINT16 *)texturebase + (tt0 << lodshift) + ts0);
							
							/* convert to ARGB */
							texel = lookup1[texel];
							ag = ((texel >> 8) & 0x00ff00ff) * factor;
							rb = (texel & 0x00ff00ff) * factor;
								
							/* texel 1 */
							factorsum += factor = ((s & 0xff) * (0x100 - (t & 0xff))) >> 8;
							if (factor)
							{
								/* fetch raw texel data */
								if (!TEXTUREMODE1_BITS(11,1))
									texel = *((UINT8 *)texturebase + (tt0 << lodshift) + ts1);
								else
									texel = *((UINT16 *)texturebase + (tt0 << lodshift) + ts1);
								
								/* convert to ARGB */
								texel = lookup1[texel];
								ag += ((texel >> 8) & 0x00ff00ff) * factor;
								rb += (texel & 0x00ff00ff) * factor;
							}

							/* texel 2 */
							factorsum += factor = ((0x100 - (s & 0xff)) * (t & 0xff)) >> 8;
							if (factor)
							{
								/* fetch raw texel data */
								if (!TEXTUREMODE1_BITS(11,1))
									texel = *((UINT8 *)texturebase + (tt1 << lodshift) + ts0);
								else
									texel = *((UINT16 *)texturebase + (tt1 << lodshift) + ts0);
								
								/* convert to ARGB */
								texel = lookup1[texel];
								ag += ((texel >> 8) & 0x00ff00ff) * factor;
								rb += (texel & 0x00ff00ff) * factor;
							}
								
							/* texel 3 */
							factor = 0x100 - factorsum;
							if (factor)
							{
								/* fetch raw texel data */
								if (!TEXTUREMODE1_BITS(11,1))
									texel = *((UINT8 *)texturebase + (tt1 << lodshift) + ts1);
								else
									texel = *((UINT16 *)texturebase + (tt1 << lodshift) + ts1);
								
								/* convert to ARGB */
								texel = lookup1[texel];
								ag += ((texel >> 8) & 0x00ff00ff) * factor;
								rb += (texel & 0x00ff00ff) * factor;
							}
							
							/* this becomes the local color */
							c_local = (ag & 0xff00ff00) | ((rb >> 8) & 0x00ff00ff);
						}
#endif
						/* zero/other selection */
						if (!TEXTUREMODE1_BITS(12,1))				/* tc_zero_other */
						{
							tr = (c_other >> 16) & 0xff;
							tg = (c_other >> 8) & 0xff;
							tb = (c_other >> 0) & 0xff;
						}
						else
							tr = tg = tb = 0;

						/* subtract local color */
						if (TEXTUREMODE1_BITS(13,1))				/* tc_sub_clocal */
						{
							tr -= (c_local >> 16) & 0xff;
							tg -= (c_local >> 8) & 0xff;
							tb -= (c_local >> 0) & 0xff;
						}

						/* scale RGB */
						if (TEXTUREMODE1_BITS(14,3) != 0)			/* tc_mselect mux */
						{
							INT32 rm = 0, gm = 0, bm = 0;
							
							switch (TEXTUREMODE1_BITS(14,3))
							{
								case 1:		/* tc_mselect mux == c_local */
									rm = (c_local >> 16) & 0xff;
									gm = (c_local >> 8) & 0xff;
									bm = (c_local >> 0) & 0xff;
									break;
								case 2:		/* tc_mselect mux == a_other */
									rm = gm = bm = c_other >> 24;
									break;
								case 3:		/* tc_mselect mux == a_local */
									rm = gm = bm = c_local >> 24;
									break;
								case 4:		/* tc_mselect mux == LOD */
									rm = gm = bm = 0x80;/*texel >> 24;*/
									break;
								case 5:		/* tc_mselect mux == LOD frac */
									rm = gm = bm = 0x80;/*texel >> 24;*/
									break;
							}

							if (TEXTUREMODE1_BITS(17,1))			/* tc_reverse_blend */
							{
								tr = (tr * (rm + 1)) >> 8;
								tg = (tg * (gm + 1)) >> 8;
								tb = (tb * (bm + 1)) >> 8;
							}
							else
							{
								tr = (tr * ((rm ^ 0xff) + 1)) >> 8;
								tg = (tg * ((gm ^ 0xff) + 1)) >> 8;
								tb = (tb * ((bm ^ 0xff) + 1)) >> 8;
							}
						}

						/* add local color */
						if (TEXTUREMODE1_BITS(18,1))				/* tc_add_clocal */
						{
							tr += (c_local >> 16) & 0xff;
							tg += (c_local >> 8) & 0xff;
							tb += (c_local >> 0) & 0xff;
						}

						/* add local alpha */
						else if (TEXTUREMODE1_BITS(19,1))			/* tc_add_alocal */
						{
							tr += c_local >> 24;
							tg += c_local >> 24;
							tb += c_local >> 24;
						}
						
						/* zero/other selection */
						if (!TEXTUREMODE1_BITS(21,1))				/* tca_zero_other */
							ta = (c_other >> 24) & 0xff;
						else
							ta = 0;
						
						/* subtract local alpha */
						if (TEXTUREMODE1_BITS(22,1))				/* tca_sub_clocal */
							ta -= (c_local >> 24) & 0xff;

						/* scale alpha */
						if (TEXTUREMODE1_BITS(23,3) != 0)			/* tca_mselect mux */
						{
							INT32 am = 0;
							
							switch (TEXTUREMODE1_BITS(23,3))
							{
								case 1:		/* tca_mselect mux == c_local */
									am = (c_local >> 24) & 0xff;
									break;
								case 2:		/* tca_mselect mux == a_other */
									am = c_other >> 24;
									break;
								case 3:		/* tca_mselect mux == a_local */
									am = c_local >> 24;
									break;
								case 4:		/* tca_mselect mux == LOD */
									am = 0x80;/*texel >> 24;*/
									break;
								case 5:		/* tca_mselect mux == LOD frac */
									am = 0x80;/*texel >> 24;*/
									break;
							}

							if (TEXTUREMODE1_BITS(26,1))			/* tca_reverse_blend */
								ta = (ta * (am + 1)) >> 8;
							else
								ta = (ta * ((am ^ 0xff) + 1)) >> 8;
						}

						/* add local color */
						if (TEXTUREMODE1_BITS(27,1) ||
							TEXTUREMODE1_BITS(28,1))				/* tca_add_clocal/tca_add_alocal */
							ta += (c_local >> 24) & 0xff;
						
						/* clamp */
						if (tr < 0) tr = 0;
						else if (tr > 255) tr = 255;
						if (tg < 0) tg = 0;
						else if (tg > 255) tg = 255;
						if (tb < 0) tb = 0;
						else if (tb > 255) tb = 255;
						if (ta < 0) ta = 0;
						else if (ta > 255) ta = 255;
						c_other = (ta << 24) | (tr << 16) | (tg << 8) | tb;

						/* invert */
						if (TEXTUREMODE1_BITS(20,1))
							c_other ^= 0x00ffffff;
						if (TEXTUREMODE1_BITS(29,1))
							c_other ^= 0xff000000;
					}
#endif
					/* perspective correct */
					fs = curs0;
					ft = curt0;
					if (TEXTUREMODE0_BITS(0,1))
					{
						invw0 = 1.0f / curw0;
						fs *= invw0;
						ft *= invw0;
#if (PER_PIXEL_LOD)
						lod = TRUNC_TO_INT(invw0 * lodbase0);
#endif
					}
#if (PER_PIXEL_LOD)
					else
						lod = TRUNC_TO_INT(lodbase0);
						
					/* compute LOD */
					if (lod < 0)
						lod = 0;
					else if (lod < 65536)
						lod = lod_lookup[lod];
					else
						lod = 8 << 2;
					
					/* clamp LOD */
					lod += trex_lodbias[0];
					if (TEXTUREMODE0_BITS(4,1))
						lod += lod_dither_matrix[(x & 3) | ((y & 3) << 2)];
					if (lod < trex_lodmin[0]) lod = trex_lodmin[0];
					if (lod > trex_lodmax[0]) lod = trex_lodmax[0];
#else
					lod = trex_lodmin[0];
#endif
					/* compute texture base */
					if (!TEXTUREMODE0_BITS(11,1))
						texturebase = textureram[0] + ((voodoo_regs[0x100 + texBaseAddr] * 8 + trex_lod_offset[0][lod >> 2]) & texram_mask);
					else
						texturebase = textureram[0] + ((voodoo_regs[0x100 + texBaseAddr] * 8 + 2 * trex_lod_offset[0][lod >> 2]) & texram_mask);
					lodshift = trex_lod_width_shift[0][lod >> 2];
#if (TRACK_LOD)
					lodbin[lod >> 2]++;
#endif

#if (BILINEAR_FILTER)
					/* point-sampled filter */
					if ((TEXTUREMODE0_BITS(1,2) == 0) || 
						(lod == trex_lodmin[0] && !TEXTUREMODE0_BITS(2,1)) || 
						(lod != trex_lodmin[0] && !TEXTUREMODE0_BITS(1,1)))
#endif
					{
						/* convert to int */
						INT32 s = TRUNC_TO_INT(fs);
						INT32 t = TRUNC_TO_INT(ft);
						lod >>= 2;

						/* clamp W */
						if (TEXTUREMODE0_BITS(3,1) && curw0 < 0.0f)
							s = t = 0;
							
						/* clamp S */
						if (TEXTUREMODE0_BITS(6,1))
						{
							if (s < 0) s = 0;
							else if (s >= trex_width[0]) s = trex_width[0] - 1;
						}
						else
							s &= trex_width[0] - 1;
						s >>= lod;
							
						/* clamp T */
						if (TEXTUREMODE0_BITS(7,1))
						{
							if (t < 0) t = 0;
							else if (t >= trex_height[0]) t = trex_height[0] - 1;
						}
						else
							t &= trex_height[0] - 1;
						t >>= lod;
						
						/* fetch raw texel data */
						if (!TEXTUREMODE0_BITS(11,1))
							texel = *((UINT8 *)texturebase + (t << lodshift) + s);
						else
							texel = *((UINT16 *)texturebase + (t << lodshift) + s);
						
						/* convert to ARGB */
						c_local = lookup0[texel];
					}
					
#if (BILINEAR_FILTER)
					/* bilinear filter */
					else
					{
						INT32 ts0, tt0, ts1, tt1;
						UINT32 factor, factorsum, ag, rb;
					
						/* convert to int */
						INT32 s, t;
						lod >>= 2;
						s = TRUNC_TO_INT(fs * 256.0f) - (128 << lod);
						t = TRUNC_TO_INT(ft * 256.0f) - (128 << lod);

						/* clamp W */
						if (TEXTUREMODE0_BITS(3,1) && curw0 < 0.0f)
							s = t = 0;

						/* clamp S0 */
						ts0 = s >> 8;
						if (TEXTUREMODE0_BITS(6,1))
						{
							if (ts0 < 0) ts0 = 0;
							else if (ts0 >= trex_width[0]) ts0 = trex_width[0] - 1;
						}
						else
							ts0 &= trex_width[0] - 1;
						ts0 >>= lod;
							
						/* clamp S1 */
						ts1 = (s >> 8) + (1 << lod);
						if (TEXTUREMODE0_BITS(6,1))
						{
							if (ts1 < 0) ts1 = 0;
							else if (ts1 >= trex_width[0]) ts1 = trex_width[0] - 1;
						}
						else
							ts1 &= trex_width[0] - 1;
						ts1 >>= lod;
							
						/* clamp T0 */
						tt0 = t >> 8;
						if (TEXTUREMODE0_BITS(7,1))
						{
							if (tt0 < 0) tt0 = 0;
							else if (tt0 >= trex_height[0]) tt0 = trex_height[0] - 1;
						}
						else
							tt0 &= trex_height[0] - 1;
						tt0 >>= lod;
							
						/* clamp T1 */
						tt1 = (t >> 8) + (1 << lod);
						if (TEXTUREMODE0_BITS(7,1))
						{
							if (tt1 < 0) tt1 = 0;
							else if (tt1 >= trex_height[0]) tt1 = trex_height[0] - 1;
						}
						else
							tt1 &= trex_height[0] - 1;
						tt1 >>= lod;
						
						s >>= lod;
						t >>= lod;
							
						/* texel 0 */
						factorsum = factor = ((0x100 - (s & 0xff)) * (0x100 - (t & 0xff))) >> 8;
						
						/* fetch raw texel data */
						if (!TEXTUREMODE0_BITS(11,1))
							texel = *((UINT8 *)texturebase + (tt0 << lodshift) + ts0);
						else
							texel = *((UINT16 *)texturebase + (tt0 << lodshift) + ts0);

						/* convert to ARGB */
						texel = lookup0[texel];
						ag = ((texel >> 8) & 0x00ff00ff) * factor;
						rb = (texel & 0x00ff00ff) * factor;
							
						/* texel 1 */
						factorsum += factor = ((s & 0xff) * (0x100 - (t & 0xff))) >> 8;
						if (factor)
						{
							/* fetch raw texel data */
							if (!TEXTUREMODE0_BITS(11,1))
								texel = *((UINT8 *)texturebase + (tt0 << lodshift) + ts1);
							else
								texel = *((UINT16 *)texturebase + (tt0 << lodshift) + ts1);
							
							/* convert to ARGB */
							texel = lookup0[texel];
							ag += ((texel >> 8) & 0x00ff00ff) * factor;
							rb += (texel & 0x00ff00ff) * factor;
						}

						/* texel 2 */
						factorsum += factor = ((0x100 - (s & 0xff)) * (t & 0xff)) >> 8;
						if (factor)
						{
							/* fetch raw texel data */
							if (!TEXTUREMODE0_BITS(11,1))
								texel = *((UINT8 *)texturebase + (tt1 << lodshift) + ts0);
							else
								texel = *((UINT16 *)texturebase + (tt1 << lodshift) + ts0);
							
							/* convert to ARGB */
							texel = lookup0[texel];
							ag += ((texel >> 8) & 0x00ff00ff) * factor;
							rb += (texel & 0x00ff00ff) * factor;
						}
							
						/* texel 3 */
						factor = 0x100 - factorsum;
						if (factor)
						{
							/* fetch raw texel data */
							if (!TEXTUREMODE0_BITS(11,1))
								texel = *((UINT8 *)texturebase + (tt1 << lodshift) + ts1);
							else
								texel = *((UINT16 *)texturebase + (tt1 << lodshift) + ts1);
							
							/* convert to ARGB */
							texel = lookup0[texel];
							ag += ((texel >> 8) & 0x00ff00ff) * factor;
							rb += (texel & 0x00ff00ff) * factor;
						}
						
						/* this becomes the local color */
						c_local = (ag & 0xff00ff00) | ((rb >> 8) & 0x00ff00ff);
					}
#endif
					/* zero/other selection */
					if (!TEXTUREMODE0_BITS(12,1))				/* tc_zero_other */
					{
						tr = (c_other >> 16) & 0xff;
						tg = (c_other >> 8) & 0xff;
						tb = (c_other >> 0) & 0xff;
					}
					else
						tr = tg = tb = 0;

					/* subtract local color */
					if (TEXTUREMODE0_BITS(13,1))				/* tc_sub_clocal */
					{
						tr -= (c_local >> 16) & 0xff;
						tg -= (c_local >> 8) & 0xff;
						tb -= (c_local >> 0) & 0xff;
					}

					/* scale RGB */
					if (TEXTUREMODE0_BITS(14,3) != 0)			/* tc_mselect mux */
					{
						INT32 rm = 0, gm = 0, bm = 0;
						
						switch (TEXTUREMODE0_BITS(14,3))
						{
							case 1:		/* tc_mselect mux == c_local */
								rm = (c_local >> 16) & 0xff;
								gm = (c_local >> 8) & 0xff;
								bm = (c_local >> 0) & 0xff;
								break;
							case 2:		/* tc_mselect mux == a_other */
								rm = gm = bm = c_other >> 24;
								break;
							case 3:		/* tc_mselect mux == a_local */
								rm = gm = bm = c_local >> 24;
								break;
							case 4:		/* tc_mselect mux == LOD */
								rm = gm = bm = 0x80;/*texel >> 24;*/
								break;
							case 5:		/* tc_mselect mux == LOD frac */
								rm = gm = bm = 0x80;/*texel >> 24;*/
								break;
						}

						if (TEXTUREMODE0_BITS(17,1))			/* tc_reverse_blend */
						{
							tr = (tr * (rm + 1)) >> 8;
							tg = (tg * (gm + 1)) >> 8;
							tb = (tb * (bm + 1)) >> 8;
						}
						else
						{
							tr = (tr * ((rm ^ 0xff) + 1)) >> 8;
							tg = (tg * ((gm ^ 0xff) + 1)) >> 8;
							tb = (tb * ((bm ^ 0xff) + 1)) >> 8;
						}
					}

					/* add local color */
					if (TEXTUREMODE0_BITS(18,1))				/* tc_add_clocal */
					{
						tr += (c_local >> 16) & 0xff;
						tg += (c_local >> 8) & 0xff;
						tb += (c_local >> 0) & 0xff;
					}

					/* add local alpha */
					else if (TEXTUREMODE0_BITS(19,1))			/* tc_add_alocal */
					{
						tr += c_local >> 24;
						tg += c_local >> 24;
						tb += c_local >> 24;
					}
					
					/* zero/other selection */
					if (!TEXTUREMODE0_BITS(21,1))				/* tca_zero_other */
						ta = (c_other >> 24) & 0xff;
					else
						ta = 0;
					
					/* subtract local alpha */
					if (TEXTUREMODE0_BITS(22,1))				/* tca_sub_clocal */
						ta -= (c_local >> 24) & 0xff;

					/* scale alpha */
					if (TEXTUREMODE0_BITS(23,3) != 0)			/* tca_mselect mux */
					{
						INT32 am = 0;
						
						switch (TEXTUREMODE0_BITS(23,3))
						{
							case 1:		/* tca_mselect mux == c_local */
								am = (c_local >> 24) & 0xff;
								break;
							case 2:		/* tca_mselect mux == a_other */
								am = c_other >> 24;
								break;
							case 3:		/* tca_mselect mux == a_local */
								am = c_local >> 24;
								break;
							case 4:		/* tca_mselect mux == LOD */
								am = texel >> 24;
								break;
							case 5:		/* tca_mselect mux == LOD frac */
								am = texel >> 24;
								break;
						}

						if (TEXTUREMODE0_BITS(26,1))			/* tca_reverse_blend */
							ta = (ta * (am + 1)) >> 8;
						else
							ta = (ta * ((am ^ 0xff) + 1)) >> 8;
					}

					/* add local color */
					/* how do you add c_local to the alpha???? */
					/* CalSpeed does this in its FMV */
					if (TEXTUREMODE0_BITS(27,1))				/* tca_add_clocal */
						ta += (c_local >> 24) & 0xff;
					if (TEXTUREMODE0_BITS(28,1))				/* tca_add_alocal */
						ta += (c_local >> 24) & 0xff;
					
					/* clamp */
					if (tr < 0) tr = 0;
					else if (tr > 255) tr = 255;
					if (tg < 0) tg = 0;
					else if (tg > 255) tg = 255;
					if (tb < 0) tb = 0;
					else if (tb > 255) tb = 255;
					if (ta < 0) ta = 0;
					else if (ta > 255) ta = 255;
					texel = (ta << 24) | (tr << 16) | (tg << 8) | tb;

					/* invert */
					if (TEXTUREMODE0_BITS(20,1))
						texel ^= 0x00ffffff;
					if (TEXTUREMODE0_BITS(29,1))
						texel ^= 0xff000000;
				}

				/* handle chroma key */
				if (FBZMODE_BITS(1,1) && ((texel ^ voodoo_regs[chromaKey]) & 0xffffff) == 0)
					goto skipdrawdepth;
				
				/* compute c_local */
				if (!FBZCOLORPATH_BITS(7,1))
				{
					if (!FBZCOLORPATH_BITS(4,1))			/* cc_localselect mux == iterated RGB */
						c_local = (curr & 0xff0000) | ((curg >> 8) & 0xff00) | ((curb >> 16) & 0xff);
					else									/* cc_localselect mux == color0 RGB */
						c_local = voodoo_regs[color0] & 0xffffff;
				}
				else
				{
					if (!(texel & 0x80000000))				/* cc_localselect mux == iterated RGB */
						c_local = (curr & 0xff0000) | ((curg >> 8) & 0xff00) | ((curb >> 16) & 0xff);
					else									/* cc_localselect mux == color0 RGB */
						c_local = voodoo_regs[color0] & 0xffffff;
				}
				
				/* compute a_local */
				if (FBZCOLORPATH_BITS(5,2) == 0)		/* cca_localselect mux == iterated alpha */
					c_local |= (cura << 8) & 0xff000000;
				else if (FBZCOLORPATH_BITS(5,2) == 1)	/* cca_localselect mux == color0 alpha */
					c_local |= voodoo_regs[color0] & 0xff000000;
				else if (FBZCOLORPATH_BITS(5,2) == 2)	/* cca_localselect mux == iterated Z */
					c_local |= (curz << 4) & 0xff000000;

				/* determine the RGB values */
				if (!FBZCOLORPATH_BITS(8,1))				/* cc_zero_other */
				{
					if (FBZCOLORPATH_BITS(0,2) == 0)		/* cc_rgbselect mux == iterated RGB */
					{
						r = curr >> 16;
						g = curg >> 16;
						b = curb >> 16;
					}
					else if (FBZCOLORPATH_BITS(0,2) == 1)	/* cc_rgbselect mux == texture RGB */
					{
						r = (texel >> 16) & 0xff;
						g = (texel >> 8) & 0xff;
						b = (texel >> 0) & 0xff;
					}
					else if (FBZCOLORPATH_BITS(0,2) == 2)	/* cc_rgbselect mux == color1 RGB */
					{
						r = (voodoo_regs[color1] >> 16) & 0xff;
						g = (voodoo_regs[color1] >> 8) & 0xff;
						b = (voodoo_regs[color1] >> 0) & 0xff;
					}
				}
				else
					r = g = b = 0;
				
				/* subtract local color */
				if (FBZCOLORPATH_BITS(9,1))					/* cc_sub_clocal */
				{
					r -= (c_local >> 16) & 0xff;
					g -= (c_local >> 8) & 0xff;
					b -= (c_local >> 0) & 0xff;
				}
				
				/* scale RGB */
				if (FBZCOLORPATH_BITS(10,3) != 0)			/* cc_mselect mux */
				{
					INT32 rm, gm, bm;
					
					switch (FBZCOLORPATH_BITS(10,3))
					{
						case 1:		/* cc_mselect mux == c_local */
							rm = (c_local >> 16) & 0xff;
							gm = (c_local >> 8) & 0xff;
							bm = (c_local >> 0) & 0xff;
							break;

						case 2:		/* cc_mselect mux == a_other */
							if (FBZCOLORPATH_BITS(2,2) == 0)			/* cca_localselect mux == iterated alpha */
								rm = gm = bm = cura >> 16;
							else if (FBZCOLORPATH_BITS(2,2) == 1)		/* cca_localselect mux == texture alpha */
								rm = gm = bm = (texel >> 24) & 0xff;
							else if (FBZCOLORPATH_BITS(2,2) == 2)		/* cca_localselect mux == color1 alpha */
								rm = gm = bm = (voodoo_regs[color1] >> 24) & 0xff;
							else
								rm = gm = bm = 0;
							break;
						
						case 3:		/* cc_mselect mux == a_local */
							rm = gm = bm = c_local >> 24;
							break;
						
						case 4:		/* cc_mselect mux == texture alpha */
							rm = gm = bm = texel >> 24;
							break;
						
						default:
							rm = gm = bm = 0;
							break;
					}

					if (FBZCOLORPATH_BITS(13,1))			/* cc_reverse_blend */
					{
						r = (r * (rm + 1)) >> 8;
						g = (g * (gm + 1)) >> 8;
						b = (b * (bm + 1)) >> 8;
					}
					else
					{
						r = (r * ((rm ^ 0xff) + 1)) >> 8;
						g = (g * ((gm ^ 0xff) + 1)) >> 8;
						b = (b * ((bm ^ 0xff) + 1)) >> 8;
					}
				}

				/* add local color */
				if (FBZCOLORPATH_BITS(14,1))				/* cc_add_clocal */
				{
					r += (c_local >> 16) & 0xff;
					g += (c_local >> 8) & 0xff;
					b += (c_local >> 0) & 0xff;
				}

				/* add local alpha */
				else if (FBZCOLORPATH_BITS(15,1))			/* cc_add_alocal */
				{
					r += c_local >> 24;
					g += c_local >> 24;
					b += c_local >> 24;
				}
				
				/* determine the A value */
				if (!FBZCOLORPATH_BITS(17,1))				/* cca_zero_other */
				{
					if (FBZCOLORPATH_BITS(2,2) == 0)			/* cca_localselect mux == iterated alpha */
						a = cura >> 16;
					else if (FBZCOLORPATH_BITS(2,2) == 1)		/* cca_localselect mux == texture alpha */
						a = (texel >> 24) & 0xff;
					else if (FBZCOLORPATH_BITS(2,2) == 2)		/* cca_localselect mux == color1 alpha */
						a = (voodoo_regs[color1] >> 24) & 0xff;
				}
				else
					a = 0;
				
				/* subtract local alpha */
				if (FBZCOLORPATH_BITS(18,1))				/* cca_sub_clocal */
					a -= c_local >> 24;
				
				/* scale alpha */
				if (FBZCOLORPATH_BITS(19,3) != 0)			/* cca_mselect mux */
				{
					INT32 am;
					
					switch (FBZCOLORPATH_BITS(19,3))
					{
						case 1:		/* cca_mselect mux == a_local */
						case 3:		/* cca_mselect mux == a_local */
							am = c_local >> 24;
							break;

						case 2:		/* cca_mselect mux == a_other */
							if (FBZCOLORPATH_BITS(2,2) == 0)			/* cca_localselect mux == iterated alpha */
								am = cura >> 16;
							else if (FBZCOLORPATH_BITS(2,2) == 1)		/* cca_localselect mux == texture alpha */
								am = (texel >> 24) & 0xff;
							else if (FBZCOLORPATH_BITS(2,2) == 2)		/* cca_localselect mux == color1 alpha */
								am = (voodoo_regs[color1] >> 24) & 0xff;
							else
								am = 0;
							break;
						
						case 4:		/* cca_mselect mux == texture alpha */
							am = texel >> 24;
							break;
						
						default:
							am = 0;
							break;
					}
					
					if (FBZCOLORPATH_BITS(22,1))			/* cca_reverse_blend */
						a = (a * (am + 1)) >> 8;
					else
						a = (a * ((am ^ 0xff) + 1)) >> 8;
				}
				
				/* add local alpha */
				if (FBZCOLORPATH_BITS(23,2))				/* cca_add_clocal */
					a += c_local >> 24;

				/* handle alpha masking */
				if (FBZMODE_BITS(13,1) && (a & 1) == 0)
					goto skipdrawdepth;

				/* apply alpha function */
				if (ALPHAMODE_BITS(0,4))
				{
					switch (ALPHAMODE_BITS(1,3))
					{
						case 1:
							if (a >= ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
						case 2:
							if (a != ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
						case 3:
							if (a > ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
						case 4:
							if (a <= ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
						case 5:
							if (a == ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
						case 6:
							if (a < ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
					}
				}
				
				/* invert */
				if (FBZCOLORPATH_BITS(16,1))				/* cc_invert_output */
				{
					r ^= 0xff;
					g ^= 0xff;
					b ^= 0xff;
				}
				if (FBZCOLORPATH_BITS(25,1))				/* cca_invert_output */
					a ^= 0xff;
					
				/* fog */
#if (FOGGING)
				if (FOGMODE_BITS(0,1))
				{
					if (FOGMODE_BITS(5,1))					/* fogconstant */
					{
						r += (voodoo_regs[fogColor] >> 16) & 0xff;
						g += (voodoo_regs[fogColor] >> 8) & 0xff;
						b += (voodoo_regs[fogColor] >> 0) & 0xff;
					}
					else
					{
						INT32 fogalpha;
					
						if (FOGMODE_BITS(4,1))				/* fogz */
							fogalpha = (curz >> 20) & 0xff;
						else if (FOGMODE_BITS(3,1))			/* fogalpha */
							fogalpha = (cura >> 16) & 0xff;
						else
						{
							INT32 wval = float_to_depth(curw);
							fogalpha = fog_blend[wval >> 10];
							fogalpha += (fog_delta[wval >> 10] * ((wval >> 2) & 0xff)) >> 10;
						}
						
						if (!FOGMODE_BITS(2,1))				/* fogmult */
						{
							if (fogalpha)
							{
								r = (r * (0x100 - fogalpha)) >> 8;
								g = (g * (0x100 - fogalpha)) >> 8;
								b = (b * (0x100 - fogalpha)) >> 8;
							}
						}
						else
							r = g = b = 0;
						
						if (!FOGMODE_BITS(1,1) && fogalpha)	/* fogadd */
						{
							r += (((voodoo_regs[fogColor] >> 16) & 0xff) * fogalpha) >> 8;
							g += (((voodoo_regs[fogColor] >> 8) & 0xff) * fogalpha) >> 8;
							b += (((voodoo_regs[fogColor] >> 0) & 0xff) * fogalpha) >> 8;
						}
					}
				}
#endif
				/* apply alpha blending */
#if (ALPHA_BLENDING)
				if (ALPHAMODE_BITS(4,1))
				{
					/* quick out for standard alpha blend with opaque pixel */
					if (ALPHAMODE_BITS(8,8) != 0x51 || a < 0xff)
					{
						int dpix = dest[x];
						int dr = (dpix >> 8) & 0xf8;
						int dg = (dpix >> 3) & 0xfc;
						int db = (dpix << 3) & 0xf8;
/* fix me -- we don't support alpha layer on the dest*/
						int da = 0xff;
						int sr = r;
						int sg = g;
						int sb = b;
						int sa = a;
						
/* fix me -- add dither subtraction here*/
/*						if (FBZMODE_BITS(19,1))*/
						
						/* compute source portion */
						switch (ALPHAMODE_BITS(8,4))
						{
							case 0:		/* AZERO*/
								r = g = b = 0;
								break;
							case 1:		/* ASRC_ALPHA*/
								r = (sr * (sa + 1)) >> 8;
								g = (sg * (sa + 1)) >> 8;
								b = (sb * (sa + 1)) >> 8;
								break;
							case 2:		/* A_COLOR*/
								r = (sr * (dr + 1)) >> 8;
								g = (sg * (dg + 1)) >> 8;
								b = (sb * (db + 1)) >> 8;
								break;
							case 3:		/* ADST_ALPHA*/
								r = (sr * (da + 1)) >> 8;
								g = (sg * (da + 1)) >> 8;
								b = (sb * (da + 1)) >> 8;
								break;
							case 4:		/* AONE*/
								break;
							case 5:		/* AOMSRC_ALPHA*/
								r = (sr * (0x100 - sa)) >> 8;
								g = (sg * (0x100 - sa)) >> 8;
								b = (sb * (0x100 - sa)) >> 8;
								break;
							case 6:		/* AOM_COLOR*/
								r = (sr * (0x100 - dr)) >> 8;
								g = (sg * (0x100 - dg)) >> 8;
								b = (sb * (0x100 - db)) >> 8;
								break;
							case 7:		/* AOMDST_ALPHA*/
								r = (sr * (0x100 - da)) >> 8;
								g = (sg * (0x100 - da)) >> 8;
								b = (sb * (0x100 - da)) >> 8;
								break;
						}
						
						/* add in dest portion */
						switch (ALPHAMODE_BITS(12,4))
						{
							case 0:		/* AZERO*/
								break;
							case 1:		/* ASRC_ALPHA*/
								r += (dr * (sa + 1)) >> 8;
								g += (dg * (sa + 1)) >> 8;
								b += (db * (sa + 1)) >> 8;
								break;
							case 2:		/* A_COLOR*/
								r += (dr * (sr + 1)) >> 8;
								g += (dg * (sg + 1)) >> 8;
								b += (db * (sb + 1)) >> 8;
								break;
							case 3:		/* ADST_ALPHA*/
								r += (dr * (da + 1)) >> 8;
								g += (dg * (da + 1)) >> 8;
								b += (db * (da + 1)) >> 8;
								break;
							case 4:		/* AONE*/
								r += dr;
								g += dg;
								b += db;
								break;
							case 5:		/* AOMSRC_ALPHA*/
								r += (dr * (0x100 - sa)) >> 8;
								g += (dg * (0x100 - sa)) >> 8;
								b += (db * (0x100 - sa)) >> 8;
								break;
							case 6:		/* AOM_COLOR*/
								r += (dr * (0x100 - sr)) >> 8;
								g += (dg * (0x100 - sg)) >> 8;
								b += (db * (0x100 - sb)) >> 8;
								break;
							case 7:		/* AOMDST_ALPHA*/
								r += (dr * (0x100 - da)) >> 8;
								g += (dg * (0x100 - da)) >> 8;
								b += (db * (0x100 - da)) >> 8;
								break;
						}
					}
				}
#endif				
				/* clamp */
				if (r < 0) r = 0;
				else if (r > 255) r = 255;
				if (g < 0) g = 0;
				else if (g > 255) g = 255;
				if (b < 0) b = 0;
				else if (b > 255) b = 255;
				if (a < 0) a = 0;
				else if (a > 255) a = 255;
				
				/* apply dithering */
				if (FBZMODE_BITS(8,1))
				{
#if 1
					UINT8 *dith;
					if (!FBZMODE_BITS(11,1))
						dith = &dither_lookup[256 * dither_matrix_4x4[((y & 3) << 2) | (x & 3)]];
					else
						dith = &dither_lookup[256 * dither_matrix_2x2[((y & 3) << 2) | (x & 3)]];
					r = dith[r];
					g = dith[g];
					b = dith[b];
					a = dith[a];
#else
					UINT8 dith;
					if (!FBZMODE_BITS(11,1))
						dith = dither_matrix_4x4[((y & 3) << 2) | (x & 3)];
					else
						dith = dither_matrix_2x2[((y & 3) << 2) | (x & 3)];
					r += dith;
					g += dith;
					b += dith;
					a += dith;
#endif
				}

				/* stuff */
				if (FBZMODE_BITS(9,1))
					dest[x] = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >> 3);
				if (FBZMODE_BITS(10,1))
					depth[x] = depthval;
				
		skipdrawdepth:
				/* advance */
				curr += tri_drdx;
				curg += tri_dgdx;
				curb += tri_dbdx;
				cura += tri_dadx;
				curz += tri_dzdx;
				curw += tri_dwdx;
				curs0 += tri_ds0dx;
				curt0 += tri_dt0dx;
				curw0 += tri_dw0dx;
#if (NUM_TMUS > 1)
				curs1 += tri_ds1dx;
				curt1 += tri_dt1dx;
				curw1 += tri_dw1dx;
#endif
			}
		}
	}

	voodoo_regs[fbiPixelsIn] &= 0xffffff;

#if (TRACK_LOD)
	if (loglod) log_cb(RETRO_LOG_DEBUG, LOGPRE "LOD: bins=%d %d %d %d %d %d %d %d %d\n", lodbin[0], lodbin[1], lodbin[2], lodbin[3], lodbin[4], lodbin[5], lodbin[6], lodbin[7], lodbin[8]);
#endif
}

/*

logparams: path=00000002 fog=00000000 alpha=00000000 mode=00000300
	RGB = color1
	--
	dither
	RGB write enable

logparams: path=08000001 fog=00000000 alpha=00000000 mode=00000300
	RGB = TREX
	texture mapped
	--
	dither
	RGB write enable

logparams: path=08000001 fog=00000000 alpha=00000000 mode=00000200
	RGB = TREX
	texture mapped
	--
	RGB write enable

logparams: path=0C000035 fog=00000000 alpha=00045119 mode=000B4779
	RGB = TREX
	alpha = TREX
	cc_localselect = color0
	cca_localselect = color0
	subpixel adjust
	texture mapped
	--
	enable alpha func
	alpha func = greater than
	enable alpha blend
	source RGB alpha factor = ASRC_ALPHA
	dest RGB alpha factor = 1 - ASRC_ALPHA
	source alpha alpha factor = 1
	dest alpha alpha factor = 0
	reference value = 0
	--
	enable clipping
	use W buffer
	depth buffer func = less than or equal
	dither
	RGB write enable
	depth/alpha write enable
	
	

logparams: path=00000002 fog=00000000 alpha=00000000 mode=00000300
logparams: path=08000001 fog=00000000 alpha=00000000 mode=00000300
logparams: path=08000001 fog=00000000 alpha=00000000 mode=00000200
logparams: path=0C000035 fog=00000000 alpha=00045119 mode=000B4779
logparams: path=0C480035 fog=00000000 alpha=00045119 mode=000B4779
logparams: path=0C002435 fog=00000000 alpha=00045119 mode=000B4779
logparams: path=0C000035 fog=00000000 alpha=00045119 mode=000B4379
logparams: path=0C482435 fog=00000000 alpha=00045119 mode=000B4379






*/


#if 0


up front:
* pre-scale curs,curt,ds,dt by 256 so that when we truncate to int we still have some bits
* create clampstmask:
    if (!clamptmu0.s) -> 0xffffffff 0xffffffff 0xffffffff 0x000000ff 
    if (!clamptmu0.t) -> 0xffffffff 0xffffffff 0x000000ff 0xffffffff 
    if (!clamptmu1.s) -> 0xffffffff 0x000000ff 0xffffffff 0xffffffff 
    if (!clamptmu1.t) -> 0x000000ff 0xffffffff 0xffffffff 0xffffffff
* create invertmask[0,1] to XOR texel data with


	regs:
		xmm4 = curw(f32)
		xmm5 = curw0(f32), curw0(f32), curw1(f32), curw1(f32)
		xmm6 = curs0(f32), curt0(f32), curs1(f32), curt1(f32)
		xmm7 = cura(i32), curr(i32), curg(i32), curb(i32)
		
		edx = curz(i32)
		esi = depth
		edi = draw
		ebp = xcoord
		curw(f32)


static UINT32 lodsave;
static UINT32 dummyscale[2] = { 0x00800080, 0x00800080 };
static UINT32 _01_01_01_01[4] = { 0x00010x0001, 0x00010x0001, 0x00010x0001, 0x00010x0001 };
static UINT32 _ff_ff_ff_ff[4] = { 0x00ff0x00ff, 0x00ff0x00ff, 0x00ff0x00ff, 0x00ff0x00ff };


void generate_load_texel(struct drccore *drc, UINT8 tmu)
{
	UINT32 curTextureMode = voodoo_regs[0x100 + tmu*0x100 + textureMode];
	#define TEXTUREMODE_BITS(a,b)	(((curTextureMode >> (a)) & ((1 << (b)) - 1)))
	
	/* we need EDX during this process */
	_push_r32(REG_EDX);

	/* compute LOD for this texel (EDX=lod) */
	_mov_r32_m32abs(REG_EDX, &trex_lodmin[tmu]);							/* mov		edx,[trex_lodmin]*/
	
	/* compute texture base from LOD (EAX=texbase) */
	_mov_r32_m32abs(REG_EAX, &voodoo_regs[0x100 + tmu*0x100 + texBaseAddr]);/* mov		eax,[texBaseAddr]*/
	_mov_r32_r32(REG_EBX, REG_EDX);											/* mov		ebx,edx*/
	_shr_r32_imm(REG_EBX, 2);												/* shr		ebx,2*/
	if (!((TEXTUREMODE_BITS(11,1))
		_add_r32_m32isd(REG_EAX, REG_EBX, 4, &trex_lod_offset[tmu][0]);		/* add		eax,[trex_lod_offset + ebx*4]*/
	else
	{
		_mov_r32_m32isd(REG_ECX, REG_ECX, 4, &trex_lod_offset[tmu][0]);		/* mov		ecx,[trex_lod_offset + ebx*4]*/
		_lea_r32_m32bisd(REG_EAX, REG_EAX, REG_ECX, 2, 0);					/* lea		eax,[eax+ecx*2]*/
	}
	
	/* compute LOD shift (ECX=lodshift) */
	_mov_r32_m32bd(REG_ECX, REG_EBX, &trex_lod_width_shift[tmu][0]);		/* mov		ecx,[trex_lod_width_shift + ebx]*/
	_ror_r32_imm(REG_ECX, 16);												/* ror		ecx,16*/
	_or_r32_r32(REG_ECX, REG_EBX);											/* or		ecx,ebx*/
	
	/* choose filter based on magnification/minification if different */
	_mov_m32abs_r32(&lodsave, REG_EDX);										/* mov		[lodsave],edx*/
/*	if (TEXTUREMODE_BITS(2,1) != TEXTUREMODE_BITS(1,1))*/
/*	{*/
/*		_cmp_r32_m32abs(REG_EDX, &trex_lodmin[tmu]);						// cmp		edx,[trex_lodmin]*/
/*		if (TEXTUREMODE_BITS(1,1))*/
/*			_jcc_near_link(COND_E, &link_dobilinear);						// je		dobilinear*/
/*		else*/
/*			_jcc_near_link(COND_E, &link_dobilinear);						// jne		dobilinear*/
/*	}*/

	/* ------------- load texel in c_local (XMM2) --------------- */

	/* point-sampled filter if it's a possibility */
/*	if (TEXTUREMODE_BITS(1,2) != 3)*/
	{
		/* shift off the fractional bits */
		_movapd_r128_r128(REG_XMM0, REG_XMM2);								/* movapd	xmm0,xmm2*/
		_psrad_r128_imm(REG_XMM0, 8);										/* psrad	xmm0,8*/
		
		/* pre-mask any coordinates that will be wrapping */
		_pand_r128_m32abs(REG_XMM0, &clampstmask);							/* pand		xmm0,clampstmask*/
		
		/* pack and clamp down into unsigned bytes */
		_packssdw_r128_r128(REG_XMM0, REG_XMM0);							/* packssdw	xmm0,xmm0*/
		_packuswb_r128_r128(REG_XMM0, REG_XMM0);							/* packuswb	xmm0,xmm0*/
		
		/* load S,T into DL,DH */
		_movd_r32_r128(REG_EDX, REG_XMM0);									/* movd		edx,xmm0*/
		if (tmu == 1)
			_shr_r32_imm(REG_EDX, 16);										/* shr		edx,16*/
		
		/* shift S,T right by (lod >> 2) and shift T left by lodshift */
		_movzx_r32_r8(REG_EBX, REG_DH);										/* movzx	ebx,dh		;ebx=T*/
		_movzx_r32_r8(REG_EDX, REG_DL);										/* movzx	edx,dl		;edx=S*/
		_shr_r32_cl(REG_EBX);												/* shr		ebx,cl*/
		_shr_r32_cl(REG_EDX);												/* shr		edx,cl*/
		_ror_r32_imm(REG_ECX);												/* ror		ecx,16*/
		_shl_r32_cl(REG_EBX);												/* shl		ebx,cl*/
		
		/* assemble them together and fetch the base of the final lookup table */
		_add_r32_r32(REG_EBX, REG_EDX);										/* add		ebx,edx*/
		_mov_r32_m32abs(REG_EDX, tmu ? &lookup1 : &lookup0);				/* mov		edx,[lookup]*/
		
		/* fetch raw texel data */
		if (!TEXTUREMODE0_BITS(11,1))
			_movzx_r32_m8bisd(REG_EAX, REG_EAX, REG_EBX, 1, 0);				/* movzx	eax,byte [eax+ebx]*/
		else
			_movzx_r32_m16bisd(REG_EAX, REG_EAX, REG_EBX, 2, 0);			/* movzx	eax,byte [eax+ebx*2]*/
		
		/* look up into ARGB values */
		_movss_r128_m32bisd(REG_XMM2, REG_EDX, REG_EAX, 4, 0);				/* movss	xmm2,[edx+eax*4]*/
		
		/* unpack into words */
		_pxor_r128_r128(REG_XMM1, REG_XMM1);								/* pxor		xmm1,xmm1*/
		_punpcklbw_r128_r128(REG_XMM2, REG_XMM1);							/* punpcklbw xmm2,xmm1*/
	}
	
	/* zero/other selection */
	if (!TEXTUREMODE1_BITS(12,1))				/* tc_zero_other */
		_movapd_r128_r128(REG_XMM0, REG_XMM3);								/* movapd	xmm0,xmm3*/
	else
		_pxor_r128_r128(REG_XMM0, REG_XMM0);								/* pxor		xmm0,xmm0*/

	/* subtract local color */
	if (TEXTUREMODE1_BITS(13,1))				/* tc_sub_clocal */
		_psubw_r128_r128(REG_XMM0, REG_XMM2);								/* psubw	xmm0,xmm2*/

	/* scale RGB */
	if (TEXTUREMODE1_BITS(14,3) != 0)			/* tc_mselect mux */
	{
		if (TEXTUREMODE1_BITS(14,3) == 1)		/* tc_mselect mux == c_local */
			_movapd_r128_r128(REG_XMM1, REG_XMM2);							/* movapd	xmm1,xmm2*/
		if (TEXTUREMODE1_BITS(14,3) == 2)		/* tc_mselect mux == a_other */
			_pshuflw(REG_XMM1, REG_XMM3, 0xff);								/* pshuflw	xmm1,xmm3,0xff*/
		if (TEXTUREMODE1_BITS(14,3) == 3)		/* tc_mselect mux == a_local */
			_pshuflw(REG_XMM1, REG_XMM2, 0xff);								/* pshuflw	xmm1,xmm2,0xff*/
		if (TEXTUREMODE1_BITS(14,3) == 4)		/* tc_mselect mux == LOD */
			_movsd_r128_m64abs(REG_XMM1, &dummyscale);						/* movsd	xmm1,[dummyscale]*/
		if (TEXTUREMODE1_BITS(14,3) == 5)		/* tc_mselect mux == LOD frac */
			_movsd_r128_m64abs(REG_XMM1, &dummyscale);						/* movsd	xmm1,[dummyscale]*/
			
		if (!TEXTUREMODE1_BITS(17,1))			/* tc_reverse_blend */
			_pxor_r128_r128(REG_XMM1, &_ff_ff_ff_ff);						/* pxor		xmm1,[_ff_ff_ff_ff]*/
		_padd_r128_r128(REG_XMM1, &one_one_one_one);						/* padd		xmm1,[_01_01_01_01]*/
		_psraw_r128_imm(REG_XMM1, 2);										/* psraw	xmm1,2*/
		_pmullw_r128_r128(REG_XMM0, REG_XMM1);								/* pmullw	xmm0,xmm1*/
		_psraw_r128_imm(REG_XMM0, 6);										/* psraw	xmm0,6*/
	}

	/* add local color */
	if (TEXTUREMODE1_BITS(13,1))				/* tc_add_clocal */
		_paddw_r128_r128(REG_XMM0, REG_XMM2);								/* paddw	xmm0,xmm2*/

	/* add local alpha */
	else if (TEXTUREMODE1_BITS(19,1))			/* tc_add_alocal */
	{
		_pshuflw(REG_XMM1, REG_XMM2, 0xff);									/* pshuflw	xmm1,xmm2,0xff*/
		_paddw_r128_r128(REG_XMM0, REG_XMM1);								/* paddw	xmm0,xmm1*/
	}
	
	/* zero/other selection */
	if (!TEXTUREMODE1_BITS(21,1))				/* tca_zero_other */
		_pextrw_r32_r128(REG_EAX, REG_XMM3, 3);								/* pextrw	eax,xmm3,3*/
	else
		_xor_r32_r32(REG_EAX, REG_EAX);										/* xor		eax,eax*/
	
	/* subtract local alpha */
	if (TEXTUREMODE1_BITS(22,1))				/* tca_sub_clocal */
	{
		_pextrw_r32_r128(REG_EBX, REG_XMM2, 3);								/* pextrw	eax,xmm2,3*/
		_sub_r32_r32(REG_EAX, REG_EBX);										/* sub		eax,ebx*/
	}

	/* scale alpha */
	if (TEXTUREMODE1_BITS(23,3) != 0)			/* tca_mselect mux */
	{
		INT32 am = 0;
		
		if (TEXTUREMODE1_BITS(23,3) == 1)		/* tca_mselect mux == c_local */
			_pextrw_r32_r128(REG_EBX, REG_XMM2, 3);							/* pextrw	eax,xmm2,3*/
		if (TEXTUREMODE1_BITS(23,3) == 2)		/* tca_mselect mux == a_other */
			_pextrw_r32_r128(REG_EBX, REG_XMM3, 3);							/* pextrw	eax,xmm3,3*/
		if (TEXTUREMODE1_BITS(23,3) == 3)		/* tca_mselect mux == a_local */
			_pextrw_r32_r128(REG_EBX, REG_XMM2, 3);							/* pextrw	eax,xmm2,3*/
		if (TEXTUREMODE1_BITS(23,3) == 4)		/* tca_mselect mux == LOD */
			_mov_r32_imm(REG_EBX, 0x80);
		if (TEXTUREMODE1_BITS(23,3) == 5)		/* tca_mselect mux == LOD frac */
			_mov_r32_imm(REG_EBX, 0x80);

		if (!TEXTUREMODE1_BITS(26,1))			/* tca_reverse_blend */
			_xor_r32_imm(REG_EBX, 0xff);									/* xor		ebx,0xff*/
		_add_r32_imm(REG_EBX, 1);											/* add		ebx,1*/
		_imul_r32_r32(REG_EAX, REG_EBX);									/* imul		eax,ebx*/
		_sar_r32_imm(REG_EAX, 8);											/* sar		eax,8*/
	}

	/* add local color */
	if (TEXTUREMODE1_BITS(27,1) ||
		TEXTUREMODE1_BITS(28,1))				/* tca_add_clocal/tca_add_alocal */
	{
		_pextrw_r32_r128(REG_EBX, REG_XMM2, 3);								/* pextrw	eax,xmm2,3*/
		_sub_r32_r32(REG_EAX, REG_EBX);										/* sub		eax,ebx*/
	}
	
	/* insert */
	_pinsrw_r128_r32(REG_XMM0, REG_EAX, 3);									/* pinsrw	xmm0,eax,3*/
	
	/* clamp down to unsigned bytes and then unclamp back into words in XMM3 */
	/* note that chroma key code relies on us leaving XMM0 with the packed version */
	_packuswb_r128_r128(REG_XMM0, REG_XMM0);								/* packuswb	xmm0,xmm0*/
	_movapd_r128_r128(REG_XMM3, REG_XMM0);									/* movapd	xmm3,xmm0*/
	_pxor_r128_r128(REG_XMM1, REG_XMM1);									/* pxor		xmm1,xmm1*/
	_punpcklbw_r128_r128(REG_XMM3, REG_XMM1);								/* punpcklbw xmm3,xmm1*/
	
	/* invert as necessary */
	if (TEXTUREMODE1_BITS(20,1) || TEXTUREMODE1_BITS(29,1))
		_pxor_r128_m128abs(REG_XMM3, &invertmask[tmu]);						/* pxor		xmm3,[invertmask]*/

	/* restore EDX */
	_pop_r32(REG_EDX);
}




	for (x = startx; x < stopx; x++)
	{
		/* rotate stipple pattern */
		if (!FBZMODE_BITS(12,1))
			_rol_m32abs_imm(&voodoo_regs[stipple], 1);
				
		/* handle stippling */
		if (FBZMODE_BITS(2,1))
		{
			/* rotate mode */
			if (!FBZMODE_BITS(12,1))
			{
				_jcc_near_link(COND_NC, &link_stippleskip);
			}
			
			/* pattern mode */
			else
			{
				_mov_r32_r32(REG_EAX, REG_EBP);
				_mov_r32_m32abs(REG_EBX, &ycoord);
				_not_r32(REG_EAX);
				_and_r32_imm(REG_EBX, 3);
				_and_r32_imm(REG_EAX, 7);
				_bt_m32bd_r32(REG_EBX, &voodoo_regs[stipple], REG_EAX);
				_jcc_near_link(COND_NC, &link_stippleskip);
			}
		}

		/* handle depth buffer testing */
		if (FBZMODE_BITS(4,1))
		{
			if (FBZMODE_BITS(5,3) == 0)
				_jmp_near_link(&link_depthskip);							/* jmp		depthskip*/
			
			if (!FBZMODE_BITS(20,1))
			{
				if (!FBZMODE_BITS(3,1))
				{
					_mov_r32_r32(REG_EAX, REG_CURZ);						/* mov		eax,curz*/
					_shr_r32_imm(REG_EAX, 12);								/* shr		eax,12*/
				}
				else
				{
					_rcpss_r128_r128(REG_XMM0, REG_XMM4);					/* rcpss	xmm0,xmm4*/
					_movd_r32_r128(REG_EAX, REG_XMM0);						/* movd		eax,xmm0*/
					_cvtss2si_r32_r128(REG_EBX, REG_XMM0);					/* cvtss2si	ebx,xmm0*/
					_sub_r32_imm(REG_EAX, 127 << 23);						/* sub		eax,127 << 23*/
					_sub_r32_imm(REG_EBX, 1);								/* sub		ebx,1*/
					_shr_r32_imm(REG_EAX, 11);								/* shr		eax,11*/
					_cmp_r32_imm(REG_EBX, 65535);							/* cmp		ebx,65535*/
					_jcc_short_link(COND_B, &link_temp1);					/* jb		skip*/
					_mov_r32_imm(REG_EAX, 0);								/* mov		eax,0*/
					_jcc_short_link(COND_L, &link_temp2);					/* jl		skip*/
					_mov_r32_imm(REG_EAX, 0xffff);							/* mov		eax,0xffff*/
					_resolve_link(&link_temp1);								/* skip:*/
					_resolve_link(&link_temp2);
				}
			}
			else
				_mov_r16_m16abs(REG_AX, &voodoo_regs[zaColor]);				/* mov		ax,[zaColor]*/

			_mov_m16abs_r16(&depthval, REG_AX);								/* mov		[depthval],ax*/
			if (FBZMODE_BITS(5,3) != 7)
			{
				if (FBZMODE_BITS(16,1))
					_add_r16_m16abs(REG_AX, &voodoo_regs[zaColor]);			/* add		ax,[zaColor]*/
				_cmp_r16_m16bisd(REG_AX, REG_ESI, REG_EBP, 2, 0);			/* cmp		ax,[esi+ebp*2]*/
				switch (FBZMODE_BITS(5,3))
				{
					case 1:
						_jmp_near_link(COND_AE, &link_depthskip);			/* jae		depthskip*/
						break;
					case 2:
						_jmp_near_link(COND_NE, &link_depthskip);			/* jne		depthskip*/
						break;
					case 3:
						_jmp_near_link(COND_A, &link_depthskip);			/* ja		depthskip*/
						break;
					case 4:
						_jmp_near_link(COND_BE, &link_depthskip);			/* jbe		depthskip*/
						break;
					case 5:
						_jmp_near_link(COND_E, &link_depthskip);			/* je		depthskip*/
						break;
					case 6:
						_jmp_near_link(COND_B, &link_depthskip);			/* jb		depthskip*/
						break;
				}
			}
		}
		
		/* load the texel if necessary */
		if (FBZCOLORPATH_BITS(0,2) == 1 || FBZCOLORPATH_BITS(2,2) == 1)
		{
			/* perspective correct both TMUs up front in XMM2 */
			_rcpps_r128_r128(REG_XMM2, REG_XMM5);							/* rcpss	xmm2,xmm5*/
			_mulps_r128_r128(REG_XMM2, REG_XMM6);							/* mulps	xmm2,xmm6*/
			_cvttps2dq_r128_r128(REG_XMM2, REG_XMM2);						/* cvttps2dq xmm2,xmm2*/

			/* start with a 0 c_other value */
			_pxor_r128_r128(REG_XMM3, REG_XMM3);							/* pxor		xmm3,xmm3*/
			
			/* if we use two TMUs, load texel from TMU1 in place of c_other */
			if (NEEDS_TEX1 && tmus > 1)
				generate_load_texel(1);										/* <load texel into xmm3>*/
			
			/* now load texel from TMU0 */
			generate_load_texel(0);											/* <load texel into xmm3>*/

			/* handle chroma key */
			if (FBZMODE_BITS(1,1))
			{
				/* note: relies on texture code leaving packed value in XMM0 */
				_movd_r32_r128(REG_EAX, REG_XMM0);							/* movd		eax,xmm0*/
				_xor_r32_m32abs(REG_EAX, &voodoo_regs[chromaKey]);			/* xor		eax,[chromaKey]*/
				_and_r32_imm(REG_EAX, 0xffffff);							/* and		eax,0xffffff*/
				_jcc_near_link(COND_Z, &link_chromaskip);					/* jz		chromaskip*/
			}
		}
				
		/* compute c_local (XMM2) */
		if (!FBZCOLORPATH_BITS(7,1))
		{
			if (!FBZCOLORPATH_BITS(4,1))			/* cc_localselect mux == iterated RGB */
			{
				_movapd_r128_r128(REG_XMM2, REG_XMM7);						/* movapd	xmm2,xmm7*/
				_psrld_r128_imm(REG_XMM2, 16);								/* psrld	xmm2,16*/
				_packssdw_r128_r128(REG_XMM2, REG_XMM2);					/* packssdw	xmm2,xmm2*/
				_pand_r128_m128abs(REG_XMM2, &_ff_ff_ff_ff);				/* pand		xmm2,[_ff_ff_ff_ff]*/
			}
			else									/* cc_localselect mux == color0 RGB */
			{
				_movss_r128_m32abs(REG_XMM2, &voodoo_regs[color0]);			/* movss	xmm2,[color0]*/
				_pxor_r128_r128(REG_XMM1, REG_XMM1);						/* pxor		xmm1,xmm1*/
				_punpcklbw_r128_r128(REG_XMM2, REG_XMM1);					/* punpcklbw xmm2,xmm1*/
			}
		}
		else
		{
			struct linkdata link_color0select, link_iteratedselect;
			_pextrw_r32_r128(REG_EAX, REG_XMM3, 3);							/* pextrw	eax,xmm3,3*/
			_and_r32_imm(REG_EAX, 0x0080);									/* and		eax,0x80*/
			_jcc_short_link(COND_NZ, &link_color0select);					/* jnz		color0select*/
			_movapd_r128_r128(REG_XMM2, REG_XMM7);							/* movapd	xmm2,xmm7*/
				_psrld_r128_imm(REG_XMM2, 16);								/* psrld	xmm2,16*/
				_packssdw_r128_r128(REG_XMM2, REG_XMM2);					/* packssdw	xmm2,xmm2*/
			_pand_r128_m128abs(REG_XMM2, &_ff_ff_ff_ff);					/* pand		xmm2,[_ff_ff_ff_ff]*/
			_jmp_short_link(&link_iteratedselect);							/* jmp		iteratedselect*/
			_resolve_link(&link_color0select);								/* color0select:*/
			_movss_r128_m32abs(REG_XMM2, &voodoo_regs[color0]);				/* movss	xmm2,[color0]*/
			_pxor_r128_r128(REG_XMM1, REG_XMM1);							/* pxor		xmm1,xmm1*/
			_punpcklbw_r128_r128(REG_XMM2, REG_XMM1);						/* punpcklbw xmm2,xmm1*/
			_resolve_link(&link_iteratedselect);							/* iteratedselect:*/
		}
		
		/* compute a_local */
		if (FBZCOLORPATH_BITS(5,2) == 0)		/* cca_localselect mux == iterated alpha */
			_pextrw_r32_r128(REG_EAX, REG_XMM7, 7);							/* pextrw	eax,xmm7,7*/
		else if (FBZCOLORPATH_BITS(5,2) == 1)	/* cca_localselect mux == color0 alpha */
			_movzx_r32_m8abs(REG_EAX, ((UINT8 *)&voodoo_regs[color0])[3]);	/* movzx	eax,[color0][3]*/
		else if (FBZCOLORPATH_BITS(5,2) == 2)	/* cca_localselect mux == iterated Z */
		{
			_mov_r32_r32(REG_EAX, REG_EDX);									/* mov		eax,edx*/
			_shr_r32_imm(REG_EAX, 20);										/* shr		eax,20*/
		}
		_pinsrw_r128_r32(REG_XMM2, REG_EAX, 3);								/* pinsrw	xmm2,eax,3*/



	pxor	mm7,mm7
	
	
	/* zero/other selection */
	if (!TEXTUREMODE1_BITS(12,1))				/* tc_zero_other */
	{
		movd	mm0,[c_other_unpacked]
	}
	else
	{
		pxor	mm0,mm0
	}
	
	/* subtract local color */
	if (TEXTUREMODE1_BITS(13,1))				/* tc_sub_clocal */
	{
		psubw	mm0,[c_local_unpacked]
	}
	
	/* scale RGB */
	if (TEXTUREMODE1_BITS(14,3) != 0)			/* tc_mselect mux */
	{
		if (TEXTUREMODE1_BITS(14,3) == 1)		/* tc_mselect mux == c_local */
		{
			movq	mm1,[c_local_unpacked]
		}
		if (TEXTUREMODE1_BITS(14,3) == 2)		/* tc_mselect mux == a_other */
		{
			movd	mm1,[c_other_unpacked + 6]
			punpcklwd mm1,mm1
			punpckldq mm1,mm1
		}
		if (TEXTUREMODE1_BITS(14,3) == 3)		/* tc_mselect mux == a_local */
		{
			movd	mm1,[c_local_unpacked + 6]
			punpcklwd mm1,mm1
			punpckldq mm1,mm1
		}
		if (TEXTUREMODE1_BITS(14,3) == 4)		/* tc_mselect mux == LOD */
		{
			movq	mm1,[_0080008000800080]
		}
		if (TEXTUREMODE1_BITS(14,3) == 5)		/* tc_mselect mux == LOD frac */
		{
			movq	mm1,[_0080008000800080]
		}

		if (TEXTUREMODE1_BITS(17,1))			/* tc_reverse_blend */
		{
			paddw	mm1,[_0001000100010001]
			pmullw	mm0,mm1
			psrlw	mm0,8
		}
		else
		{
		`	movq	mm2,[_0100010001000100]
			psubw	mm2,mm1
			pmullw	mm0,mm2
			psrlw	mm0,8
		}
	}

	/* add local color */
	if (TEXTUREMODE1_BITS(18,1))				/* tc_add_clocal */
	{
		paddw	mm0,[c_local_unpacked]
	}

	/* add local alpha */
	else if (TEXTUREMODE1_BITS(19,1))			/* tc_add_alocal */
	{
		movd	mm1,[c_local_unpacked + 6]
		punpcklwd mm1,mm1
		punpckldq mm1,mm1
		paddw	mm0,mm1
	}
	


#endif
