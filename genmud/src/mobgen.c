#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "serv.h"
#include "society.h"
#include "areagen.h"
#include "worldgen.h"
#include "objectgen.h"
#include "mobgen.h"

/* We assume the area has a certain level, if not, set it to the
   area size/5 and go with it. */


void
areagen_generate_persons (THING *area)
{
  int num_times, i;
  int num_rooms = 0;
  THING *room;
  if (!area || !IS_AREA (area))
    return;
  
  for (room = area->cont; room; room = room->next_cont)
    {
      if (IS_ROOM (room))
	num_rooms++;
    }
  
  num_times = nr (2 + num_rooms/30, 2 + num_rooms/15);
  
  area->level = MID (AREA_LEVEL_MIN, area->level, AREA_LEVEL_MAX);
  
  for (i = 0; i < num_times; i++)
    areagen_generate_person (area, 0);
  

  add_reset (area, MOB_RANDPOP_VNUM, 50, area->mv/3, 1);
  
}

/* This finds an appropriate society to use to name the person. It looks
   for generated societies that aren't destructible. */

SOCIETY *
persongen_find_society (void)
{
  int count, num_choices = 0, num_chose = 0;
  SOCIETY *soc;

  for (count = 0; count < 2; count++)
    {
      for (soc = society_list; soc; soc = soc->next)
	{
	  if (IS_SET (soc->society_flags, SOCIETY_DESTRUCTIBLE) ||
	      soc->generated_from == 0 || !soc->name ||
	      !*soc->name)
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return NULL;
	  
	  num_chose = nr (1, num_choices);
	}
    }
  return soc;
}
	      

  
/* This finds a kind of type for the person by using the persongen area. */

THING *
find_persongen_type (void)
{
  THING *area, *person; 
  int count, num_choices = 0, num_chose = 0;

  if ((area = find_thing_num (PERSONGEN_AREA_VNUM)) == NULL ||
      !IS_AREA (area))
    return NULL;
  
  
  for (count = 0; count < 2; count++)
    {
      for (person = area->cont; person; person = person->next_cont)
	{
	  if (IS_ROOM (person) || !person->name || !*person->name)
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return NULL;
	  
	  num_chose = nr (1, num_choices);
	}
    }
  return person;
}

/* This generates a person for an area. */

