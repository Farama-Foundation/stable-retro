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
/* r_light.c */

#include "quakedef.h"
#include "r_local.h"

extern int coloredlights;

/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight(void)
{
   int j, k;

   /* light animations
    * 'm' is normal light, 'a' is no light, 'z' is double bright */
   int i = (int)(cl.time * 10);

   for (j = 0; j < MAX_LIGHTSTYLES; j++)
   {
      if (!cl_lightstyle[j].length)
      {
         d_lightstylevalue[j] = 256;
         continue;
      }
      k = i % cl_lightstyle[j].length;
      k = cl_lightstyle[j].map[k] - 'a';
      k = k * 22;
      d_lightstylevalue[j] = k;
   }
}


/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights (dlight_t *light, int num, mnode_t *node)  /* qbism- adapted from MH tute - increased dlights */
{
   mplane_t   *splitplane;
   float      dist;
   msurface_t   *surf;
   int         i;

start:
   if (node->contents < 0)
      return;

   splitplane = node->plane;
   dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;

   if (dist > light->radius)
   {
      node = node->children[0];
      goto start;
   }

   if (dist < -light->radius)
   {
      node = node->children[1];
      goto start;
   }

   /* mark the surfaces */
   surf = cl.worldmodel->surfaces + node->firstsurface;

   for (i = 0; i < node->numsurfaces; i++, surf++)
   {
      if (surf->dlightframe != r_framecount)
      {
         memset (surf->dlightbits, 0, sizeof (surf->dlightbits));
         surf->dlightframe = r_framecount;
      }

      surf->dlightbits[num >> 5] |= 1 << (num & 31);
   }

   R_MarkLights (light, num, node->children[0]);
   R_MarkLights (light, num, node->children[1]);
}


/*
=============
R_PushDlights
=============
*/
void R_PushDlights(mnode_t *headnode) /* qbism- from MH tute - increased dlights */
{
    int i;
    dlight_t *l = cl_dlights;

    for (i = 0; i < MAX_DLIGHTS; i++, l++)
    {
       if (l->die <= 0 || (l->radius <= 0))
          continue;

       R_MarkLights(l, i, headnode);
    }
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

static int RecursiveLightPoint(mnode_t *node, vec3_t start, vec3_t end)
{
   int r;
   float front, back, frac;
   int side;
   mplane_t *plane;
   vec3_t mid;
   msurface_t *surf;
   int s, t, ds, dt;
   int i;
   mtexinfo_t *tex;
   byte *lightmap;
   unsigned scale;
   int maps;

restart:
   if (node->contents < 0)
      return -1;		/* didn't hit anything */

   /* calculate mid point */

   plane = node->plane;
   switch (plane->type) {
      case PLANE_X:
      case PLANE_Y:
      case PLANE_Z:
         front = start[plane->type - PLANE_X] - plane->dist;
         back = end[plane->type - PLANE_X] - plane->dist;
         break;
      default:
         front = DotProduct(start, plane->normal) - plane->dist;
         back = DotProduct(end, plane->normal) - plane->dist;
         break;
   }
   side = front < 0;

   if ((back < 0) == side) {
	    /* Completely on one side - tail recursion optimization */
	    node = node->children[side];
	    goto restart;
    }

   frac = front / (front - back);
   mid[0] = start[0] + (end[0] - start[0]) * frac;
   mid[1] = start[1] + (end[1] - start[1]) * frac;
   mid[2] = start[2] + (end[2] - start[2]) * frac;

   /* go down front side */
   r = RecursiveLightPoint(node->children[side], start, mid);
   if (r >= 0)
      return r;		/* hit something */

   if ((back < 0) == side)
      return -1;		/* didn't hit anuthing */

   /* check for impact on this node */

   surf = cl.worldmodel->surfaces + node->firstsurface;
   for (i = 0; i < node->numsurfaces; i++, surf++) {
      if (surf->flags & SURF_DRAWTILED)
         continue;		/* no lightmaps */

      tex = surf->texinfo;

      s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
      t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];;

      if (s < surf->texturemins[0] || t < surf->texturemins[1])
         continue;

      ds = s - surf->texturemins[0];
      dt = t - surf->texturemins[1];

      if (ds > surf->extents[0] || dt > surf->extents[1])
         continue;

      if (!surf->samples)
         return 0;

      ds >>= 4;
      dt >>= 4;

      /* FIXME: does this account properly for dynamic lights? e.g. rocket */
      lightmap = surf->samples;
      r = 0;
      if (lightmap) {
         lightmap += dt * ((surf->extents[0] >> 4) + 1) + ds;
         for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
               maps++) {
            scale = d_lightstylevalue[surf->styles[maps]];
            r += *lightmap * scale;
            if (coloredlights)
               lightmap += ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1) * 3;
            else
               lightmap += ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);	/* colored lighting change */

         }
         r >>= 8;
      }

      return r;
   }

   /* Go down back side */
   return RecursiveLightPoint(node->children[!side], mid, end);
}


mplane_t		*lightplane;
vec3_t			lightspot;
/* LordHavoc: .lit support begin */
/* LordHavoc: original code replaced entirely */

