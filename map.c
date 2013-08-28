/* $Id: map.c,v 1.4 2000/07/28 05:12:53 esr Exp $  - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
map.c

This file contains routines for playing around with view_maps,
real_maps, path_maps, and cont_maps.
*/

#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif

#include "empire.h"
#include "extern.h"

#ifndef PROFILE
#define STATIC static
#else
#define STATIC
/* can't get accurate profile when procedures are static */
#endif

#define SWAP(a,b) { \
	perimeter_t *x; \
	x = a; a = b; b = x; \
}

STATIC void expand_perimeter(path_map_t *pmap,view_map_t *vmap,move_info_t *move_info,perimeter_t *curp,int type,int cur_cost,int inc_wcost,int inc_lcost,perimeter_t *waterp,perimeter_t *landp);
STATIC void expand_prune(view_map_t *vmap,path_map_t *pmap,long loc,int type,perimeter_t *to,int *explored);
STATIC int objective_cost(view_map_t *vmap,move_info_t *move_info,long loc,int base_cost);
STATIC int terrain_type(path_map_t *pmap,view_map_t *vmap,move_info_t *move_info,long from_loc,long to_loc);
STATIC void start_perimeter(path_map_t *pmap,perimeter_t *perim,long loc,int terrain);
STATIC void add_cell(path_map_t *pmap,long new_loc,perimeter_t *perim,int terrain,int cur_cost,int inc_cost);
STATIC int vmap_count_path (path_map_t *pmap,long loc);

static perimeter_t p1; /* perimeter list for use as needed */
static perimeter_t p2;
static perimeter_t p3;
static perimeter_t p4;

static int best_cost; /* cost and location of best objective */
static long best_loc;

/*
Map out a continent.  We are given a location on the continent.
We mark each square that is part of the continent and unexplored
territory adjacent to the continent.  By adjusting the value of
'bad_terrain', this routine can map either continents of land,
or lakes.
*/

void
vmap_cont (cont_map, vmap, loc, bad_terrain)
int *cont_map;
view_map_t *vmap;
long loc;
char bad_terrain;
{
	(void) bzero ((char *)cont_map, MAP_SIZE * sizeof (int));
	vmap_mark_up_cont (cont_map, vmap, loc, bad_terrain);
}

/*
Mark all squares of a continent and the squares that are adjacent
to the continent which are on the board.  Our passed location is
known to be either on the continent or adjacent to the continent.
*/

void
vmap_mark_up_cont (cont_map, vmap, loc, bad_terrain)
int *cont_map;
view_map_t *vmap;
long loc;
char bad_terrain;
{
	int i, j;
	long new_loc;
	char this_terrain;
	perimeter_t *from, *to;

	from = &p1;
	to = &p2;
	
	from->len = 1; /* init perimeter */
	from->list[0] = loc;
	cont_map[loc] = 1; /* loc is on continent */
	
	while (from->len) {
		to->len = 0; /* nothing in new perimeter yet */
		
		for (i = 0; i < from->len; i++) /* expand perimeter */
		FOR_ADJ_ON(from->list[i], new_loc, j)
		if (!cont_map[new_loc]) {
			/* mark, but don't expand, unexplored territory */
			if (vmap[new_loc].contents == ' ')
				cont_map[new_loc] = 1;
			else {
				if (vmap[new_loc].contents == '+') this_terrain = '+';
				else if (vmap[new_loc].contents == '.') this_terrain = '.';
				else this_terrain = map[new_loc].contents;
				
				if (this_terrain != bad_terrain) { /* on continent? */
					cont_map[new_loc] = 1;
					to->list[to->len] = new_loc;
					to->len += 1;
				}
			}
		}
		SWAP (from, to);
	}
}

/*
Map out a continent.  We are given a location on the continent.
We mark each square that is part of the continent.
By adjusting the value of
'bad_terrain', this routine can map either continents of land,
or lakes.
*/

static void rmap_mark_up_cont();

void
rmap_cont (cont_map, loc, bad_terrain)
int *cont_map;
long loc;
char bad_terrain;
{
	(void) bzero ((char *)cont_map, MAP_SIZE * sizeof (int));
	rmap_mark_up_cont (cont_map, loc, bad_terrain);
}

/*
Mark all squares of a continent and the squares that are adjacent
to the continent which are on the board.  Our passed location is
known to be either on the continent or adjacent to the continent.

Someday this should be tweaked to use perimeter lists.
*/

