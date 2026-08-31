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
/* world.c -- world query functions */

#include "bspfile.h"
#include "console.h"
#include "mathlib.h"
#include "model.h"
#include "progs.h"
#include "server.h"
#include "world.h"

#include "host.h"
#include "quakedef.h"
#include "sys.h"
/* FIXME - quick hack to enable merging of NQ/QWSV shared code */
#define SV_Error Sys_Error

/*

entities never clip against themselves, or their owner

line of sight checks trace->crosscontent, but bullets don't

*/

cvar_t	sys_quake2 = {"sys_quake2","1",true};

typedef struct {
    vec3_t boxmins, boxmaxs;	/* enclose the test object along entire move */
    float *mins, *maxs;		/* size of the moving object */
    vec3_t mins2, maxs2;	/* size when clipping against mosnters */
    float *start, *end;
    trace_t trace;
    int type;
    edict_t *passedict;
} moveclip_t;


int SV_HullPointContents(hull_t *hull, int num, vec3_t p);

/*
===============================================================================

HULL BOXES

===============================================================================
*/


static hull_t box_hull;
static mclipnode_t box_clipnodes[6];
static mplane_t box_planes[6];
static	int			move_type;

/*
===================
SV_InitBoxHull

Set up the planes and clipnodes so that the six floats of a bounding box
can just be stored out and get a proper hull_t structure.
===================
*/
static void SV_InitBoxHull(void)
{
   int i;

   box_hull.clipnodes = box_clipnodes;
   box_hull.planes = box_planes;
   box_hull.firstclipnode = 0;
   box_hull.lastclipnode = 5;

   for (i = 0; i < 6; i++)
   {
      int side;

      box_clipnodes[i].planenum = i;

      side = i & 1;

      box_clipnodes[i].children[side] = CONTENTS_EMPTY;
      if (i != 5)
         box_clipnodes[i].children[side ^ 1] = i + 1;
      else
         box_clipnodes[i].children[side ^ 1] = CONTENTS_SOLID;

      box_planes[i].type = i >> 1;
      box_planes[i].normal[i >> 1] = 1;
   }
}


/*
===================
SV_HullForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
static hull_t *SV_HullForBox(vec3_t mins, vec3_t maxs)
{
    box_planes[0].dist = maxs[0];
    box_planes[1].dist = mins[0];
    box_planes[2].dist = maxs[1];
    box_planes[3].dist = mins[1];
    box_planes[4].dist = maxs[2];
    box_planes[5].dist = mins[2];

    return &box_hull;
}

/*
================
SV_HullForEntity

Returns a hull that can be used for testing or clipping an object of mins/maxs
size.
Offset is filled in to contain the adjustment that must be added to the
testing object's origin to get a point to use with the returned hull.
================
*/
static hull_t *SV_HullForEntity(edict_t *ent, vec3_t mins, vec3_t maxs, vec3_t offset, edict_t *move_ent)
{
   hull_t *hull;

   /* decide which clipping hull to use, based on the size */
   if (ent->v.solid == SOLID_BSP)
   {
      /* explicit hulls in the BSP model */

      vec3_t size;
      model_t *model;

      if (ent->v.movetype != MOVETYPE_PUSH)
         SV_Error("SOLID_BSP without MOVETYPE_PUSH");

      model = sv.models[(int)ent->v.modelindex];

      if (!model || model->type != mod_brush)
         SV_Error("MOVETYPE_PUSH with a non bsp model");

      VectorSubtract(maxs, mins, size);

      {
         if (size[0] < 3)              /* Point */
            hull = &model->hulls[0];
         else if ((size[0] <= 8)&&((int)(sv.edicts->v.spawnflags)&1))  /* Pentacles */
            hull = &model->hulls[4];
         else if (size[0] <= 32)
            hull = &model->hulls[1];
         else
            hull = &model->hulls[2];
      }

      /* calculate an offset value to center the origin */
      VectorSubtract(hull->clip_mins, mins, offset);
		if((int)move_ent->v.flags&FL_MONSTER)
      {
         if(offset[0]!=0||offset[1]!=0)
         {
            offset[0]=0;
            offset[1]=0;
         }
      }
      VectorAdd(offset, ent->v.origin, offset);
   }
   else
   {
      /* create a temp hull from bounding box sizes */

      vec3_t hullmins, hullmaxs;

      VectorSubtract(ent->v.mins, maxs, hullmins);
      VectorSubtract(ent->v.maxs, mins, hullmaxs);
      hull = SV_HullForBox(hullmins, hullmaxs);

      VectorCopy(ent->v.origin, offset);
   }

   return hull;
}