static int RecursiveLightPointRGB(vec3_t color, mnode_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	vec3_t		mid;

loc0:
	if (node->contents < 0)
		return false;		/* didn't hit anything */
	
/* calculate mid point */
	if (node->plane->type < 3)
	{
		front = start[node->plane->type] - node->plane->dist;
		back = end[node->plane->type] - node->plane->dist;
	}
	else
	{
		front = DotProduct(start, node->plane->normal) - node->plane->dist;
		back = DotProduct(end, node->plane->normal) - node->plane->dist;
	}

	/* LordHavoc: optimized recursion */
	if ((back < 0) == (front < 0))
/* 		return RecursiveLightPointRGB (color, node->children[front < 0], start, end); */
	{
		node = node->children[front < 0];
		goto loc0;
	}
	
	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	
/* go down front side */
	if (RecursiveLightPointRGB (color, node->children[front < 0], start, mid))
		return true;	/* hit something */
	else
	{
		int i, ds, dt;
		msurface_t *surf;
	/* check for impact on this node */
		VectorCopy (mid, lightspot);
		lightplane = node->plane;

		surf = cl.worldmodel->surfaces + node->firstsurface;
		for (i = 0;i < node->numsurfaces;i++, surf++)
		{
			if (surf->flags & SURF_DRAWTILED)
				continue;	/* no lightmaps */

			ds = (int) ((float) DotProduct (mid, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3]);
			dt = (int) ((float) DotProduct (mid, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3]);

			if (ds < surf->texturemins[0] || dt < surf->texturemins[1])
				continue;
			
			ds -= surf->texturemins[0];
			dt -= surf->texturemins[1];
			
			if (ds > surf->extents[0] || dt > surf->extents[1])
				continue;

			if (surf->samples)
			{
				/* LordHavoc: enhanced to interpolate lighting */
				byte *lightmap;
				int maps, line3, dsfrac = ds & 15, dtfrac = dt & 15, r00 = 0, g00 = 0, b00 = 0, r01 = 0, g01 = 0, b01 = 0, r10 = 0, g10 = 0, b10 = 0, r11 = 0, g11 = 0, b11 = 0;
				float scale;
				line3 = ((surf->extents[0]>>4)+1)*3;

				lightmap = surf->samples + ((dt>>4) * ((surf->extents[0]>>4)+1) + (ds>>4))*3; /* LordHavoc: *3 for color */

				for (maps = 0;maps < MAXLIGHTMAPS && surf->styles[maps] != 255;maps++)
				{
					scale = (float) d_lightstylevalue[surf->styles[maps]] * 1.0 / 256.0;
					r00 += (float) lightmap[      0] * scale;g00 += (float) lightmap[      1] * scale;b00 += (float) lightmap[2] * scale;
					r01 += (float) lightmap[      3] * scale;g01 += (float) lightmap[      4] * scale;b01 += (float) lightmap[5] * scale;
					r10 += (float) lightmap[line3+0] * scale;g10 += (float) lightmap[line3+1] * scale;b10 += (float) lightmap[line3+2] * scale;
					r11 += (float) lightmap[line3+3] * scale;g11 += (float) lightmap[line3+4] * scale;b11 += (float) lightmap[line3+5] * scale;
					lightmap += ((surf->extents[0]>>4)+1) * ((surf->extents[1]>>4)+1)*3; /* LordHavoc: *3 for colored lighting */
				}

				color[0] += (float) ((int) ((((((((r11-r10) * dsfrac) >> 4) + r10)-((((r01-r00) * dsfrac) >> 4) + r00)) * dtfrac) >> 4) + ((((r01-r00) * dsfrac) >> 4) + r00)));
				color[1] += (float) ((int) ((((((((g11-g10) * dsfrac) >> 4) + g10)-((((g01-g00) * dsfrac) >> 4) + g00)) * dtfrac) >> 4) + ((((g01-g00) * dsfrac) >> 4) + g00)));
				color[2] += (float) ((int) ((((((((b11-b10) * dsfrac) >> 4) + b10)-((((b01-b00) * dsfrac) >> 4) + b00)) * dtfrac) >> 4) + ((((b01-b00) * dsfrac) >> 4) + b00)));
			}
			return true; /* success */
		}

	/* go down back side */
		return RecursiveLightPointRGB (color, node->children[front >= 0], mid, end);
	}
}


/*
 * FIXME - check what the callers do, but I don't think this will check the
 * light value of a bmodel below the point. Models could easily be standing on
 * a func_plat or similar...
 */

vec3_t lightcolor; /* for colored lighting */
extern int coloredlights;

int R_LightPoint(vec3_t p)
{
   vec3_t end;
   int r;

   if (!cl.worldmodel->lightdata){
	 lightcolor[0] = lightcolor[1] = lightcolor[2] = 255;
     	 return 255;
	}




	if (coloredlights)
   {
      end[0] = p[0];
      end[1] = p[1];
      end[2] = p[2] - (8192 + 2); 
      lightcolor[0] = lightcolor[1] = lightcolor[2] = 0;
      r = RecursiveLightPointRGB(lightcolor, cl.worldmodel->nodes, p, end);
      return ((lightcolor[0] + lightcolor[1] + lightcolor[2]) * (1.0f / 3.0f));
   }
	else
   {
      end[0] = p[0];
      end[1] = p[1];
      end[2] = p[2] - (8192 + 2); 


      r = RecursiveLightPoint(cl.worldmodel->nodes, p, end);

      if (r == -1)
         r = 0;

      if (r < r_refdef.ambientlight)
         r = r_refdef.ambientlight;

      return r;
   }

}
