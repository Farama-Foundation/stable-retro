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
/* r_bsp.c */

#include "quakedef.h"
#include "r_local.h"
#include "console.h"

/**/
/* current entity info */
/**/
qboolean insubmodel;

/* modelorg is the viewpoint reletive to */
/* the currently rendering entity */
vec3_t modelorg, base_modelorg;

vec3_t r_entorigin;	/* the currently rendering entity in world coordinates */

static float entity_rotation[3][3];

int r_currentbkey;

typedef enum { touchessolid, drawnode, nodrawnode, ENSURE_INT_SOLID_STATE = 0x70000000 } solidstate_t;

#define MAX_BMODEL_VERTS	500	/* 6K */
#define MAX_BMODEL_EDGES	1000	/* 12K */

static mvertex_t *pbverts;
static bedge_t *pbedges;
static int numbverts, numbedges;

static mvertex_t *pfrontenter, *pfrontexit;

static qboolean makeclippededge;

/* =========================================================================== */

/*
================
R_EntityRotate
================
*/
static void R_EntityRotate(vec3_t vec)
{
   vec3_t tvec;

   VectorCopy(vec, tvec);
   vec[0] = DotProduct(entity_rotation[0], tvec);
   vec[1] = DotProduct(entity_rotation[1], tvec);
   vec[2] = DotProduct(entity_rotation[2], tvec);
}


/*
================
R_RotateBmodel
================
*/
void R_RotateBmodel(const entity_t *e)
{
   float temp1[3][3], temp2[3][3], temp3[3][3];

   /* TODO: should use a look-up table */
   /* TODO: should really be stored with the entity instead of being reconstructed */
   /* TODO: could cache lazily, stored in the entity */
   /* TODO: share work with R_SetUpAliasTransform */

   /* yaw */
   float angle = e->angles[YAW] * M_PI * 2 / 360;
   float s = sinf(angle);
   float c = cosf(angle);

   temp1[0][0] = c;
   temp1[0][1] = s;
   temp1[0][2] = 0;
   temp1[1][0] = -s;
   temp1[1][1] = c;
   temp1[1][2] = 0;
   temp1[2][0] = 0;
   temp1[2][1] = 0;
   temp1[2][2] = 1;

   /* pitch */
   angle = e->angles[PITCH];
   angle = angle * M_PI * 2 / 360;
   s = sinf(angle);
   c = cosf(angle);

   temp2[0][0] = c;
   temp2[0][1] = 0;
   temp2[0][2] = -s;
   temp2[1][0] = 0;
   temp2[1][1] = 1;
   temp2[1][2] = 0;
   temp2[2][0] = s;
   temp2[2][1] = 0;
   temp2[2][2] = c;

   R_ConcatRotations(temp2, temp1, temp3);

   /* roll */
   angle = e->angles[ROLL];
   angle = angle * M_PI * 2 / 360;
   s = sinf(angle);
   c = cosf(angle);

   temp1[0][0] = 1;
   temp1[0][1] = 0;
   temp1[0][2] = 0;
   temp1[1][0] = 0;
   temp1[1][1] = c;
   temp1[1][2] = s;
   temp1[2][0] = 0;
   temp1[2][1] = -s;
   temp1[2][2] = c;

   R_ConcatRotations(temp1, temp3, entity_rotation);

   /**/
   /* rotate modelorg and the transformation matrix */
   /**/
   R_EntityRotate(modelorg);
   R_EntityRotate(vpn);
   R_EntityRotate(vright);
   R_EntityRotate(vup);

   R_TransformFrustum();
}


