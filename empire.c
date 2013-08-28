/* $Id: empire.c,v 1.4 2000/07/28 05:12:51 esr Exp $  - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
empire.c -- this file contains initialization code, the main command
parser, and the simple commands.
*/

#include <stdio.h>
#include <curses.h>
#include "empire.h"
#include "extern.h"

void c_examine(), c_movie();

void
empire () {
	void do_command();
	void print_zoom();

	char order;
	int turn = 0;

	ttinit (); /* init tty */
	rndini (); /* init random number generator */

	clear_screen (); /* nothing on screen */
	(void) move (7, 0);
	ver ();
	pos_str (8, 0, "Detailed directions are in EMPIRE.DOC\n",0,0,0,0,0,0,0,0);
	(void) refresh ();

	if (!restore_game ()) /* try to restore previous game */
		init_game (); /* otherwise init a new game */

	/* Command loop starts here. */

	for (;;) { /* until user quits */
	    if (automove) { /* don't ask for cmd in auto mode */
		user_move ();
		comp_move (1);
		if (++turn % save_interval == 0)
			save_game ();
	    }
	    else {
		prompt (0,0,0,0,0,0,0,0,0); /* blank top line */
		(void) refresh ();
	        prompt ("Your orders? ",0,0,0,0,0,0,0,0);
	        order = get_chx (); /* get a command */
		do_command (order);
	    }
	}
}

/*
Execute a command.
*/

void
do_command (orders)
char orders;
{
	void c_debug(), c_quit(), c_sector(), c_map();
	void c_give();

	char e;
	int ncycle;

	switch (orders) {
	case 'A': /* turn on auto move mode */
		automove = TRUE;
		error ("Now in Auto-Mode",0,0,0,0,0,0,0,0);
		user_move ();
		comp_move (1);
		save_game ();
		break;

	case 'C': /* give a city to the computer */
		c_give ();
		break;
	
	case 'D': /* display round number */
		error ("Round #%d", date,0,0,0,0,0,0,0);
		break;

	case 'E': /* examine enemy map */
		if (resigned) c_examine ();
		else huh (); /* illegal command */
		break;

	case 'F': /* print map to file */
		c_map ();
		break;

	case 'G': /* give one free enemy move */
		comp_move (1);
		break;

	case 'H': /* help */
		help (help_cmd, cmd_lines);
		break;

	case 'J': /* edit mode */
		ncycle = cur_sector ();
		if (ncycle == -1) ncycle = 0;
		edit (sector_loc (ncycle));
		break;

	case 'M': /* move */
		user_move ();
		comp_move (1);
		save_game ();
		break;

	case 'N': /* give enemy free moves */
		ncycle = getint ("Number of free enemy moves: ");
		comp_move (ncycle);
		save_game ();
		break;

	case 'P': /* print a sector */
		c_sector ();
		break;

	case '\026': /* some interrupt */
	case 'Q': /* quit */
		c_quit ();
		break;

	case 'R': /* restore game */
		clear_screen ();
		e = restore_game ();
		break;

	case 'S': /* save game */
		save_game ();
		break;
	
	case 'T': /* trace: toggle save_movie flag */
		save_movie = !save_movie;
		if (save_movie) comment ("Saving movie screens to 'empmovie.dat'.",0,0,0,0,0,0,0,0);
		else comment ("No longer saving movie screens.",0,0,0,0,0,0,0,0);
		break;

	case 'W': /* watch movie */
		if (resigned || debug) replay_movie ();
		else error ("You cannot watch movie until computer resigns.",0,0,0,0,0,0,0,0);
		break;
	
	case 'Z': /* print compressed map */
		(void) clear ();
		print_zoom (user_map);
		(void) refresh ();
		break;

	case '\014': /* redraw the screen */
		redraw ();
		break;

	case '+': /* change debug state */
		e = get_chx();
		if ( e  ==  '+' ) debug = TRUE;
		else if ( e  ==  '-' ) debug = FALSE;
		else huh ();
		break;

	default:
		if (debug) c_debug (orders); /* debug */
		else huh (); /* illegal command */
		break;
	}
	e = e; /* keep lint quiet */
}

