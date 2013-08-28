/* %W% %G% %U% - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
compmove.c -- Make a move for the computer. 

For each move the user wants us to make, we do the following:

    1)  Handle city production;
    2)  Move computer's pieces;
    3)  Check to see if the game is over.
*/

#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif

#include <curses.h>
#include "empire.h"
#include "extern.h"

static view_map_t emap[MAP_SIZE]; /* pruned explore map */

int load_army(piece_info_t *obj);
void move_objective(piece_info_t *obj,path_map_t pathmap[],long new_loc,char *adj_list);

void
comp_move (nmoves) 
int nmoves;
{
	void do_cities(), do_pieces(), check_endgame();

	int i;
	piece_info_t *obj;

	/* Update our view of the world. */
	
	for (i = 0; i < NUM_OBJECTS; i++)
	for (obj = comp_obj[i]; obj != NULL; obj = obj->piece_link.next)
		scan (comp_map, obj->loc); /* refresh comp's view of world */

	for (i = 1; i <= nmoves; i++) { /* for each move we get... */
		comment ("Thinking...",0,0,0,0,0,0,0,0);

		(void) memcpy (emap, comp_map, MAP_SIZE * sizeof (view_map_t));
		vmap_prune_explore_locs (emap);
	
		do_cities (); /* handle city production */
		do_pieces (); /* move pieces */
		
		if (save_movie) save_movie_screen ();
		check_endgame (); /* see if game is over */

		topini ();
		(void) refresh ();
	}
}

/*
Handle city production.  First, we set production for new cities.
Then we produce new pieces.  After producing a piece, we will see
if we should change our production.

Our goals for city production are first, not to change production
while something is in the process of being built.  Second, we attempt
to have enough city producing armies on a continent to counter any
threat on the continent, and to adequately explore and control the
continent.  Third, we attempt to always have at least one transport
producer.  Fourth, we attempt to maintain a good ratio between the
number of producers we have of each type of piece.  Fifth, we never
build carriers, as we don't have a good strategy for moving these.
*/

void
do_cities () {
	void comp_prod();
	
	int i;
	int is_lake;

	for (i = 0; i < NUM_CITY; i++) /* new production */
	if (city[i].owner == COMP) {
		scan (comp_map, city[i].loc);

		if (city[i].prod == NOPIECE)
			comp_prod (&city[i], lake (city[i].loc));
	}
	for (i = 0; i < NUM_CITY; i++) /* produce and change */
	if (city[i].owner == COMP) {
		is_lake = lake (city[i].loc);
		if (city[i].work++ >= (long)piece_attr[city[i].prod].build_time) {
			produce (&city[i]);
			comp_prod (&city[i], is_lake);
		}
		/* don't produce ships in lakes */
		else if (city[i].prod > FIGHTER && city[i].prod != SATELLITE && is_lake)
			comp_prod (&city[i], is_lake);
	}
}
			
/*
Define ratios for numbers of cities we want producing each object.

Early in the game, we want to produce armies and transports for
rapid expansion.  After a while, we add fighters and pt boats
for exploration.  Later, we add ships of all kinds for control of
the sea.
*/
                                 /* A    F    P    S    D    T    C    B   Z*/
static int ratio1[NUM_OBJECTS] = { 60,   0,  10,   0,   0,  20,   0,   0,  0};
static int ratio2[NUM_OBJECTS] = { 90,  10,  10,  10,  10,  40,   0,   0,  0};
static int ratio3[NUM_OBJECTS] = {120,  20,  20,  10,  10,  60,  10,  10,  0};
static int ratio4[NUM_OBJECTS] = {150,  30,  30,  20,  20,  70,  10,  10,  0};
static int *ratio;

/*
Set city production if necessary.

The algorithm below contains three parts:

1)  Defend continents we own.

2)  Produce a TT and a Satellite.

3)  Meet the ratio requirements given above.
*/