/*
===============================================================================

ENTITY AREA CHECKING

===============================================================================
*/

typedef struct areanode_s {
    int axis;			/* -1 = leaf node */
    float dist;
    struct areanode_s *children[2];
    link_t trigger_edicts;
    link_t solid_edicts;
} areanode_t;

#define	AREA_DEPTH	4
#define	AREA_NODES	32

static areanode_t sv_areanodes[AREA_NODES];
static int sv_numareanodes;

/*
===============
SV_CreateAreaNode

===============
*/
static areanode_t *
SV_CreateAreaNode(int depth, vec3_t mins, vec3_t maxs)
{
   areanode_t *anode;
   vec3_t size;
   vec3_t mins1, maxs1, mins2, maxs2;

   anode = &sv_areanodes[sv_numareanodes];
   sv_numareanodes++;

   ClearLink(&anode->trigger_edicts);
   ClearLink(&anode->solid_edicts);

   if (depth == AREA_DEPTH)
   {
      anode->axis = -1;
      anode->children[0] = anode->children[1] = NULL;
      return anode;
   }

   VectorSubtract(maxs, mins, size);
   if (size[0] > size[1])
      anode->axis = 0;
   else
      anode->axis = 1;

   anode->dist = 0.5 * (maxs[anode->axis] + mins[anode->axis]);
   VectorCopy(mins, mins1);
   VectorCopy(mins, mins2);
   VectorCopy(maxs, maxs1);
   VectorCopy(maxs, maxs2);

   maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

   anode->children[0] = SV_CreateAreaNode(depth + 1, mins2, maxs2);
   anode->children[1] = SV_CreateAreaNode(depth + 1, mins1, maxs1);

   return anode;
}

/*
===============
SV_ClearWorld

===============
*/
void
SV_ClearWorld(void)
{
   SV_InitBoxHull();

   memset(sv_areanodes, 0, sizeof(sv_areanodes));
   sv_numareanodes = 0;
   SV_CreateAreaNode(0, sv.worldmodel->mins, sv.worldmodel->maxs);
}


/*
===============
SV_UnlinkEdict

===============
*/
void
SV_UnlinkEdict(edict_t *ent)
{
   if (!ent->area.prev)
      return;			/* not linked in anywhere */
   RemoveLink(&ent->area);
   ent->area.prev = ent->area.next = NULL;
}


/*
====================
SV_TouchLinks
====================
*/
static void SV_TouchLinks(edict_t *ent, areanode_t *node)
{
   link_t *l, *lnext;
   int old_self, old_other;

   /* touch linked edicts */
   for (l = node->trigger_edicts.next; l != &node->trigger_edicts; l = lnext)
   {
      edict_t *touch;
      /*
       * FIXME - Just paranoia? Check if this can really happen...
       *         (I think it was related to the E2M2 drawbridge bug)
       */
      if (!l || !l->next)
      {
         /* my area got removed out from under me! */
         break;
      }

      lnext = l->next;
      touch = EDICT_FROM_AREA(l);
      if (touch == ent)
         continue;
      if (!touch->v.touch || touch->v.solid != SOLID_TRIGGER)
         continue;
      if (ent->v.absmin[0] > touch->v.absmax[0]
            || ent->v.absmin[1] > touch->v.absmax[1]
            || ent->v.absmin[2] > touch->v.absmax[2]
            || ent->v.absmax[0] < touch->v.absmin[0]
            || ent->v.absmax[1] < touch->v.absmin[1]
            || ent->v.absmax[2] < touch->v.absmin[2])
         continue;

      old_self = pr_global_struct->self;
      old_other = pr_global_struct->other;

      pr_global_struct->self = EDICT_TO_PROG(touch);
      pr_global_struct->other = EDICT_TO_PROG(ent);
      pr_global_struct->time = sv.time;
      PR_ExecuteProgram(touch->v.touch);

      pr_global_struct->self = old_self;
      pr_global_struct->other = old_other;
   }

   /* recurse down both sides */
   if (node->axis == -1)
      return;

   if (ent->v.absmax[node->axis] > node->dist)
      SV_TouchLinks(ent, node->children[0]);
   if (ent->v.absmin[node->axis] < node->dist)
      SV_TouchLinks(ent, node->children[1]);
}