/*
================
R_RecursiveClipBPoly
================
*/
static void R_RecursiveClipBPoly(const entity_t *e, bedge_t *pedges, mnode_t *pnode,
      msurface_t *psurf)
{
   bedge_t *psideedges[2], *pnextedge, *ptedge;
   int i, side, lastside;
   float dist, frac, lastdist;
   mplane_t *splitplane, tplane;
   mvertex_t *pvert, *plastvert, *ptvert;
   mnode_t *pn;

   psideedges[0] = psideedges[1] = NULL;

   makeclippededge = false;

   /* transform the BSP plane into model space */
   /* FIXME: cache these? */
   splitplane = pnode->plane;
   tplane.dist = splitplane->dist -
      DotProduct(r_entorigin, splitplane->normal);
   tplane.normal[0] = DotProduct(entity_rotation[0], splitplane->normal);
   tplane.normal[1] = DotProduct(entity_rotation[1], splitplane->normal);
   tplane.normal[2] = DotProduct(entity_rotation[2], splitplane->normal);

   /* clip edges to BSP plane */
   for (; pedges; pedges = pnextedge) {
      pnextedge = pedges->pnext;

      /* set the status for the last point as the previous point */
      /* FIXME: cache this stuff somehow? */
      plastvert = pedges->v[0];
      lastdist = DotProduct(plastvert->position, tplane.normal) -
         tplane.dist;

      if (lastdist > 0)
         lastside = 0;
      else
         lastside = 1;

      pvert = pedges->v[1];

      dist = DotProduct(pvert->position, tplane.normal) - tplane.dist;

      if (dist > 0)
         side = 0;
      else
         side = 1;

      if (side != lastside) {
         /* clipped */
         if (numbverts >= MAX_BMODEL_VERTS)
            return;

         /* generate the clipped vertex */
         frac = lastdist / (lastdist - dist);
         ptvert = &pbverts[numbverts++];
         ptvert->position[0] = plastvert->position[0] +
            frac * (pvert->position[0] - plastvert->position[0]);
         ptvert->position[1] = plastvert->position[1] +
            frac * (pvert->position[1] - plastvert->position[1]);
         ptvert->position[2] = plastvert->position[2] +
            frac * (pvert->position[2] - plastvert->position[2]);

         /* split into two edges, one on each side, and remember entering */
         /* and exiting points */
         /* FIXME: share the clip edge by having a winding direction flag? */
         if (numbedges > MAX_BMODEL_EDGES - 2) {
            Con_Printf("Out of edges for bmodel\n");
            return;
         }

         ptedge = &pbedges[numbedges];
         ptedge->pnext = psideedges[lastside];
         psideedges[lastside] = ptedge;
         ptedge->v[0] = plastvert;
         ptedge->v[1] = ptvert;

         ptedge = &pbedges[numbedges + 1];
         ptedge->pnext = psideedges[side];
         psideedges[side] = ptedge;
         ptedge->v[0] = ptvert;
         ptedge->v[1] = pvert;

         numbedges += 2;

         if (side == 0) {
            /* entering for front, exiting for back */
            pfrontenter = ptvert;
            makeclippededge = true;
         } else {
            pfrontexit = ptvert;
            makeclippededge = true;
         }
      } else {
         /* add the edge to the appropriate side */
         pedges->pnext = psideedges[side];
         psideedges[side] = pedges;
      }
   }

   /* if anything was clipped, reconstitute and add the edges along the clip */
   /* plane to both sides (but in opposite directions) */
   if (makeclippededge) {
      if (numbedges > MAX_BMODEL_EDGES - 2) {
         Con_Printf("Out of edges for bmodel\n");
         return;
      }

      ptedge = &pbedges[numbedges];
      ptedge->pnext = psideedges[0];
      psideedges[0] = ptedge;
      ptedge->v[0] = pfrontexit;
      ptedge->v[1] = pfrontenter;

      ptedge = &pbedges[numbedges + 1];
      ptedge->pnext = psideedges[1];
      psideedges[1] = ptedge;
      ptedge->v[0] = pfrontenter;
      ptedge->v[1] = pfrontexit;

      numbedges += 2;
   }

   /* draw or recurse further */
   for (i = 0; i < 2; i++) {
      if (psideedges[i]) {
         /*
          * draw if we've reached a non-solid leaf, done if all that's left
          * is a solid leaf, and continue down the tree if it's not a leaf
          */
         pn = pnode->children[i];

         /* we're done with this branch if the node or leaf isn't in the PVS */
         if (pn->visframe == r_visframecount) {
            if (pn->contents < 0) {
               if (pn->contents != CONTENTS_SOLID) {
                  r_currentbkey = ((mleaf_t *)pn)->key;
                  R_RenderBmodelFace(e, psideedges[i], psurf);
               }
            } else {
               R_RecursiveClipBPoly(e, psideedges[i], pn, psurf);
            }
         }
      }
   }
}


