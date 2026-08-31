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
/* r_efrag.c */

#include "console.h"
#include "model.h"
#include "quakedef.h"
#include "sys.h"

#include "r_local.h"

mnode_t *r_pefragtopnode;


/* =========================================================================== */

/*
===============================================================================

			ENTITY FRAGMENT FUNCTIONS

===============================================================================
*/

static efrag_t **lastlink;
static entity_t *r_addent;

vec3_t r_emins, r_emaxs;

/*
================
R_ValidEfrag

Returns true iff e points to an aligned slot inside the
cl_efrags array.  Used to gate the free-list pop in
R_SplitEntityOnNode against corruption of cl.free_efrags or
some node's entnext pointer -- same defense pattern as
R_ValidParticle in r_part.c.  Bailing out when corruption is
detected treats the situation as "free list exhausted",
which is the existing graceful-skip path.
================
*/
static int
R_ValidEfrag(const efrag_t *e)
{
    const char *base = (const char *)cl_efrags;
    const char *end  = base + sizeof(cl_efrags);
    const char *q    = (const char *)e;
    if (!e)
        return 0;
    if (q < base || q >= end)
        return 0;
    if (((size_t)(q - base) % sizeof(efrag_t)) != 0)
        return 0;
    return 1;
}

/*
================
R_RemoveEfrags

Call when removing an object from the world or moving it to another position
================
*/
void
R_RemoveEfrags(entity_t *ent)
{
    efrag_t *ef, *old, *walk, **prev;

    ef = ent->efrag;

    while (ef) {
	/* Defensive: if the entity's efrag chain links into
	 * memory outside cl_efrags[], abort the walk rather
	 * than dereferencing garbage and propagating corruption
	 * onto the free list. */
	if (!R_ValidEfrag(ef)) {
	    ent->efrag = NULL;
	    return;
	}
	prev = &ef->leaf->efrags;
	while (1) {
	    walk = *prev;
	    if (!walk)
		break;
	    if (walk == ef) {	/* remove this fragment */
		*prev = ef->leafnext;
		break;
	    } else
		prev = &walk->leafnext;
	}

	old = ef;
	ef = ef->entnext;

	/* put it on the free list */
	old->entnext = cl.free_efrags;
	cl.free_efrags = old;
    }

    ent->efrag = NULL;
}

/*
===================
R_SplitEntityOnNode
===================
*/
static void
R_SplitEntityOnNode(mnode_t *node)
{
    efrag_t *ef;
    mplane_t *splitplane;
    mleaf_t *leaf;
    int sides;

    if (node->contents == CONTENTS_SOLID) {
	return;
    }
/* add an efrag if the node is a leaf */

    if (node->contents < 0) {
	if (!r_pefragtopnode)
	    r_pefragtopnode = node;

	leaf = (mleaf_t *)node;

/* grab an efrag off the free list */
	ef = cl.free_efrags;
	if (!R_ValidEfrag(ef)) {
	    /* Either the free list is exhausted (NULL) or its
	     * head has been corrupted by a stomp from another
	     * subsystem.  Either way, drop this efrag silently --
	     * matches the existing "Too many efrags!" graceful
	     * path. */
	    if (!ef)
		Con_Printf("Too many efrags!\n");
	    return;
	}
	cl.free_efrags = cl.free_efrags->entnext;

	ef->entity = r_addent;

/* add the entity link */
	*lastlink = ef;
	lastlink = &ef->entnext;
	ef->entnext = NULL;

/* set the leaf links */
	ef->leaf = leaf;
	ef->leafnext = leaf->efrags;
	leaf->efrags = ef;

	return;
    }
/* NODE_MIXED */

    splitplane = node->plane;
    sides = BOX_ON_PLANE_SIDE(r_emins, r_emaxs, splitplane);

    if (sides == PSIDE_BOTH) {
	/* split on this plane */
	/* if this is the first splitter of this bmodel, remember it */
	if (!r_pefragtopnode)
	    r_pefragtopnode = node;
    }
/* recurse down the contacted sides */
    if (sides & PSIDE_FRONT)
	R_SplitEntityOnNode(node->children[0]);

    if (sides & PSIDE_BACK)
	R_SplitEntityOnNode(node->children[1]);
}

/*
===================
R_SplitEntityOnNode2
===================
*/
void
R_SplitEntityOnNode2(mnode_t *node)
{
    mplane_t *splitplane;
    int sides;

    if (node->visframe != r_visframecount)
	return;
    if (node->clipflags == BMODEL_FULLY_CLIPPED)
	return;

    if (node->contents < 0) {
	if (node->contents != CONTENTS_SOLID)
	    r_pefragtopnode = node;
	/* we've reached a non-solid leaf, so it's */
	/*  visible and not BSP clipped */
	return;
    }

    splitplane = node->plane;
    sides = BOX_ON_PLANE_SIDE(r_emins, r_emaxs, splitplane);

    if (sides == PSIDE_BOTH) {
	/* remember first splitter */
	r_pefragtopnode = node;
	return;
    }
/* not split yet; recurse down the contacted side */
    if (sides & PSIDE_FRONT)
	R_SplitEntityOnNode2(node->children[0]);
    else
	R_SplitEntityOnNode2(node->children[1]);
}


/*
===========
R_AddEfrags
===========
*/
void
R_AddEfrags(entity_t *ent)
{
    if (!ent->model)
	return;

    r_addent = ent;
    lastlink = &ent->efrag;
    r_pefragtopnode = NULL;

    VectorAdd(ent->origin, ent->model->mins, r_emins);
    VectorAdd(ent->origin, ent->model->maxs, r_emaxs);

    R_SplitEntityOnNode(cl.worldmodel->nodes);
    ent->topnode = r_pefragtopnode;
}


/*
================
R_StoreEfrags

// FIXME: a lot of this goes away with edge-based
================
*/
void
R_StoreEfrags(efrag_t **ppefrag)
{
    entity_t *pent;
    model_t *clmodel;
    efrag_t *pefrag;

    while ((pefrag = *ppefrag) != NULL) {
	pent = pefrag->entity;
	clmodel = pent->model;
	switch (clmodel->type) {
	case mod_alias:
	case mod_brush:
	case mod_sprite:
	    if ((pent->visframe != r_framecount) &&
		(cl_numvisedicts < MAX_VISEDICTS)) {
		/* mark that we've recorded this entity for this frame */
		pent->visframe = r_framecount;
		cl_visedicts[cl_numvisedicts++] = *pent;
	    }
	    ppefrag = &pefrag->leafnext;
	    break;
	default:
	    Sys_Error("%s: Bad entity type %d", __func__, clmodel->type);
	}
    }
}