/*
===============
SV_FindTouchedLeafs

===============
*/
static void
SV_FindTouchedLeafs(edict_t *ent, mnode_t *node)
{
   mplane_t *splitplane;
   mleaf_t *leaf;
   int sides;

   if (node->contents == CONTENTS_SOLID)
      return;

   /* add an efrag if the node is a leaf */

   if (node->contents < 0)
   {
      int leafnum;
      if (ent->num_leafs == MAX_ENT_LEAFS)
         return;

      leaf = (mleaf_t *)node;
      leafnum = leaf - sv.worldmodel->leafs - 1;

      ent->leafnums[ent->num_leafs] = leafnum;
      ent->num_leafs++;
      return;
   }

   /* NODE_MIXED */

   splitplane = node->plane;
   sides = BOX_ON_PLANE_SIDE(ent->v.absmin, ent->v.absmax, splitplane);

   /* recurse down the contacted sides */
   if (sides & PSIDE_FRONT)
      SV_FindTouchedLeafs(ent, node->children[0]);

   if (sides & PSIDE_BACK)
      SV_FindTouchedLeafs(ent, node->children[1]);
}

/*
===============
SV_LinkEdict

===============
*/
void
SV_LinkEdict(edict_t *ent, qboolean touch_triggers)
{
   areanode_t *node;

   if (ent->area.prev)
      SV_UnlinkEdict(ent);	/* unlink from old position */

   if (ent == sv.edicts)
      return;			/* don't add the world */

   if (ent->free)
      return;

   /* set the abs box */
   {
      VectorAdd(ent->v.origin, ent->v.mins, ent->v.absmin);
      VectorAdd(ent->v.origin, ent->v.maxs, ent->v.absmax);
   }

   /*
    * To make items easier to pick up and allow them to be grabbed off of
    * shelves, the abs sizes are expanded
    */
   if ((int)ent->v.flags & FL_ITEM) {
      ent->v.absmin[0] -= 15;
      ent->v.absmin[1] -= 15;
      ent->v.absmax[0] += 15;
      ent->v.absmax[1] += 15;
   } else {
      /*
       * because movement is clipped an epsilon away from an actual edge, we
       * must fully check even when bounding boxes don't quite touch
       */
      ent->v.absmin[0] -= 1;
      ent->v.absmin[1] -= 1;
      ent->v.absmin[2] -= 1;
      ent->v.absmax[0] += 1;
      ent->v.absmax[1] += 1;
      ent->v.absmax[2] += 1;
   }

   /* link to PVS leafs */
   ent->num_leafs = 0;
   if (ent->v.modelindex)
      SV_FindTouchedLeafs(ent, sv.worldmodel->nodes);

   if (ent->v.solid == SOLID_NOT)
      return;

   /* find the first node that the ent's box crosses */
   node = sv_areanodes;
   while (1) {
      if (node->axis == -1)
         break;
      if (ent->v.absmin[node->axis] > node->dist)
         node = node->children[0];
      else if (ent->v.absmax[node->axis] < node->dist)
         node = node->children[1];
      else
         break;		/* crosses the node */
   }

   /* link it in */
   if (ent->v.solid == SOLID_TRIGGER)
      InsertLinkBefore(&ent->area, &node->trigger_edicts);
   else
      InsertLinkBefore(&ent->area, &node->solid_edicts);

   if (touch_triggers)
      /* touch all entities at this node and decend for more */
      SV_TouchLinks(ent, sv_areanodes);
}


