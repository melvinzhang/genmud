#include<stdio.h>
#include<ctype.h>
#include<string.h>
#include<stdlib.h>
#include "serv.h"
#include "society.h"
#include "historygen.h"
#include "areagen.h"


/* Each align gets an ancient race that can help it if it's found. 
   The neutral race is designed to be BUFFED so that players have a
   damn hard time dealing with it. */

char *old_races[ALIGN_MAX];

/* Each align gets certain gods they can worship. */

char *old_gods[ALIGN_MAX][MAX_GODS_PER_ALIGN];

/* Lists of what the god is a god of...*/

char *old_gods_spheres[ALIGN_MAX][MAX_GODS_PER_ALIGN];

void
init_historygen_vars (void)
{
  int al, gd;
  
  for (al = 0; al < ALIGN_MAX; al++)
    {
      old_races[al] = nonstr;
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  old_gods[al][gd] = nonstr;
	  old_gods_spheres[al][gd] = nonstr;
	}
    }
  return;
}

/* Read in the history data on the races and gods and so forth. */

void
read_history_data (void)
{
  int al, gd;
  FILE *f;
  
  if ((f = wfopen ("history.dat", "r")) == NULL)
    return;
  
  for (al = 0; al < ALIGN_MAX; al++)
    {
      free_str (old_races[al]);
      old_races[al] = new_str (read_string (f));
    }
  
  for (al = 0; al < ALIGN_MAX; al++)
    {
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  free_str (old_gods[al][gd]);
	  old_gods[al][gd] = new_str (read_string (f));
	}
    }
  for (al = 0; al < ALIGN_MAX; al++)
    {
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  free_str (old_gods_spheres[al][gd]);
	  old_gods_spheres[al][gd] = new_str (read_string (f));
	}
    }
  fclose (f);
  return;
}

/* This writes out the history data: ailgn races and align gods and
   spheres. */

void
write_history_data (void)
{
  FILE *f;
  int al, gd;
  if ((f = wfopen ("history.dat", "w")) == NULL)
    {
      log_it ("Couldn't open history.dat for writing!");
      return;
    }

  for (al = 0; al < ALIGN_MAX; al++)
    {
      fprintf (f, "%s`\n", old_races[al]);
    }
  for (al = 0; al < ALIGN_MAX; al++)
    {
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  fprintf (f, "%s`\n", old_gods[al][gd]);
	}
    }
  for (al = 0; al < ALIGN_MAX; al++)
    {
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  fprintf (f, "%s`\n", old_gods_spheres[al][gd]);
	}
    }
  fclose (f);
  return;
}

void
historygen (void)
{
  int num_ages, age;
  HELP *help;
  bigbuf[0] = '\0';

  /* Make sure the helpfile exists. */

  if ((help = find_help_name (NULL, "HISTORY")) == NULL &&
      (help = find_help_name (NULL, "BACKSTORY")) == NULL)
    {
      help = new_help();
      help->keywords = new_str ("HISTORY BACKSTORY");
      setup_help (help);
    }
  
  /* Set up the ancient races and the gods and so forth. */

  historygen_setup ();

  num_ages = nr (2,4);
  
  /* Now generate each of the ages. */

  for (age = 0; age < num_ages; age++)
    add_to_bigbuf (historygen_age(age));
  
  free_str (help->text);
  help->text = new_str (bigbuf);
  write_helps();
  return;
}

/* This shows the mythology of the world to a player. */

