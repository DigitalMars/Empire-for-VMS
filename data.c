/* %W% %G% %U% - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
Static data.

One of our hopes is that changing the types of pieces that
exist in the game is mostly a matter of modifying this file.
However, see also the help routine, empire.h, and empire.doc.
*/

#include "empire.h"

/*
Piece attributes.  Notice that the Transport is allowed only one hit.
In the previous version of this game, the user could easily win simply
by building armies and troop transports.  We attempt to alleviate this
problem by making transports far more fragile.  We have also increased
the range of a fighter from 20 to 30 so that fighters will be somewhat
more useful.
*/

piece_attr_t piece_attr[] = {
	{'A', /* character for printing piece */
	 "army", /* name of piece */ 
	 "army", /* nickname */
	 "an army", /* name with preceding article */
	 "armies", /* plural */
	 "+", /* terrain */
	 5, /* units to build */
	 1, /* strength */
	 1, /* max hits */
	 1, /* movement */
	 0, /* capacity */
	 INFINITY}, /* range */

	/*
	 For fighters, the range is set to an even multiple of the speed.
	 This allows user to move fighter, say, two turns out and two
	 turns back.
	*/
	 
	{'F', "fighter", "fighter", "a fighter", "fighters",
		".+", 10, 1,  1, 8, 0, 32},

	{'P', "patrol boat", "patrol", "a patrol boat", "patrol boats",
		".",  15, 1,  1, 4, 0, INFINITY},
		
	{'D', "destroyer", "destroyer", "a destroyer", "destroyers",
		".",  20, 1,  3, 2, 0, INFINITY},

	{'S', "submarine", "submarine", "a submarine", "submarines",
		".",  20, 3,  2, 2, 0, INFINITY},

	{'T', "troop transport", "transport", "a troop transport", "troop transports",
		".",  30, 1,  1, 2, 6, INFINITY},

	{'C', "aircraft carrier", "carrier", "an aircraft carrier", "aircraft carriers",
		".",  30, 1,  8, 2, 8, INFINITY},

	{'B', "battleship", "battleship", "a battleship", "battleships",
		".",  40, 2, 10, 2, 0, INFINITY},
		
	{'Z', "satellite", "satellite", "a satellite", "satellites",
		".+", 50, 0, 1, 10, 0, 500}
};

/* Direction offsets. */

int dir_offset [] = {-MAP_WIDTH, /* north */
		     -MAP_WIDTH+1, /* northeast */
		     1, /* east */
		     MAP_WIDTH+1, /* southeast */
		     MAP_WIDTH, /* south */
		     MAP_WIDTH-1, /* southwest */
		     -1, /* west */
		     -MAP_WIDTH-1}; /* northwest */

/* Names of movement functions. */

char *func_name[] = {"none", "random", "sentry", "fill", "land",
			"explore", "load", "attack", "load", "repair",
			"transport",
			"W", "E", "D", "C", "X", "Z", "A", "Q"};

/* The order in which pieces should be moved. */
int move_order[] = {SATELLITE, TRANSPORT, CARRIER, BATTLESHIP, 
		    PATROL, SUBMARINE, DESTROYER, ARMY, FIGHTER};

/* types of pieces, in declared order */
char type_chars[] = "AFPDSTCBZ";

/* Lists of attackable objects if object is adjacent to moving piece. */

char tt_attack[] = "T";
char army_attack[] = "O*TACFBSDP";
char fighter_attack[] = "TCFBSDPA";
char ship_attack[] = "TCBSDP";

/* Define various types of objectives */

move_info_t tt_explore = { /* water objectives */
	COMP, /* home city */
	" ", /* objectives */
	{1} /* weights */
};
move_info_t tt_load = { /* land objectives */
	COMP, "$",           {1}
};

