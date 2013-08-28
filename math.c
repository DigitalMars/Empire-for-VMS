/* %W% %G% %U% - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
math.c -- various mathematical routines.

This file contains routines used to create random integers.  The
initialization routine 'rndini' should be called at program startup.
The flavors of random integers that can be generated are:

    irand(n) -- returns a random integer in the range 0..n-1
    rndint(a,b) -- returns a random integer in the range a..b

Other routines include:

    dist (a, b) -- returns the straight-line distance between two locations.
*/

#include "empire.h"
#include "extern.h"

void rndini()
{
	srand((unsigned)(time(0) & 0xFFFF));
}

long irand(high)
long high;
{
	if (high < 2) {
		return (0);
	}
	return (rand() % high);
}

int rndint(minp, maxp)
int minp, maxp;
{
	int size;

	size = maxp - minp + 1;
	return ((rand() % size) + minp);
}

/*
Return the distance between two locations.  This is simply
the max of the absolute differnce between the x and y coordinates.
*/

#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? -(a) : (a))

int
dist (a, b)
long a, b;
{
	int ax, ay, bx, by;

	ax = loc_row (a);
	ay = loc_col (a);
	bx = loc_row (b);
	by = loc_col (b);

	return (MAX (ABS (ax-bx), ABS (ay-by)));
}

/*
Find the square root of an integer.  We actually return the floor
of the square root using Newton's method.
*/

int isqrt (n)
int n;
{
	int guess;
	
	ASSERT (n >= 0); /* can't take sqrt of negative number */

	if (n <= 1) return (n); /* do easy cases and avoid div by zero */
		
	guess = 2; /* gotta start somewhere */
	guess = (guess + n/guess) / 2;
	guess = (guess + n/guess) / 2;
	guess = (guess + n/guess) / 2;
	guess = (guess + n/guess) / 2;
	guess = (guess + n/guess) / 2;
	
	if (guess * guess > n) guess -= 1; /* take floor */
	return (guess);
}
