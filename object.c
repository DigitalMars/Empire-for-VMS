/* %W% %G% %U% - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
object.c -- routines for manipulating objects.
*/

#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif

#include <ctype.h>
#include "empire.h"
#include "extern.h"

/*
Find the nearest city to a location.  Return the location
of the city and the estimated cost to reach the city.
Distances are computed as straight-line distances.
*/

int
find_nearest_city (loc, owner, city_loc)
long loc;
int owner;
long *city_loc;
{
	long best_dist, best_loc;
	long new_dist, i;
	
	best_dist = INFINITY;
	best_loc = loc;
	
	for (i = 0; i < NUM_CITY; i++) 
	if (city[i].owner == owner) {
	       new_dist = dist (loc, city[i].loc);
	       if (new_dist < best_dist) {
		       best_dist = new_dist;
		       best_loc = city[i].loc;
	       }
	}
	*city_loc = best_loc;
	return best_dist;
}

/*
Given the location of a city, return the index of that city.
*/

city_info_t *find_city (loc)
long loc;
{
	return (map[loc].cityp);
}

/*
Return the number of moves an object gets to make.  This amount
is based on the damage the object has suffered and the number of
moves it normally gets to make.  The object always gets to make
at least one move, assuming it is not dead.  Damaged objects move
at a fraction of their normal speed.  An object which has lost
half of its hits moves at half-speed, for example.
*/

int obj_moves (obj)
piece_info_t *obj;
{
	return (piece_attr[obj->type].speed * obj->hits
	       + piece_attr[obj->type].max_hits - 1) /* round up */
	       / piece_attr[obj->type].max_hits;
}

/*
Figure out the capacity for an object.
*/

int obj_capacity (obj)
piece_info_t *obj;
{
	return (piece_attr[obj->type].capacity * obj->hits
	       + piece_attr[obj->type].max_hits - 1) /* round up */
	       / piece_attr[obj->type].max_hits;
}

/*
Search for an object of a given type at a location.  We scan the
list of objects at the given location for one of the given type.
*/

piece_info_t *find_obj (type, loc)
int type;
long loc;
{
	piece_info_t *p;

	for (p = map[loc].objp; p != NULL; p = p->loc_link.next)
	if (p->type == type) return (p);

	return (NULL);
}

/*
Find a non-full item of the appropriate type at the given location.
*/

piece_info_t *find_nfull (type, loc)
int type;
long loc;
{
	piece_info_t *p;

	for (p = map[loc].objp; p != NULL; p = p->loc_link.next)
	if (p->type == type) {
		if (obj_capacity (p) > p->count) return (p);
	}
	return (NULL);
}

/*
Look around a location for an unfull transport.  Return the location
of the transport if there is one.
*/

long
find_transport (owner, loc)
int owner;
long loc;
{
	int i;
	long new_loc;
	piece_info_t *t;

	for (i = 0; i < 8; i++) { /* look around */
		new_loc = loc + dir_offset[i];
		t = find_nfull (TRANSPORT, new_loc);
		if (t != NULL && t->owner == owner) return (new_loc);
	}
	return (loc); /* no tt found */
}

/*
Search a list of objects at a location for any kind of object.
We prefer transports and carriers to other objects.
*/

piece_info_t *
find_obj_at_loc (loc)
long loc;
{
	piece_info_t *p, *best;
	
	best = map[loc].objp;
	if (best == NULL) return (NULL); /* nothing here */

	for (p = best->loc_link.next; p != NULL; p = p->loc_link.next)
	if (p->type > best->type && p->type != SATELLITE)
		best = p;

	return (best);
}

/*
If an object is on a ship, remove it from that ship.
*/

void disembark (obj)
piece_info_t *obj;
{
	if (obj->ship) {
		UNLINK (obj->ship->cargo, obj, cargo_link);
		obj->ship->count -= 1;
		obj->ship = NULL;
	}
}

/*
Move an object onto a ship.
*/

void embark (ship, obj)
piece_info_t *ship, *obj;
{
	obj->ship = ship;
	LINK (ship->cargo, obj, cargo_link);
	ship->count += 1;
}

/*
Kill an object.  We scan around the piece and free it.  If there is
anything in the object, it is killed as well.
*/

