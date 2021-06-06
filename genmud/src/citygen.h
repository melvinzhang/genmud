


/* This code makes cities. It starts with a NxNxH grid of empty rooms
   and sets up a base city shape by adding a block of rooms to the
   city grid.

   Then, it goes through and starts to add roads and alleyways 
   and gates inside the grid. It also sets up "walls" outside the
   city by adding a path around the city a few rooms deep, but only
   connects to the gates. 

   Then, it starts adding big blocks to the city like marketplace and
   governmental centers and so forth. It adds things like houses
   and towers and other things piece by piece.

*/

/* Cities are max CITY_SIZE across and CITY_HEIGHT tall. (You shouldn't use anywhere
   NEAR this number of rooms for a given city, however. */

#define CITY_SIZE     100
#define CITY_HEIGHT   10
#define CITY_STREET_LEVEL  2 /* Street level is height 2...(allows for
				 sewers and such below the city). */

/* Min and max numbers of rooms in the city to generate. */
#define CITYGEN_MIN_SIZE  100
#define CITYGEN_MAX_SIZE  2000



THING *city_grid[CITY_SIZE][CITY_SIZE][CITY_HEIGHT];


/* This generates a city area. */

void citygen (THING *th, char *arg);

/* This clears the city grid...the variable tells if we nuke the things
   in it or not. */
void clear_city_grid (bool nuke_things);

/* This checks if a given coord is within the city grid limits. */
bool city_coord_is_ok (int x, int y, int z);

/* This generates a base city grid of rooms or a base block of rooms for
   a building at the given vnum and puts it into the area
   the start_vnum is in. */

void generate_base_city_grid (THING *obj, int start_vnum);


/* This adds a detail to a city. This is normally first called in
   citygen() by going down the list of words in the citygen area
   and looking for the rooms corresponding to those words. */

void citygen_add_detail (THING *obj, THING *area_to, VALUE *search_dims, 
		     int depth);

/* This shows the city map to a player (street level only) */

void show_city_map (THING *th);

/* This connects blocks of rooms inside of the city. */

void connect_city_blocks (int x, int y, int z, int dir);