THING *
areagen_generate_person (THING *area, int level_bonus)
{
  THING *person, *proto, *oldperson, *room;
  VALUE *val;
  int curr_vnum;
  int area_room_flags;
  char namebuf[STD_LEN];
  char socinamebuf[STD_LEN];
  char typebuf[STD_LEN];
  char a_anbuf[STD_LEN];
  char buf[STD_LEN];
  SPELL *spl;
  
  namebuf[0] = '\0';
  if (!area || !IS_AREA (area))
    return NULL;

  area->level = MID (AREA_LEVEL_MIN, area->level, AREA_LEVEL_MAX);
  /* Find an open slot to make this person. */

  for (curr_vnum = area->vnum + area->mv+1; curr_vnum <
	 area->vnum+area->max_mv; curr_vnum++)
    {
      if ((oldperson = find_thing_num (curr_vnum)) == NULL)
	break;
    }
  if (curr_vnum <= area->vnum + area->mv ||
      curr_vnum >= area->vnum + area->max_mv)
    return NULL;
  
  /* Get the society name. */

  strcpy (socinamebuf, find_random_society_name ('n', TRUE));
  
  
  if ((proto = find_persongen_type()) == NULL &&
      proto->name && *proto->name)
    return NULL;
  
  /* Now we have a society, a name and a type. Now get name and 
     move on. */
  
  if ((person = new_thing()) == NULL)
    return NULL;
  
  person->vnum = curr_vnum;
  if (curr_vnum > top_vnum)
    top_vnum = curr_vnum;
  
  person->level = MAX (10, nr (area->level/2, area->level) +
		       nr (0, proto->level) + level_bonus);
  if (proto->sex == SEX_NEUTER)
    person->sex = nr (SEX_FEMALE, SEX_MALE);
  else
    person->sex = proto->sex;
  /* Set to unique. */
  person->max_mv = 1;
  person->thing_flags = proto->thing_flags;
  thing_to (person, area);
  add_thing_to_list (person);
  
  
  if (nr (1,7) != 3)
    {
      strcpy (namebuf, create_society_name (NULL));
      if (*namebuf)
	strcat (namebuf, " ");
    }
  else
    namebuf[0] = '\0';
  
  strcpy (typebuf, proto->name);
  
  if (namebuf[0] && nr (1,5) == 3)
    socinamebuf[0] = '\0';
  else if (*socinamebuf)
    strcat (socinamebuf, " ");
  
  sprintf (buf, "%s%s%s", namebuf, socinamebuf, typebuf);  
  free_str (person->name);
  person->name = new_str (buf);
  add_flagval (person, FLAG_ROOM1, 
	       flagbits(area->flags, FLAG_ROOM1) & ~ROOM_WATERY);
  if (namebuf[0])
    {
      sprintf (buf, "%sthe %s%s",
	       namebuf, socinamebuf, typebuf);
      free_str (person->short_desc);
      person->short_desc = new_str (buf);
      strcat (buf, " is here.");
      buf[0] = UC(buf[0]);
      free_str (person->long_desc);
      person->long_desc = new_str (buf);
    }
  else
    {
      if (*socinamebuf)
	strcpy (a_anbuf, a_an(socinamebuf));
      else
	strcpy (a_anbuf, a_an (typebuf));
		
      sprintf (buf, "%s %s%s",
	       a_anbuf, socinamebuf, typebuf);
       free_str (person->short_desc);
      person->short_desc = new_str (buf);
      strcat (buf, " is here.");
      buf[0] = UC(buf[0]);
      free_str (person->long_desc);
      person->long_desc = new_str (buf);
    }

  /* Now copy values and flags. */

  copy_flags (proto, person);
  copy_values (proto, person);
  
  /* Set up casting persons. */
  for (val = person->values; val; val = val->next)
    {
      if (val->type == VAL_CAST)
	{
	  int i;
	  for (i = 0; i < NUM_VALS; i++)
	    {
	      val->val[i] = nr (1, 200);
	      if ((spl = find_spell (NULL, val->val[i])) == NULL ||
		  IS_SET (flagbits (spl->flags, FLAG_SPELL), 
			  SPELL_TRANSPORT))
		{
		  val->val[i] = 0;
		}
	    }
	}
    }

  if (person->level >= 80)
    add_flagval (person, FLAG_DET, AFF_DARK);
  if (person->level >= 120)
    add_flagval (person, FLAG_DET, AFF_INVIS);
  if (person->level >= 200)
    add_flagval (person, FLAG_DET, AFF_CHAMELEON);
  
  /* The reset gets put into the area. Used to be the room. */

  area_room_flags = flagbits (area->flags, FLAG_ROOM1) & ROOM_SECTOR_FLAGS & ~BADROOM_BITS;
  
  if (nr (1,3) == 2 &&
      (room = find_random_room (area, FALSE, area_room_flags, BADROOM_BITS)) != NULL)
    add_reset (room, curr_vnum, 100, 1, 1);
  else
    add_reset (area, curr_vnum, 100, 1, 1);
  
  /* Now add objects to the person. */
  
  add_objects_to_person (person, namebuf);
  
  return person;
}


/* This adds some objects to the person. A lot of the time, it doesn't
   have any objects, but sometimes it does and we add the objects then. */

