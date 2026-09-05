/*
Copyright (C) 2002 Kevin Shanahan

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

/*
 * This whole setup is butt-ugly. Proceed with caution.
 */

#include "compat/strl.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <compat/msvc.h>
#endif
#include <compat/posix_string.h>

#include "console.h"
#include "cvar.h"
#include "mathlib.h"
#include "rb_tree.h"
#include "shell.h"
#include "sys.h"
#include "zone.h"

/*
 * When we want to build up temporary trees of strings for completions, file
 * listings, etc. we can use the "temp" hunk since we don't want to keep them
 * around. These allocator functions attempt to make more efficient use of the
 * hunk space by keeping the tree nodes together in blocks and allocating
 * strings right next to each other.
 */
static struct stree_node *st_node_next;
static int st_node_space;
static char *st_string_next;
static int st_string_space;

#define ST_NODE_CHUNK	2048 /* 2kB / 16B => 128 nodes */
#define ST_STRING_CHUNK	4096 /* 4k of strings together */

void
STree_AllocInit(void)
{
    /* Init the temp hunk.  In practice this 2KB allocation
     * cannot fail, but Hunk_TempAlloc -> Hunk_HighAlloc
     * returns NULL on out-of-memory rather than Sys_Error'ing.
     * Match the NULL-handling pattern used by STree_AllocNode
     * / STree_AllocString below so the failure mode is
     * "tree refuses inserts" instead of "writes through
     * NULL on first AllocNode call". */
    st_node_next = (struct stree_node*)Hunk_TempAlloc(ST_NODE_CHUNK);
    st_node_space = st_node_next ? ST_NODE_CHUNK : 0;

    /* Allocate string space on demand */
    st_string_space = 0;
}

static struct stree_node *
STree_AllocNode(void)
{
    struct stree_node *ret = NULL;

    if (st_node_space < sizeof(struct stree_node)) {
	st_node_next = (struct stree_node*)Hunk_TempAllocExtend(ST_NODE_CHUNK);
	st_node_space = st_node_next ? ST_NODE_CHUNK : 0;
    }
    if (st_node_space >= sizeof(struct stree_node)) {
	ret = st_node_next++;
	st_node_space -= sizeof(struct stree_node);
    }

    return ret;
}

static void *
STree_AllocString(unsigned int length)
{
    char *ret = NULL;

    if (st_string_space < length) {
	/*
	 * Note: might want to consider different allocation scheme here if we
	 * end up wasting a lot of space. E.g. if space wasted > 16, may as
	 * well use another chunk. This may cause excessive calls to
	 * Cache_FreeHigh, so maybe only do it if wasting more than that
	 * (32/64/?).
	 */
	st_string_next = (char*)Hunk_TempAllocExtend(ST_STRING_CHUNK);
	st_string_space = st_string_next ? ST_STRING_CHUNK : 0;
    }
    if (st_string_space >= length) {
	ret = st_string_next;
	st_string_next += length;
	st_string_space -= length;
    }

    return ret;
}

/*
 * Insert string node "node" into rb_tree rooted at "root"
 */
qboolean
STree_Insert(struct stree_root *root, struct stree_node *node)
{
    struct rb_node **p = &root->root.rb_node;
    struct rb_node *parent = NULL;
    unsigned int len;
    int cmp;

    while (*p) {
	parent = *p;
	cmp = strcasecmp(node->string, stree_entry(parent)->string);
	if (cmp < 0)
	    p = &(*p)->rb_left;
	else if (cmp > 0)
	    p = &(*p)->rb_right;
	else
	    return false; /* string already present */
    }
    root->entries++;
    len = strlen(node->string);
    if (len > root->maxlen)
	root->maxlen = len;
    if (len < root->minlen)
	root->minlen = len;
    rb_link_node(&node->node, parent, p);
    rb_insert_color(&node->node, &root->root);

    return true;
}

/*
 * Insert string into rb tree, allocating the node dynamically.
 * If alloc_str != 0, allocate and copy the string as well.
 * NOTE: These allocations are only on the Temp hunk.
 */