void
comp_prod (cityp, is_lake)
city_info_t *cityp;
int is_lake;
{
	void comp_set_prod(), comp_set_needed();
	
	int city_count[NUM_OBJECTS]; /* # of cities producing each piece */
	int cont_map[MAP_SIZE];
	int total_cities;
	long i;
	int comp_ac;
	city_info_t *p;
	int need_count, interest;
	scan_counts_t counts;

	/* Make sure we have army producers for current continent. */
	
	/* map out city's continent */
	vmap_cont (cont_map, comp_map, cityp->loc, '.');

	/* count items of interest on the continent */
	counts = vmap_cont_scan (cont_map, comp_map);
	comp_ac = 0; /* no army producing computer cities */
	
	for (i = 0; i < MAP_SIZE; i++)
	if (cont_map[i]) { /* for each cell of continent */
		if (comp_map[i].contents == 'X') {
			p = find_city (i);
			ASSERT (p != NULL && p->owner == COMP);
			if (p->prod == ARMY) comp_ac += 1;
		}
	}
	/* see if anything of interest is on continent */
	interest = (counts.unexplored || counts.user_cities
		 || counts.user_objects[ARMY]
		 || counts.unowned_cities);
	
	/* we want one more army producer than enemy has cities */
	/* and one more if anything of interest on cont */
	need_count = counts.user_cities - comp_ac + interest;
	if (counts.user_cities) need_count += 1;
	
	if (need_count > 0) { /* need an army producer? */
		comp_set_prod (cityp, ARMY);
		return;
	}
	
	/* Produce armies in new cities if there is a city to attack. */
	if (counts.user_cities && cityp->prod == NOPIECE) {
		comp_set_prod (cityp, ARMY);
		return;
	}

	/* Produce a TT and SAT if we don't have one. */
	
	/* count # of cities producing each piece */
	for (i = 0; i < NUM_OBJECTS; i++)
		city_count[i] = 0;
		
	total_cities = 0;
		
	for (i = 0; i < NUM_CITY; i++)
	if (city[i].owner == COMP && city[i].prod != NOPIECE) {
		city_count[city[i].prod] += 1;
		total_cities += 1;
	}
	if (total_cities <= 10) ratio = ratio1;
	else if (total_cities <= 20) ratio = ratio2;
	else if (total_cities <= 30) ratio = ratio3;
	else ratio = ratio4;
		
	/* if we have one army producer, and this is it, return */
	if (city_count[ARMY] == 1 && cityp->prod == ARMY) return;
	
	/* first available non-lake becomes a tt producer */
	if (city_count[TRANSPORT] == 0) {
		if (!is_lake) {
			comp_set_prod (cityp, TRANSPORT);
			return;
		}
		/* if we have one army producer that is not on a lake, */
		/* produce armies here instead */
		if (city_count[ARMY] == 1) {
			for (i = 0; i < NUM_CITY; i++)
			if (city[i].owner == COMP && city[i].prod == ARMY) break;
		
			if (!lake (city[i].loc)) {
				comp_set_prod (cityp, ARMY);
				return;
			}
		}
	}
#if 0
	/* Now we need a SATELLITE. */
	if (cityp->prod == NOPIECE && city_count[SATELLITE] == 0) {
		comp_set_prod (cityp, SATELLITE);
		return;
	}
	if (cityp->prod == SATELLITE) return;
	/* "The satellites are out tonight." -- Lori Anderson */
#endif
	/* don't change prod from armies if something on continent */
	if (cityp->prod == ARMY && interest) return;
			
	/* Produce armies in new cities if there is a city to attack. */
	if (counts.unowned_cities && cityp->prod == NOPIECE) {
		comp_set_prod (cityp, ARMY);
		return;
	}

	/* Set production to item most needed.  Continents with one
	city and nothing interesting may not produce armies.  We
	set production for unset cities, and change production for
	cities that produce objects for which we have many city producers.
	Ship producers on lakes also get there production changed. */
	
	interest = (counts.comp_cities != 1 || interest);
	
	if (cityp->prod == NOPIECE
	    || cityp->prod == ARMY && counts.comp_cities == 1
	    || overproduced (cityp, city_count)
	    || cityp->prod > FIGHTER && is_lake)
		comp_set_needed (cityp, city_count, interest, is_lake);
}
	
/*
Set production for a computer city to a given type.  Don't
reset production if it is already correct.
*/

void
comp_set_prod (cityp, type)
city_info_t *cityp;
int type;
{
	if (cityp->prod == type) return;
	
	pdebug ("Changing city prod at %d from %d to %d\n",cityp->loc, cityp->prod, type,0,0,0,0,0);
	cityp->prod = type;
	cityp->work = -(piece_attr[type].build_time / 5);
}

/*
See if a city is producing an object which is being overproduced.
*/

int
overproduced (cityp, city_count)
city_info_t *cityp;
int *city_count;
{
	int i;

	for (i = 0; i < NUM_OBJECTS; i++) {
		/* return true if changing production would improve balance */
		if (i != cityp->prod
		 && ((city_count[cityp->prod] - 1) * ratio[i]
		   > (city_count[i] + 1) * ratio[cityp->prod]))
  		return (TRUE);
	}
	return (FALSE);
}

