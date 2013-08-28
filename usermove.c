/* %W% %G% %U% - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
usermove.c -- Let the user move her troops.
*/

#include <curses.h>
#include <ctype.h>
#include "empire.h"
#include "extern.h"

void fatal(piece_info_t *obj,long loc,char *message,char *response);
void move_to_dest(piece_info_t *obj,long dest);
void move_army_to_city(piece_info_t *obj,long city_loc);

void
user_move () {
	void piece_move();

	int i, j, sec, sec_start;
	piece_info_t *obj, *next_obj;
	int prod;

	/* First we loop through objects to update the user's view
	of the world and perform any other necessary processing.
	We would like to have the world view up to date before
	asking the user any questions.  This means that we should
	also scan through all cities before possibly asking the
	user what to produce in each city. */

	for (i = 0; i < NUM_OBJECTS; i++)
	for (obj = user_obj[i]; obj != NULL; obj = obj->piece_link.next) {
		obj->moved = 0; /* nothing moved yet */
		scan (user_map, obj->loc); /* refresh user's view of world */
	}

	/* produce new hardware */
	for (i = 0; i < NUM_CITY; i++)
	    if (city[i].owner == USER) {
		scan (user_map, city[i].loc);
		prod = city[i].prod;

		if (prod == NOPIECE) { /* need production? */
			set_prod (&(city[i])); /* ask user what to produce */
		}
		else if (city[i].work++ >= (long)piece_attr[prod].build_time) {
				/* kermyt begin */
			ksend1("%s has been completed at city %d.\n", piece_attr[prod].article,city[i].loc,0,0,0,0,0,0);
				/* kermyt end */
			comment1 ("%s has been completed at city %d.\n", piece_attr[prod].article,city[i].loc,0,0,0,0,0,0);

			produce (&city[i]);
			/* produce should set object.moved to 0 */
		}
	}

	/* move all satellites */
	for (obj = user_obj[SATELLITE]; obj != NULL; obj = next_obj) {
		next_obj = obj->piece_link.next;
		move_sat (obj);
	}
	
	sec_start = cur_sector (); /* get currently displayed sector */
	if (sec_start == -1) sec_start = 0;

	/* loop through sectors, moving every piece in the sector */
	for (i = sec_start; i < sec_start + NUM_SECTORS; i++) {
		sec = i % NUM_SECTORS;
		sector_change (); /* allow screen to be redrawn */

		for (j = 0; j < NUM_OBJECTS; j++) /* loop through obj lists */
		for (obj = user_obj[move_order[j]]; obj != NULL;
			obj = next_obj) { /* loop through objs in list */
			next_obj = obj->piece_link.next;

			if (!obj->moved) /* object not moved yet? */
			if (loc_sector (obj->loc) == sec) /* object in sector? */
			piece_move (obj); /* yup; move the object */
		}
		if (cur_sector () == sec) { /* is sector displayed? */
			print_sector_u (sec); /* make screen up-to-date */
			(void) refresh (); /* show it to the user */
		}
	}
	if (save_movie) save_movie_screen ();
}

/*
Move a piece.  We loop until all the moves of a piece are made.  Within
the loop, we first awaken the piece if it is adjacent to an enemy piece.
Then we attempt to handle any preprogrammed function for the piece.  If
the piece has not moved after this, we ask the user what to do.
*/