/*
================
R_DrawSolidClippedSubmodelPolygons
================
*/
void R_DrawSolidClippedSubmodelPolygons(const entity_t *e, model_t *pmodel)
{
   int i, j, lindex;
   mvertex_t bverts[MAX_BMODEL_VERTS];
   bedge_t bedges[MAX_BMODEL_EDGES], *pbedge;
   medge_t *pedge;

   msurface_t *psurf = &pmodel->surfaces[pmodel->firstmodelsurface];
   int numsurfaces = pmodel->nummodelsurfaces;
   medge_t *pedges = pmodel->edges;

   for (i = 0; i < numsurfaces; i++, psurf++)
   {
      if (psurf->clipflags == BMODEL_FULLY_CLIPPED)
         continue;

      /* draw the polygon */
      /* copy the edges to bedges, flipping if necessary so always */
      /* clockwise winding */

      /*
       * FIXME: if edges and vertices get caches, these assignments must
       * move outside the loop, and overflow checking must be done here
       */
      pbverts = bverts;
      pbedges = bedges;
      numbverts = numbedges = 0;

      pbedge = &bedges[numbedges];
      numbedges += psurf->numedges;

      for (j = 0; j < psurf->numedges; j++) {
         lindex = pmodel->surfedges[psurf->firstedge + j];

         if (lindex > 0) {
            pedge = &pedges[lindex];
            pbedge[j].v[0] = &r_pcurrentvertbase[pedge->v[0]];
            pbedge[j].v[1] = &r_pcurrentvertbase[pedge->v[1]];
         } else {
            lindex = -lindex;
            pedge = &pedges[lindex];
            pbedge[j].v[0] = &r_pcurrentvertbase[pedge->v[1]];
            pbedge[j].v[1] = &r_pcurrentvertbase[pedge->v[0]];
         }
         pbedge[j].pnext = &pbedge[j + 1];
      }
      pbedge[j - 1].pnext = NULL;	/* mark end of edges */

      R_RecursiveClipBPoly(e, pbedge, e->topnode, psurf);
   }
}


/*
================
R_DrawSubmodelPolygons
================
*/
void R_DrawSubmodelPolygons(const entity_t *e, model_t *pmodel, int clipflags)
{
   int i;

   /* FIXME: use bounding-box-based frustum clipping info? */

   msurface_t *psurf = &pmodel->surfaces[pmodel->firstmodelsurface];
   int numsurfaces   = pmodel->nummodelsurfaces;

   for (i = 0; i < numsurfaces; i++, psurf++)
   {
      if (psurf->clipflags == BMODEL_FULLY_CLIPPED)
         continue;

      r_currentkey = ((mleaf_t *)e->topnode)->key;
      R_RenderFace(e, psurf, clipflags);
   }
}

/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode(const entity_t *e, mnode_t *node)
{
   int count, side;
   mplane_t *plane;
   msurface_t *surf;
   mleaf_t *pleaf;
   vec_t dot;

   if (node->contents == CONTENTS_SOLID)
      return;
   if (node->visframe != r_visframecount)
      return;
   if (node->clipflags == BMODEL_FULLY_CLIPPED)
      return;

   /* if a leaf node, draw stuff */
   if (node->contents < 0) {
      pleaf = (mleaf_t *)node;
      pleaf->key = r_currentkey;
      r_currentkey++;		/* all bmodels in a leaf share the same key */

      return;
   }

   /*
    * The node is a decision point, so go down the apropriate sides.
    * Find which side of the node we are on.
    */
   plane = node->plane;
   switch (plane->type) {
      case PLANE_X:
         dot = modelorg[0] - plane->dist;
         break;
      case PLANE_Y:
         dot = modelorg[1] - plane->dist;
         break;
      case PLANE_Z:
         dot = modelorg[2] - plane->dist;
         break;
      default:
         dot = DotProduct(modelorg, plane->normal) - plane->dist;
         break;
   }
   side = (dot >= 0) ? 0 : 1;

   /* recurse down the children, front side first */
   R_RecursiveWorldNode(e, node->children[side]);

   /* draw stuff */
   count = node->numsurfaces;
   if (count) {
      surf = cl.worldmodel->surfaces + node->firstsurface;
      for (count = node->numsurfaces; count; count--, surf++) {
         if (surf->visframe != r_visframecount)
            continue;
         if (surf->clipflags == BMODEL_FULLY_CLIPPED)
            continue;
         R_RenderFace(e, surf, surf->clipflags);
      }

      /* all surfaces on the same node share the same sequence number */
      r_currentkey++;
   }

   /* recurse down the back side */
   R_RecursiveWorldNode(e, node->children[!side]);
}



/*
================
R_RenderWorld
================
*/
void R_RenderWorld(void)
{
   VectorCopy(r_origin, modelorg);
   r_pcurrentvertbase = r_worldentity.model->vertexes;
   R_RecursiveWorldNode(&r_worldentity, r_worldentity.model->nodes);
}