/*
See if one type of production is needed more than another type.
Return the most-needed type of production.
*/

int
need_more (city_count, prod1, prod2)
int *city_count;
int prod1, prod2;
{
	if (city_count[prod1] * ratio[prod2]
		 <= city_count[prod2] * ratio[prod1])
	return (prod1);

	else return (prod2);
}

/*
Figure out the most needed type of production.  We are passed
a flag telling us if armies are ok to produce.
*/

void
comp_set_needed (cityp, city_count, army_ok, is_lake)
city_info_t *cityp;
int *city_count;
int army_ok;
int is_lake;
{
	int best_prod;
	int prod;

	if (!army_ok) city_count[ARMY] = INFINITY;

	if (is_lake) { /* choose fighter or army */
		comp_set_prod (cityp, need_more (city_count, ARMY, FIGHTER));
		return;
	}
	/* don't choose fighter */
	city_count[FIGHTER] = INFINITY;
	
	best_prod = ARMY; /* default */
	for (prod = 0; prod < NUM_OBJECTS; prod++) {
		best_prod = need_more (city_count, best_prod, prod);
	}
	comp_set_prod (cityp, best_prod);
}

/*
See if a city is on a lake.  We define a lake to be a body of
water (where cities are considered to be water) that does not
touch either an attackable city or unexplored territory.

Be careful, because we use the 'emap'.  This predicts whether
unexplored territory will be land or water.  The prediction should
be helpful, because small bodies of water that enclose unexplored
territory will appear as solid water.  Big bodies of water should
have unexplored territory on the edges.
*/

int
lake (loc)
long loc;
{
	int cont_map[MAP_SIZE];
	scan_counts_t counts;

	vmap_cont (cont_map, emap, loc, '+'); /* map lake */
	counts = vmap_cont_scan (cont_map, emap);

	return !(counts.unowned_cities || counts.user_cities || counts.unexplored);
}

/*
Move all computer pieces.
*/

static view_map_t amap[MAP_SIZE]; /* temp view map */
static path_map_t path_map[MAP_SIZE];

void
do_pieces () { /* move pieces */
	void cpiece_move();

	int i;
	piece_info_t *obj, *next_obj;

	for (i = 0; i < NUM_OBJECTS; i++) { /* loop through obj lists */
		for (obj = comp_obj[move_order[i]]; obj != NULL;
		    obj = next_obj) { /* loop through objs in list */
			next_obj = obj->piece_link.next;
			cpiece_move (obj); /* yup; move the object */
		}
	}
}

/*
Move a piece.  We loop until all the moves of a piece are made.  Within
the loop, we find a direction to move that will take us closer to an
objective.
*/

void
cpiece_move (obj)
piece_info_t *obj;
{
	void move1();

	int changed_loc;
	int max_hits;
	long saved_loc;
	city_info_t *cityp;

	if (obj->type == SATELLITE) {
		move_sat (obj);
		return;
	}
	
	obj->moved = 0; /* not moved yet */
	changed_loc = FALSE; /* not changed yet */
	max_hits = piece_attr[obj->type].max_hits;

	if (obj->type == FIGHTER) { /* init fighter range */
		cityp = find_city (obj->loc);
		if (cityp != NULL) obj->range = piece_attr[FIGHTER].range;
	}
	
	while (obj->moved < obj_moves (obj)) {
		saved_loc = obj->loc; /* remember starting location */
		move1 (obj);
		if (saved_loc != obj->loc) changed_loc = TRUE;
		
		if (obj->type == FIGHTER && obj->hits > 0) {
			if (comp_map[obj->loc].contents == 'X')
				obj->moved = piece_attr[FIGHTER].speed;
			else if (obj->range == 0) {
				pdebug ("Fighter at %d crashed and burned\n", obj->loc,0,0,0,0,0,0,0);
				ksend ("Fighter at %d crashed and burned\n", obj->loc,0,0,0,0,0,0,0);
				kill_obj (obj, obj->loc); /* crash & burn */
			}
		}
	}
	/* if a boat is in port, damaged, and never moved, fix some damage */
	if (obj->hits > 0 /* live piece? */
		&& !changed_loc /* object never changed location? */
		&& obj->type != ARMY && obj->type != FIGHTER /* it is a boat? */
		&& obj->hits != max_hits /* it is damaged? */
		&& comp_map[obj->loc].contents == 'X') /* it is in port? */
	obj->hits++; /* fix some damage */
}