void
piece_move (obj)
piece_info_t *obj;
{
	void move_random(), move_fill(), move_land(), move_explore();
	void move_path(), move_dir(), move_armyload(), ask_user();
	void move_armyattack(), move_ttload(), move_repair();
	void move_transport();

	int changed_loc;
	int speed, max_hits;
	int saved_moves;
	int need_input;
	long saved_loc;
	city_info_t *cityp;

	/* set func for piece if on city */
	cityp = find_city (obj->loc);
	if (cityp != NULL)
		if (cityp->func[obj->type] != NOFUNC)
			obj->func = cityp->func[obj->type];

	changed_loc = FALSE; /* not changed yet */
	speed = piece_attr[obj->type].speed;
	max_hits = piece_attr[obj->type].max_hits;
	need_input = FALSE; /* don't require user input yet */

	while (obj->moved < obj_moves (obj)) {
		saved_moves = obj->moved; /* save moves made */
		saved_loc = obj->loc; /* remember starting location */

		if (awake (obj) || need_input){ /* need user input? */
			ask_user (obj);
			topini (); /* clear info lines */
			display_loc_u (obj->loc); /* let user see result */
			(void) refresh ();
			need_input = FALSE; /* we got it */
		}
		
		if (obj->moved == saved_moves) /* user set function? */
		switch (obj->func) { /* handle preprogrammed function */
		case NOFUNC:    break;
		case RANDOM:    move_random (obj); break;
		case SENTRY:    obj->moved = speed; break;
		case FILL:      move_fill (obj); break;
		case LAND:      move_land (obj); break;
		case EXPLORE:   move_explore (obj); break;
		case ARMYLOAD:  move_armyload (obj); break;
		case ARMYATTACK:move_armyattack (obj); break;
		case TTLOAD:    move_ttload (obj); break;
		case REPAIR:    move_repair (obj); break;
		case WFTRANSPORT: move_transport (obj); break;

		case MOVE_N:
		case MOVE_NE:
		case MOVE_E:
		case MOVE_SE:
		case MOVE_S:
		case MOVE_SW:
		case MOVE_W:
		case MOVE_NW:
			move_dir (obj); break;

		default: move_path (obj); break;
		}

		if (obj->moved == saved_moves) need_input = TRUE;
		
		/* handle fighters specially.  If in a city or carrier, turn
		is over and reset range to max.  Otherwise, if
		range = 0, fighter crashes and burns and turn is over. */

		if (obj->type == FIGHTER && obj->hits > 0) {
			if ((user_map[obj->loc].contents == 'O'
			  || user_map[obj->loc].contents == 'C')
			&& obj->moved > 0) {
				obj->range = piece_attr[FIGHTER].range;
				obj->moved = speed;
				obj->func = NOFUNC;
				comment ("Landing confirmed.",0,0,0,0,0,0,0,0);
			}
			else if (obj->range == 0) {
				comment ("Fighter at %d crashed and burned.",obj->loc,0,0,0,0,0,0,0);
				kill_obj (obj, obj->loc);
			}
		}

		if (saved_loc != obj->loc) changed_loc = TRUE;
	}
	/* if a boat is in port, damaged, and never moved, fix some damage */
	if (obj->hits > 0 /* still alive? */
		&& !changed_loc /* object never changed location? */
		&& obj->type != ARMY && obj->type != FIGHTER /* it is a boat? */
		&& obj->hits < max_hits /* it is damaged? */
		&& user_map[obj->loc].contents == 'O') /* it is in port? */
	obj->hits++; /* fix some damage */
}

/*
Move a piece at random.  We create a list of empty squares to which
the piece can move.  If there are none, we do nothing, otherwise we 
move the piece to a random adjacent square.
*/

void move_random (obj)
piece_info_t *obj;
{
	long loc_list[8];
	int i, nloc;
	long loc;

	nloc = 0;

	for (i = 0; i < 8; i++) {
		loc = obj->loc + dir_offset[i];
		if (good_loc (obj, loc)) {
			loc_list[nloc] = loc; /* remember this location */
			nloc++; /* count locations we can move to */
		}
	}
	if (nloc == 0) return; /* no legal move */
	i = irand ((long)nloc-1); /* choose random direction */
	move_obj (obj, loc_list[i]); /* move the piece */
}

/*
Have a piece explore.  We look for the nearest unexplored territory
which the piece can reach and have to piece move toward the
territory.
*/

void move_explore (obj)
piece_info_t *obj;
{
	path_map_t path_map[MAP_SIZE];
	long loc;
	char *terrain;

	switch (obj->type) {
	case ARMY:
		loc = vmap_find_lobj (path_map, user_map, obj->loc, &user_army);
		terrain = "+";
		break;
	case FIGHTER:
		loc = vmap_find_aobj (path_map, user_map, obj->loc, &user_fighter);
		terrain = "+.O";
		break;
	default:
		loc = vmap_find_wobj (path_map, user_map, obj->loc, &user_ship);
		terrain = ".O";
		break;
	}
	
	if (loc == obj->loc) return; /* nothing to explore */

	if (user_map[loc].contents == ' ' && path_map[loc].cost == 2)
		vmap_mark_adjacent (path_map, obj->loc);
	else vmap_mark_path (path_map, user_map, loc);

	loc = vmap_find_dir (path_map, user_map, obj->loc, terrain, " ");
	if (loc != obj->loc) move_obj (obj, loc);
}