/*
===============================================================================

				POINT TESTING IN HULLS

===============================================================================
*/

/*
==================
SV_HullPointContents

==================
*/
int
SV_HullPointContents(hull_t *hull, int num, vec3_t p)
{
   float d;

   /* Defensive: bail out gracefully if the hull is wholly bogus.
    * Treat unrecognisable input as empty space so the physics
    * tick can complete without crashing.  Observed during map
    * transitions where a stale hull pointer survives into a
    * physics call (sv.worldmodel reload, server frame race). */
   if (!hull || !hull->clipnodes || !hull->planes)
      return CONTENTS_EMPTY;

   while (num >= 0)
   {
      mclipnode_t *node;
      mplane_t *plane;
      if (num < hull->firstclipnode || num > hull->lastclipnode)
         SV_Error("%s: bad node number (%i)", __func__, num);

      node = hull->clipnodes + num;
      plane = hull->planes + node->planenum;

      if (plane->type < 3)
         d = p[plane->type] - plane->dist;
      else
         d = DotProduct(plane->normal, p) - plane->dist;
      if (d < 0)
         num = node->children[1];
      else
         num = node->children[0];
   }

   return num;
}

/*
==================
SV_PointContents

==================
*/
int
SV_PointContents(vec3_t p)
{

   /* Defensive: physics callers (SV_CheckWater etc.) can fire
    * during a brief window between map unload and reload when
    * sv.worldmodel is being torn down.  Treat that as empty
    * space rather than dereferencing a stale or NULL pointer. */
   if (!sv.worldmodel)
      return CONTENTS_EMPTY;
   return SV_HullPointContents(&sv.worldmodel->hulls[0], 0, p);
}

/* =========================================================================== */

/*
============
SV_TestEntityPosition

A small wrapper around SV_BoxInSolidEntity that never clips against the
supplied entity.  TODO: This could be a lot more efficient?
============
*/
edict_t *SV_TestEntityPosition(edict_t *ent)
{
   trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, 0, ent);

   if (trace.startsolid)
      return sv.edicts;

   return NULL;
}

/*
===============================================================================

LINE TESTING IN HULLS

===============================================================================
*/

/* 1/32 epsilon to keep floating point happy */
#define	DIST_EPSILON	(0.03125)

