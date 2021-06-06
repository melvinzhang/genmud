


/* These functions are used to generate caves and 3d mazes. */

#define CAVEGEN_ALLOWED   0x00000001   /* Can we use this spot to cavegen? */
#define CAVEGEN_USED      0x00000002   /* Is this a part of the cave? */
#define CAVEGEN_CONNECTED 0x00000004   /* Is this room connected? */
#define CAVEGEN_ADDED_ADJACENT 0x00000008 /* Next to something that
					     was added this time...dont'
					     add this one. */

/* Size of cavegen grid is [CG_MAX*2+1][CG_MAX*2+1][CG_MAX] Middle starts
   at GC_MAX, CG_MAX, CG_MAX/2.  We assume that we don't go to the edges
   of the map when setting the dimensions of any particular cave. */

#define CAVEGEN_MAX  21


/* Min number of cavegen rooms: */

#define CAVEGEN_MIN_ROOMS 100

/* This clears 1 or more bits from the cave_grid matrix. */
void cavegen_clear_bit (int bit);



/* This runs the cavegen command...sets up the rooms. It only returns
   true if you actually just made a map just now. */
bool cavegen (THING *, char *);

/* This checks if all of the cavegen rooms are connected or not. */

bool cavegen_is_connected (void);


/* This is used for the DFS to see if the rooms are connected or not. */
void cavegen_check_connected (int x, int y, int z);

/* This seeds rooms into the cavegen map. */
void cavegen_seed_rooms (int dx, int dy, int dz); 


/* This seeds a few rooms at the edge of the map. The 0 args are randomized
   within the min/max_values and the one that's nonzero is set for that
   coord. */

void cavegen_seed_room (int x, int y, int z);



/* This attempts to attach more rooms to the cave. It does this by attaching
   new rooms adjacent to existing rooms in such a way that the rooms are
   not likely to end up in clumps. */

void cavegen_add_rooms (int dx, int dy, int dz, bool force_it);

/* This actually generates the cavegen rooms for use within an area.*/
bool cavegen_generate (THING *th, int start_vnum);

void cavegen_show (THING *th);