void
add_objects_to_person (THING *person, char *name)
{
  THING *area, *obj;
  VALUE *cast;
  int num_objects = 0, i;
  
  if (!person || (area = person->in) == NULL || !IS_AREA (area))
    return;
  
  num_objects = nr (1,4);
  
  if (person->vnum < area->vnum + area->mv ||
      person->vnum + num_objects >= area->vnum + area->max_mv)
    return;
  
  if ((cast = FNV (person, VAL_CAST)) != NULL &&
      nr (1,3) == 2)
    {
      add_reset (person, 277, MAX(1, 10-person->level/15), 1, 1);
    }
  
      /* Add standard armor/wpn pops. */
  
  if (nr (1,3) == 2)
    add_reset (person, 270, 30, 2, 1);
  if (nr (1,3) == 2)
    add_reset (person, 271, 20, 5, 1);
  
  
  /* Sometimes there are no objects. */
  
  if (nr (1,5) == 2)
    return;
  
  num_objects = nr (1,  4+person->level/100);
  if (num_objects > 6)
    num_objects = 6;
  
  for (i = 0; i < num_objects; i++)
    {
      if ((obj = objectgen (NULL, 0,  nr (person->level*2/3,person->level*5/4),
			    person->vnum+i+1, NULL, 
			    (nr (1,6) == 3 ? name : NULL))) != NULL)
	{
	  add_reset (person, obj->vnum, MAX(2, 50-obj->level/2), 1, 1);	
	}
    }
  
  
  return;
}

/* This nukes all mobs in the mobgen area. */

void
clear_base_randpop_mobs (THING *th)
{
  THING *area, *obj;
  
  
  if (!th || LEVEL (th) < MAX_LEVEL || !IS_PC (th))
    {
      stt ("You can't clear the base randpop mobs.\n\r", th);
      return;
    }
  
  if ((area = find_thing_num (MOBGEN_LOAD_AREA_VNUM)) == NULL ||
      !IS_AREA (area))
    {
      stt ("The mobgen load area doesn't exist\n\r", th);
      return;
    }

  for (obj = area->cont; obj; obj = obj->next_cont)
    {
      if (!IS_ROOM (obj))
	{
	  SBIT (obj->thing_flags, TH_NUKE);
	}
    }
  area->thing_flags |= TH_CHANGED;
  stt ("Mobgen load area cleared.\n\r", th);
  return;
}


/* This attempts to generate the mobgen mobs in the mobgen area. First
   it checks to make sure that the area is empty. */


