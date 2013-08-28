/* $Id: util.c,v 1.3 2000/07/28 05:12:54 esr Exp $  - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
util.c -- various utility routines.
*/

#include <curses.h>
#include <ctype.h>
#include "empire.h"
#include "extern.h"


/*
Convert a string to uppercase.
Shirley this is defined elsewhere?
*/

#include <ctype.h>

void
tupper (str)
char	*str;
{
	while (*str) {
		if (islower (*str)) *str = upper (*str);
		str++;
	}
}

/*
Convert a character to uppercase (if it is lowercase)
*/

char
upper (c)
char c;
{
	if (islower (c))
		return toupper (c);
	else return c;
}

/*
Clear the end of a specified line starting at the specified column.
*/

void
clreol(linep, colp)
int linep, colp;
{
	(void) move (linep, colp);
	(void) clrtoeol();
}

/*
Initialize the terminal.
*/

void
ttinit()
{
	(void) initscr();
	(void) noecho();
	(void) crmode();
#ifdef A_COLOR
	init_colors();
#endif /* A_COLOR */
	lines = LINES;
	cols = COLS;
	if (lines > MAP_HEIGHT + NUMTOPS + 1)
		lines = MAP_HEIGHT + NUMTOPS + 1;
	if (cols > MAP_WIDTH + NUMSIDES)
		cols = MAP_WIDTH + NUMSIDES;
}


/*
Clear the screen.  We must also kill information maintained about the
display.
*/

void
clear_screen () {
	(void) clear ();
	(void) refresh ();
	kill_display ();
}

/*
Redraw the screen.
*/

void
redraw () {
	(void) clearok (curscr, TRUE);
	(void) refresh ();
}

/*
Wait a little bit to give user a chance to see a message.  We refresh
the screen and pause for a few milliseconds.
*/

void
delay () {
	(void) refresh ();
	(void) napms (delay_time); /* pause a bit */
}


/*
Clean up the display.  This routine gets called as we leave the game.
*/

void
close_disp()
{
	(void) move (LINES - 1, 0);
	(void) clrtoeol ();
	(void) refresh ();
	(void) endwin ();
}

/*
Position the cursor and output a string.
*/

void
pos_str1 (row, col, str, a, b, c, d, e, f, g, h)
int row, col;
char *str, *a;
int b, c, d, e, f, g, h;
{
	(void) move (row, col);
	addprintf1 (str, a, b, c, d, e, f, g, h);
}
void
pos_str (row, col, str, a, b, c, d, e, f, g, h)
int row, col;
char *str;
int a, b, c, d, e, f, g, h;
{
	(void) move (row, col);
	addprintf (str, a, b, c, d, e, f, g, h);
}

void
/* VARARGS1 */
addprintf (str, a, b, c, d, e, f, g, h)
char *str;
int a, b, c, d, e, f, g, h;
{
	char junkbuf[STRSIZE];
	
	(void) sprintf (junkbuf, str, a, b, c, d, e, f, g, h);
	(void) addstr (junkbuf);
}
void
/* VARARGS1 */
addprintf1 (str, a, b, c, d, e, f, g, h)
char *str;
char *a;
int b, c, d, e, f, g, h;
{
	char junkbuf[STRSIZE];
	
	(void) sprintf (junkbuf, str, a, b, c, d, e, f, g, h);
	(void) addstr (junkbuf);
}
void
/* VARARGS1 */
addprintf2 (str, a, b, c, d, e, f, g, h)
char *str;
char *a, *e, *f;
int b, c, d, g, h;
{
	char junkbuf[STRSIZE];
	
	(void) sprintf (junkbuf, str, a, b, c, d, e, f, g, h);
	(void) addstr (junkbuf);
}

/*
Report a bug.
*/

void
assert (expression, file, line)
char *expression;
char *file;
int line;
{
	char buf[STRSIZE];
	int a;

	(void) move (lines, 0);
	close_disp ();

	(void) sprintf (buf, "assert failed: file %s line %d: %s",
			file, line, expression);

	a = 1; /* keep lint quiet */
	a /= 0; /* force a core dump */
	a = a; /* keep lint quiet */
}

/*
End the game by cleaning up the display.
*/

void
empend ()
{
	close_disp ();
	exit (0);
}

/*
 * 03a 01Apr88 aml .Hacked movement algorithms for computer.
 * 02b 01Jun87 aml .First round of bug fixes.
 * 02a 01Jan87 aml .Translated to C.
 * 01b 27May85 cal .Fixed round number update bug. Made truename simple.
 * 01a 01Sep83 cal .Taken from a Decus tape
 */

void
ver ()
{
        (void) addstr ("EMPIRE, Version 5.00 site Amdahl 1-Apr-1988");
}

/*
Here is a little routine to perform consistency checking on the
database.  I'm finding that my database is becoming inconsistent,
and I've no idea why.  Possibly this will help.

We perform the following functions:

1)  Make sure no list contains loops.

2)  Make sure every object is in either the free list with 0 hits,
or it is in the correct object list and a location list with non-zero hits,
and an appropriate owner.

3)  Make sure every city is on the map.

4)  Make sure every object is in the correct location and that
objects on the map have non-zero hits.

5)  Make sure every object in a cargo list has a ship pointer.

6)  Make sure every object with a ship pointer is in that ship's
cargo list.
*/