qboolean
STree_InsertAlloc(struct stree_root *root, const char *s, qboolean alloc_str)
{
    qboolean ret = false;
    struct stree_node *n;
    char *tmp;

    n = STree_AllocNode();
    if (n) {
	if (alloc_str) {
	    size_t slen = strlen(s) + 1;
	    tmp = (char*)STree_AllocString(slen);
	    if (tmp) {
		memcpy(tmp, s, slen);
		n->string = tmp;
	    }
	} else {
	    n->string = s;
	}
	ret = STree_Insert(root, n);
    }

    return ret;
}

void
STree_Remove(struct stree_root *root, struct stree_node *node)
{
    rb_erase(&node->node, &root->root);
}

/* STree_MaxMatch helper */
static int
ST_node_match(struct rb_node *n, const char *str, int min_match, int max_match)
{
    if (n) {
	max_match = ST_node_match(n->rb_left, str, min_match, max_match);

	/* How much does this node match */
	while (max_match > min_match) {
	    if (!strncasecmp(str, stree_entry(n)->string, max_match))
		break;
	    max_match--;
	}
	max_match = ST_node_match(n->rb_right, str, min_match, max_match);
    }

    return max_match;
}

/*
 * Given a prefix, return the maximum common prefix of all other strings in
 * the tree which match the given prefix.
 */
char *
STree_MaxMatch(struct stree_root *root, const char *pfx)
{
    int max_match, min_match, match;
    struct rb_node *n;
    struct stree_node *sn;
    char *result = NULL;

    /* Can't be more than the shortest string */
    max_match = root->minlen;
    min_match = strlen(pfx);

    n = root->root.rb_node;
    sn = stree_entry(n);

    if (root->entries == 1) {
	match = strlen(sn->string);
	result = (char*)Z_Malloc(match + 2);
	if (result) {
	    memcpy(result, sn->string, match);
	    result[match] = ' ';
	    result[match + 1] = 0;
	}
    } else if (root->entries > 1) {
	match = ST_node_match(n, sn->string, min_match, max_match);
	result = (char*)Z_Malloc(match + 1);
	if (result) {
	    strlcpy(result, sn->string, match + 1);
	}
    }

    return result;
}

struct stree_node *
STree_Find(struct stree_root *root, const char *s)
{
    struct rb_node *p = root->root.rb_node;
    struct stree_node *ret = NULL;
    struct stree_node *node;
    int cmp;

    while (p) {
	node = stree_entry(p);
	cmp = strcasecmp(s, node->string);
	if (cmp < 0)
	    p = p->rb_left;
	else if (cmp > 0)
	    p = p->rb_right;
	else {
	    ret = node;
	    break;
	}
    }

    return ret;
}

/* An R-B Tree with n entries has a maximum height of 2log(n +1) */
static int
STree_MaxDepth(struct stree_root *root)
{
    return 2 * Q_log2(root->entries + 1);
}

static void
STree_StackInit(struct stree_root *root)
{
    root->stack = (struct stree_stack*)Z_Malloc(sizeof(struct stree_stack));
    if (root->stack) {
	struct stree_stack *s = root->stack;
	s->depth = 0;
	s->max_depth = STree_MaxDepth(root);
	s->stack = (struct rb_node**)Z_Malloc(s->max_depth * sizeof(struct rb_node *));
	if (!s->stack) {
	    Z_Free(s);
	    root->stack = NULL;
	}
    }
    /* Possibly this harsh failure is not suitable in all cases? */
#ifndef _WIN32
    if (!root->stack)
	Sys_Error("%s: not enough mem for stack! (%i)", __func__,
		  STree_MaxDepth(root));
#endif
}

void
STree_ForEach_Init__(struct stree_root *root, struct stree_node **n)
{
    /* Allocate the stack */
    STree_StackInit(root);

    /* Point to the first node */
    if (root->root.rb_node)
	*n = stree_entry(root->root.rb_node);
    else
	*n = NULL;
}

void
STree_ForEach_Cleanup__(struct stree_root *root)
{
    if (root->stack) {
	Z_Free(root->stack->stack);
	Z_Free(root->stack);
	root->stack = NULL;
    }
}

static void
STree_StackPush(struct stree_root *root, struct rb_node *n)
{
    struct stree_stack *s = root->stack;
    assert(s->depth < s->max_depth);
    s->stack[s->depth++] = n;
}