/*
Move a piece one square.
*/

void
move1 (obj)
piece_info_t *obj;
{
	void army_move(), transport_move(), fighter_move(), ship_move();

	switch (obj->type) {
	case ARMY: army_move (obj); break;
	case TRANSPORT: transport_move (obj); break;
	case FIGHTER: fighter_move (obj); break;
	default: ship_move (obj); break;
	}
}

/*
Move an army.

This is a multi-step algorithm:

1)  See if there is an object we can attack immediately.
If so, attack it.

2)  Look for the nearest land objective.

3)  If we find an objective reachable by land, figure out
how far away that objective is.  Based on the found objective,
also figure out how close a loadable tt must be to be of
interest.  If the objective is closer than the tt must be,
head towards the objective.

4)  Otherwise, look for the nearest loading tt (or tt producing
city).  If the nearest loading tt is farther than our land objective,
head towards the land objective.

5)  Otherwise, head for the tt.

6)  If we still have no destination and we are in a city,
attempt to leave the city.

7)  Once we have a destination, find the best move toward that
destination.  (If there is no destination, sit around and wait.)
*/

void
army_move (obj)
piece_info_t *obj;
{
	long move_away();
	long find_attack();
	void make_army_load_map(), make_unload_map(), make_tt_load_map();
	void board_ship();
	
	long new_loc;
	path_map_t path_map2[MAP_SIZE];
	long new_loc2;
	int cross_cost; /* cost to enter water */
	
	obj->func = 0; /* army doesn't want a tt */
	if (vmap_at_sea (comp_map, obj->loc)) { /* army can't move? */
		(void) load_army (obj);
		obj->moved = piece_attr[ARMY].speed;
		if (!obj->ship) obj->func = 1; /* load army on ship */
		return;
	}
	if (obj->ship) /* is army on a transport? */
		new_loc = find_attack (obj->loc, army_attack, "+*");
	else new_loc = find_attack (obj->loc, army_attack, ".+*");
		
	if (new_loc != obj->loc) { /* something to attack? */
		attack (obj, new_loc); /* attack it */
		if (map[new_loc].contents == '.' /* moved to ocean? */
		  && obj->hits > 0) { /* object still alive? */
			kill_obj (obj, new_loc);
			scan (user_map, new_loc); /* rescan for user */
		}
		return;
	}
	if (obj->ship) {
		if (obj->ship->func == 0) {
			if (!load_army (obj)) ABORT; /* load army on best ship */
			return; /* armies stay on a loading ship */
		}
		make_unload_map (amap, comp_map);
		new_loc = vmap_find_wlobj (path_map, amap, obj->loc, &tt_unload);
		move_objective (obj, path_map, new_loc, " ");
		return;
	}

	new_loc = vmap_find_lobj (path_map, comp_map, obj->loc, &army_fight);
	
	if (new_loc != obj->loc) { /* something interesting on land? */
		switch (comp_map[new_loc].contents) {
		case 'A':
		case 'O':
			cross_cost = 60; /* high cost if enemy present */
			break;
		case '*':
			cross_cost = 30; /* medium cost for attackable city */
			break;
		case ' ':
			cross_cost = 14; /* low cost for exploring */
			break;
		default:
			ABORT;
		}
		cross_cost = path_map[new_loc].cost * 2 - cross_cost;
	}
	else cross_cost = INFINITY;
	
	if (new_loc == obj->loc || cross_cost > 0) {
		/* see if there is something interesting to load */
		make_army_load_map (obj, amap, comp_map);
		new_loc2 = vmap_find_lwobj (path_map2, amap, obj->loc, &army_load, cross_cost);
		
		if (new_loc2 != obj->loc) { /* found something? */
			board_ship (obj, path_map2, new_loc2);
			return;
		}
	}

	move_objective (obj, path_map, new_loc, " ");
}

/*
Remove pruned explore locs from a view map.
*/

void
unmark_explore_locs (xmap)
view_map_t *xmap;
{
	long i;

	for (i = 0; i < MAP_SIZE; i++)
	if (map[i].on_board && xmap[i].contents == ' ')
		xmap[i].contents = emap[i].contents;
}

/*
Make a load map.  We copy the view map and mark each loading
transport and tt producing city with a '$'.
*/

