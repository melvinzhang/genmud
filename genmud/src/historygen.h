

/* This is for the history generator. The idea is to make the
   histories of a sequence of ages. Each age looks at the list of
   societies and makes them "fight" and so forth and sets up
   leaders for different societies at different points and makes
   societies come into existence and leave. It also makes up some
   ancient protector/evil races and so forth so that when the players
   are in the world, they have certain things that they must ally with
   or avoid. */

#define MAX_GODS_PER_ALIGN          10


extern char *old_races[ALIGN_MAX];
extern char *old_gods[ALIGN_MAX][MAX_GODS_PER_ALIGN];

/* Read and write the historygen data. */

void read_history_data (void);
void write_history_data (void);

/* Initialize the history vars. */

void init_historygen_vars (void);

/* Generates all of the history. */
void historygen (void);

/* Set up the historygen data. */
void historygen_setup (void);

/* Creates the ancient races each alignment has...*/
void historygen_generate_races (void);

/* Generate gods for each race. */
void historygen_generate_gods (void);

/* Generate an age. */
char *historygen_age (int age);


/* Find a random generated society. */
SOCIETY *find_random_generated_society (int align);