static void
rmap_mark_up_cont (cont_map, loc, bad_terrain)
int *cont_map;
long loc;
char bad_terrain;
{
	int i;
	long new_loc;
	
	if (!map[loc].on_board) return; /* off board */
	if (cont_map[loc]) return; /* already marked */
	if (map[loc].contents == bad_terrain) return; /* off continent */
	
	cont_map[loc] = 1; /* on continent */

	FOR_ADJ (loc, new_loc, i)
		rmap_mark_up_cont (cont_map, new_loc, bad_terrain);
}

/*
Scan a continent recording items of interest on the continent.

This could be done as we mark up the continent.
*/

#define COUNT(c,item) case c: item += 1; break

scan_counts_t
vmap_cont_scan (cont_map, vmap)
int *cont_map;
view_map_t *vmap;
{
	scan_counts_t counts;
	long i;

	(void) bzero ((char *)&counts, sizeof (scan_counts_t));
	
	for (i = 0; i < MAP_SIZE; i++) {
		if (cont_map[i]) { /* cell on continent? */
			counts.size += 1;
			
			switch (vmap[i].contents) {
			COUNT (' ', counts.unexplored);
			COUNT ('O', counts.user_cities);
			COUNT ('A', counts.user_objects[ARMY]);
			COUNT ('F', counts.user_objects[FIGHTER]);
			COUNT ('P', counts.user_objects[PATROL]);
			COUNT ('D', counts.user_objects[DESTROYER]);
			COUNT ('S', counts.user_objects[SUBMARINE]);
			COUNT ('T', counts.user_objects[TRANSPORT]);
			COUNT ('C', counts.user_objects[CARRIER]);
			COUNT ('B', counts.user_objects[BATTLESHIP]);
			COUNT ('X', counts.comp_cities);
			COUNT ('a', counts.comp_objects[ARMY]);
			COUNT ('f', counts.comp_objects[FIGHTER]);
			COUNT ('p', counts.comp_objects[PATROL]);
			COUNT ('d', counts.comp_objects[DESTROYER]);
			COUNT ('s', counts.comp_objects[SUBMARINE]);
			COUNT ('t', counts.comp_objects[TRANSPORT]);
			COUNT ('c', counts.comp_objects[CARRIER]);
			COUNT ('b', counts.comp_objects[BATTLESHIP]);
			COUNT ('*', counts.unowned_cities);
			case '+': break;
			case '.': break;
			default: /* check for city underneath */
				if (map[i].contents == '*') {
					switch (map[i].cityp->owner) {
					COUNT (USER, counts.user_cities);
					COUNT (COMP, counts.comp_cities);
					COUNT (UNOWNED, counts.unowned_cities);
					}
				}
			}
		}
	}
	return counts;
}

/*
Scan a real map as above.  Only the 'size' and 'unowned_cities'
fields are valid.
*/

scan_counts_t
rmap_cont_scan (cont_map)
int *cont_map;
{
	scan_counts_t counts;
	long i;

	(void) bzero ((char *)&counts, sizeof (scan_counts_t));
	
	for (i = 0; i < MAP_SIZE; i++) {
		if (cont_map[i]) { /* cell on continent? */
			counts.size += 1;
			if (map[i].contents == '*')
				counts.unowned_cities += 1;
		}
	}
	return counts;
}

/*
Return TRUE if a location is on the edge of a continent.
*/

int
map_cont_edge (cont_map, loc)
int *cont_map;
long loc;
{
	long i, j;

	if (!cont_map[loc]) return FALSE; /* not on continent */
 
	FOR_ADJ (loc, j, i)
		if (!cont_map[j]) return TRUE; /* edge of continent */

	return FALSE;
}

/*
Find the nearest objective for a piece.  This routine actually does
some real work.  This code represents my fourth rewrite of the
algorithm.  This algorithm is central to the strategy used by the
computer.

Given a view_map, we create a path_map.  On the path_map, we record
the distance from a location to the nearest objective.  We are
given information about what the interesting objectives are, and
how interesting each objective is.

We use a breadth first search to find the nearest objective.
We maintain something called a "perimeter list".  This list
initially contains a list of squares that we can reach in 'n' moves.
On each pass through our loop, we add all squares that are adjacent
to the perimeter list and which lie outside the perimeter to our
list.  (The loop is only slightly more complicated for armies and
transports.)

When our perimeter list becomes empty, or when the distance to
the current perimeter is at least as large as the weighted distance
to the best objective, we return the location of the best objective
found.

The 'cost' field in a path_map must be INFINITY if the cell lies
outside of the current perimeter.  The cost for cells that lie
on or within the current perimeter doesn't matter, except that
the information must be consistent with the needs of 'vmap_mark_path'.
*/

/* Find an objective over a single type of terrain. */

