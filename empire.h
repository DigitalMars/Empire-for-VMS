/* %W% %G% %U% - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
empire.h -- type and constant declarations
*/

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

/* Redefine some functions for portability. */

#ifndef SYSV
#define memcpy(dst,src,len) bcopy((src),(dst),(len))
#define strchr(s,c) index(s,c)
typedef char chtype;
#define beep() (putchar('\7'))
#define napms(d) (usleep((d)*1000))
#else
char *memset();
char *memcpy();
#define bzero(dst,len) memset(dst,0,len)
#endif

typedef unsigned char uchar;

#define ASSERT(x) if (!(x)) assert ("x", __FILE__, __LINE__);
#define ABORT assert ("aborting", __FILE__, __LINE__)

/* directions one can move */
#define NORTH 0
#define NORTHEAST 1
#define EAST 2
#define SOUTHEAST 3
#define SOUTH 4
#define SOUTHWEST 5
#define WEST 6
#define NORTHWEST 7

#define NUMTOPS 3 /* number of lines at top of screen for messages */
#define NUMSIDES 6 /* number of lines at side of screen */
#define STRSIZE 80 /* number of characters in a string */

/* Information we maintain about cities. */

#define NUM_CITY 70
#define UNOWNED 0
#define USER 1
#define COMP 2

/* Piece types. */
#define ARMY 0
#define FIGHTER 1
#define PATROL 2
#define DESTROYER 3
#define SUBMARINE 4
#define TRANSPORT 5
#define CARRIER 6
#define BATTLESHIP 7
#define SATELLITE 8
#define NUM_OBJECTS 9 /* number of defined objects */
#define NOPIECE ((char)255) /* a 'null' piece */

#define LIST_SIZE 5000 /* max number of pieces on board */

typedef struct city_info {
	long loc; /* location of city */
	uchar owner; /* UNOWNED, USER, COMP */
	long func[NUM_OBJECTS]; /* function for each object */
	long work; /* units of work performed */
	char prod; /* item being produced */
} city_info_t;

/*
Types of programmed movement.  Use negative numbers for special
functions, use positive numbers to move toward a specific location.
*/

#define NOFUNC -1	/* no programmed function */
#define RANDOM -2	/* move randomly */
#define SENTRY -3	/* sleep */
#define FILL -4         /* fill transport */
#define LAND -5         /* land fighter at city */
#define EXPLORE -6      /* piece explores nearby */
#define ARMYLOAD -7     /* army moves toward and boards a transport */
#define ARMYATTACK -8   /* army looks for city to attack */
#define TTLOAD -9       /* transport moves toward loading armies */
#define REPAIR -10      /* ship moves toward port */
#define WFTRANSPORT -11 /* army boards a transport */
#define MOVE_N -12      /* move north */
#define MOVE_NE -13     /* move northeast */
#define MOVE_E -14      /* move east */
#define MOVE_SE -15     /* move southeast */
#define MOVE_S -16      /* move south */
#define MOVE_SW -17     /* move southwest */
#define MOVE_W -18      /* move west */
#define MOVE_NW -19     /* move northwest */

/* Index to list of function names. */
#define FUNCI(x) (-(x)-1)

/*
Macro to convert a movement function into a direction.
*/

#define MOVE_DIR(a) (-(a)+MOVE_N)

/*
Information we maintain about each piece.
*/

typedef struct { /* ptrs for doubly linked list */
	struct piece_info *next; /* pointer to next in list */
	struct piece_info *prev; /* pointer to prev in list */
} link_t;

typedef struct piece_info {
	link_t piece_link; /* linked list of pieces of this type */
	link_t loc_link; /* linked list of pieces at a location */
	link_t cargo_link; /* linked list of cargo pieces */
	int owner; /* owner of piece */
	int type; /* type of piece */
	long loc; /* location of piece */
	long func; /* programmed type of movement */
	short hits; /* hits left */
	int moved; /* moves made */
	struct piece_info *ship; /* pointer to containing ship */
	struct piece_info *cargo; /* pointer to cargo list */
	short count; /* count of items on board */
	short range; /* current range (if applicable) */
} piece_info_t;

/*
Macros to link and unlink an object from a doubly linked list.
*/

#define LINK(head,obj,list) { \
	obj->list.prev = NULL; \
	obj->list.next = head; \
	if (head) head->list.prev = obj; \
	head = obj; \
}

