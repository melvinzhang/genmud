


/* Number of parts to an objgen name. */

/* The type part of the name. */
#define OBJECTGEN_NAME_OWNER     0
#define OBJECTGEN_NAME_A_AN      1
#define OBJECTGEN_NAME_PREFIX    2
#define OBJECTGEN_NAME_COLOR     3
#define OBJECTGEN_NAME_GEM       4
#define OBJECTGEN_NAME_METAL     5
#define OBJECTGEN_NAME_NONMETAL  6
#define OBJECTGEN_NAME_SOCIETY   7
#define OBJECTGEN_NAME_TYPE      8
#define OBJECTGEN_NAME_SUFFIX    9
#define OBJECTGEN_NAME_MAX      10

/* There are four things that are autoset on an object name: those are
   a_an, owner, society and suffix...the other 6 are optional. */

#define OBJECTGEN_NAME_EXTRA     6

/* Only N affects per rank in the object. */
#define MAX_AFF_PER_RANK 2

/* This is the function that generates an object. It attempts to 
   make an object of a certain level of a certain type (that can be
   specified or chosen randomly) and in a certain area. 
   You can also specify the vnum you want to fill right away and
   you can specify a society name or an owner name if you want. */

THING *objectgen (THING *area, int wear_loc, int level, int curr_vnum,
		  char *society_name, char *owner_name);

/* This calculates a random wear location: 20pct wield (weapon),
   70pct single (large) armor, 20pct jewelry (small) armor. 
   The size is based on the number of wear slots you have: 1 or
   more than 1. */


int objectgen_find_wear_location (void);

/* This returns an item name based on the type requested. */

char *objectgen_find_typename (int wear_loc, int *weapon_type);

/* This generates all of the names that will go into the object. */

void objectgen_generate_names (char names[OBJECTGEN_NAME_MAX][STD_LEN], 
			       char color[OBJECTGEN_NAME_MAX][STD_LEN],
			       int wear_loc, char *society_name, 
			       char *owner_name, int *weapon_type, int level);
/* This sets up the various names on the object that we're actually making. */

void objectgen_setup_names (THING *obj, char name[OBJECTGEN_NAME_MAX][STD_LEN],
			    char color[OBJECTGEN_NAME_MAX][STD_LEN]);

/* This sets up the stats and bonuses for the object. */

void objectgen_setup_stats (THING *object, int weapon_type);


/* This generates some fake metal names. */

void generate_metal_names (void);
