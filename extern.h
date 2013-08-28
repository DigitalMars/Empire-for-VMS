/* $Id: extern.h,v 1.4 2000/07/28 05:12:52 esr Exp $  - (c) Copyright 1987, 1988 Chuck Simmons */

/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
extern.h -- define global non-constant storage.
*/

/* user-supplied parameters */
int SMOOTH;        /* number of times to smooth map */
int WATER_RATIO;   /* percentage of map that is water */
int MIN_CITY_DIST; /* cities must be at least this far apart */
int delay_time;
int save_interval; /* turns between autosaves */

real_map_t map[MAP_SIZE]; /* the way the world really looks */
view_map_t comp_map[MAP_SIZE]; /* computer's view of the world */
view_map_t user_map[MAP_SIZE]; /* user's view of the world */

city_info_t city[NUM_CITY]; /* city information */

/*
There is one array to hold all allocated objects no matter who
owns them.  Objects are allocated from the array and placed on
a list corresponding to the type of object and its owner.
*/

piece_info_t *free_list; /* index to free items in object list */
piece_info_t *user_obj[NUM_OBJECTS]; /* indices to user lists */
piece_info_t *comp_obj[NUM_OBJECTS]; /* indices to computer lists */
piece_info_t object[LIST_SIZE]; /* object list */

/* Display information. */
int lines; /* lines on screen */
int cols; /* columns on screen */

/* constant data */
extern piece_attr_t piece_attr[];
extern int dir_offset[];
extern char *func_name[];
extern int move_order[];
extern char type_chars[];
extern char tt_attack[];
extern char army_attack[];
extern char fighter_attack[];
extern char ship_attack[];

extern move_info_t tt_load;
extern move_info_t tt_explore;
extern move_info_t tt_unload;
extern move_info_t army_fight;
extern move_info_t army_load;
extern move_info_t fighter_fight;
extern move_info_t ship_fight;
extern move_info_t ship_repair;
extern move_info_t user_army;
extern move_info_t user_army_attack;
extern move_info_t user_fighter;
extern move_info_t user_ship;
extern move_info_t user_ship_repair;

extern char *help_cmd[];
extern char *help_edit[];
extern char *help_user[];
extern int cmd_lines;
extern int edit_lines;
extern int user_lines;

/* miscellaneous */
long date; /* number of game turns played */
char automove; /* TRUE iff user is in automove mode */
char resigned; /* TRUE iff computer resigned */
char debug; /* TRUE iff in debugging mode */
char print_debug; /* TRUE iff we print debugging stuff */
char print_vmap; /* TRUE iff we print view maps */
char trace_pmap; /* TRUE if we are tracing pmaps */
int win; /* set when game is over */
char jnkbuf[STRSIZE]; /* general purpose temporary buffer */
char save_movie; /* TRUE iff we should save movie screens */
int user_score; /* "score" for user and computer */
int comp_score;

/* Screen updating macros */
#define display_loc_u(loc) display_loc(USER,user_map,loc)
#define display_loc_c(loc) display_loc(COMP,comp_map,loc)
#define print_sector_u(sector) print_sector(USER,user_map,sector)
#define print_sector_c(sector) print_sector(COMP,comp_map,sector)
#define loc_row(loc) ((loc)/MAP_WIDTH)
#define loc_col(loc) ((loc)%MAP_WIDTH)
#define row_col_loc(row,col) ((long)((row)*MAP_WIDTH + (col)))
#define sector_row(sector) ((sector)%SECTOR_ROWS)
#define sector_col(sector) ((sector)/SECTOR_ROWS)
#define row_col_sector(row,col) ((int)((col)*SECTOR_ROWS+(row)))

#define loc_sector(loc) \
	row_col_sector(loc_row(loc)/ROWS_PER_SECTOR, \
                       loc_col(loc)/COLS_PER_SECTOR)
		       
#define sector_loc(sector) row_col_loc( \
		sector_row(sector)*ROWS_PER_SECTOR+ROWS_PER_SECTOR/2, \
		sector_col(sector)*COLS_PER_SECTOR+COLS_PER_SECTOR/2)
		
/* global routines */

void empire();

void attack(piece_info_t *att_obj, long loc);
void comp_move(int nmoves);
void user_move();
void edit(long edit_cursor);

/* map routines */
void vmap_cont (int *cont_map, view_map_t *vmap, long loc, char bad_terrain);
void rmap_cont (int *cont_map, long loc, char bad_terrain);
void vmap_mark_up_cont (int *cont_map, view_map_t *vmap, long loc, char bad_terrain);
scan_counts_t vmap_cont_scan (int *cont_map, view_map_t *vmap);
scan_counts_t rmap_cont_scan (int *cont_map);
int map_cont_edge (int *cont_map, long loc);
long vmap_find_aobj (path_map_t path_map[], view_map_t *vmap, long loc, move_info_t *move_info);
long vmap_find_wobj (path_map_t path_map[], view_map_t *vmap, long loc, move_info_t *move_info);
long vmap_find_lobj (path_map_t path_map[], view_map_t *vmap, long loc, move_info_t *move_info);
long vmap_find_lwobj (path_map_t path_map[],view_map_t *vmap,long loc,move_info_t *move_info,int beat_cost);
long vmap_find_wlobj (path_map_t path_map[], view_map_t *vmap, long loc, move_info_t *move_info);
long vmap_find_dest (path_map_t path_map[], view_map_t vmap[], long cur_loc, long dest_loc, int owner, int terrain);
void vmap_prune_explore_locs (view_map_t *vmap);
void vmap_mark_path (path_map_t *path_map, view_map_t *vmap, long dest);
void vmap_mark_adjacent (path_map_t path_map[], long loc);
void vmap_mark_near_path (path_map_t path_map[], long loc);
long vmap_find_dir (path_map_t path_map[], view_map_t *vmap, long loc,  char *terrain, char *adjchar);
int vmap_count_adjacent (view_map_t *vmap, long loc, char *adj_char);
int vmap_shore (view_map_t *vmap, long loc);
int rmap_shore (long loc);
int vmap_at_sea (view_map_t *vmap, long loc);
int rmap_at_sea (long loc);