long
vmap_find_xobj (path_map, vmap, loc, move_info, start, expand)
path_map_t path_map[];
view_map_t *vmap;
long loc;
move_info_t *move_info;
int start;
int expand;
{
	perimeter_t *from;
	perimeter_t *to;
	int cur_cost;

	from = &p1;
	to = &p2;
	
	start_perimeter (path_map, from, loc, start);
	cur_cost = 0; /* cost to reach current perimeter */

	for (;;) {
		to->len = 0; /* nothing in perim yet */
		expand_perimeter (path_map, vmap, move_info, from, expand,
				  cur_cost, 1, 1, to, to);
		
		if (trace_pmap)
			print_pzoom ("After xobj loop:", path_map, vmap);

		cur_cost += 1;
		if (to->len == 0 || best_cost <= cur_cost)
			return best_loc;

		SWAP (from, to);
	}
}
	
/* Find an objective for a piece that crosses land and water. */

long
vmap_find_aobj (path_map, vmap, loc, move_info)
path_map_t path_map[];
view_map_t *vmap;
long loc;
move_info_t *move_info;
{
	return vmap_find_xobj (path_map, vmap, loc, move_info, T_LAND, T_AIR);
}

/* Find an objective for a piece that crosses only water. */

long
vmap_find_wobj (path_map, vmap, loc, move_info)
path_map_t path_map[];
view_map_t *vmap;
long loc;
move_info_t *move_info;
{
	return vmap_find_xobj (path_map, vmap, loc, move_info, T_WATER, T_WATER);
}

/* Find an objective for a piece that crosses only land. */

long
vmap_find_lobj (path_map, vmap, loc, move_info)
path_map_t path_map[];
view_map_t *vmap;
long loc;
move_info_t *move_info;
{
	return vmap_find_xobj (path_map, vmap, loc, move_info, T_LAND, T_LAND);
}

/*
Find an objective moving from land to water.
This is mildly complicated.  It costs 2 to move on land
and one to move on water.  To handle this, we expand our current
perimeter by one cell, where land can be expanded to either
land or water, and water is only expanded to water.  Then
we expand any water one more cell.

We have different objectives depending on whether the objective
is being approached from the land or the water.
*/

long
vmap_find_lwobj (path_map, vmap, loc, move_info, beat_cost)
path_map_t path_map[];
view_map_t *vmap;
long loc;
move_info_t *move_info;
int beat_cost;
{
	perimeter_t *cur_land;
	perimeter_t *cur_water;
	perimeter_t *new_land;
	perimeter_t *new_water;
	int cur_cost;

	cur_land = &p1;
	cur_water = &p2;
	new_water = &p3;
	new_land = &p4;
	
	start_perimeter (path_map, cur_land, loc, T_LAND);
	cur_water->len = 0;
	best_cost = beat_cost; /* we can do this well */
	cur_cost = 0; /* cost to reach current perimeter */

	for (;;) {
		/* expand current perimeter one cell */
		new_water->len = 0;
		new_land->len = 0;
		expand_perimeter (path_map, vmap, move_info, cur_water,
				  T_WATER, cur_cost, 1, 1, new_water, NULL);

		expand_perimeter (path_map, vmap, move_info, cur_land,
				  T_AIR, cur_cost, 1, 2, new_water, new_land);
				  
		/* expand new water one cell */
		cur_water->len = 0;
		expand_perimeter (path_map, vmap, move_info, new_water,
				  T_WATER, cur_cost+1, 1, 1, cur_water, NULL);
				  
		if (trace_pmap)
			print_pzoom ("After lwobj loop:", path_map, vmap);
		
		cur_cost += 2;
		if (cur_water->len == 0 && new_land->len == 0 || best_cost <= cur_cost) {
			return best_loc;
		}

		SWAP (cur_land, new_land);
	}
}

/*
Return the cost to reach the adjacent cell of the correct type
with the lowest cost.
*/

STATIC int
best_adj (pmap, loc, type)
path_map_t *pmap;
long loc;
int type;
{
	int i;
	long new_loc;
	int best;

	best = INFINITY;
	
	FOR_ADJ (loc, new_loc, i)
	if (pmap[new_loc].terrain == type && pmap[new_loc].cost < best)
			best = pmap[new_loc].cost;

	return best;
}

/*
Find an objective moving from water to land.
Here, we expand water to either land or water.
We expand land only to land.

We cheat ever so slightly, but this cheating accurately reflects
the mechanics o moving.  The first time we expand water we can
expand to land or water (army moving off tt or tt moving on water),
but the second time, we only expand water (tt taking its second move).
*/