#define UNLINK(head,obj,list) { \
	if (obj->list.next) \
		obj->list.next->list.prev = obj->list.prev; \
        if (obj->list.prev) \
		obj->list.prev->list.next = obj->list.next; \
        else head = obj->list.next; \
	obj->list.next = NULL; /* encourage mem faults in buggy code */ \
	obj->list.prev = NULL; \
}

/* macros to set map and list of an object */
#define MAP(owner) ((owner) == USER ? user_map : comp_map)
#define LIST(owner) ((owner) == USER ? user_obj : comp_obj)

/* macro to step through adjacent cells */
#define FOR_ADJ(loc,new_loc,i) for (i=0; (i<8 ? new_loc=loc+dir_offset[i],1 : 0); i++)
#define FOR_ADJ_ON(loc,new_loc,i) FOR_ADJ(loc,new_loc,i) if (map[new_loc].on_board)

/*
We maintain attributes for each piece.  Attributes are currently constant,
but the way has been paved to allow user's to change attributes at the
beginning of a game.
*/

#define INFINITY 10000000 /* a large number */

typedef struct piece_attr {
	char sname; /* eg 'C' */
	char name[20]; /* eg "aircraft carrier" */
	char nickname[20]; /* eg "carrier" */
	char article[20]; /* eg "an aircraft carrier" */
	char plural[20]; /* eg "aircraft carriers" */
	char terrain[4]; /* terrain piece can pass over eg "." */
	uchar build_time; /* time needed to build unit */
	uchar strength; /* attack strength */
	uchar max_hits; /* number of hits when completely repaired */
	uchar speed; /* number of squares moved per turn */
	uchar capacity; /* max objects that can be held */
	long range; /* range of piece */
} piece_attr_t;

/*
There are 3 maps.  'map' describes the world as it actually
exists; it tells whether each map cell is land, water or a city;
it tells whether or not a square is on the board.

'user_map' describes the user's view of the world.  'comp_map' describes
the computer's view of the world.
*/

#define MAP_WIDTH 100
#define MAP_HEIGHT 60
#define MAP_SIZE (MAP_WIDTH * MAP_HEIGHT)

typedef struct real_map { /* a cell of the actual map */
	char contents; /* '+', '.', or '*' */
	uchar on_board; /* TRUE iff on the board */
	city_info_t *cityp; /* ptr to city at this location */
	piece_info_t *objp; /* list of objects at this location */
} real_map_t;

typedef struct view_map { /* a cell of one player's world view */
	char contents; /* '+', '.', '*', 'A', 'a', etc */
	long seen; /* date when last updated */
} view_map_t;

/* Define information we maintain for a pathmap. */

typedef struct {
	int cost; /* total cost to get here */
	int inc_cost; /* incremental cost to get here */
	char terrain; /* T_LAND, T_WATER, T_UNKNOWN, T_PATH */
} path_map_t;

#define T_UNKNOWN 0
#define T_PATH 1
#define T_LAND 2
#define T_WATER 4
#define T_AIR (T_LAND | T_WATER)

/* A record for counts we obtain when scanning a continent. */

typedef struct {
	int user_cities; /* number of user cities on continent */
	int user_objects[NUM_OBJECTS];
	int comp_cities;
	int comp_objects[NUM_OBJECTS];
	int size; /* size of continent in cells */
	int unowned_cities; /* number of unowned cities */
	int unexplored; /* unexplored territory */
} scan_counts_t;

/* Define useful constants for accessing sectors. */

#define SECTOR_ROWS 5 /* number of vertical sectors */
#define SECTOR_COLS 2 /* number of horizontal sectors */
#define NUM_SECTORS (SECTOR_ROWS * SECTOR_COLS) /* total sectors */
#define ROWS_PER_SECTOR ((MAP_HEIGHT+SECTOR_ROWS-1)/SECTOR_ROWS)
#define COLS_PER_SECTOR ((MAP_WIDTH+SECTOR_COLS-1)/SECTOR_COLS)

/* Information we need for finding a path for moving a piece. */

typedef struct {
	char city_owner; /* char that represents home city */
	char *objectives; /* list of objectives */
	int weights[11]; /* weight of each objective */
} move_info_t;

/* special weights */
#define W_TT_BUILD -1 /* special cost for city building a tt */

/* List of cells in the perimeter of our searching for a path. */

typedef struct {
	long len; /* number of items in list */
	long list[MAP_SIZE]; /* list of locations */
} perimeter_t;
