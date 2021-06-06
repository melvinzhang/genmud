



/* The world is maxxed out at 13x13 areas. */
/* If you change this number it must be congruent to 1 (mod 4). In
   other words, it must be 4k+1 for some positive integer k. */
#define WORLDGEN_MAX     33

/* The size of each of the worldgen areas. */
#define WORLDGEN_AREA_SIZE  500

/* The start vnums for the upper and lower worldgen areas. */
/* Note that this may overlap with the wilderness area if the numbers
   are too big. Deal with it by making the vnum size a bit smaller. */
#define WORLDGEN_START_VNUM             200000
#define WORLDGEN_VNUM_SIZE              400000
#define WORLDGEN_UNDERWORLD_START_VNUM  (WORLDGEN_START_VNUM+WORLDGEN_VNUM_SIZE)
#define WORLDGEN_END_VNUM (WORLDGEN_UNDERWORLD_START_VNUM+WORLDGEN_VNUM_SIZE)


/* This generates a world. */
void do_worldgen (THING *, char *);
void worldgen (THING *, char *);

/* This checks if there are any areas within the worldgen 
   range. */

bool worldgen_check_vnums (void);

/* This removes an object if it's an object from a worldgen area. */

void worldgen_remove_object (THING *obj);

/* This removes all worldgen objects from all players online or on disk. */

void worldgen_clear_player_items (void);

/* This nukes all areas within the worldgen range so a new worldgen can
   be made next reboot. */

void worldgen_clear (void);

/* This sets up the worldgen sectors by trying to match adjacent 
   sectors to what they want to be next to. */

void worldgen_add_sector (int x, int y);

/* This checks if the worldgen sectors are attached or not. */

bool worldgen_sectors_attached (void);

/* Check if an individual sector is attached. */

void worldgen_check_sector_attachment(int x, int y);

/* This sets the levels of the worldgen areas. */

void worldgen_generate_area_levels (void);


/* This shows the sectors to a player. */

void worldgen_show_sectors (THING *th);

/* This generates the actual areas to be used in the worldgen. */


void worldgen_generate_areas (int area_size);

/* This links the pieces of the map together. */

void worldgen_link_areas (void);

/* This links up 2 individual areas. */


void worldgen_add_exit (THING *area, int dir_to, int x, int y,  bool underwld);

/* This puts a link between the area in the top world and the
   area in the underworld below it. */

void worldgen_link_top_to_underworld (int x, int y);

/* This seeds the world with societies that have room flags as part of
   their definition and which aren't destructible. */

void worldgen_society_seed(void);

/* This finds areas on the edge of the worldgen map (just as it's being
   created) so that we can use these to link the alignment outposts to
   the world. */

THING *find_area_on_edge_of_world (int dir);

/* This links the alignment outposts to the world. */

void worldgen_link_align_outposts (void);

/* This adds catwalks between areas. */

void worldgen_add_catwalks_between_areas (void);

/* Create and clear quests */
void worldgen_clear_quests (THING *);
void worldgen_generate_quests (void);