long
vmap_find_wlobj (path_map, vmap, loc, move_info)
path_map_t path_map[];
view_map_t *vmap;
long loc;
move_info_t *move_info;
{
	perimeter_t *cur_land;
	perimeter_t *cur_water;
	perimeter_t *new_land;
	perimeter_t *new_water;
	int cur_cost;

	cur_land = &p1;
	cur_water = &p2;
	new_water = &p3;
	new_land = &p4;
	
	start_perimeter (path_map, cur_water, loc, T_WATER);
	cur_land->len = 0;
	cur_cost = 0; /* cost to reach current perimeter */

	for (;;) {
		/* expand current perimeter one cell */
		new_water->len = 0;
		new_land->len = 0;
		expand_perimeter (path_map, vmap, move_info, cur_water,
				  T_AIR, cur_cost, 1, 2, new_water, new_land);

		expand_perimeter (path_map, vmap, move_info, cur_land,
				  T_LAND, cur_cost, 1, 2, NULL, new_land);
				  
		/* expand new water one cell to water */
		cur_water->len = 0;
		expand_perimeter (path_map, vmap, move_info, new_water,
				  T_WATER, cur_cost+1, 1, 1, cur_water, NULL);
				  
		if (trace_pmap)
			print_pzoom ("After wlobj loop:", path_map, vmap);
		
		cur_cost += 2;
		if (cur_water->len == 0 && new_land->len == 0 || best_cost <= cur_cost) {
			return best_loc;
		}
		SWAP (cur_land, new_land);
	}
}

/*
Initialize the perimeter searching.

This routine was taking a significant amount of the program time (10%)
doing the initialization of the path map.  We now use an external
constant and 'memcpy'.
*/

static path_map_t pmap_init[MAP_SIZE];
static int init_done = 0;

STATIC void
start_perimeter (pmap, perim, loc, terrain)
path_map_t *pmap;
perimeter_t *perim;
long loc;
int terrain;
{
	int i;
	
	/* zap the path map */
	if (!init_done) {
		init_done = 1;
		for (i = 0; i < MAP_SIZE; i++) {
			pmap_init[i].cost = INFINITY; /* everything lies outside perim */
			pmap_init[i].terrain = T_UNKNOWN;
		}
	}
	(void) memcpy ((char *)pmap, (char *)pmap_init, sizeof (pmap_init));
	
	/* put first location in perimeter */
	pmap[loc].cost = 0;
	pmap[loc].inc_cost = 0;
	pmap[loc].terrain = terrain;

	perim->len = 1;
	perim->list[0] = loc;
	
	best_cost = INFINITY; /* no best yet */
	best_loc = loc; /* if nothing found, result is current loc */
}

/*
Expand the perimeter.

Note that 'waterp' and 'landp' may be the same.

For each cell of the current perimeter, we examine each
cell adjacent to that cell which lies outside of the current
perimeter.  If the adjacent cell is an objective, we update
best_cost and best_loc.  If the adjacent cell is of the correct
type, we turn place the adjacent cell in either the new water perimeter
or the new land perimeter.

We set the cost to reach the current perimeter.
*/

STATIC void
expand_perimeter (pmap, vmap, move_info, curp, type, cur_cost, inc_wcost, inc_lcost, waterp, landp)
path_map_t *pmap; /* path map to update */
view_map_t *vmap;
move_info_t *move_info; /* objectives and weights */
perimeter_t *curp; /* perimeter to expand */
int type; /* type of terrain to expand */
int cur_cost; /* cost to reach cells on perimeter */
int inc_wcost; /* cost to enter new water cells */
int inc_lcost; /* cost to enter new land cells */
perimeter_t *waterp; /* pointer to new water perimeter */
perimeter_t *landp; /* pointer to new land perimeter */
{
	register long i;
	register int j;
	long new_loc;
	int obj_cost;
	register int new_type;

	for (i = 0; i < curp->len; i++) /* for each perimeter cell... */
	FOR_ADJ_ON (curp->list[i], new_loc, j) {/* for each adjacent cell... */
		register path_map_t *pm = pmap + new_loc;

		if (pm->cost == INFINITY) {
			new_type = terrain_type (pmap, vmap, move_info, curp->list[i], new_loc);

			if (new_type == T_LAND && (type & T_LAND))
				add_cell (pmap, new_loc, landp, new_type, cur_cost, inc_lcost);
			else if (new_type == T_WATER && (type & T_WATER))
				add_cell (pmap, new_loc, waterp, new_type, cur_cost, inc_wcost);
			else if (new_type == T_UNKNOWN) { /* unreachable cell? */
				pm->terrain = new_type;
				pm->cost = cur_cost + INFINITY/2;
				pm->inc_cost = INFINITY/2;
			}
			if (pmap[new_loc].cost != INFINITY) { /* did we expand? */
				obj_cost = objective_cost (vmap, move_info, new_loc, cur_cost);
				if (obj_cost < best_cost) {
					best_cost = obj_cost;
					best_loc = new_loc;
					if (new_type == T_UNKNOWN) {
						pm->cost=cur_cost+2;
						pm->inc_cost = 2;
					}
				}
			}
		}
	}
}
			