void
generate_randpop_mobs (THING *th)
{
  THING *area, *proto_area, *proto, *mob;

  int start_vnum, curr_vnum, end_vnum;
  int weak_mobs = 0, strong_mobs = 0;
  char buf[STD_LEN];
  
  /* 2 passes: LEVEL < STRONG_RANDPOP_MOB_MINLEVEL are "lower" mobs, 
     >= STRONG_RANDPOP_MOB_MINLEVEL  are "higher" mobs. */
  int pass, num_long_names, num_short_names, total_num_names; 
  
  /* Args for the long short and name of the mob proto. */
  char lname[STD_LEN], *larg;
  char sname[STD_LEN], *sarg;
  char name[STD_LEN], *narg;
  
   if (!th || LEVEL (th) < MAX_LEVEL || !IS_PC (th))
    {
      stt ("You can't set up the randpop mobs.\n\r", th);
      return;
    }
   
   if ((area = find_thing_num (MOBGEN_LOAD_AREA_VNUM)) == NULL ||
       !IS_AREA (area))
     {
       sprintf (buf, "The mobgen loading area %d doesn't exist!\n\r", 
		MOBGEN_LOAD_AREA_VNUM);
       stt (buf, th);
    
       return;
     }
   
   start_vnum = area->vnum + MAX (area->mv+1, 100);
   end_vnum = area->vnum + area->max_mv-1;
   curr_vnum = start_vnum;
   area->thing_flags |= TH_CHANGED;
   
   for (mob = area->cont; mob; mob = mob->next_cont)
     {
       if (!IS_ROOM (mob))
	 {
	   stt ("The mobgen loading area isn't empty! Clear the world and reboot to make the mobgen mobs!\n\r", th);
	   return;
	 }
     }
   
   if ((proto_area = find_thing_num (MOBGEN_PROTO_AREA_VNUM)) == NULL)
     {
       sprintf (buf, "The mobgen prototype area %d doesn't exist!\n\r", 
		MOBGEN_PROTO_AREA_VNUM);
       stt (buf, th);
       return;
     }
   
   /* So now both areas exist, and what we need to do is start looping
      through all of the protos in the proto area and for each
      combination of words the proto has, generate a mob. */
   
   for (pass = 0; pass < 2; pass++)
     {   
       for (proto = proto_area->cont; proto; proto = proto->next_cont)
	 {
	   
	   if (IS_ROOM (proto))
	     continue;
	   
	   /* Weak mobs are done in pass 0 and powerful mobs are
	      done in pass 1. */
	   if ((pass == 0) != (LEVEL(proto) < STRONG_RANDPOP_MOB_MINLEVEL))
	     continue;
	   
	   /* Make sure the names exist (Even if they're blank.) */
	   
	   if (!proto->long_desc || !proto->short_desc || !proto->name ||
	       !*proto->name)
	     continue;
	   
	   /* Find the number of long names and short names since we
	      only want a few kinds of mobs with each name. */

	   larg = proto->long_desc;
	   num_long_names = 1;	   
	   while (*larg)
	     {
	       larg = f_word (larg, lname);
	       num_long_names++;
	     }
	   sarg = proto->short_desc;
	   num_short_names = 1;
	   while (*sarg)
	     {
	       sarg = f_word (sarg, sname);
	       num_short_names++;
	     }
	  
	   total_num_names = num_long_names*num_short_names;
	  
	   /* Cycle through the names. */

	   narg = proto->name;
	   do
	     { 
	       narg = f_word (narg, name);
	       *name = LC(*name);
	       if (!*name)
		 continue;
	       sarg = proto->short_desc;

	       do
		 {
		   sarg = f_word (sarg, sname);	       
		   *sname = LC (*sname);
		   larg = proto->long_desc;

		   do
		     {
		       larg = f_word (larg, lname);	   
		       *lname = LC(*lname);
		       
		       if (nr (1, total_num_names) >= 10)
			 continue;
		       /* Make sure the vnum is ok before continuing. */
		     
		       if (curr_vnum < start_vnum || curr_vnum > end_vnum)
			 break;
		       /* At this point we have the three names, so set up
			  the base name. */

		       if ((mob = generate_randpop_mob (area, proto, name, sname, lname, curr_vnum)) == NULL)
			 continue;
		       if (pass == 0)
			 weak_mobs++;
		       else
			 strong_mobs++;	 
		       curr_vnum++; 
		       if (curr_vnum - 1 > top_vnum)
			 top_vnum = curr_vnum;	       		       
		     }
		   

		   
		   /* These whiles are different since I want to
		      require a name, I stop when there are no 
		      more names queued up, but the shortname and
		      longname can be blank, so I just allow them 
		      to have one pass where they're blank. */
		   
		   while (*lname);
		 }
	       while (*sname);
	     }
	   while (*narg);
	 }
       
       curr_vnum += 100;
       weak_mobs += 100;
       
     }
   
   
   /* Not quite correct since if there are more strong mobs than
      weak mobs, we miss some of the strong, but it's more important to
      have no strong mobs loaded when weak ones are wanted. */
      
   setup_mob_randpop_item (start_vnum, weak_mobs);
   echo ("Randpop mobs generated.\n\r");
   return;
   
}

/* This sets up the mob randpop item. It requires the start vnum
   for the mobs and max number of weak/strong mobs. */

void
setup_mob_randpop_item (int start_vnum, int size)
{
  THING *randpop_item;
  VALUE *randpop;
  
   /* Now do the setup on the randpop item. */
  
   if ((randpop_item = find_thing_num (MOB_RANDPOP_VNUM)) == NULL)
     {
       randpop_item = new_thing();
       randpop_item->vnum = MOB_RANDPOP_VNUM;
       randpop_item->thing_flags = OBJ_SETUP_FLAGS | TH_NO_MOVE_BY_OTHER |
	 TH_NO_TAKE_BY_OTHER;
       randpop_item->name = new_str ("randpop randmob");
       randpop_item->short_desc = new_str ("the randmob item");
       randpop_item->long_desc = new_str ("The randmob item is here.");
       
       thing_to (randpop_item, the_world->cont);
       add_thing_to_list (randpop_item);
     }
   if (the_world->cont)
     the_world->cont->thing_flags |= TH_CHANGED;
   
   if ((randpop = FNV (randpop_item, VAL_RANDPOP)) == NULL)
     {
       randpop = new_value();
       randpop->type = VAL_RANDPOP;
       add_value (randpop_item, randpop);
     }
   
   randpop->val[0] = start_vnum;
   randpop->val[1] = size;
   randpop->val[2] = 2;
   
   /* Increase the tier used if nr (1,1000) < area level. Should work
      pretty well. */
   randpop->val[3] = 1000;
   randpop->val[4] = 0;
   randpop->val[5] = 0;
   return;
}