/*
==================
SV_RecursiveHullCheck

==================
*/
qboolean SV_RecursiveHullCheck(hull_t *hull, int num, float p1f, float p2f,
		      vec3_t p1, vec3_t p2, trace_t *trace)
{
   mclipnode_t *node;
   mplane_t *plane;
   float t1, t2;
   float frac;
   int i;
   vec3_t mid;
   int side;
   float midf;

   /* check for empty */
   if (num < 0) {
      if (num != CONTENTS_SOLID) {
         trace->allsolid = false;
         if (num == CONTENTS_EMPTY)
            trace->inopen = true;
         else
            trace->inwater = true;
      } else
         trace->startsolid = true;
      return true;		/* empty */
   }

   if (num < hull->firstclipnode || num > hull->lastclipnode)
      SV_Error("%s: bad node number", __func__);

   /* find the point distances */
   node = hull->clipnodes + num;
   plane = hull->planes + node->planenum;

   if (plane->type < 3)
   {
      t1 = p1[plane->type] - plane->dist;
      t2 = p2[plane->type] - plane->dist;
   }
   else
   {
      t1 = DotProduct(plane->normal, p1) - plane->dist;
      t2 = DotProduct(plane->normal, p2) - plane->dist;
   }

   /* If either distance is NaN/Inf the recursive split below
    * never makes progress: the (>= 0) and (< 0) comparisons
    * are both false (NaN compares unequal to everything),
    * frac = (t1 +/- eps) / (t1 - t2) is NaN, the
    * (frac < 0) / (frac > 1) clamps don't catch NaN, mid
    * becomes NaN, and we recurse forever until the C stack
    * blows up.  SV_Move's input guard catches the most
    * common entry point (PF_traceline from QC), but
    * SV_ClipMoveToEntity also feeds here from
    * SV_ClipToLinks where an entity in the area tree may
    * have a NaN origin.  Treat non-finite distances as
    * "wholly inside / outside" by collapsing to the
    * children[0] path -- a clipnode tree has finite depth
    * so the loop equivalent in SV_HullPointContents (which
    * does the same descent) is known to terminate. */
   if (IS_NAN(t1) || IS_NAN(t2))
      return SV_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);

   if (t1 >= 0 && t2 >= 0)
      return SV_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);
   if (t1 < 0 && t2 < 0)
      return SV_RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);

   /* put the crosspoint DIST_EPSILON pixels on the near side */
   if (t1 < 0)
      frac = (t1 + DIST_EPSILON) / (t1 - t2);
   else
      frac = (t1 - DIST_EPSILON) / (t1 - t2);
   if (frac < 0)
      frac = 0;
   if (frac > 1)
      frac = 1;

   midf = p1f + (p2f - p1f) * frac;
   for (i = 0; i < 3; i++)
      mid[i] = p1[i] + frac * (p2[i] - p1[i]);

   side = (t1 < 0);

   /* move up to the node */
   if (!SV_RecursiveHullCheck(hull, node->children[side], p1f, midf, p1, mid, trace))
      return false;

   if (SV_HullPointContents(hull, node->children[side ^ 1], mid) != CONTENTS_SOLID)
      /* go past the node */
      return SV_RecursiveHullCheck(hull, node->children[side ^ 1], midf, p2f, mid, p2, trace);

   if (trace->allsolid)
      return false;		/* never got out of the solid area */

   /* ================== */
   /* the other side of the node is solid, this is the impact point */
   /* ================== */
   if (!side)
   {
      VectorCopy(plane->normal, trace->plane.normal);
      trace->plane.dist = plane->dist;
   }
   else
   {
      VectorSubtract(vec3_origin, plane->normal, trace->plane.normal);
      trace->plane.dist = -plane->dist;
   }

   /* shouldn't really happen, but does occasionally */
   while (SV_HullPointContents(hull, hull->firstclipnode, mid) == CONTENTS_SOLID) {
      frac -= 0.1;
      if (frac < 0) {
         trace->fraction = midf;
         VectorCopy(mid, trace->endpos);
         Con_DPrintf("backup past 0\n");
         return false;
      }
      midf = p1f + (p2f - p1f) * frac;
      for (i = 0; i < 3; i++)
         mid[i] = p1[i] + frac * (p2[i] - p1[i]);
   }

   trace->fraction = midf;
   VectorCopy(mid, trace->endpos);

   return false;
}

