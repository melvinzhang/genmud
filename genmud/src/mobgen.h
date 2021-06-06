

/* Mobs >= this level are strong, otherwise weak. */

#define STRONG_RANDPOP_MOB_MINLEVEL   70


/* These are for generating regular mobs within an area. */

void areagen_generate_persons (THING *area);

/* This generates a person in an area and gives it a level bonus. */

THING *areagen_generate_person (THING *area, int level_bonus);

/* Find a society name to give a mob. */

SOCIETY *persongen_find_society (void);

/* Find a person type to give to the person...it adds some stuff like flags
   and values and alters levels and things of that nature. */

THING *find_persongen_type (void);

/* This creates some objects and adds their resets to this person. */

void add_objects_to_person (THING *person, char *name);



/* These are used to generate base/background animal/creature type
   mobs within areas. */


void clear_base_randpop_mobs (THING *);

void generate_randpop_mobs (THING *);

/* This generates a single randpop mob for an area based on the names
   given. */

THING *generate_randpop_mob (THING *area, THING *proto, char *name, char *sname, char *lname, int curr_vnum);


/* This sets up the mob randpop item. */

void setup_mob_randpop_item (int start_vnum, int tier_size);