/*
Move an army onto a transport when it arrives.  We scan around the
army to find a non-full transport.  If one is present, we move the
army to the transport and waken the army.
*/

void
move_transport (obj)
piece_info_t *obj;
{
	long loc;

	/* look for an adjacent transport */
	loc = find_transport (USER, obj->loc);
	
	if (loc != obj->loc) {
		move_obj (obj, loc);
		obj->func = NOFUNC;
	}
	else obj->moved = piece_attr[obj->type].speed;
}

/*
Move an army toward the nearest loading transport.
If there is an adjacent transport, move the army onto
the transport, and awaken the army.
*/

static view_map_t amap[MAP_SIZE];

void
move_armyload (obj)
piece_info_t *obj;
{
	long loc;
	piece_info_t *p;
	int i;

	ABORT;
	
	/* look for an adjacent transport */
	loc = find_transport (USER, obj->loc);

	if (loc != obj->loc) {
		move_obj (obj, loc);
		obj->func = NOFUNC;
	}
	else { /* look for nearest non-full transport */
		(void) memcpy (amap, user_map, sizeof (view_map_t) * MAP_SIZE);

		/* mark loading transports or cities building transports */
		for (p = user_obj[TRANSPORT]; p; p = p->piece_link.next)
		if (p->count < obj_capacity (p)) /* not full? */
		amap[p->loc].contents = '$';
		
		for (i = 0; i < NUM_CITY; i++)
		if (city[i].owner == USER && city[i].prod == TRANSPORT)
		amap[city[i].loc].contents = '$';
	}
}
		
/*
Move an army toward an attackable city or enemy army.
*/

void
move_armyattack (obj)
piece_info_t *obj;
{
	path_map_t path_map[MAP_SIZE];
	long loc;

	ASSERT (obj->type == ARMY);

	loc = vmap_find_lobj (path_map, user_map, obj->loc, &user_army_attack);
	
	if (loc == obj->loc) return; /* nothing to attack */

	vmap_mark_path (path_map, user_map, loc);

	loc = vmap_find_dir (path_map, user_map, obj->loc, "+", "X*a");
	if (loc != obj->loc) move_obj (obj, loc);
}

void
move_ttload (obj)
piece_info_t *obj;
{
	ABORT;
	obj = obj;
}

/*
Move a ship toward port.  If the ship is healthy, wake it up.
*/

void
move_repair (obj)
piece_info_t *obj;
{
	path_map_t path_map[MAP_SIZE];
	long loc;

	ASSERT (obj->type > FIGHTER);
	
	if (obj->hits == piece_attr[obj->type].max_hits) {
		obj->func = NOFUNC;
		return;
	}
	
	if (user_map[obj->loc].contents == 'O') { /* it is in port? */
		obj->moved += 1;
		return;
	}

	loc = vmap_find_wobj (path_map, user_map, obj->loc, &user_ship_repair);
	
	if (loc == obj->loc) return; /* no reachable city */

	vmap_mark_path (path_map, user_map, loc);

	/* try to be next to ocean to avoid enemy pieces */
	loc = vmap_find_dir (path_map, user_map, obj->loc, ".O", ".");
	if (loc != obj->loc) move_obj (obj, loc);
}

/*
Here we have a transport or carrier waiting to be filled.  If the
object is not full, we set the move count to its maximum value.
Otherwise we awaken the object.
*/

void move_fill (obj)
piece_info_t *obj;
{
	if (obj->count == obj_capacity (obj)) /* full? */
		obj->func = NOFUNC; /* awaken full boat */
	else obj->moved = piece_attr[obj->type].speed;
}

/*
Here we have a piece that wants to land at the nearest carrier or
owned city.  We scan through the lists of cities and carriers looking
for the closest one.  We then move toward that item's location.
The nearest landing field must be within the object's range.
*/

void
move_land (obj)
piece_info_t *obj;
{
	long best_dist, best_loc;
	long new_dist;
	piece_info_t *p;

	best_dist = find_nearest_city (obj->loc, USER, &best_loc);

	for (p = user_obj[CARRIER]; p != NULL; p = p->piece_link.next) {
		new_dist = dist (obj->loc, p->loc);
		if (new_dist < best_dist) {
			best_dist = new_dist;
			best_loc = p->loc;
		}
	}
	if (best_dist == 0) obj->moved += 1; /* fighter is on a city */
	
	else if (best_dist <= obj->range)
		move_to_dest (obj, best_loc);
		
	else obj->func = NOFUNC; /* can't reach city or carrier */
}