void kill_display (); /* display routines */
void sector_change ();
int cur_sector ();
long cur_cursor ();
void display_loc (int whose, view_map_t vmap[], long loc);
void display_locx (int whose, view_map_t vmap[], long loc);
void print_sector (int whose, view_map_t vmap[], int sector);
int move_cursor (long *cursor, int offset);
void print_zoom (view_map_t *vmap);
void print_pzoom (char *s, path_map_t *pmap, view_map_t *vmap);
void print_xzoom (view_map_t *vmap);
void display_score ();
#ifdef A_COLOR
void init_colors ();
#endif /* A_COLOR */

void init_game (); /* game routines */
void save_game ();
int restore_game ();
void save_movie_screen ();
void replay_movie ();

void get_str (char *buf, int sizep); /* input routines */
void get_strq (char *buf, int sizep);
char get_chx ();
int getint (char *message);
char get_c ();
char get_cq ();
int getyn (char *message);
int get_range (char *message, int low, int high);

void rndini (); /* math routines */
long irand (long high);
int dist (long a, long b);
int isqrt (int n);

int find_nearest_city (long loc, int owner, long *city_loc);
city_info_t *find_city (long loc); /* object routines */
piece_info_t *find_obj (int type, long loc);
piece_info_t *find_nfull (int type, long loc);
long find_transport (int owner, long loc);
piece_info_t *find_obj_at_loc (long loc);
int obj_moves (piece_info_t *obj);
int obj_capacity (piece_info_t *obj);
void kill_obj (piece_info_t *obj, long loc);
void kill_city (city_info_t *cityp);
void produce (city_info_t *cityp);
void move_obj (piece_info_t *obj, long new_loc);
void move_sat (piece_info_t *obj);
int good_loc (piece_info_t *obj, long loc);
void embark (piece_info_t *ship, piece_info_t *obj);
void disembark (piece_info_t *obj);
void describe_obj (piece_info_t *obj);
void scan (view_map_t vmap[], long loc);
void scan_sat (view_map_t *vmap, long loc);
void set_prod (city_info_t *cityp);

/* terminal routines */
void pdebug (char *s, int a, int b, int c, int d, int e, int f, int g, int h);
void topini ();
void clreol (int linep, int colp);
void topmsg (int linep, char *buf, int a, int b, int c, int d, int e, int f, int g, int h);
void topmsg1 (int linep, char *buf, char *a, int b, int c, int d, int e, int f, int g, int h);
void topmsg2 (int linep, char *buf, char *a, int b, int c, int d, char *e, char *f, int g, int h);
void prompt (char *buf, int a, int b, int c, int d, int e, int f, int g, int h);
void prompt1 (char *buf, char *a, int b, int c, int d, int e, int f, int g, int h);
void prompt2 (char *buf, char *a, int b, int c, int d, char *e, char *f, int g, int h);
void error (char *buf, int a, int b, int c, int d, int e, int f, int g, int h);
void info (char *a, char *b, char *c);
void comment (char *buf, int a, int b, int c, int d, int e, int f, int g, int h);
void comment1 (char *buf,char *a, int b, int c, int d, int e, int f, int g, int h);
void extra (char *buf, int a, int b, int c, int d, int e, int f, int g, int h);
void huh ();
void help (char **text, int nlines);
void set_need_delay ();
void ksend (char *buf, int a, int b, int c, int d, int e, int f, int g, int h);
void ksend1 (char *buf, char *a, int b, int c, int d, int e, int f, int g, int h);

/* utility routines */
void ttinit ();
void redraw ();
void clear_screen ();
void delay ();
void close_disp ();
void pos_str (int row, int col, char *str, int a, int b, int c, int d, int e, int f, int g, int h);
void pos_str1 (int row, int col, char *str, char *a, int b, int c, int d, int e, int f, int g, int h);
void addprintf (char *str, int a, int b, int c, int d, int e, int f, int g, int h);
void addprintf1 (char *str, char *a, int b, int c, int d, int e, int f, int g, int h);
void addprintf2 (char *str, char *a, int b, int c, int d, char *e, char *f, int g, int h);
void assert (char *expression, char *file, int line);
void empend ();
void ver ();
char upper (char c);
void tupper (char *str);
void check ();

/* randon routines we use */
long time();
void exit();
void perror();
void srand();
char *strcpy();