void kill_obj (obj, loc)
piece_info_t *obj;
long loc;
{
	void kill_one();

	piece_info_t **list;
	view_map_t *vmap;
	
	vmap = MAP(obj->owner);
	list = LIST(obj->owner);
	
	while (obj->cargo != NULL) /* kill contents */
		kill_one (list, obj->cargo);

	kill_one (list, obj);
	scan (vmap, loc); /* scan around new location */
}

/* kill an object without scanning */

void kill_one (list, obj)
piece_info_t **list;
piece_info_t *obj;
{
	UNLINK (list[obj->type], obj, piece_link); /* unlink obj from all lists */
	UNLINK (map[obj->loc].objp, obj, loc_link);
	disembark (obj);

	LINK (free_list, obj, piece_link); /* return object to free list */
	obj->hits = 0; /* let all know this object is dead */
	obj->moved = piece_attr[obj->type].speed; /* object has moved */
}

/*
Kill a city.  We kill off all objects in the city and set its type
to unowned.  We scan around the city's location.
*/

void kill_city (cityp)
city_info_t *cityp;
{
	view_map_t *vmap;
	piece_info_t *p;
	piece_info_t *next_p;
	piece_info_t **list;
	int i;
	
	/* change ownership of hardware at this location; but not satellites */
	for (p = map[cityp->loc].objp; p; p = next_p) {
		next_p = p->loc_link.next;
		
		if (p->type == ARMY) kill_obj (p, cityp->loc);
		else if (p->type != SATELLITE) {
			if (p->type == TRANSPORT) {
				list = LIST(p->owner);
				
				while (p->cargo != NULL) /* kill contents */
					kill_one (list, p->cargo);
			}
			list = LIST (p->owner);
			UNLINK (list[p->type], p, piece_link);
			p->owner = (p->owner == USER ? COMP : USER);
			list = LIST (p->owner);
			LINK (list[p->type], p, piece_link);
			
			p->func = NOFUNC;
		}
	}

	if (cityp->owner != UNOWNED) {
		vmap = MAP(cityp->owner);
		cityp->owner = UNOWNED;
		cityp->work = 0;
		cityp->prod = NOPIECE;
		
		for (i = 0; i < NUM_OBJECTS; i++)
			cityp->func[i] = NOFUNC;
		
		scan (vmap, cityp->loc);
	}
}

/*
Produce an item for a city.
*/

static int sat_dir[4] = {MOVE_NW, MOVE_SW, MOVE_NE, MOVE_SE};

void
produce (cityp)
city_info_t *cityp;
{
	piece_info_t **list;
	piece_info_t *new;
	
	list = LIST (cityp->owner);

	cityp->work -= piece_attr[cityp->prod].build_time;
	
	ASSERT (free_list); /* can we allocate? */
	new = free_list;
	UNLINK (free_list, new, piece_link);
	LINK (list[cityp->prod], new, piece_link);
	LINK (map[cityp->loc].objp, new, loc_link);
	new->cargo_link.next = NULL;
	new->cargo_link.prev = NULL;
	
	new->loc = cityp->loc;
	new->func = NOFUNC;
	new->hits = piece_attr[cityp->prod].max_hits;
	new->owner = cityp->owner;
	new->type = cityp->prod;
	new->moved = 0;
	new->cargo = NULL;
	new->ship = NULL;
	new->count = 0;
	new->range = piece_attr[cityp->prod].range;
	
	if (new->type == SATELLITE) { /* set random move direction */
		new->func = sat_dir[irand (4)];
	}
}

/*
Move an object to a location.  We mark the object moved, we move
the object to the new square, and we scan around the object.
We also do lots of little maintenance like updating the range
of an object, keeping track of the number of pieces on a boat, 
etc.
*/