/* Add a cell to a perimeter list. */
	
STATIC void
add_cell (pmap, new_loc, perim, terrain, cur_cost, inc_cost)
path_map_t *pmap;
long new_loc;
perimeter_t *perim;
int terrain;
int cur_cost;
int inc_cost;
{
	register	path_map_t	*pm = &pmap[new_loc];

	pm->terrain = terrain;
	pm->inc_cost = inc_cost;
	pm->cost = cur_cost + inc_cost;

	perim->list[perim->len] = new_loc;
	perim->len += 1;
}

/* Compute the cost to move to an objective. */

STATIC int
objective_cost (vmap, move_info, loc, base_cost)
view_map_t *vmap;
move_info_t *move_info;
long loc;
int base_cost;
{
	char *p;
	int w;
	city_info_t *cityp;

	p = strchr (move_info->objectives, vmap[loc].contents);
	if (!p) return INFINITY;

	w = move_info->weights[p - move_info->objectives];
	if (w >= 0) return w + base_cost;

	switch (w) {
	case W_TT_BUILD:
		/* handle special case of moving to tt building city */
		cityp = find_city (loc);
		if (!cityp) return base_cost + 2; /* tt is already here */
		if (cityp->prod != TRANSPORT) return base_cost + 2; /* just finished a tt */
	
		/* compute time to wait for tt to be built */
		w = piece_attr[TRANSPORT].build_time - cityp->work;
		w *= 2; /* had to cross land to get here */
		if (w < base_cost + 2) w = base_cost + 2;
		return w;

	default:
		ABORT;
		/* NOTREACHED */
	}
}

/*
Return the type of terrain at a vmap location.
*/

STATIC int
terrain_type (pmap, vmap, move_info, from_loc, to_loc)
path_map_t *pmap;
view_map_t *vmap;
move_info_t *move_info;
long from_loc;
long to_loc;
{
	if (vmap[to_loc].contents == '+') return T_LAND;
	if (vmap[to_loc].contents == '.') return T_WATER;
	if (vmap[to_loc].contents == '%') return T_UNKNOWN; /* magic objective */
	if (vmap[to_loc].contents == ' ') return pmap[from_loc].terrain;
	
	switch (map[to_loc].contents) {
	case '.': return T_WATER;
	case '+': return T_LAND;
	case '*':
		if (map[to_loc].cityp->owner == move_info->city_owner)
			return T_WATER;
		else return T_UNKNOWN; /* cannot cross */
	}
	ABORT;
	/*NOTREACHED*/
}

/*
Prune unexplored territory.  We take a view map and we modify it
so that unexplored territory that is adjacent to a lot of land
or a lot of water is marked as being either that land or water.
So basically, we are making a predicition about what we expect
for land and water.  We iterate this algorithm until either
the next iteration would remove all unexplored territory, or
there is nothing more about which we can make an assumption.

First, we use a pathmap to save the number of adjacent land
and water cells for each unexplored cell.  Cells which have
adjacent explored territory are placed in a perimeter list.
We also count the number of cells that are not unexplored.

We now take this perimeter list and make high-probability
predictions.

Then we round things off by making one pass of medium
probability predictions.

Then we make multiple passes extending our predictions.

We stop if at any point all remaining unexplored cells are
in a perimeter list, or if no predictions were made during
one of the final passes.

Unlike other algorithms, here we deal with "off board" locations.
So be careful.
*/

