/* %W% %G% %U% - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
term.c -- this file contains various routines used to control the
user communications area of the terminal.  This area consists of
the top 3 lines of the terminal where messages are displayed to the
user and input is acquired from the user.

There are two types of output in this area.  One type is interactive
output.  This consists of a prompt line and an error message line.
The other type of output is informational output.  The user must
be given time to read informational output.

Whenever input is received, the top three lines are cleared and the
screen refreshed as the user has had time to read these lines.  We
also clear the 'need_delay' flag, saying that the user has read the
information on the screen.

When information is to be displayed, if the 'need_delay' flag is set,
we refresh the screen and pause momentarily to give the user a chance
to read the lines.  The new information is then displayed, and the
'need_delay' flag is set.
*/

#include <stdio.h>
#include <curses.h>
#include <ctype.h>

#include "empire.h"
#include "extern.h"

static int need_delay;
static FILE *my_stream;
void
/* VARARGS1 */
pdebug (s, a, b, c, d, e, f, g, h)
char *s;
int a, b, c, d, e, f, g, h;
{
	if (!print_debug) return;
	comment (s, a, b, c, d, e, f, g, h);
}

/*
Here are routines that handle printing to the top few lines of the
screen.  'topini' should be called at initialization, and whenever
we finish printing information to the screen.
*/

void
topini()
{
	info (0, 0, 0);
}
/*
Write a message to one of the top lines.
*/

void
/* VARARGS2 */
topmsg(linep, buf, a, b, c, d, e, f, g, h)
int linep;
char *buf;
int a, b, c, d, e, f, g, h;
{
	if (linep < 1 || linep > NUMTOPS)
		linep = 1;
	(void) move (linep - 1, 0);
	
	if (buf != NULL && strlen (buf) > 0)
		addprintf (buf, a, b, c, d, e, f, g, h);
	
	(void) clrtoeol ();
}

void
topmsg1 (linep, buf, a, b, c, d, e, f, g, h)
int linep;
char *buf;
char *a;
int b, c, d, e, f, g, h;
{
	if (linep < 1 || linep > NUMTOPS)
		linep = 1;
	(void) move (linep - 1, 0);
	
	if (buf != NULL && strlen (buf) > 0)
		addprintf1 (buf, a, b, c, d, e, f, g, h);
	
	(void) clrtoeol ();
}
void
topmsg2 (linep, buf, a, b, c, d, e, f, g, h)
int linep;
char *buf;
char *a, *e, *f;
int b, c, d, g, h;
{
	if (linep < 1 || linep > NUMTOPS)
		linep = 1;
	(void) move (linep - 1, 0);
	
	if (buf != NULL && strlen (buf) > 0)
		addprintf2 (buf, a, b, c, d, e, f, g, h);
	
	(void) clrtoeol ();
}
/*
Print a prompt on the first message line.
*/

void
/* VARARGS1 */
prompt (buf, a, b, c, d, e, f, g, h)
char *buf;
int a,b,c,d,e,f,g,h;
{
	topmsg (1, buf, a, b, c, d, e, f, g, h);
}
void
prompt1 (buf, a, b, c, d, e, f, g, h)
char *buf, *a;
int b,c,d,e,f,g,h;
{
	topmsg1 (1, buf, a, b, c, d, e, f, g, h);
}
void
prompt2 (buf, a, b, c, d, e, f, g, h)
char *buf, *a, *e, *f;
int b,c,d,g,h;
{
	topmsg2 (1, buf, a, b, c, d, e, f, g, h);
}

/*
Print an error message on the second message line.
*/

void
/* VARARGS1 */
error (buf, a, b, c, d, e, f, g, h)
char *buf;
int a, b, c, d, e, f, g, h;
{
	topmsg (2, buf, a, b, c, d, e, f, g, h);
}

/*
Print out extra information.
*/

void
/* VARARGS1 */
extra (buf, a, b, c, d, e, f, g, h)
char *buf;
int a, b, c, d, e, f, g, h;
{
	topmsg (3, buf, a, b, c, d, e, f, g, h);
}

/*
Print out a generic error message.
*/

void
huh ()
{
	error ("Type H for Help.",0,0,0,0,0,0,0,0);
}

/*
Display information on the screen.  If the 'need_delay' flag is set,
we force a delay, then print the information.  After we print the
information, we set the need_delay flag.
*/

void
info (a, b, c)
char *a, *b, *c;
{
	if (need_delay) delay ();
	topmsg (1, a,0,0,0,0,0,0,0,0);
	topmsg (2, b,0,0,0,0,0,0,0,0);
	topmsg (3, c,0,0,0,0,0,0,0,0);
	need_delay = (a || b || c);
}

void
set_need_delay () {
	need_delay = 1;
}

void
comment (buf, a, b, c, d, e, f, g, h)
char *buf;
int a, b, c, d, e, f, g, h;
{
	if (need_delay) delay ();
	topmsg (1, 0,0,0,0,0,0,0,0,0);
	topmsg (2, 0,0,0,0,0,0,0,0,0);
	topmsg (3, buf, a, b, c, d, e, f, g, h);
	need_delay = (buf != 0);
}
	
void
comment1 (buf, a, b, c, d, e, f, g, h)
char *buf, *a;
int b, c, d, e, f, g, h;
{
	if (need_delay) delay ();
	topmsg1 (1, 0,0,0,0,0,0,0,0,0);
	topmsg1 (2, 0,0,0,0,0,0,0,0,0);
	topmsg1 (3, buf, a, b, c, d, e, f, g, h);
	need_delay = (buf != 0);
}
	