/*
==================
SV_ClipMoveToEntity

Handles selection or creation of a clipping hull, and offseting (and
eventually rotation) of the end points
==================
*/
static trace_t
SV_ClipMoveToEntity(edict_t *ent, vec3_t start, vec3_t mins, vec3_t maxs,
		    vec3_t end, edict_t *move_ent)
{
   trace_t trace;
   vec3_t offset;
   vec3_t start_l, end_l;
   hull_t *hull;

   /* fill in a default trace */
   memset(&trace, 0, sizeof(trace_t));
   trace.fraction = 1;
   trace.allsolid = true;
   VectorCopy(end, trace.endpos);

   /* get the clipping hull */
   hull = SV_HullForEntity(ent, mins, maxs, offset, move_ent);

   VectorSubtract(start, offset, start_l);
   VectorSubtract(end, offset, end_l);

	if (sys_quake2.value)
	{
		/* rotate start and end into the models frame of reference */
		if (ent->v.solid == SOLID_BSP && 
		(fabsf(ent->v.angles[0]) > 1 || fabsf(ent->v.angles[1]) > 1 || fabsf(ent->v.angles[2]) > 1) )
		{
			vec3_t	forward, right, up;
			vec3_t	temp;

			AngleVectors (ent->v.angles, forward, right, up);

			VectorCopy (start_l, temp);
			start_l[0] = DotProduct (temp, forward);
			start_l[1] = -DotProduct (temp, right);
			start_l[2] = DotProduct (temp, up);

			VectorCopy (end_l, temp);
			end_l[0] = DotProduct (temp, forward);
			end_l[1] = -DotProduct (temp, right);
			end_l[2] = DotProduct (temp, up);
		}
	}

   /* trace a line through the apropriate clipping hull */
   SV_RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, start_l, end_l,
         &trace);
	if (move_type == MOVE_WATER)
	{
		if (SV_PointContents (trace.endpos) != CONTENTS_WATER)
		{
			VectorCopy(start_l, trace.endpos);
			trace.fraction = 0;
		}
	}

	if (sys_quake2.value)
	{
		/* rotate endpos back to world frame of reference */
		if (ent->v.solid == SOLID_BSP && 
		(fabsf(ent->v.angles[0]) > 1 || fabsf(ent->v.angles[1]) > 1 || fabsf(ent->v.angles[2]) > 1) )
		{
			vec3_t	a;
			vec3_t	forward, right, up;
			vec3_t	temp;

			if (trace.fraction != 1)
			{
				VectorSubtract (vec3_origin, ent->v.angles, a);
				AngleVectors (a, forward, right, up);

				VectorCopy (trace.endpos, temp);
				trace.endpos[0] = DotProduct (temp, forward);
				trace.endpos[1] = -DotProduct (temp, right);
				trace.endpos[2] = DotProduct (temp, up);

				VectorCopy (trace.plane.normal, temp);
				trace.plane.normal[0] = DotProduct (temp, forward);
				trace.plane.normal[1] = -DotProduct (temp, right);
				trace.plane.normal[2] = DotProduct (temp, up);
			}
		}
	}

   /* fix trace up by the offset */
   if (trace.fraction != 1)
      VectorAdd(trace.endpos, offset, trace.endpos);

   /* did we clip the move? */
   if (trace.fraction < 1 || trace.startsolid)
      trace.ent = ent;

   return trace;
}

/* =========================================================================== */

/*
====================
SV_ClipToLinks

Mins and maxs enclose the entire area swept by the move
====================
*/
static void SV_ClipToLinks(areanode_t *node, moveclip_t *clip)
{
   link_t *l, *next;
   edict_t *touch;
   trace_t trace;

   /* touch linked edicts */
   for (l = node->solid_edicts.next; l != &node->solid_edicts; l = next)
   {
      next = l->next;
      touch = EDICT_FROM_AREA(l);
      if (touch->v.solid == SOLID_NOT)
         continue;
      if (touch == clip->passedict)
         continue;
      if (touch->v.solid == SOLID_TRIGGER)
			Sys_Error ("Trigger in clipping list (%s)",touch->v.classname + pr_strings);

      if ((clip->type == MOVE_NOMONSTERS ||
               clip->type == MOVE_PHASE) && touch->v.solid != SOLID_BSP)
         continue;

      if (clip->boxmins[0] > touch->v.absmax[0]
            || clip->boxmins[1] > touch->v.absmax[1]
            || clip->boxmins[2] > touch->v.absmax[2]
            || clip->boxmaxs[0] < touch->v.absmin[0]
            || clip->boxmaxs[1] < touch->v.absmin[1]
            || clip->boxmaxs[2] < touch->v.absmin[2])
         continue;

      if (clip->passedict && clip->passedict->v.size[0]
            && !touch->v.size[0])
         continue;		/* points never interact */

      /* might intersect, so do an exact clip */
      if (clip->trace.allsolid)
         return;

      if (clip->passedict)
      {
         if (PROG_TO_EDICT(touch->v.owner) == clip->passedict)
            continue;	/* don't clip against own missiles */
         if (PROG_TO_EDICT(clip->passedict->v.owner) == touch)
            continue;	/* don't clip against owner */
      }

      if ((int)touch->v.flags & FL_MONSTER)
         trace = SV_ClipMoveToEntity(
               touch, clip->start, clip->mins2, clip->maxs2, clip->end, touch);
      else
         trace = SV_ClipMoveToEntity(
               touch, clip->start, clip->mins, clip->maxs, clip->end, touch);

      if (trace.allsolid || trace.startsolid
            || trace.fraction < clip->trace.fraction)
      {
         trace.ent = touch;
         if (clip->trace.startsolid)
         {
            clip->trace = trace;
            clip->trace.startsolid = true;
         } else
            clip->trace = trace;
      }
      else if (trace.startsolid)
         clip->trace.startsolid = true;
   }

   /* recurse down both sides */
   if (node->axis == -1)
      return;

   if (clip->boxmaxs[node->axis] > node->dist)
      SV_ClipToLinks(node->children[0], clip);
   if (clip->boxmins[node->axis] < node->dist)
      SV_ClipToLinks(node->children[1], clip);
}