void move_obj (obj, new_loc)
piece_info_t *obj;
long new_loc;
{
	view_map_t *vmap;
	long old_loc;
	piece_info_t *p;

	ASSERT (obj->hits);
	vmap = MAP(obj->owner);

	old_loc = obj->loc; /* save original location */
	obj->moved += 1;
	obj->loc = new_loc;
	obj->range--;
	
	disembark (obj); /* remove object from any ship */
	
	UNLINK (map[old_loc].objp, obj, loc_link);
	LINK (map[new_loc].objp, obj, loc_link);

	/* move any objects contained in object */
	for (p = obj->cargo; p != NULL; p = p->cargo_link.next) {
		p->loc = new_loc;
		UNLINK (map[old_loc].objp, p, loc_link);
		LINK (map[new_loc].objp, p, loc_link);
	}
	
	switch (obj->type) { /* board new ship */
	case FIGHTER:
		if (map[obj->loc].cityp == NULL) { /* not in a city? */
			p = find_nfull (CARRIER, obj->loc);
			if (p != NULL) embark (p, obj);
		}
		break;

	case ARMY:
		p = find_nfull (TRANSPORT, obj->loc);
		if (p != NULL) embark (p, obj);
		break;
	}

	if (obj->type == SATELLITE)
		scan_sat (vmap, obj->loc);
	scan (vmap, obj->loc);
}

/*
Move a satellite.  It moves according to the preset direction.
Satellites bounce off the edge of the board.

We start off with some preliminary routines.
*/

/* Return next direction for a sattellite to travel. */

static long
bounce (loc, dir1, dir2, dir3)
long loc, dir1, dir2, dir3;
{
	int new_loc;

	new_loc = loc + dir_offset[MOVE_DIR (dir1)];
	if (map[new_loc].on_board) return dir1;

	new_loc = loc + dir_offset[MOVE_DIR (dir2)];
	if (map[new_loc].on_board) return dir2;

	return dir3;
}

/* Move a satellite one square. */

static void
move_sat1 (obj)
piece_info_t *obj;
{
	int dir;
	long new_loc;

	dir = MOVE_DIR(obj->func);
	new_loc = obj->loc + dir_offset[dir];

	if (!map[new_loc].on_board) {
		switch (obj->func) {
		case MOVE_NE:
			obj->func = bounce (obj->loc, MOVE_NW, MOVE_SE, MOVE_SW);
			break;
		case MOVE_NW:
			obj->func = bounce (obj->loc, MOVE_NE, MOVE_SW, MOVE_SE);
			break;
		case MOVE_SE:
			obj->func = bounce (obj->loc, MOVE_SW, MOVE_NE, MOVE_NW);
			break;
		case MOVE_SW:
			obj->func = bounce (obj->loc, MOVE_SE, MOVE_NW, MOVE_NE);
			break;
		default: ABORT;
		}
		dir = MOVE_DIR(obj->func);
		new_loc = obj->loc + dir_offset[dir];
	}
	move_obj (obj, new_loc);
}

/*
Now move the satellite all of its squares.
Satellite burns iff it's range reaches zero.
*/
		
void
move_sat (obj)
piece_info_t *obj;
{
	obj->moved = 0;
	
	while (obj->moved < obj_moves (obj)) {
		move_sat1 (obj);
		if (obj->range == 0) {
			if (obj->owner == USER)
				comment ("Satellite at %d crashed and burned.", obj->loc,0,0,0,0,0,0,0);
				ksend ("Satellite at %d crashed and burned.", obj->loc,0,0,0,0,0,0,0);
			kill_obj (obj, obj->loc);
		}
	}
}

/*
Return true if a piece can move to a specified location.
We are passed the object and the location.  The location
must be on the board, and the player's view map must have an appropriate
terrain type for the location.  Boats may move into port, armies may
move onto transports, and fighters may move onto cities or carriers.
*/

int good_loc (obj, loc)
piece_info_t *obj;
long loc;
{
	view_map_t *vmap;
	piece_info_t *p;
	
	if (!map[loc].on_board) return (FALSE);

	vmap = MAP (obj->owner);

	if (strchr (piece_attr[obj->type].terrain, vmap[loc].contents) != NULL)
		return (TRUE);

	/* armies can move into unfull transports */
	if (obj->type == ARMY) {
		p = find_nfull (TRANSPORT, loc);
		return (p != NULL && p->owner == obj->owner);
	}

	/* ships and fighters can move into cities */
	if (map[loc].cityp && map[loc].cityp->owner == obj->owner)
		return (TRUE);

	/* fighters can move onto unfull carriers */
	if (obj->type == FIGHTER) {
		p = find_nfull (CARRIER, loc);
		return (p != NULL && p->owner == obj->owner);
	}

	return (FALSE);
}