/* kermyt begin */
void
ksend (buf, a, b, c, d, e, f, g, h)
char *buf;
int a,b,c,d,e,f,g,h;
{
	if(!(my_stream=fopen("info_list.txt","a")))
	{
		error("Cannot open info_list.txt",0,0,0,0,0,0,0,0);
		return;
	}
	fprintf(my_stream, buf, a, b, c, d, e, f, g, h);
	fclose(my_stream);
	return;
}
void
ksend1 (buf, a, b, c, d, e, f, g, h)
char *buf, *a;
int b,c,d,e,f,g,h;
{
	if(!(my_stream=fopen("info_list.txt","a")))
	{
		error("Cannot open info_list.txt",0,0,0,0,0,0,0,0);
		return;
	}
	fprintf(my_stream, buf, a, b, c, d, e, f, g, h);
	fclose(my_stream);
	return;
}
/* kermyt end */
/*
Get a string from the user, echoing characters all the while.
*/

void
get_str (buf, sizep)
char *buf;
int sizep;
{
	(void) echo();
	get_strq(buf, sizep);
	(void) noecho();
}

/*
Get a string from the user, ignoring the current echo mode.
*/

void
get_strq (buf, sizep)
char *buf;
int sizep;
{
	sizep = sizep; /* size of buf, currently unused */

	(void) nocrmode ();
	(void) refresh ();
	(void) getstr (buf);
	need_delay = FALSE;
	info (0, 0, 0);
	(void) crmode ();
}

/*
Get a character from the user and convert it to uppercase.
*/

char
get_chx()
{
	char c;

	c = get_cq ();

	if (islower(c))
		return (upper(c));
	else
		return (c);
}

/*
Input an integer from the user.
*/

int
getint (message)
char *message;
{
	char buf[STRSIZE];
	char *p;

	for (;;) { /* until we get a legal number */
		prompt (message,0,0,0,0,0,0,0,0);
		get_str (buf, sizeof (buf));
		
		for (p = buf; *p; p++) {
			if (*p < '0' || *p > '9') {
				error ("Please enter an integer.",0,0,0,0,0,0,0,0);
				break;
			}
		}
		if (*p == 0) { /* no error yet? */
			if (p - buf > 7) /* too many digits? */
				error ("Please enter a small integer.",0,0,0,0,0,0,0,0);
			else return (atoi (buf));
		}
	}
}

/*
Input a character from the user with echoing.
*/

char
get_c ()
{
	char c; /* one char and a null */

	(void) echo ();
	c = get_cq ();
	(void) noecho ();
	return (c);
}

/*
Input a character quietly.
*/

char
get_cq ()
{
	char c;

	(void) crmode ();
	(void) refresh ();
	c = getch ();
	topini (); /* clear information lines */
	(void) nocrmode ();
	return (c);
}

/*
Input a yes or no response from the user.  We loop until we get
a valid response.  We return TRUE iff the user replies 'y'.
*/

int
getyn (message)
char *message;
{
	char c;

	for (;;) {
		prompt (message,0,0,0,0,0,0,0,0);
		c = get_chx ();

		if (c == 'Y') return (TRUE);
		if (c == 'N') return (FALSE);

		error ("Please answer Y or N.",0,0,0,0,0,0,0,0);
	}
}

/*
Input an integer in a range.
*/

int
get_range (message, low, high)
char *message;
int low, high;
{
	int result;

	for (;;) {
		result = getint (message);

		if (result >= low && result <= high) return (result);

		error ("Please enter an integer in the range %d..%d.",low, high,0,0,0,0,0,0);
	}
}

/*
Print a screen of help information.
*/

void
help (text, nlines)
char **text;
int nlines;
{
	int i, r, c;
	int text_lines;

	text_lines = (nlines + 1) / 2; /* lines of text */

	clear_screen ();

	pos_str (NUMTOPS, 1, text[0],0,0,0,0,0,0,0,0); /* mode */
	pos_str (NUMTOPS, 41, "See empire.doc for more information.",0,0,0,0,0,0,0,0);

	for (i = 1; i < nlines; i++) {
		if (i > text_lines)
			pos_str (i - text_lines + NUMTOPS + 1, 41, text[i],0,0,0,0,0,0,0,0);
		else pos_str (i + NUMTOPS + 1, 1, text[i],0,0,0,0,0,0,0,0);
	}

	pos_str (text_lines + NUMTOPS + 2,  1, "--Piece---Yours-Enemy-Moves-Hits-Cost",0,0,0,0,0,0,0,0);
	pos_str (text_lines + NUMTOPS + 2, 41, "--Piece---Yours-Enemy-Moves-Hits-Cost",0,0,0,0,0,0,0,0);

	for (i = 0; i < NUM_OBJECTS; i++) {
		if (i >= (NUM_OBJECTS+1)/2) {
			r = i - (NUM_OBJECTS+1)/2;
			c = 41;
		}
		else {
			r = i;
			c = 1;
		}
		pos_str1 (r + text_lines + NUMTOPS + 3, c,"%-12s%c     %c%6d%5d%6d",
			piece_attr[i].nickname,
			piece_attr[i].sname,
			tolower (piece_attr[i].sname),
			piece_attr[i].speed,
			piece_attr[i].max_hits,
			piece_attr[i].build_time,0,0);		//FLAG

	}
	(void) refresh ();
}