void
make_army_load_map (obj, xmap, vmap)
piece_info_t *obj;
view_map_t *xmap;
view_map_t *vmap;
{
	piece_info_t *p;
	int i;
	
	(void) memcpy (xmap, vmap, sizeof (view_map_t) * MAP_SIZE);

	/* mark loading transports or cities building transports */
	for (p = comp_obj[TRANSPORT]; p; p = p->piece_link.next)
	if (p->func == 0) /* loading tt? */
	xmap[p->loc].contents = '$';
	
	for (i = 0; i < NUM_CITY; i++)
	if (city[i].owner == COMP && city[i].prod == TRANSPORT) {
		if (nearby_load (obj, city[i].loc))
			xmap[city[i].loc].contents = 'x'; /* army is nearby so it can load */
		else if (nearby_count (city[i].loc) < piece_attr[TRANSPORT].capacity)
			xmap[city[i].loc].contents = 'x'; /* city needs armies */
	}
	
	if (print_vmap == 'A') print_xzoom (xmap);
}

/* Return true if an army is considered near a location for loading. */

int
nearby_load (obj, loc)
piece_info_t *obj;
long loc;
{
	return obj->func == 1 && dist (obj->loc, loc) <= 2;
}
	
/* Return number of nearby armies. */

int
nearby_count (loc)
long loc;
{
	piece_info_t *obj;
	int count;

	count = 0;
	for (obj = comp_obj[ARMY]; obj; obj = obj->piece_link.next) {
		if (nearby_load (obj, loc)) count += 1;
	}
	return count;
}

/* Make load map for a ship. */

void
make_tt_load_map (xmap, vmap)
view_map_t *xmap;
view_map_t *vmap;
{
	piece_info_t *p;
	
	(void) memcpy (xmap, vmap, sizeof (view_map_t) * MAP_SIZE);

	/* mark loading armies */
	for (p = comp_obj[ARMY]; p; p = p->piece_link.next)
	if (p->func == 1) /* loading army? */
	xmap[p->loc].contents = '$';
	
	if (print_vmap == 'L') print_xzoom (xmap);
}
	
/*
Make an unload map.  We copy the view map.  We then create
a continent map.  For each of our cities, we mark out the continent
that city is on.  Then, for each city that we don't own and which
doesn't appear on our continent map, we set that square to a digit.

We want to assign weights to each attackable city.
Cities are more valuable if they are on a continent which
has lots of cities.  Cities are also valuable if either it
will be easy for us to take over the continent, or if we
need to defend that continent from an enemy.

To implement the above, we assign numbers to each city as follows:

a)  if unowned_cities > user_cities && comp_cities == 0
       set number to min (total_cities, 9)

b)  if comp_cities != 0 && user_cities != 0
       set number to min (total_cities, 9)
       
b)  if enemy_cities == 1 && total_cities == 1, set number to 2.
    (( taking the sole enemy city on a continent is as good as
       getting a two city continent ))
       
c)  Any other attackable city is marked with a '0'.
*/

static int owncont_map[MAP_SIZE];
static int tcont_map[MAP_SIZE];

void
make_unload_map (xmap, vmap)
view_map_t *xmap;
view_map_t *vmap;
{
	long i;
	scan_counts_t counts;

	(void) memcpy (xmap, vmap, sizeof (view_map_t) * MAP_SIZE);
	unmark_explore_locs (xmap);
	
	for (i = 0; i < MAP_SIZE; i++)
		owncont_map[i] = 0; /* nothing marked */

	for (i = 0; i < NUM_CITY; i++)
	if (city[i].owner == COMP)
	vmap_mark_up_cont (owncont_map, xmap, city[i].loc, '.');

	for (i = 0; i < MAP_SIZE; i++)
	if (strchr ("O*", vmap[i].contents)) {
		int total_cities;
		
		vmap_cont (tcont_map, xmap, i, '.'); /* map continent */
		counts = vmap_cont_scan (tcont_map, xmap);
		
		total_cities = counts.unowned_cities
			     + counts.user_cities
			     + counts.comp_cities;
			     
		if (total_cities > 9) total_cities = 0;
		
		if (counts.user_cities && counts.comp_cities)
			xmap[i].contents = '0' + total_cities;

		else if (counts.unowned_cities > counts.user_cities
			 && counts.comp_cities == 0)
			xmap[i].contents = '0' + total_cities;

		else if (counts.user_cities == 1 && counts.comp_cities == 0)
			xmap[i].contents = '2';
			
		else xmap[i].contents = '0';
	}
	if (print_vmap == 'U') print_xzoom (xmap);
}