/*
Give an unowned city (if any) to the computer.  We make
a list of unowned cities, choose one at random, and mark
it as the computers.
*/

void
c_give () {
	int unowned[NUM_CITY];
	long i, count;

	count = 0; /* nothing in list yet */
	for (i = 0; i < NUM_CITY; i++) {
		if (city[i].owner == UNOWNED) {
			unowned[count] = i; /* remember this city */
			count += 1;
		}
	}
	if (count == 0) {
		error ("There are no unowned cities.",0,0,0,0,0,0,0,0);
		ksend ("There are no unowned cities.",0,0,0,0,0,0,0,0);
		return;
	}
	i = irand (count);
	i = unowned[i]; /* get city index */
	city[i].owner = COMP;
	city[i].prod = NOPIECE;
	city[i].work = 0;
	scan (comp_map, city[i].loc);
}

/*
Debugging commands should be implemented here.  
The order cannot be any legal command.
*/

void
c_debug (order)
char order;
{
	char e;

	switch (order) {
	case '#' : c_examine (); break;
	case '%' : c_movie (); break;
	
	case '@': /* change trace state */
		e = get_chx();
		if ( e  ==  '+' ) trace_pmap = TRUE;
		else if ( e  ==  '-' ) trace_pmap = FALSE;
		else huh ();
		break;

	case '$': /* change print_debug state */
		e = get_chx();
		if ( e  ==  '+' ) print_debug = TRUE;
		else if ( e  ==  '-' ) print_debug = FALSE;
		else huh ();
		break;

	case '&': /* change print_vmap state */
		print_vmap = get_chx();
		break;

	default: huh (); break;
	}
}

/*
The quit command.  Make sure the user really wants to quit.
*/

void
c_quit () {
	if (getyn ("QUIT - Are you sure? ")) {
	    empend ();
	}
}

/*
Print a sector.  Read the sector number from the user
and print it.
*/

void
c_sector () {
	int num;

	num = get_range ("Sector number? ", 0, NUM_SECTORS-1);
	print_sector_u (num);
}

/*
Print the map to a file.  We ask for a filename, attempt to open the
file, and if successful, print out the user's information to the file.
We print the map sideways to make it easier for the user to print
out the map.
*/

void
c_map () {
	FILE *f;
	int i, j;
	char line[MAP_HEIGHT+2];

	prompt ("Filename? ",0,0,0,0,0,0,0,0);
	get_str (jnkbuf, STRSIZE);

	f = fopen (jnkbuf, "w");
	if (f == NULL) {
		error ("I can't open that file.",0,0,0,0,0,0,0,0);
		return;
	}
	for (i = 0; i < MAP_WIDTH; i++) { /* for each column */
		for (j = MAP_HEIGHT-1; j >= 0; j--) { /* for each row */
                        line[MAP_HEIGHT-1-j] = user_map[row_col_loc(j,i)].contents;
		}
		j = MAP_HEIGHT-1;
		while (j >= 0 && line[j] == ' ') /* scan off trailing blanks */
			j -= 1;
			
		line[++j] = '\n';
		line[++j] = 0; /* trailing null */
		(void) fputs (line, f);
	}
	(void) fclose (f);
}

/*
Allow user to examine the computer's map.
*/

void
c_examine () {
	int num;

	num = get_range ("Sector number? ", 0, NUM_SECTORS-1);
	print_sector_c (num);
}

/*
We give the computer lots of free moves and
Print a "zoomed" version of the computer's map.
*/

void
c_movie () {
	(void) clear ();
	for (;;) {
		comp_move (1);
		print_zoom (comp_map);
		save_game ();
#ifdef PROFILE
		if (date == 125) empend();
#endif
	}
}
