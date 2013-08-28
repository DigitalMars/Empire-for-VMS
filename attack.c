/* %W% %G% %U% - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
attack.c -- handle an attack between two pieces.  We do everything from
fighting it out between the pieces to notifying the user who won and
killing off the losing object.  Somewhere far above, our caller is
responsible for actually removing the object from its list and actually
updating the player's view of the world.

Find object being attacked.  If it is a city, attacker has 50% chance
of taking city.  If successful, give city to attacker.  Otherwise
kill attacking piece.  Tell user who won.

If attacking object is not a city, loop.  On each iteration, select one
piece to throw a blow.  Damage the opponent by the strength of the blow
thrower.  Stop looping when one object has 0 or fewer hits.  Kill off 
the dead object.  Tell user who won and how many hits her piece has left,
if any.
*/

#include "empire.h"
#include "extern.h"

void
attack (att_obj, loc)
piece_info_t *att_obj;
long loc;
{
	void attack_city();
	void attack_obj();
	
	if (map[loc].contents == '*') /* attacking a city? */
		attack_city (att_obj, loc);
	else attack_obj (att_obj, loc); /* attacking a piece */
}

void
attack_city (att_obj, loc)
piece_info_t *att_obj;
long loc;
{
	city_info_t *cityp;
	int att_owner, city_owner;

	cityp = find_city (loc);
	ASSERT (cityp);
	
	att_owner = att_obj->owner;
	city_owner = cityp->owner;

	if (irand (2) == 0) { /* attack fails? */
		if (att_owner == USER) {
			comment ("The scum defending the city crushed your attacking blitzkrieger.",0,0,0,0,0,0,0,0);
			ksend ("The scum defending the city crushed your attacking blitzkrieger.\n",0,0,0,0,0,0,0,0); //kermyt
		}
		else if (city_owner == USER) {
			ksend ("Your city at %d is under attack.\n",cityp->loc,0,0,0,0,0,0,0); //kermyt
			comment ("Your city at %d is under attack.",cityp->loc,0,0,0,0,0,0,0);
		}
		kill_obj (att_obj, loc);
	}
	else { /* attack succeeded */
		kill_city (cityp);
		cityp->owner = att_owner;
		kill_obj (att_obj, loc);

		if (att_owner == USER) {
			ksend ("City at %d has been subjugated!\n",cityp->loc,0,0,0,0,0,0,0); //kermyt
			error ("City at %d has been subjugated!",cityp->loc,0,0,0,0,0,0,0);

			extra ("Your army has been dispersed to enforce control.",0,0,0,0,0,0,0,0);
			ksend ("Your army has been dispersed to enforce control.",0,0,0,0,0,0,0,0);
			set_prod (cityp);
		}
		else if (city_owner == USER) {
			ksend("City at %d has been lost to the enemy!\n",cityp->loc,0,0,0,0,0,0,0); //kermyt
			comment ("City at %d has been lost to the enemy!",cityp->loc,0,0,0,0,0,0,0);
		}
	}
	/* let city owner see all results */
	if (city_owner != UNOWNED) scan (MAP(city_owner), loc);
}

/*
Attack a piece other than a city.  The piece could be anyone's.
First we have to figure out what is being attacked.
*/

void
attack_obj (att_obj, loc)
piece_info_t *att_obj;
long loc;
{
	void describe(), survive();
	
	piece_info_t *def_obj; /* defender */
	int owner;

	def_obj = find_obj_at_loc (loc);
	ASSERT (def_obj != NULL); /* can't find object to attack? */
	
	if (def_obj->type == SATELLITE) return; /* can't attack a satellite */

	while (att_obj->hits > 0 && def_obj->hits > 0) {
		if (irand (2) == 0) /* defender hits? */
		     att_obj->hits -= piece_attr[def_obj->type].strength;
		else def_obj->hits -= piece_attr[att_obj->type].strength;
	}

	if (att_obj->hits > 0) { /* attacker won? */
		describe (att_obj, def_obj, loc);
		owner = def_obj->owner;
		kill_obj (def_obj, loc); /* kill loser */
		survive (att_obj, loc); /* move attacker */
	}
	else { /* defender won */
		describe (def_obj, att_obj, loc);
		owner = att_obj->owner;
		kill_obj (att_obj, loc);
		survive (def_obj, loc);
	}
	/* show results to first killed */
	scan (MAP(owner), loc);
}

/*
Here we look to see if any cargo was killed in the attack.  If
a ships contents exceeds its capacity, some of the survivors
fall overboard and drown.  We also move the survivor to the given
location.
*/

void
survive (obj, loc)
piece_info_t *obj;
long loc;
{
	while (obj_capacity (obj) < obj->count)
		kill_obj (obj->cargo, loc);
		
	move_obj (obj, loc);
}

void
describe (win_obj, lose_obj, loc)
piece_info_t *win_obj, *lose_obj;
long loc;
{
	char buf[STRSIZE];
	char buf2[STRSIZE];
	int diff;
	
	*buf = '\0';
	*buf2 = '\0';
	
	if (win_obj->owner != lose_obj->owner) {
		if (win_obj->owner == USER) {
			user_score += piece_attr[lose_obj->type].build_time; 
			ksend1 ("Enemy %s at %d destroyed.\n",piece_attr[lose_obj->type].name,loc,0,0,0,0,0,0); //kermyt
			topmsg1 (1, "Enemy %s at %d destroyed.",piece_attr[lose_obj->type].name,loc,0,0,0,0,0,0);
			ksend1 ("Your %s has %d hits left\n",piece_attr[win_obj->type].name,win_obj->hits,0,0,0,0,0,0); //kermyt
			topmsg1 (2, "Your %s has %d hits left.", piece_attr[win_obj->type].name, win_obj->hits,0,0,0,0,0,0);
				
			diff = win_obj->count - obj_capacity (win_obj);
			if (diff > 0) switch (win_obj->cargo->type) {
			case ARMY:
				ksend("%d armies fell overboard and drowned in the assault.\n",diff,0,0,0,0,0,0,0); //kermyt
			     topmsg (3,"%d armies fell overboard and drowned in the assault.",diff,0,0,0,0,0,0,0);
			     break;
			case FIGHTER:
				ksend("%d fighters fell overboard and were lost in the assult.\n",diff,0,0,0,0,0,0,0); //kermyt
			     topmsg (3,"%d fighters fell overboard and were lost in the assault.",diff,0,0,0,0,0,0,0);
			     break;
			}
		}
		else {
			comp_score += piece_attr[lose_obj->type].build_time;
			ksend1 ("Your %s at %d destroyed.\n",piece_attr[lose_obj->type].name,loc,0,0,0,0,0,0); //kermyt
			topmsg1 (3, "Your %s at %d destroyed.",piece_attr[lose_obj->type].name,loc,0,0,0,0,0,0);
		}
		set_need_delay ();
	}
}