static int in_free[LIST_SIZE]; /* TRUE if object in free list */
static int in_obj[LIST_SIZE]; /* TRUE if object in obj list */
static int in_loc[LIST_SIZE]; /* TRUE if object in a loc list */
static int in_cargo[LIST_SIZE]; /* TRUE if object in a cargo list */

void
check () {
	void check_cargo(), check_obj(), check_obj_cargo();
	
	long i, j;
	piece_info_t *p;
	
	/* nothing in any list yet */
	for (i = 0; i < LIST_SIZE; i++) {
		in_free[i] = 0;
		in_obj[i] = 0;
		in_loc[i] = 0;
		in_cargo[i] = 0;
	}
		
	/* Mark all objects in free list.  Make sure objects in free list
	have zero hits. */
	
	for (p = free_list; p != NULL; p = p->piece_link.next) {
		i = p - object;
		ASSERT (!in_free[i]);
		in_free[i] = 1;
		ASSERT (p->hits == 0);
		if (p->piece_link.prev)
			ASSERT (p->piece_link.prev->piece_link.next == p);
	}
	
	/* Mark all objects in the map.
	Check that cities are in corect location.
	Check that objects are in correct location,
	have a good owner, and good hits. */
	
	for (i = 0; i < MAP_SIZE; i++) {
		if (map[i].cityp) ASSERT (map[i].cityp->loc == i);
		
		for (p = map[i].objp; p != NULL; p = p->loc_link.next) {
			ASSERT (p->loc == i);
			ASSERT (p->hits > 0);
			ASSERT (p->owner == USER || p->owner == COMP);
				
			j = p - object;
			ASSERT (!in_loc[j]);
			in_loc[j] = 1;
			
			if (p->loc_link.prev)
				ASSERT (p->loc_link.prev->loc_link.next == p);
		}
	}

	/* make sure all cities are on map */

	for (i = 0; i < NUM_CITY; i++)
		ASSERT (map[city[i].loc].cityp == &(city[i]));

	/* Scan object lists. */
	
	check_obj (comp_obj, COMP);
	check_obj (user_obj, USER);
	
	/* Scan cargo lists. */
	
	check_cargo (user_obj[TRANSPORT], ARMY);
	check_cargo (comp_obj[TRANSPORT], ARMY);
	check_cargo (user_obj[CARRIER], FIGHTER);
	check_cargo (comp_obj[CARRIER], FIGHTER);
	
	/* Make sure all objects with ship pointers are in cargo. */

	check_obj_cargo (comp_obj);
	check_obj_cargo (user_obj);
	
	/* Make sure every object is either free or in loc and obj list. */

	for (i = 0; i < LIST_SIZE; i++)
		ASSERT (in_free[i] != (in_loc[i] && in_obj[i]));
}

/*
Check object lists.  We look for:

1)  Loops and bad prev pointers.

2)  Dead objects.

3)  Invalid types.

4)  Invalid owners.
*/

void
check_obj (list, owner)
piece_info_t **list;
int owner;
{
	long i, j;
	piece_info_t *p;
	
	for (i = 0; i < NUM_OBJECTS; i++)
	for (p = list[i]; p != NULL; p = p->piece_link.next) {
		ASSERT (p->owner == owner);
		ASSERT (p->type == i);
		ASSERT (p->hits > 0);
		
		j = p - object;
		ASSERT (!in_obj[j]);
		in_obj[j] = 1;
	
		if (p->piece_link.prev)
			ASSERT (p->piece_link.prev->piece_link.next == p);
	}
}

/*
Check cargo lists.  We assume object lists are valid.
as we will place bits in the 'in_cargo' array that are used by
'check_obj'.

Check for:

1)  Number of items in list is same as cargo count.

2)  Type of cargo is correct.

3)  Location of cargo matches location of ship.

4)  Ship pointer of cargo points to correct ship.

5)  There are no loops in cargo list and prev ptrs are correct.

6)  All cargo is alive.
*/

void
check_cargo (list, cargo_type)
piece_info_t *list;
int cargo_type;
{
	piece_info_t *p, *q;
	long j, count;
	
	for (p = list; p != NULL; p = p->piece_link.next) {
		count = 0;
		for (q = p->cargo; q != NULL; q = q->cargo_link.next) {
			count += 1; /* count items in list */
			ASSERT (q->type == cargo_type);
			ASSERT (q->owner == p->owner);
			ASSERT (q->hits > 0);
			ASSERT (q->ship == p);
			ASSERT (q->loc == p->loc);
			
			j = q - object;
			ASSERT (!in_cargo[j]);
			in_cargo[j] = 1;

			if (p->cargo_link.prev)
				ASSERT (p->cargo_link.prev->cargo_link.next == p);
                }
		ASSERT (count == p->count);
        }
}

/*
Scan through object lists making sure every object with a ship
pointer appears in a cargo list.  We assume object and cargo
lists are valid.
*/

void
check_obj_cargo (list)
piece_info_t **list;
{
	piece_info_t *p;
	long i;

	for (i = 0; i < NUM_OBJECTS; i++)
	for (p = list[i]; p != NULL; p = p->piece_link.next) {
		if (p->ship) ASSERT (in_cargo[p-object]);
	}
}