/*
==================
SV_MoveBounds
==================
*/
static void SV_MoveBounds(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end,
	      vec3_t boxmins, vec3_t boxmaxs)
{
   int i;

   for (i = 0; i < 3; i++)
   {
      if (end[i] > start[i])
      {
         boxmins[i] = start[i] + mins[i] - 1;
         boxmaxs[i] = end[i] + maxs[i] + 1;
      }
      else
      {
         boxmins[i] = end[i] + mins[i] - 1;
         boxmaxs[i] = start[i] + maxs[i] + 1;
      }
   }
}

/*
==================
SV_Move
==================
*/
trace_t SV_Move(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type,
	edict_t *passedict)
{
   moveclip_t clip;

   memset(&clip, 0, sizeof(moveclip_t));

   /* QC-controlled vectors (PF_traceline, PF_aim, etc.) feed
    * here unchecked.  NaN / Inf in start or end propagates
    * through SV_RecursiveHullCheck: the t1, t2 distance
    * computations become NaN, the (t1 >= 0 && t2 >= 0) /
    * (t1 < 0 && t2 < 0) comparisons are both false (NaN
    * compares unequal to everything), so the recursive path
    * always splits the segment.  frac = (t1 +/- eps) /
    * (t1 - t2) with NaN inputs is NaN; the
    * (frac < 0) / (frac > 1) clamps don't catch NaN; mid =
    * p1 + frac*(p2-p1) is NaN; the recursion descends into
    * SV_RecursiveHullCheck with NaN coordinates forever
    * until the stack overflows.  Reject before tracing.
    *
    * Also reject mins/maxs to guard SV_MoveBounds and the
    * box-vs-entity intersect tests downstream.  (SV_LinkEdict
    * doesn't currently validate ent->v.origin either, but
    * fixing that is upstream of what we're allowed to touch
    * from a trace path -- callers are responsible.) */
   {
      int i;
      for (i = 0; i < 3; i++) {
         if (IS_NAN(start[i]) || IS_NAN(end[i])
             || IS_NAN(mins[i]) || IS_NAN(maxs[i])) {
            /* Return a "no move possible" trace -- fraction
             * 1.0, end == start, no impact.  This matches what
             * the engine sees when a trace starts and ends at
             * the same point in empty space. */
            memset(&clip.trace, 0, sizeof(clip.trace));
            clip.trace.fraction = 1.0;
            VectorCopy(start, clip.trace.endpos);
            return clip.trace;
         }
      }
   }

   /* clip to world */
   clip.trace = SV_ClipMoveToEntity(sv.edicts, start, mins, maxs, end, passedict);

   clip.start = start;
   clip.end = end;
   clip.mins = mins;
   clip.maxs = maxs;
   clip.type = type;
   clip.passedict = passedict;

	if (type == MOVE_MISSILE || type == MOVE_PHASE)
   {
      /* Larger for projectiles against monsters */
      int i;
      for (i = 0; i < 3; i++)
      {
         clip.mins2[i] = -15;
         clip.maxs2[i] = 15;
      }
   }
   else
   {
      VectorCopy(mins, clip.mins2);
      VectorCopy(maxs, clip.maxs2);
   }

   /* create the bounding box of the entire move */
   SV_MoveBounds(start, clip.mins2, clip.maxs2, end, clip.boxmins,
         clip.boxmaxs);

   /* clip to entities */
   SV_ClipToLinks(sv_areanodes, &clip);

   return clip.trace;
}
