/* $Id: main.c,v 1.3 1994/12/01 15:54:38 esr Exp $  - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
main.c -- parse command line for empire

options:

    -w water: percentage of map that is water.  Must be in the range
              10..90.  Default is 70.
	      
    -s smooth: amount of smoothing performed to generate map.  Must
	       be a nonnegative integer.  Default is 5.
	       
    -d delay:  number of milliseconds to delay between output.
               default is 2000 (2 seconds).

    -S saveinterval: sets turn interval between saves.
	       default is 10
*/

#include <stdio.h>
#include "empire.h"
#include "extern.h"

#define OPTFLAGS "w:s:d:S:"

main (argc, argv)
int argc;
char *argv[];
{
	int c;
	extern char *optarg;
	extern int optind;
	extern int opterr;      /* set to 1 to suppress error msg */
	int errflg = 0;
	int wflg, sflg, dflg, Sflg;
	int land;
	
	wflg = 70; /* set defaults */
	sflg = 5;
	dflg = 2000;
	Sflg = 10;

	/*
	 * extract command line options
	 */

	while ((c = getopt (argc, argv, OPTFLAGS)) != EOF) {
		switch (c) {
		case 'w':
			wflg = atoi (optarg);
			break;
		case 's':
			sflg = atoi (optarg);
			break;
		case 'd':
			dflg = atoi (optarg);
			break;
		case 'S':
			Sflg = atoi (optarg);
			break;
		case '?': /* illegal option? */
			errflg++;
			break;
		}
	}
	if (errflg || (argc-optind) != 0) {
		(void) printf ("empire: usage: empire [-w water] [-s smooth] [-d delay]\n");
		exit (1);
	}

	if (wflg < 10 || wflg > 90) {
		(void) printf ("empire: -w argument must be in the range 0..90.\n");
		exit (1);
	}
	if (sflg < 0) {
		(void) printf ("empire: -s argument must be greater or equal to zero.\n");
		exit (1);
	}
	
	if (dflg < 0 || dflg > 30000) {
		(void) printf ("empire: -d argument must be in the range 0..30000.\n");
		exit (1);
	}

	SMOOTH = sflg;
	WATER_RATIO = wflg;
	delay_time = dflg;
	save_interval = Sflg;

	/* compute min distance between cities */
	land = MAP_SIZE * (100 - WATER_RATIO) / 100; /* available land */
	land /= NUM_CITY; /* land per city */
	MIN_CITY_DIST = isqrt (land); /* distance between cities */

	empire (); /* call main routine */
	return (0);
}