static struct rb_node *
STree_StackPop(struct stree_root *root)
{
    struct stree_stack *s = root->stack;
    assert(s->depth > 0);
    return s->stack[--s->depth];
}

/*
 * STree_WalkLeft - Helper for STree_ForEach
 *
 * Explanation of implied semantics:
 * 1. If *n is not null, we haven't looked at it at all yet
 *    - Check the left child.
 *    - If child is non-null, push *n onto the stack, *n = (*n)->left; Goto 1.
 *    - If left child is null, keep this *n and exit (true)
 * 2. If *n is null, we need to grab the node from the top of the stack
 *    - If the stack is empty, we're finished
 *      - Free the stack and exit (false)
 *    - Otherwise, *n = <pop the top of the stack> and exit (true)
 */
qboolean
STree_WalkLeft__(struct stree_root *root, struct stree_node **n)
{
    struct rb_node *rb;

    if (*n) {
	rb = &(*n)->node;
	while (rb->rb_left) {
	    STree_StackPush(root, rb);
	    rb = rb->rb_left;
	}
	*n = stree_entry(rb);
    } else {
	/* Null signifies that we need to pop from the stack */
	if (root->stack->depth > 0) {
	    rb = STree_StackPop(root);
	    *n = stree_entry(rb);
	} else
	    STree_ForEach_Cleanup__(root);
    }

    return *n != NULL;
}

/*
 * STree_WalkRight__ - Helper for STree_ForEach
 *
 * Called at the end of a loop iteration. So, *n has been processed.
 * - If *n has a right child, *n = right child. Exit.
 * - Otherwise, *n = NULL (tells WalkLeft to grab parent next). Exit.
 */
void
STree_WalkRight__(struct stree_node **n)
{
    struct rb_node *rb;

    rb = &(*n)->node;
    if (rb->rb_right)
	*n = stree_entry(rb->rb_right);
    else
	*n = NULL;
}

/*
 * Skip through the tree to a specified entry, without iterating
 * through everything. This is basically STree_Find, but with stack
 * pushes so the iteration can be continued. We then move n to the
 * next node.
 */
void
STree_ForEach_After__(struct stree_root *root, struct stree_node **n,
		      const char *s)
{
    struct rb_node *p;
    struct stree_node *node;
    int cmp;

    *n = NULL;
    p = root->root.rb_node;
    while (p) {
	node = stree_entry(p);
	cmp = strcasecmp(s, node->string);
	if (cmp < 0) {
	    STree_StackPush(root, p);
	    p = p->rb_left;
	} else if (cmp > 0) {
	    p = p->rb_right;
	} else {
	    *n = node;
	    break;
	}
    }

    if (*n) {
	/* found the exact node; skip on to the next one */
	if (p->rb_right)
	    *n = stree_entry(p->rb_right);
	else
	    *n = NULL;
    } else {
	/* Didn't find str. Don't walk the tree at all! */
	root->stack->depth = 0;
    }
}

void
STree_Completions(struct stree_root *out, struct stree_root *in, const char *s)
{
    struct stree_node *n;
    struct rb_node *rb = NULL;
    int cmp, len;

    len = strlen(s);
    rb = in->root.rb_node;

    /* Work our way down to the subtree required */
    while (rb) {
	n = stree_entry(rb);
	cmp = strncasecmp(s, n->string, len);
	if (cmp < 0)
	    rb = rb->rb_left;
	if (cmp > 0)
	    rb = rb->rb_right;
	else
	    break;
    }

    STree_StackInit(in);

    while (rb) {
	n = stree_entry(rb);
	cmp = strncasecmp(s, n->string, len);
	if (!cmp) {
	    STree_InsertAlloc(out, n->string, false);
	    if (rb->rb_left) {
		if (rb->rb_right)
		    STree_StackPush(in, rb->rb_right);
	        rb = rb->rb_left;
	    } else {
		rb = rb->rb_right;
	    }
	} else if (cmp < 0) {
	    rb = rb->rb_left;
	} else {
	    rb = rb->rb_right;
	}
	if (!rb && in->stack->depth > 0)
	    rb = STree_StackPop(in);
    }

    STree_ForEach_Cleanup__(in);
}