/*
Move a piece in the specified direction if possible.
If the object is a fighter which has travelled for half its range,
we wake it up.
*/

void move_dir (obj)
piece_info_t *obj;
{
	long loc;
	int dir;

	dir = MOVE_DIR (obj->func);
	loc = obj->loc + dir_offset[dir];

	if (good_loc (obj, loc))
		move_obj (obj, loc);
}

/*
Move a piece toward a specified destination if possible.  For each
direction, we see if moving in that direction would bring us closer
to our destination, and if there is nothing in the way.  If so, we
move in the first direction we find.
*/

void move_path (obj)
piece_info_t *obj;
{
	if (obj->loc == obj->func)
		obj->func = NOFUNC;
	else move_to_dest (obj, obj->func);
}

/*
Move a piece toward a specific destination.  We first map out
the paths to the destination, if we can't get there, we return.
Then we mark the paths to the destination.  Then we choose a
move.
*/

void move_to_dest (obj, dest)
piece_info_t *obj;
long dest;
{
	path_map_t path_map[MAP_SIZE];
	int fterrain;
	char *mterrain;
	long new_loc;
	
	switch (obj->type) {
	case ARMY:
		fterrain = T_LAND;
		mterrain = "+";
		break;
	case FIGHTER:
		fterrain = T_AIR;
		mterrain = "+.O";
		break;
	default:
		fterrain = T_WATER;
		mterrain = ".O";
		break;
	}
	
	new_loc = vmap_find_dest (path_map, user_map, obj->loc, dest,
                                  USER, fterrain);
	if (new_loc == obj->loc) return; /* can't get there */
	
	vmap_mark_path (path_map, user_map, dest);
	new_loc = vmap_find_dir (path_map, user_map, obj->loc, mterrain, " .");
	if (new_loc == obj->loc) return; /* can't move ahead */
	ASSERT (good_loc (obj, new_loc));
	move_obj (obj, new_loc); /* everything looks good */
}

/*
Ask the user to move her piece.
*/

void ask_user (obj)
piece_info_t *obj;
{
	void user_skip(), user_fill(), user_dir(), user_set_dir();
	void user_wake(), user_set_city_func(), user_cancel_auto();
	void user_redraw(), user_random(), user_land(), user_sentry();
	void user_help(), reset_func(), user_explore();
	void user_build(), user_transport();
	void user_armyattack(), user_repair();

	char c;

    for (;;) {
	display_loc_u (obj->loc); /* display piece to move */
	describe_obj (obj); /* describe object to be moved */
	display_score (); /* show current score */
	display_loc_u (obj->loc); /* reposition cursor */

	c = get_chx (); /* get command from user (no echo) */
	switch (c) {
	case 'Q': user_dir (obj, NORTHWEST); return;
	case 'W': user_dir (obj, NORTH); return;
	case 'E': user_dir (obj, NORTHEAST); return;
	case 'D': user_dir (obj, EAST); return;
	case 'C': user_dir (obj, SOUTHEAST); return;
	case 'X': user_dir (obj, SOUTH); return;
	case 'Z': user_dir (obj, SOUTHWEST); return;
	case 'A': user_dir (obj, WEST); return;

	case 'J': edit (obj->loc); reset_func (obj); return;
	case 'V': user_set_city_func (obj); reset_func (obj); return;
	
	case ' ': user_skip (obj); return;
	case 'F': user_fill (obj); return;
	case 'I': user_set_dir (obj); return;
	case 'R': user_random (obj); return;
	case 'S': user_sentry (obj); return;
	case 'L': user_land (obj); return;
	case 'G': user_explore (obj); return;
	case 'T': user_transport (obj); return;
	case 'U': user_repair (obj); return;
	case 'Y': user_armyattack (obj); return;

	case 'B': user_build (obj); break;
	case 'H': user_help (); break;
	case 'K': user_wake (obj); break;
	case 'O': user_cancel_auto (); break;
	case '\014':
	case 'P': user_redraw (); break;
	case '?': describe_obj (obj); break;

	default: (void) beep ();
	}
    }
}

/*
Here, if the passed object is on a city, we assign
the city's function to the object.  However, we then awaken the
object if necessary because the user only changed the city
function, and did not tell us what to do with the object.
*/