/*
Load an army onto a ship.  First look for an adjacent ship.
If that doesn't work, move to the objective, trying to be
close to the ocean.
*/

void
board_ship (obj, pmap, dest)
piece_info_t *obj;
path_map_t *pmap;
long dest;
{
	if (!load_army (obj)) {
		obj->func = 1; /* loading */
		move_objective (obj, pmap, dest, "t.");
	}
}

/*
Look for the most full, non-full transport at a location.
Prefer switching to staying.  If we switch, we force
one of the ships to become more full.
*/

piece_info_t *
find_best_tt (best, loc)
piece_info_t *best;
long loc;
{
	piece_info_t *p;

	for (p = map[loc].objp; p != NULL; p = p->loc_link.next)
	if (p->type == TRANSPORT && obj_capacity (p) > p->count) {
		if (!best) best = p;
		else if (p->count >= best->count) best = p;
	}
	return best;
}

/*
Load an army onto the most full non-full ship.
*/

int
load_army (obj)
piece_info_t *obj;
{
	piece_info_t *p;
	int i;
	long x_loc;

	p = find_best_tt (obj->ship, obj->loc); /* look here first */

	for (i = 0; i < 8; i++) { /* try surrounding squares */
		x_loc = obj->loc + dir_offset[i];
		if (map[x_loc].on_board)
			p = find_best_tt (p, x_loc);

	}
	if (!p) return FALSE; /* no tt to be found */

	if (p->loc == obj->loc) { /* stay in same place */
		obj->moved = piece_attr[ARMY].speed;
	}
	else move_obj (obj, p->loc); /* move to square with ship */

	if (p->ship != obj->ship) { /* reload army to new ship */
		disembark (obj);
		embark (p, obj);
	}
	return TRUE;
}

/*
Return the first location we find adjacent to the current location of
the correct terrain.
*/

long
move_away (vmap, loc, terrain)
view_map_t *vmap;
long loc;
char *terrain;
{
	long new_loc;
	int i;

	for (i = 0; i < 8; i++) {
		new_loc = loc + dir_offset[i];
		if (map[new_loc].on_board
		 && strchr (terrain, vmap[new_loc].contents))
			return (new_loc);
	}
	return (loc);
}

/*
Look to see if there is an adjacent object to attack.  We are passed
a location and a list of items we attack sorted in order of most
valuable first.  We look at each surrounding on board location.
If there is an object we can attack, we return the location of the
best of these.
*/

long
find_attack (loc, obj_list, terrain)
long loc;
char *obj_list;
char *terrain;
{
	long new_loc, best_loc;
	int i, best_val;
	char *p;

	best_loc = loc; /* nothing found yet */
	best_val = INFINITY;
	for (i = 0; i < 8; i++) {
		new_loc = loc + dir_offset[i];

		if (map[new_loc].on_board /* can we move here? */
		    && strchr (terrain, map[new_loc].contents)) {
			p = strchr (obj_list, comp_map[new_loc].contents);
			if (p != NULL && p - obj_list < best_val) {
				best_val = p - obj_list;
				best_loc = new_loc;
			}
		}
	}
	return (best_loc);
}

/*
Move a transport.

There are two kinds of transports:  loading and unloading.

Loading transports move toward loading armies.  Unloading
transports move toward attackable cities on unowned continents.

An empty transport is willing to attack adjacent enemy transports.
Transports become 'loading' when empty, and 'unloading' when full.
*/

void
transport_move (obj)
piece_info_t *obj;
{
	void tt_do_move();

	long new_loc;

	/* empty transports can attack */
	if (obj->count == 0) { /* empty? */
		obj->func = 0; /* transport is loading */
		new_loc = find_attack (obj->loc, tt_attack, ".");
		if (new_loc != obj->loc) { /* something to attack? */
			attack (obj, new_loc); /* attack it */
			return;
		}
	}

	if (obj->count == obj_capacity (obj)) /* full? */
		obj->func = 1; /* unloading */

	if (obj->func == 0) { /* loading? */
		make_tt_load_map (amap, comp_map);
		new_loc = vmap_find_wlobj (path_map, amap, obj->loc, &tt_load);
		
		if (new_loc == obj->loc) { /* nothing to load? */
			(void) memcpy (amap, comp_map, MAP_SIZE * sizeof (view_map_t));
			unmark_explore_locs (amap);
			if (print_vmap == 'S') print_xzoom (amap);
			new_loc = vmap_find_wobj (path_map, amap, obj->loc,&tt_explore);
		}
		
		move_objective (obj, path_map, new_loc, "a ");
	}
	else {
		make_unload_map (amap, comp_map);
		new_loc = vmap_find_wlobj (path_map, amap, obj->loc, &tt_unload);
		move_objective (obj, path_map, new_loc, " ");
	}
}