void
vmap_prune_explore_locs (vmap)
view_map_t *vmap;
{
	path_map_t pmap[MAP_SIZE];
	perimeter_t *from, *to;
	int explored;
	long loc, new_loc;
	long i;
	long copied;

	(void) bzero (pmap, sizeof (pmap));
	from = &p1;
	to = &p2;
	from->len = 0;
	explored = 0;
	
	/* build initial path map and perimeter list */
	for (loc = 0; loc < MAP_SIZE; loc++) {
		if (vmap[loc].contents != ' ') explored += 1;
		else { /* add unexplored cell to perim */
			FOR_ADJ (loc, new_loc, i) {
				if (new_loc < 0 || new_loc >= MAP_SIZE); /* ignore off map */
				else if (vmap[new_loc].contents == ' '); /* ignore adjacent unexplored */
				else if (map[new_loc].contents != '.')
					pmap[loc].cost += 1; /* count land */
				else pmap[loc].inc_cost += 1; /* count water */
			}
			if (pmap[loc].cost || pmap[loc].inc_cost) {
				from->list[from->len] = loc;
				from->len += 1;
			}
		}
	}
				
	if (print_vmap == 'I') print_xzoom (vmap);
		
	for (;;) { /* do high probability predictions */
		if (from->len + explored == MAP_SIZE) return;
		to->len = 0;
		copied = 0;
		
		for (i = 0; i < from->len; i++) {
			loc = from->list[i];
			if (pmap[loc].cost >= 5)
				expand_prune (vmap, pmap, loc, T_LAND, to, &explored);
			else if (pmap[loc].inc_cost >= 5)
				expand_prune (vmap, pmap, loc, T_WATER, to, &explored);
			else if ((loc < MAP_WIDTH || loc >= MAP_SIZE-MAP_WIDTH) && pmap[loc].cost >= 3)
				expand_prune (vmap, pmap, loc, T_LAND, to, &explored);
			else if ((loc < MAP_WIDTH || loc >= MAP_SIZE-MAP_WIDTH) && pmap[loc].inc_cost >= 3)
				expand_prune (vmap, pmap, loc, T_WATER, to, &explored);
			else if ((loc == 0 || loc == MAP_SIZE-1) && pmap[loc].cost >= 2)
				expand_prune (vmap, pmap, loc, T_LAND, to, &explored);
			else if ((loc == 0 || loc == MAP_SIZE-1) && pmap[loc].inc_cost >= 2)
				expand_prune (vmap, pmap, loc, T_WATER, to, &explored);
			else { /* copy perimeter cell */
				to->list[to->len] = loc;
				to->len += 1;
				copied += 1;
			}
		}
		if (copied == from->len) break; /* nothing expanded */
		SWAP (from, to);
	}
	
	if (print_vmap == 'I') print_xzoom (vmap);
		
	/* one pass for medium probability predictions */
	if (from->len + explored == MAP_SIZE) return;
	to->len = 0;
	
	for (i = 0; i < from->len; i++) {
		loc = from->list[i];
		if (pmap[loc].cost > pmap[loc].inc_cost)
			expand_prune (vmap, pmap, loc, T_LAND, to, &explored);
		else if (pmap[loc].cost < pmap[loc].inc_cost)
			expand_prune (vmap, pmap, loc, T_WATER, to, &explored);
		else { /* copy perimeter cell */
			to->list[to->len] = loc;
			to->len += 1;
		}
	}
	SWAP (from, to);

	if (print_vmap == 'I') print_xzoom (vmap);
		
	/* multiple low probability passes */
	for (;;) {
		/* return if very little left to explore */
		if (from->len + explored >= MAP_SIZE - MAP_HEIGHT) {
			if (print_vmap == 'I') print_xzoom (vmap);
			return;
		}
		to->len = 0;
		copied = 0;
		
		for (i = 0; i < from->len; i++) {
			loc = from->list[i];
			if (pmap[loc].cost >= 4 && pmap[loc].inc_cost < 4)
				expand_prune (vmap, pmap, loc, T_LAND, to, &explored);
			else if (pmap[loc].inc_cost >= 4 && pmap[loc].cost < 4)
				expand_prune (vmap, pmap, loc, T_WATER, to, &explored);
			else if ((loc < MAP_WIDTH || loc >= MAP_SIZE-MAP_WIDTH) && pmap[loc].cost > pmap[loc].inc_cost)
				expand_prune (vmap, pmap, loc, T_LAND, to, &explored);
			else if ((loc < MAP_WIDTH || loc >= MAP_SIZE-MAP_WIDTH) && pmap[loc].inc_cost > pmap[loc].cost)
				expand_prune (vmap, pmap, loc, T_WATER, to, &explored);
			else { /* copy perimeter cell */
				to->list[to->len] = loc;
				to->len += 1;
				copied += 1;
			}
		}
		if (copied == from->len) break; /* nothing expanded */
		SWAP (from, to);
	}
	if (print_vmap == 'I') print_xzoom (vmap);
}

/*
Expand an unexplored cell.  We increment the land or water count
of each neighbor.  Any neighbor that acquires a non-zero count
is added to the 'to' perimiter list.  The count of explored
territory is incremented.

Careful:  'loc' may be "off board".
*/