void
reset_func (obj)
piece_info_t *obj;
{
	city_info_t *cityp;
	
	cityp = find_city (obj->loc);

	if (cityp != NULL)
	if (cityp->func[obj->type] != NOFUNC) {
		obj->func = cityp->func[obj->type];
		(void) awake (obj);
	} 
}

/*
Increment the number of moves a piece has used.  If the piece
is an army and the army is in a city, move the army to
the city.
*/

void
user_skip (obj)
piece_info_t *obj;
{
	if (obj->type == ARMY && user_map[obj->loc].contents == 'O')
		move_army_to_city (obj, obj->loc);
	else obj->moved++;
}

/*
Put an object in FILL mode.  The object must be either a transport
or carrier.  If not, we beep at the user.
*/

void
user_fill (obj)
piece_info_t *obj;
{
	if (obj->type != TRANSPORT && obj->type != CARRIER) (void) beep ();
	else obj->func = FILL;
}

/*
Print out help information.
*/

void
user_help () {
	char c;

	help (help_user, user_lines);
	prompt ("Press any key to continue: ",0,0,0,0,0,0,0,0);
	c = get_chx ();
	c = c; /* keep lint happy */
}

/*
Set an object's function to move in a certain direction.
*/

void
user_set_dir (obj)
piece_info_t *obj;
{
	char c;

	c = get_chx ();
	switch (c) {
	case 'Q': obj->func = MOVE_NW; break;
	case 'W': obj->func = MOVE_N ; break;
	case 'E': obj->func = MOVE_NE; break;
	case 'D': obj->func = MOVE_E ; break;
	case 'C': obj->func = MOVE_SE; break;
	case 'X': obj->func = MOVE_S ; break;
	case 'Z': obj->func = MOVE_SW; break;
	case 'A': obj->func = MOVE_W ; break;
	default: (void) beep (); break;
	}
}

/*
Wake up the current piece.
*/

void
user_wake (obj)
piece_info_t *obj;
{
	obj->func = NOFUNC;
}

/*
Set the piece's func to random.  
*/

void
user_random (obj)
piece_info_t *obj;
{
	obj->func = RANDOM;
}

/*
Set a piece's function to sentry.
*/

void
user_sentry (obj)
piece_info_t *obj;
{
	obj->func = SENTRY;
}

/*
Set a fighter's function to land at the nearest city.
*/

void
user_land (obj)
piece_info_t *obj;
{
	if (obj->type != FIGHTER) (void) beep ();
	else obj->func = LAND;
}

/*
Set the piece's func to explore.
*/

void
user_explore (obj)
piece_info_t *obj;
{
	obj->func = EXPLORE;
}

/*
Set an army's function to WFTRANSPORT.
*/

void
user_transport (obj)
piece_info_t *obj;
{
	if (obj->type != ARMY) (void) beep ();
	else obj->func = WFTRANSPORT;
}

/*
Set an army's function to ARMYATTACK.
*/

void
user_armyattack (obj)
piece_info_t *obj;
{
	if (obj->type != ARMY) (void) beep ();
	else obj->func = ARMYATTACK;
}

/*
Set a ship's function to REPAIR.
*/

void
user_repair (obj)
piece_info_t *obj;
{
	if (obj->type == ARMY || obj->type == FIGHTER) (void) beep ();
	else obj->func = REPAIR;
}

/*
Set a city's function.
*/

void
user_set_city_func (obj)
piece_info_t *obj;
{
	void e_city_fill(), e_city_explore(), e_city_stasis();
	void e_city_wake(), e_city_random(), e_city_repair();
	void e_city_attack();
	
	int type;
	char e;
	city_info_t *cityp;

	cityp = find_city (obj->loc);
	if (!cityp || cityp->owner != USER) {
		(void) beep ();
		return;
	}

	type = get_piece_name();
	if (type == NOPIECE) {
		(void) beep ();
		return;
	}
	
	e = get_chx ();
	
	switch (e) {
	case 'F': /* fill */
		e_city_fill (cityp, type);
		break;
	case 'G': /* explore */
		e_city_explore (cityp, type);
		break;
	case 'I': /* directional stasis */
		e_city_stasis (cityp, type);
		break;
	case 'K': /* turn off function */
		e_city_wake (cityp, type);
		break;
	case 'R': /* make piece move randomly */
		e_city_random (cityp, type);
		break;
	case 'U': /* repair ship */
		e_city_repair (cityp, type);
		break;
	case 'Y': /* set army func to attack */
		e_city_attack (cityp, type);
		break;
	default: /* bad command? */
		(void) beep ();
		break;
	}
}