/* This generates a randpop mob based on a proto and the three names. */

THING *
generate_randpop_mob (THING *area, THING *proto, char *name, char *sname, char *lname, int curr_vnum)
{
  char basename[STD_LEN];
  char fullname[STD_LEN];
  char longfullname[STD_LEN];
  char word[STD_LEN];
  THING *mob;
  int prot_flags = 0, i;
  bool use_notice = FALSE;

  if (!area || !proto || !name || !sname || !lname || !curr_vnum)
    return NULL;

   basename[0] = '\0';
   
   if (*lname)
     {
       strcat (basename, lname);
       strcat (basename, " ");
     }
   if (*sname)
     {
       strcat (basename, sname);
       strcat (basename, " ");
     }
   strcat (basename, name);
   
   /* Now create the mob and set its base stats. */

   mob = new_thing();
   
   mob->thing_flags = MOB_SETUP_FLAGS | TH_NO_TALK;
   copy_thing (proto, mob);
   mob->vnum = curr_vnum; 
  
   thing_to (mob, area);
   add_thing_to_list (mob);
   
   /* Now set up the names. */
   
   free_str (mob->name);
   mob->name = new_str (basename);
   
   free_str (mob->short_desc);
   sprintf (fullname, "%s %s", a_an (basename), basename);
   mob->short_desc = new_str (fullname);
   
   /* Make the long desc a bit more complicated. */
   free_str (mob->long_desc);

   longfullname[0] = '\0';
   /* You notice */
   
   if (nr (1,5) == 4)
     {
       strcpy (word, find_gen_word (MOBGEN_DESC_AREA_VNUM, "mob_notice", NULL));
       if (*word)
	 {
	   strcat (longfullname, word);
	   strcat (longfullname, " ");
	   use_notice = TRUE;
	 }
     }
   
   /* Mob name */
   strcat (longfullname, fullname);
   strcat (longfullname, " ");
   /* Must have is if no "you see" or "there is" or sometimes else. */
   if (!use_notice)
     strcat (longfullname, "is ");
   


   /* Is doing something */
   
   if (nr (1,3) != 2)
     {
       
       if (!IS_AFF (proto, AFF_FLYING))
	   strcpy (word, find_gen_word (MOBGEN_DESC_AREA_VNUM, "mob_action", NULL));
       else
	    strcpy (word, find_gen_word (MOBGEN_DESC_AREA_VNUM, "mob_flying", NULL));
       if (*word)
	 {
	   strcat (longfullname, word);
	   strcat (longfullname, " ");
	 }
     }
   
   /* Here */
   if (nr (1,7) != 3)
     strcat (longfullname, "here");
   else
     strcat (longfullname, "around here");
   
   /* Searching */
   if (nr (1,3) != 2)
     {
       /* Searching action */
       strcpy (word, find_gen_word (MOBGEN_DESC_AREA_VNUM, "mob_searching", NULL));
       
       if (*word)
	 {
	   strcat (longfullname, " ");
	   strcat (longfullname, word);
	   
	   /* Searching for */
	   if (!IS_AFF (proto, AFF_FLYING) || nr (1,10) != 3)
	   strcpy (word, find_gen_word (MOBGEN_DESC_AREA_VNUM, "mob_searchfor", NULL));
       else
	 strcpy (word, find_gen_word (MOBGEN_DESC_AREA_VNUM, "mob_searchrest", NULL));
	   if (*word)
	     {
	       strcat (longfullname, " ");
	       strcat (longfullname, word);
	     }
	 }
     }
   strcat (longfullname, ".");
   *longfullname = UC(*longfullname);
   mob->long_desc = new_str (longfullname);
   
   /* Fix the room flags for "Arctic" creatures. */
   
   if (proto->level < STRONG_RANDPOP_MOB_MINLEVEL &&
       (strstr (basename, "arctic") ||
	strstr (basename, "ice") ||
	strstr (basename, "snow") ||
	strstr (basename, "white") ||
	strstr (basename, "polar")))
     {
       remove_flagval (mob, FLAG_ROOM1,
		       (flagbits (mob->flags, FLAG_ROOM1) &
			ROOM_SECTOR_FLAGS));
			   add_flagval (mob, FLAG_ROOM1, ROOM_SNOW);
     }
   
   /* Modify the height and weight and level of the
      mob somewhat. */
   
   /* First modify on size. */
   
   if (strstr (basename, "small") ||
       strstr (basename, "little") ||
       strstr (basename, "tiny"))
     {
       mob->level /= 2;
       mob->height /= 2;
       mob->weight /= 2;
     }
   else if (strstr (basename, "large"))
     {
       mob->level = mob->level *3/2;
       mob->height = mob->level *3/2;
       mob->weight = mob->level *3/2;
     }
   else if (strstr (basename, "giant"))
     {
       mob->level *= 2;
       mob->height *= 2;
       mob->weight *= 2;
     }
   else if (strstr (basename, "huge"))
     {
       mob->level *= 3;
       mob->height *= 3;
       mob->weight *= 3;
     }
   
   /* Then modify on general principles. */
   
   mob->level -= nr (0, mob->level/10);
   mob->height -= nr (0, mob->height/10);
   mob->weight -= nr (0, mob->weight/10);
   mob->level += nr (0, mob->level/10);
   mob->height += nr (0, mob->height/10);
   mob->weight += nr (0, mob->weight/10);
   
   if (mob->sex == SEX_NEUTER)
     mob->sex = nr (SEX_FEMALE, SEX_MALE);
   
   /* Set up det flags. */
   
   if (mob->level >= STRONG_RANDPOP_MOB_MINLEVEL)
     add_flagval (mob, FLAG_DET, AFF_DARK);
   if (mob->level >= 100)
     add_flagval (mob, FLAG_DET, AFF_HIDDEN);
   if (mob->level >= 125)
     add_flagval (mob, FLAG_DET, AFF_CHAMELEON);
   if (mob->level >= 150)
     add_flagval (mob, FLAG_DET, AFF_INVIS);
   

   if (LEVEL(proto) >= STRONG_RANDPOP_MOB_MINLEVEL)
     {
       for (i = 0; aff1_flags[i].flagval != 0; i++)
	 {
	   if (is_named (mob, aff1_flags[i].name))
	     prot_flags |= aff1_flags[i].flagval;
	 }
       if (is_named (mob, "ice"))
	 prot_flags |= AFF_WATER;
       
       if (prot_flags)
	 add_flagval (mob, FLAG_PROT, prot_flags);
       
       if (IS_SET (prot_flags, AFF_FIRE))
	 {
	   add_flagval (mob, FLAG_HURT, AFF_WATER);
	   add_flagval (mob, FLAG_MOB, MOB_FIRE);
	   add_flagval (mob, FLAG_AFF, AFF_FIRE);
	 } 
       if (IS_SET (prot_flags, AFF_WATER))
	 {
	   add_flagval (mob, FLAG_HURT, AFF_FIRE);
	   add_flagval (mob, FLAG_MOB, MOB_WATER);
	   add_flagval (mob, FLAG_AFF, AFF_WATER);
	 }
       if (IS_SET (prot_flags, AFF_AIR))
	 {
	   add_flagval (mob, FLAG_HURT, AFF_EARTH);
	   add_flagval (mob, FLAG_MOB, MOB_AIR);
	   add_flagval (mob, FLAG_AFF, AFF_AIR);
	 } 
       if (IS_SET (prot_flags, AFF_EARTH))
	 {
	   add_flagval (mob, FLAG_HURT, AFF_AIR);
	   add_flagval (mob, FLAG_MOB, MOB_EARTH);
	   add_flagval (mob, FLAG_AFF, AFF_EARTH);
	 }
     }
   return mob;
}