STATIC void
expand_prune (vmap, pmap, loc, type, to, explored)
view_map_t *vmap;
path_map_t *pmap;
long loc;
int type;
perimeter_t *to;
int *explored;
{
	int i;
	long new_loc;
	
	*explored += 1;
	
	if (type == T_LAND) vmap[loc].contents = '+';
	else vmap[loc].contents = '.';
	
	FOR_ADJ (loc, new_loc, i)
	if (new_loc >= 0 && new_loc < MAP_SIZE && vmap[new_loc].contents == ' ') {
		if (!pmap[new_loc].cost && !pmap[new_loc].inc_cost) {
			to->list[to->len] = new_loc;
			to->len += 1;
		}
		if (type == T_LAND)
			pmap[new_loc].cost += 1;
		else pmap[new_loc].inc_cost += 1;
	}
}
	
/*
Find the shortest path from the current location to the
destination which passes over valid terrain.  We return
the destination if a path exists.  Otherwise we return the
origin.

This is similar to 'find_objective' except that we know our destination.
*/

long
vmap_find_dest (path_map, vmap, cur_loc, dest_loc, owner, terrain)
path_map_t path_map[];
view_map_t vmap[];
long cur_loc; /* current location of piece */
long dest_loc; /* destination of piece */
int owner; /* owner of piece being moved */
int terrain; /* terrain we can cross */
{
	perimeter_t *from;
	perimeter_t *to;
	int cur_cost;
	int start_terrain;
	move_info_t move_info;
	char old_contents;

	old_contents = vmap[dest_loc].contents;
	vmap[dest_loc].contents = '%'; /* mark objective */
	move_info.city_owner = owner;
	move_info.objectives = "%";
	move_info.weights[0] = 1;

	from = &p1;
	to = &p2;
	
	if (terrain == T_AIR) start_terrain = T_LAND;
	else start_terrain = terrain;
	
	start_perimeter (path_map, from, cur_loc, start_terrain);
	cur_cost = 0; /* cost to reach current perimeter */

	for (;;) {
		to->len = 0; /* nothing in perim yet */
		expand_perimeter (path_map, vmap, &move_info, from,
				  terrain, cur_cost, 1, 1, to, to);
		cur_cost += 1;
		if (to->len == 0 || best_cost <= cur_cost) {
			vmap[dest_loc].contents = old_contents;
			return best_loc;
		}
		SWAP (from, to);
	}
}

/*
Starting with the destination, we recursively back track toward the source
marking all cells which are on a shortest path between the start and the
destination.  To do this, we know the distance from the destination to
the start.  The destination is on a path.  We then find the cells adjacent
to the destination and nearest to the source and place them on the path.

If we know square P is on the path, then S is on the path if S is
adjacent to P, the cost to reach S is less than the cost to reach P,
and the cost to move from S to P is the difference in cost between
S and P.

Someday, this routine should probably use perimeter lists as well.
*/

void
vmap_mark_path (path_map, vmap, dest)
path_map_t *path_map;
view_map_t *vmap;
long dest;
{
	int n;
	long new_dest;

	if (path_map[dest].cost == 0) return; /* reached end of path */
	if (path_map[dest].terrain == T_PATH) return; /* already marked */

	path_map[dest].terrain = T_PATH; /* this square is on path */

	/* loop to mark adjacent squares on shortest path */
	FOR_ADJ (dest, new_dest, n)
	if (path_map[new_dest].cost == path_map[dest].cost - path_map[dest].inc_cost)
			vmap_mark_path (path_map, vmap, new_dest);

}

/*
Create a marked path map.  We mark those squares adjacent to the
starting location which are on the board.  'find_dir' must be
invoked to decide which squares are actually valid.
*/

void
vmap_mark_adjacent (path_map, loc)
path_map_t path_map[];
long loc;
{
	int i;
	long new_loc;

	FOR_ADJ_ON (loc, new_loc, i)
		path_map[new_loc].terrain = T_PATH;
}

/*
Modify a marked path map.  We mark those squares adjacent to the
starting location which are on the board and which are adjacent
to a location on the existing shortest path.
*/

void
vmap_mark_near_path (path_map, loc)
path_map_t path_map[];
long loc;
{
	int i, j;
	long new_loc, xloc;
	int hit_loc[8];

	(void) bzero ((char *)hit_loc, sizeof(int)*8);
	
	FOR_ADJ_ON (loc, new_loc, i) {
		FOR_ADJ_ON (new_loc, xloc, j)
		if (xloc != loc && path_map[xloc].terrain == T_PATH) {
			hit_loc[i] = 1;
			break;
		}
	}
	for (i = 0; i < 8; i++)
	if (hit_loc[i])
	path_map[loc + dir_offset[i]].terrain = T_PATH;
}