/*
Change a city's production.
*/

void
user_build (obj)
piece_info_t *obj;
{
	city_info_t *cityp;

	if (user_map[obj->loc].contents != 'O') { /* no user city here? */
		(void) beep ();
		return;
	}
	cityp = find_city (obj->loc);
	ASSERT (cityp != NULL);
	set_prod (cityp);
}

/*
Move a piece in the direction specified by the user.
This routine handles attacking objects.
*/

void
user_dir (obj, dir)
piece_info_t *obj;
int dir;
{
	void user_dir_army(), user_dir_fighter(), user_dir_ship();

	long loc;

	loc = obj->loc + dir_offset[dir];

	if (good_loc (obj, loc)) {
		move_obj (obj, loc);
		return;
	}
	if (!map[loc].on_board) {
		error ("You cannot move to the edge of the world.",0,0,0,0,0,0,0,0);
		delay ();
		return;
	}
	switch (obj->type) {
	case ARMY: user_dir_army (obj, loc); break;
	case FIGHTER: user_dir_fighter (obj, loc); break;
	default: user_dir_ship (obj, loc); break;
	}
}

/*
We have an army that wants to attack something or move onto some
unreasonable terrain.  We check for errors, question the user if
necessary, and attack if necessary.
*/

void
user_dir_army (obj, loc)
piece_info_t *obj;
long loc;
{
	int enemy_killed;
	
	enemy_killed = FALSE;

	if (user_map[loc].contents == 'O') /* attacking own city */
		move_army_to_city (obj, loc);

	else if (user_map[loc].contents == 'T') /* transport full? */
		fatal (obj, loc,
	"Sorry, sir.  There is no more room on the transport.  Do you insist? ",
	"Your army jumped into the briny and drowned.");

	else if (map[loc].contents == '.') { /* going for a swim? */
		if (!getyn ( /* thanks to Craig Hansen for this next message */
	"Troops can't walk on water, sir.  Do you really want to go to sea? "))
		return;

		if (user_map[obj->loc].contents == 'T')
		{
			comment ("Your army jumped into the briny and drowned.",0,0,0,0,0,0,0,0);
			ksend ("Your army jumped into the briny and drowned.\n",0,0,0,0,0,0,0,0);
		}
		else if (user_map[loc].contents == '.')
		{
			comment ("Your army marched dutifully into the sea and drowned.",0,0,0,0,0,0,0,0);
			ksend ("Your army marched dutifully into the sea and drowned.\n",0,0,0,0,0,0,0,0);
		}
		else { /* attack something at sea */
			enemy_killed = islower (user_map[loc].contents);
			attack (obj, loc);
	
			if (obj->hits > 0) /* ship won? */
			{
				comment ("Your army regretfully drowns after its successful assault.",0,0,0,0,0,0,0,0);
				ksend ("Your army regretfully drowns after it's successful assault.",0,0,0,0,0,0,0,0);
			}
		}
		if (obj->hits > 0) {
			kill_obj (obj, loc);
			if (enemy_killed) scan (comp_map, loc);
		}
	}
		
	else if (isupper (user_map[loc].contents)
		&& user_map[loc].contents != 'X') { /* attacking self */
		if (!getyn (
	"Sir, those are our men!  Do you really want to attack them? "))
		return;

		attack (obj, loc);
	}

	else attack (obj, loc);
}

/*
Here we have a fighter wanting to attack something.  There are only
three cases:  attacking a city, attacking ourself, attacking the enemy.
*/

void
user_dir_fighter (obj, loc)
piece_info_t *obj;
long loc;
{
	if (map[loc].contents == '*')
		fatal (obj, loc,
	"That's never worked before, sir.  Do you really want to try? ",
	"Your fighter was shot down.");

	else if (isupper (user_map[loc].contents)) {
		if (!getyn (
	"Sir, those are our men!  Do you really want to attack them? "))
		return;

		attack (obj, loc);
	}

	else attack (obj, loc);
}

/*
Here we have a ship attacking something, or trying to move on
shore.  Our cases are: moving ashore (and subcases), attacking
a city, attacking self, attacking enemy.
*/
	