/*
Move a fighter.

1)  See if there is an object we can attack immediately.
If so, attack it.

2)  Otherwise, if fighter is low on fuel, move toward nearest city
if there is one in range.

3)  Otherwise, look for an objective.
*/

void
fighter_move (obj)
piece_info_t *obj;
{
	long new_loc;

	new_loc = find_attack (obj->loc, fighter_attack, ".+");
	if (new_loc != obj->loc) { /* something to attack? */
		attack (obj, new_loc); /* attack it */
		return;
	}
	/* return to base if low on fuel */
	if (obj->range <= find_nearest_city (obj->loc, COMP, &new_loc) + 2) {
		if (new_loc != obj->loc)
			new_loc = vmap_find_dest (path_map, comp_map, obj->loc,
						  new_loc, COMP, T_AIR);
	}
	else new_loc = obj->loc;
	
	if (new_loc == obj->loc) { /* no nearby city? */
		new_loc = vmap_find_aobj (path_map, comp_map, obj->loc,
					       &fighter_fight);
	}
	move_objective (obj, path_map, new_loc, " ");
}

/*
Move a ship.

Attack anything adjacent.  If nothing adjacent, explore or look for
something to attack.
*/

void
ship_move (obj)
piece_info_t *obj;
{
	long new_loc;
	char *adj_list;

	if (obj->hits < piece_attr[obj->type].max_hits) { /* head to port */
		if (comp_map[obj->loc].contents == 'X') { /* stay in port */
			obj->moved = piece_attr[obj->type].speed;
			return;
		}
		new_loc = vmap_find_wobj (path_map, comp_map, obj->loc, &ship_repair);
		adj_list = ".";

	}
	else {
		new_loc = find_attack (obj->loc, ship_attack, ".");
		if (new_loc != obj->loc) { /* something to attack? */
			attack (obj, new_loc); /* attack it */
			return;
		}
		/* look for an objective */
		(void) memcpy (amap, comp_map, MAP_SIZE * sizeof (view_map_t));
		unmark_explore_locs (amap);
		if (print_vmap == 'S') print_xzoom (amap);
		
		new_loc = vmap_find_wobj (path_map, amap, obj->loc,&ship_fight);
		adj_list = ship_fight.objectives;
	}

	move_objective (obj, path_map, new_loc, adj_list);
}

/*
Move to an objective.
*/