void
do_mythology (THING *th, char *arg)
{
  int al, gd;
  RACE *align;
  char buf[STD_LEN];

  if (LEVEL (th) == MAX_LEVEL && IS_PC (th) &&
      !str_cmp (arg, "setup"))
    historygen_setup();
  
  for (al = 0; al < ALIGN_MAX; al++)
    {
      if ((align = find_align (NULL, al)) == NULL ||
	  !old_gods[al][0] || !*old_gods[al][0])
	continue;
      
      sprintf (buf, "The \x1b[1;3%dm%s\x1b[0;37m Deities:\n\r", 
	       MID(0,align->vnum,7), align->name);
      stt (buf, th);
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  if (old_gods[al][gd] && *old_gods[al][gd])
	    {
	      sprintf (buf, "%-30s %s\n", old_gods[al][gd], old_gods_spheres[al][gd]);
	      stt (buf, th);
	    }
	}
      stt ("\n\r", th);
    }
  return;
}
  

/* This sets up the historygen data. */

void
historygen_setup (void)
{

  historygen_generate_races();
  historygen_generate_gods();
  write_history_data();  
  return;
}

/* This sets up the historygen races. */

void
historygen_generate_races (void)
{
  int al;
  RACE *align;
  for (al = 0; al < ALIGN_MAX; al++)
    {
      if ((align = find_align (NULL, al)) != NULL)
	{
	  free_str (old_races[al]);
	  old_races[al] = new_str (create_society_name(NULL));
	}
    }
  return;
}

/* This sets up the historygen gods and their spheres of influence. */

void
historygen_generate_gods (void)
{
  int al, gd, max_gods;
  
  int num_spheres, sph, count;
  char buf[STD_LEN]; /* Big buffer for all spheres. */
  char buf2[STD_LEN]; /* Small buffer for each sphere. */
  char spheretype[STD_LEN]; /* Good or bad spheres. */
  RACE *align;
  int name_tries;
  bool name_ok = FALSE;
  for (al = 0; al < ALIGN_MAX; al++)
    {
      
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  free_str (old_gods[al][gd]);
	  old_gods[al][gd] = nonstr;
	  free_str (old_gods_spheres[al][gd]);
	  old_gods_spheres[al][gd] = nonstr;
	}
      
      if ((align = find_align (NULL, al)) == NULL)
	continue;
      
      max_gods = nr (MAX_GODS_PER_ALIGN/2, MAX_GODS_PER_ALIGN);
      for (gd = 0; gd < max_gods; gd++)
	{
	  if (nr (1,6) != 3)
	    strcpy (spheretype, "godly_spheres_good");
	  else
	    strcpy (spheretype, "godly_spheres_bad");
	  
	  name_tries = 0;
	  do 
	    {
	      name_ok = TRUE;
	      free_str (old_gods[al][gd]);
	      old_gods[al][gd] = nonstr;
	      old_gods[al][gd] = new_str (create_society_name (NULL));
	      
	      
	      if (strlen(old_gods[al][gd]) < 4)
		name_ok = FALSE;
	      /* Truncate name */
	      old_gods[al][gd][8] = '\0';
	    }
	  while (!name_ok && ++name_tries < 100);
	  
	  num_spheres = 2;
	  if (nr (1,5) == 3)
	    num_spheres--;
	  if (nr (1,5) == 1)
	    num_spheres++;
	  if (nr (1,5) == 2)
	    num_spheres++;
	  buf[0] = '\0';
	  for (sph = 0; sph < num_spheres; sph++)
	    {
	      /* Keep trying for words while none are made or the
		 word has already been used for this deity. */
	      count = 0;
	      do
		{
		  buf2[0] = '\0';
		  strcpy (buf2, find_gen_word (HISTORYGEN_AREA_VNUM, spheretype, NULL));
		}
	      while ((!*buf2 || strstr (buf, buf2)) && ++count < 100);
	      
	      /* Now add the new sphere to the god's list. */
	      if (*buf && *buf2)
		{
		  if (sph == num_spheres - 1)
		    strcat (buf, " and ");
		  else
		    strcat (buf, ", ");
		}
	      capitalize_all_words (buf2);
	      strcat (buf, buf2);
	      old_gods_spheres[al][gd] = new_str (buf);
	    }
	}
    }
  return;
}


/* This generates an age of history. */

char *
historygen_age (int age)
{
  

  return nonstr;
}