/*
Rationale for 'tt_unload':

     Any continent with four or more cities is extremely attractive,
and we should grab it quickly.  A continent with three cities is
fairly attractive, but we are willing to go slightly out of our
way to find a better continent.  Similarily for two cities on a
continent.  At one city on a continent, things are looking fairly
unattractive, and we are willing to go quite a bit out of our way
to find a better continent.

     Cities marked with a '0' are on continents where we already
have cities, and these cities will likely fall to our armies anyway,
so we don't need to dump armies on them unless everything else is
real far away.  We would prefer to use unloading transports for
taking cities instead of exploring, but we are willing to explore
if interesting cities are too far away.

     It has been suggested that continents containing one city
are not interesting.  Unfortunately, most of the time what the
computer sees is a single city on a continent next to lots of
unexplored territory.  So it dumps an army on the continent to
explore the continent and the army ends up attacking the city
anyway.  So, we have decided we might as well send the tt to
the city in the first place and increase the speed with which
the computer unloads its tts.
*/

move_info_t tt_unload     = {
	COMP, "9876543210 ", {1, 1, 1, 1, 1, 1, 11, 21, 41, 101, 61}
};

/*
 '$' represents loading tt must be first
 'x' represents tt producing city
 '0' represnets explorable territory
*/
move_info_t army_fight    = { /* land objectives */
	COMP, "O*TA ",       {1, 1, 1, 1, 11}
};
move_info_t army_load     = { /* water objectives */
	COMP, "$x",          {1, W_TT_BUILD}
};

move_info_t fighter_fight = {
	COMP, "TCFBSDPA ",   {1, 1, 5, 5, 5, 5, 5, 5, 9}
};
move_info_t ship_fight    = {
	COMP, "TCBSDP ",     {1, 1, 3, 3, 3, 3, 21}
};
move_info_t ship_repair   = {
	COMP, "X",           {1}
};

move_info_t user_army        = {
	USER, " ",   {1}
};
move_info_t user_army_attack = {
	USER, "*Xa ", {1, 1, 1, 12}
};
move_info_t user_fighter     = {
	USER, " ",   {1}
};
move_info_t user_ship        = {
	USER, " ",   {1}
};
move_info_t user_ship_repair = {
	USER, "O",   {1}
};

/*
Various help texts.
*/

char *help_cmd[] = {
	"COMMAND MODE",
	"Auto:     enter automove mode",
	"City:     give city to computer",
	"Date:     print round",
	"Examine:  examine enemy map",
	"File:     print map to file",
	"Give:     give move to computer",
	"Help:     display this text",
	"J:        enter edit mode",
	"Move:     make a move",
	"N:        give N moves to computer",
	"Print:    print a sector",
	"Quit:     quit game",
	"Restore:  restore game",
	"Save:     save game",
	"Trace:    save movie in empmovie.dat",
	"Watch:    watch movie",
	"Zoom:     display compressed map",
	"<ctrl-L>: redraw screen"
};
int cmd_lines = 19;

char *help_user[] = {
	"USER MODE",
	"QWE",
	"A D       movement directions",
	"ZXC",
	"<space>:  sit",
	"Build:    change city production",
	"Fill:     set func to fill",
	"Grope:    set func to explore",
	"Help:     display this text",
	"I <dir>:  set func to dir",
	"J:        enter edit mode",
	"Kill:     set func to awake",
	"Land:     set func to land",
	"Out:      leave automove mode",
	"Print:    redraw screen",
	"Random:   set func to random",
	"Sentry:   set func to sentry",
	"Upgrade:  set func to repair",
	"V <piece> <func>:  set city func",
	"Y:        set func to attack",
	"<ctrl-L>: redraw screen",
	"?:        describe piece"
};
int user_lines = 22;
	
char *help_edit[] = {
	"EDIT MODE",
	"QWE",
	"A D       movement directions",
	"ZXC",
	"Build:    change city production",
	"Fill:     set func to fill",
	"Grope:    set func to explore",
	"Help:     display this text",
	"I <dir>:  set func to dir",
	"Kill:     set func to awake",
	"Land:     set func to land",
	"Mark:     mark piece",
	"N:        set dest for marked piece",
	"Out:      exit edit mode",
	"Print:    print sector",
	"Random:   set func to random",
	"Sentry:   set func to sentry",
	"Upgrade:  set func to repair",
	"V <piece> <func>:  set city func",
	"Y:        set func to attack",
	"<ctrl-L>: redraw screen",
	"?:        describe piece"
};
int edit_lines = 22;