void
user_dir_ship (obj, loc)
piece_info_t *obj;
long loc;
{
	int enemy_killed;

	enemy_killed = FALSE;

	if (map[loc].contents == '*') {
		(void) sprintf (jnkbuf, "Your %s broke up on shore.",
				piece_attr[obj->type].name);

		fatal (obj, loc,
	"That's never worked before, sir.  Do you really want to try? ",
			jnkbuf);
	}

	else if (map[loc].contents == '+') { /* moving ashore? */
		if (!getyn ("Ships need sea to float, sir.  Do you really want to go ashore? ")) return;

		if (user_map[loc].contents == '+')
		{
			comment1 ("Your %s broke up on shore.", piece_attr[obj->type].name,0,0,0,0,0,0,0);
			ksend1 ("Your %s broke up on shore.", piece_attr[obj->type].name,0,0,0,0,0,0,0);
		}
		else { /* attack something on shore */
			enemy_killed = islower (user_map[loc].contents);
			attack (obj, loc);

			if (obj->hits > 0) /* ship won? */
			{
				comment1 ("Your %s breaks up after its successful assault.", piece_attr[obj->type].name,0,0,0,0,0,0,0);
				ksend1 ("Your %s breaks up after its successful assault.", piece_attr[obj->type].name,0,0,0,0,0,0,0);
			}
		}
		if (obj->hits > 0) {
			kill_obj (obj, loc);
			if (enemy_killed) scan (comp_map, loc);
		}
	}
		
	else if (isupper (user_map[loc].contents)) { /* attacking self */
		if (!getyn (
	"Sir, those are our men!  Do you really want to attack them? "))
		return;

		attack (obj, loc);
	}

	else attack (obj, loc);
}

/*
Here a user wants to move an army to a city.  If the city contains
a non-full transport, we make the move.  Otherwise we ask the user
if she really wants to attack the city.
*/

void
move_army_to_city (obj, city_loc)
piece_info_t *obj;
long city_loc;
{
	piece_info_t *tt;

	tt = find_nfull (TRANSPORT, city_loc);

	if (tt != NULL) move_obj (obj, city_loc);

	else fatal (obj, city_loc,
	"That's our city, sir!  Do you really want to attack the garrison? ",
	"Your rebel army was liquidated.");
}

/*
Cancel automove mode.
*/

void
user_cancel_auto () {
	if (!automove)
		comment ("Not in auto mode!",0,0,0,0,0,0,0,0);
	else {
		automove = FALSE;
		comment ("Auto mode cancelled.",0,0,0,0,0,0,0,0);
	}
}

/*
Redraw the screen.
*/

void
user_redraw () {
	redraw ();
}

/*
Awaken an object if it needs to be.  Normally, objects are awakened
when they are next to an enemy object or an unowned city.  Armies
on troop transports are not awakened if they are surrounded by sea.
We return true if the object is now awake.  Objects are never
completely awoken here if their function is a destination.  But we
will return TRUE if we want the user to have control.
*/

int
awake (obj)
piece_info_t *obj;
{
	int i;
	char c;
	long t;

	if (obj->type == ARMY && vmap_at_sea (user_map, obj->loc)) {
	    obj->moved = piece_attr[ARMY].range;
	    return (FALSE);
	}
	if (obj->func == NOFUNC) return (TRUE); /* object is awake */
	
	if (obj->type == FIGHTER /* wake fighters */
	    && obj->func != LAND /* that aren't returning to base */
	    && obj->func < 0 /* and which don't have a path */
	    && obj->range <= find_nearest_city (obj->loc, USER, &t) + 2) {
		obj->func = NOFUNC; /* wake piece */
		return (TRUE);
	}
	for (i = 0; i < 8; i++) { /* for each surrounding cell */
		c = user_map[obj->loc+dir_offset[i]].contents;

		if (islower (c) || c == '*' || c == 'X') {
			if (obj->func < 0) obj->func = NOFUNC; /* awaken */
			return (TRUE);
		}
	}
	return (FALSE);
}

/*
Question the user about a fatal move.  If the user responds 'y',
print out the response and kill the object.
*/

void
fatal (obj, loc, message, response)
piece_info_t *obj;
long loc;
char *message;
char *response;
{
	if (getyn (message)) {
		comment (response,0,0,0,0,0,0,0,0);
		kill_obj (obj, loc);
	}
}