void
move_objective (obj, pathmap, new_loc, adj_list)
piece_info_t *obj;
path_map_t pathmap[];
long new_loc;
char *adj_list;
{
	char *terrain;
	char *attack_list;
	int d;
	int reuse; /* true iff we should reuse old map */
	long old_loc;
	long old_dest;
	
	if (new_loc == obj->loc) {
		obj->moved = piece_attr[obj->type].speed;
		obj->range -= 1;
		pdebug ("No destination found for %d at %d; func=%d\n",	obj->type, obj->loc, obj->func,0,0,0,0,0);
		return;
	}
	old_loc = obj->loc; /* remember where we are */
	old_dest = new_loc; /* and where we're going */
	
	d = dist (new_loc, obj->loc);
	reuse = 1; /* try to reuse unless we learn otherwise */
	
	if (comp_map[new_loc].contents == ' ' && d == 2) { /* are we exploring? */
		vmap_mark_adjacent (pathmap, obj->loc);
		reuse = 0;
	}
	else vmap_mark_path (pathmap, comp_map, new_loc); /* find routes to destination */
	
	/* path terrain and move terrain may differ */
	switch (obj->type) {
	case ARMY: terrain = "+"; break;
	case FIGHTER: terrain = "+.X"; break;
	default: terrain = ".X"; break;
	}
	
	new_loc = vmap_find_dir (pathmap, comp_map, obj->loc,
				 terrain, adj_list);
	
	if (new_loc == obj->loc /* path is blocked? */
	    && (obj->type != ARMY || !obj->ship)) { /* don't unblock armies on a ship */
		vmap_mark_near_path (pathmap, obj->loc);
		reuse = 0;
		new_loc = vmap_find_dir (pathmap, comp_map, obj->loc,
					 terrain, adj_list);
	}
	
	/* encourage army to leave city */
	if (new_loc == obj->loc && map[obj->loc].cityp != NULL
				&& obj->type == ARMY) {
		new_loc = move_away (comp_map, obj->loc, "+");
		reuse = 0;
	}
	if (new_loc == obj->loc) {
		obj->moved = piece_attr[obj->type].speed;
		
		if (obj->type == ARMY && obj->ship) ;
		else pdebug ("Cannot move %d at %d toward objective; func=%d\n", obj->type, obj->loc, obj->func,0,0,0,0,0);
	}
	else move_obj (obj, new_loc);
	
	/* Try to make more moves using same path map. */
	if (reuse && obj->moved < obj_moves (obj) && obj->loc != old_dest) {
		/* check for immediate attack */
		switch (obj->type) {
		case FIGHTER:
			if (comp_map[old_dest].contents != 'X' /* watch fuel */
				&& obj->range <= piece_attr[FIGHTER].range / 2)
					return;
			attack_list = fighter_attack;
			terrain = "+.";
			break;
		case ARMY:
			attack_list = army_attack;
			if (obj->ship) terrain = "+*";
			else terrain = "+.*";
			break;
		case TRANSPORT:
			terrain = ".*";
			if (obj->cargo) attack_list = tt_attack;
			else attack_list = "*O"; /* causes tt to wake up */
			break;
		default:
			attack_list = ship_attack;
			terrain = ".";
			break;
		}
		if (find_attack (obj->loc, attack_list, terrain) != obj->loc)
			return;
		
		/* clear old path */
		pathmap[old_loc].terrain = T_UNKNOWN;
		for (d = 0; d < 8; d++) {
			new_loc = old_loc + dir_offset[d];
			pathmap[new_loc].terrain = T_UNKNOWN;
		}
		/* pathmap is already marked, but this should work */
		move_objective (obj, pathmap, old_dest, adj_list);
	}
}

/*
Check to see if the game is over.  We count the number of cities
owned by each side.  If either side has no cities and no armies, then
the game is over.  If the computer has less than half as many cities
and armies as the user, then the computer will give up.
*/

void
check_endgame () { /* see if game is over */

	int nuser_city, ncomp_city;
	int nuser_army, ncomp_army;
	piece_info_t *p;
	int i;
	
	date += 1; /* one more turn has passed */
	if (win != 0) return; /* we already know game is over */

	nuser_city = 0; /* nothing counted yet */
	ncomp_city = 0;
	nuser_army = 0;
	ncomp_army = 0;
	
	for (i = 0; i < NUM_CITY; i++) {
		if (city[i].owner == USER) nuser_city++;
		else if (city[i].owner == COMP) ncomp_city++;
	}
	
	for (p = user_obj[ARMY]; p != NULL; p = p->piece_link.next)
		nuser_army++;
	
	for (p = comp_obj[ARMY]; p != NULL; p = p->piece_link.next)
		ncomp_army++;
		
	if (ncomp_city < nuser_city/3 && ncomp_army < nuser_army/3) {
		clear_screen ();
		prompt ("The computer acknowledges defeat. Do",0,0,0,0,0,0,0,0);
		ksend ("The computer acknowledges defeat.",0,0,0,0,0,0,0,0);
		error ("you wish to smash the rest of the enemy? ",0,0,0,0,0,0,0,0);

		if (get_chx() !=  'Y') empend ();
		(void) addstr ("\nThe enemy inadvertantly revealed its code used for");
		(void) addstr ("\nreceiving battle information. You can display what");
		(void) addstr ("\nthey've learned with the ''E'' command.");
		resigned = TRUE;
		win = 2;
		automove = FALSE;
	}
	else if (ncomp_city == 0 && ncomp_army == 0) {
		clear_screen ();
		(void) addstr ("The enemy is incapable of defeating you.\n");
		(void) addstr ("You are free to rape the empire as you wish.\n");
	    	(void) addstr ("There may be, however, remnants of the enemy fleet\n");
	    	(void) addstr ("to be routed out and destroyed.\n");
		win = 1;
		automove = FALSE;
	}
	else if (nuser_city == 0 && nuser_army == 0) {
	    	clear_screen ();
	    	(void) addstr ("You have been rendered incapable of\n");
	    	(void) addstr ("defeating the rampaging enemy fascists! The\n");
	    	(void) addstr ("empire is lost. If you have any ships left, you\n");
	    	(void) addstr ("may attempt to harass enemy shipping.");
		win = 1;
		automove = FALSE;
	}
}