void describe_obj (obj)
piece_info_t *obj;
{
	char func[STRSIZE];
	char other[STRSIZE];

	if (obj->func >= 0) (void) sprintf (func, "%d", obj->func);
	else (void) sprintf (func, func_name[FUNCI(obj->func)]);
	
	other[0] = 0;

	switch (obj->type) { /* set other information */
	case FIGHTER:
		(void) sprintf (other,"; range = %d",obj->range);
		break;

	case TRANSPORT:
		(void) sprintf (other,"; armies = %d",obj->count);
		break;

	case CARRIER:
		(void) sprintf (other,"; fighters = %d",obj->count);
		break;
	}

	prompt2 ("%s at %d:  moves = %d; hits = %d; func = %s%s",
		piece_attr[obj->type].name,
		obj->loc,
		obj_moves (obj) - obj->moved,
		obj->hits,
		func,
		other,0,0);
}

/*
Scan around a location to update a player's view of the world.  For each
surrounding cell, we remember the date the cell was examined, and the
contents of the cell.  Notice how we carefully update the cell to first
reflect land, water, or city, then army or fighter, then boat, and finally
city owner.  This guarantees that the object we want to display will appear
on top.
*/

void
scan (vmap, loc)
view_map_t vmap[];
long loc;
{
	void update(), check();

	int i;
	long xloc;

#ifdef DEBUG
	check (); /* perform a consistency check */
#endif
	ASSERT (map[loc].on_board); /* passed loc must be on board */

	for (i = 0; i < 8; i++) { /* for each surrounding cell */
		xloc = loc + dir_offset[i];
		update (vmap, xloc);
	}
	update (vmap, loc); /* update current location as well */
}

/*
Scan a portion of the board for a satellite.
*/

void
scan_sat (vmap, loc)
view_map_t *vmap;
long loc;
{
	int i;
	long xloc;
	
	ASSERT (map[loc].on_board);

	for (i = 0; i < 8; i++) { /* for each surrounding cell */
		xloc = loc + 2 * dir_offset[i];
		if (xloc >= 0 && xloc < MAP_SIZE && map[xloc].on_board)
			scan (vmap, xloc);
	}
	scan (vmap, loc);
}

/*
Update a location.  We set the date seen, the land type, object
contents starting with armies, then fighters, then boats, and the
city type.
*/

char city_char[] = {'*', 'O', 'X'};

void
update (vmap, loc)
view_map_t vmap[];
long loc;
{
	piece_info_t *p;

	vmap[loc].seen = date;
	
	if (map[loc].cityp) /* is there a city here? */
		vmap[loc].contents = city_char[map[loc].cityp->owner];
	
	else {
		p = find_obj_at_loc (loc);
		
		if (p == NULL) /* nothing here? */
			vmap[loc].contents = map[loc].contents;
		else if (p->owner == USER)
			vmap[loc].contents = piece_attr[p->type].sname;
		else vmap[loc].contents = tolower (piece_attr[p->type].sname);
	}
	if (vmap == comp_map)
		display_locx (COMP, comp_map, loc);
	else if (vmap == user_map)
		display_locx (USER, user_map, loc);
}

/*
Set the production for a city.  We make sure the city is displayed
on the screen, and we ask the user for the new production.  We keep
asking until we get a valid answer.
*/

void
set_prod (cityp)
city_info_t *cityp;
{
	int i;

	scan (user_map, cityp->loc);
	display_loc_u (cityp->loc);

	for (;;) {
		prompt ("What do you want the city at %d to produce? ",cityp->loc,0,0,0,0,0,0,0);

		i = get_piece_name ();
		
		if (i == NOPIECE)
			error ("I don't know how to build those.",0,0,0,0,0,0,0,0);
			
		else {
			cityp->prod = i;
			city->work = -(piece_attr[i].build_time / 5);
			return;
		}
	}
}

/* Get the name of a type of object. */

int
get_piece_name ()
{
	char c;
	int i;
	
	c = get_chx (); /* get the answer */

	for (i = 0; i < NUM_OBJECTS; i++)
	if (piece_attr[i].sname == c) {
		return i;
	}
	return NOPIECE;
}