/*
Look at each neighbor of 'loc'.  Select the first marked cell which
is on a short path to the desired destination, and which holds a valid
terrain.  Note that while this terrain is matched against a 'vmap',
it differs slightly from terrains used above.  This terrain is the
terrain to which we can move immediately, and does not include terrain
for which we would have to wait for another piece to move off of.

We prefer diagonal moves, and we try to have as many squares
as possible containing something in 'adj_char'.

For tie-breaking, we prefer moving to cells that are adjacent to
as many other squares on the path.  This should have a few benefits:

1)  Fighters are less likely to be blocked from reaching a city
because they stay in the center of the path and increase the number
of options for subsequent moves.

2)  Transports will approach a city so that as many armies
as possible can hop off the tt on one turn to take a valid
path toward the city.

3)  User pieces will move more intuitively by staying in the
center of the best path.
*/

static int order[] = {NORTHWEST, NORTHEAST, SOUTHWEST, SOUTHEAST, 
			WEST, EAST, NORTH, SOUTH};

long
vmap_find_dir (path_map, vmap, loc, terrain, adj_char)
path_map_t path_map[];
view_map_t *vmap;
long loc;
char *terrain;
char *adj_char;
{
	int i, count, bestcount;
	long bestloc, new_loc;
	int path_count, bestpath;
	char *p;
	
	if (trace_pmap)
		print_pzoom ("Before vmap_find_dir:", path_map, vmap);
		
	bestcount = -INFINITY; /* no best yet */
	bestpath = -1;
	bestloc = loc;
	
	for (i = 0; i < 8; i++) { /* for each adjacent square */
		new_loc = loc + dir_offset[order[i]];
		if (path_map[new_loc].terrain == T_PATH) { /* which is on path */
			p = strchr (terrain, vmap[new_loc].contents);
			
			if (p != NULL) { /* desirable square? */
				count = vmap_count_adjacent (vmap, new_loc, adj_char);
				path_count = vmap_count_path (path_map, new_loc);
				
				/* remember best location */
				if (count > bestcount
				    || count == bestcount && path_count > bestpath) {
					bestcount = count;
					bestpath = path_count;
					bestloc = new_loc;
				}
			}
		}
	}
	return (bestloc);
}
	
/*
Count the number of adjacent squares of interest.
Squares are weighted so that the first in the list
is the most interesting.
*/

int
vmap_count_adjacent (vmap, loc, adj_char)
view_map_t *vmap;
long loc;
char *adj_char;
{
	int i, count;
	long new_loc;
	char *p;
	int len;

	len = strlen (adj_char);

	count = 0;
	
	FOR_ADJ_ON (loc, new_loc, i) {
		p = strchr (adj_char, vmap[new_loc].contents);
		if (p) count += 8 * (len - (p - adj_char));
	}
	return (count);
}

/*
Count the number of adjacent cells that are on the path.
*/

int
vmap_count_path (pmap, loc)
path_map_t *pmap;
long loc;
{
	int i, count;
	long new_loc;

	count = 0;
	
	FOR_ADJ_ON (loc, new_loc, i)
	if (pmap[new_loc].terrain == T_PATH)
		count += 1;

	return (count);
}

/*
See if a location is on the shore.  We return true if a surrounding
cell contains water and is on the board.
*/

int
rmap_shore (loc)
long loc;
{
	long i, j;

	FOR_ADJ_ON (loc, j, i)
	if (map[j].contents == '.') return (TRUE);

	return (FALSE);
}

int
vmap_shore (vmap, loc)
view_map_t *vmap;
long loc;
{
	long i, j;

	FOR_ADJ_ON (loc, j, i)
	if (vmap[j].contents != ' ' && vmap[j].contents != '+' && map[j].contents == '.')
			return (TRUE);

	return (FALSE);
}

/*
Return true if a location is surrounded by ocean.  Off board locations
which cannot be moved to are treated as ocean.
*/

int
vmap_at_sea (vmap, loc)
view_map_t *vmap;
long loc;
{
	long i, j;

	FOR_ADJ_ON (loc, j, i)
	if (vmap[j].contents == ' ' || vmap[j].contents == '+' || map[j].contents != '.')
			return (FALSE);

	return (TRUE);
}

int
rmap_at_sea (loc)
long loc;
{
	long i, j;

	FOR_ADJ_ON (loc, j, i) {
		if (map[j].contents != '.') return (FALSE);
	}
	return (TRUE);
}

