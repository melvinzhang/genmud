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
#include "detailgen.h"

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
  
  num_times = 8 + nr (num_rooms/20, num_rooms/10);

 
  /* If this is a "pk world" -- autogenerated -- then there are more
     persons in each area. */
  
  if (IS_SET (server_flags, SERVER_AUTO_WORLDGEN) &&
      IS_SET (server_flags, SERVER_WORLDGEN))
    {
      num_times += nr (num_times/2, num_times);
    }
  
  area->level = MID (AREA_LEVEL_MIN, area->level, AREA_LEVEL_MAX);
  
  for (i = 0; i < num_times; i++)
    areagen_generate_person (area, 0);
  
  
  add_reset (area, MOB_RANDPOP_VNUM, 50, area->mv/4, 1);
  
}



  
/* This finds a kind of type for the person by using the persongen area. */

THING *
find_persongen_type (void)
{
  THING *area, *person = NULL; 
  int count, num_choices = 0, num_chose = 0, return_flags = 0;
  VALUE *cast;
  /* Use a society caste name...*/
  
  if ((area = find_thing_num (PERSONGEN_AREA_VNUM)) == NULL ||
      !IS_AREA (area))
    return NULL;
  
  if ((person = area->cont) == NULL)
    return NULL;
  
  if (nr (1,3) == 2)
    {
      free_str (person->name);
      person->name = new_str (find_random_caste_name_and_flags (BATTLE_CASTES, &return_flags));
      person->level = nr (10,40);

      /* Add or remove a cast value depending on whether or not
	 the thing is a caster or not. */
      if (IS_SET (return_flags, CASTE_WIZARD | CASTE_HEALER))
	{	 
	  if ((cast = FNV (person, VAL_CAST)) == NULL)
	    {
	      cast = new_value();
	      cast->type = VAL_CAST;
	      add_value (person, cast);
	    }
	}
      else if ((cast = FNV (person, VAL_CAST)) != NULL)
	remove_value (person, cast);
      
      /* Add or remove sneak and hide based on if its a thief or not. */
     
      if (IS_SET (return_flags, CASTE_THIEF))
	add_flagval (person, FLAG_AFF, AFF_SNEAK | AFF_HIDDEN);
      else
	remove_flagval (person, FLAG_AFF, AFF_SNEAK | AFF_HIDDEN);
      if (person)
	return person;
    }
  
  /* Find a person type from the persongen area. */
  
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
  char buf[STD_LEN];
  int i, num_names = 0;
  char name[MOBGEN_NAME_MAX][STD_LEN];
  int num_names_used = 0;
  int name_pass = 0; /* Take 2 passes at making names in case
			we miss out on making any. */
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
  
  /* Get the names. */

  for (i = 0; i < MOBGEN_NAME_MAX; i++)
    name[i][0] = '\0';
  
  /* Give 3 chances to generate some kind of extra description besides a
     job. */

  for (name_pass = 0; name_pass < 3; name_pass++)
    {
      
      /* Get the proper name. */
      
      if (nr (1,3) == 1)
	{
	  strcpy (name[MOBGEN_NAME_PROPER], create_society_name(NULL));
	  num_names_used++;
	}
  
      /* Get the adjective name. */
      
      if (nr (1,3) == 2)
	{
	  strcpy (name[MOBGEN_NAME_ADJECTIVE],
		  find_gen_word (WORDLIST_AREA_VNUM, "mob_adjective", NULL));
	  num_names_used++;
	}
      
      /* Get the society name. */
      
      if (nr (1,3) == 3)
	{
	  strcpy (name[MOBGEN_NAME_SOCIETY], find_random_society_name ('n', TRUE));
	  num_names_used++;
	}
      if (num_names_used > 0)
	break;
    }
      

  /* Get the "job" name for the mob. */
  
  if ((proto = find_persongen_type()) == NULL ||
      !proto->name || !*proto->name)
    return NULL;
  
  strcpy (name[MOBGEN_NAME_JOB], proto->name);

  /* Now we have a society, a name, an adjective and a type. Make sure we
     don't have all of the names, and set the A_AN name. */
  
  for (i = 1; i < MOBGEN_NAME_MAX; i++)
    {
      if (*name[i])
	num_names++;
    }
  
  /* Too many names... remove one. */
  
  if (num_names == MOBGEN_NAME_MAX-1)
    name[nr(1,3)][0] = '\0';
      

  /* Set the A_AN. */

  for (i = 1; i < MOBGEN_NAME_MAX; i++)
    {
      if (*name[i])
	{
	  strcpy (name[MOBGEN_NAME_A_AN], a_an (name[i]));
	  break;
	}
    }
  
  /* Now add the spaces to the words. */
  
  for (i = 0; i < MOBGEN_NAME_MAX - 1; i++)
    {
      if (*name[i])
	strcat (name[i],  " ");
    }

  if ((person = new_thing()) == NULL)
    return NULL;
  
  person->vnum = curr_vnum;
  if (curr_vnum > top_vnum)
    top_vnum = curr_vnum;
  
  person->level = MAX (10, nr (area->level/2, area->level) +
		       nr (0, proto->level) + level_bonus);
  if (area->level < WORLDGEN_AGGRO_AREA_MIN_LEVEL)
    person->level += nr (0, person->level/2);
  if (proto->sex == SEX_NEUTER)
    person->sex = nr (SEX_FEMALE, SEX_MALE);
  else
    person->sex = proto->sex;
  /* Set to unique. */
  person->max_mv = 1;
  if (IS_SET (proto->thing_flags, TH_IS_ROOM | TH_IS_AREA) ||
      proto->thing_flags == 0)
    person->thing_flags = MOB_SETUP_FLAGS;
  else
    person->thing_flags = proto->thing_flags;
  thing_to (person, area);
  add_thing_to_list (person);
  
  
  /* Now set the names. */
  
  buf[0] = '\0';
  for (i = 1; i < MOBGEN_NAME_MAX; i++)
    {
      strcat (buf, name[i]);
    }
 
  free_str (person->name);
  person->name = new_str (buf);
  add_flagval (person, FLAG_ROOM1, 
	       flagbits(area->flags, FLAG_ROOM1) & ~ROOM_WATERY);
  
  /* Now set the short desc and long desc. */
  
  namebuf[0] = '\0';
  if (*name[MOBGEN_NAME_PROPER])
    {
      sprintf (namebuf, "%sthe ", name[MOBGEN_NAME_PROPER]);
      for (i = MOBGEN_NAME_PROPER + 1; i < MOBGEN_NAME_MAX; i++)
	{
	  strcat (namebuf, name[i]);
	}
    }
  else
    {
      for (i = 0; i < MOBGEN_NAME_MAX; i++)
	{
	  strcat (namebuf, name[i]);
	}
    }
  
  free_str (person->short_desc);
  person->short_desc = new_str (namebuf);
  strcat (namebuf, " is here.");
  namebuf[0] = UC(namebuf[0]);
  free_str (person->long_desc);
  person->long_desc = new_str (namebuf);
  
  
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
  
  if (nr (1,4) != 2 &&
      (room = find_random_room (area, FALSE, area_room_flags, BADROOM_BITS)) != NULL)
    add_reset (room, curr_vnum, 100, 1, 1);
  else
    add_reset (area, curr_vnum, 100, 1, 1);
  
  /* Now add objects to the person. */
  
  add_objects_to_person (person, name[MOBGEN_NAME_PROPER]);
  
  /* Add money to the person...proportional to level squared... */
  
  add_money (person, nr (LEVEL(person), LEVEL(person)*LEVEL(person))/10);
  
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
  char buf[STD_LEN];
  if (!person || (area = person->in) == NULL || !IS_AREA (area))
    return;
  
  if (person->vnum < area->vnum + area->mv ||
      person->vnum + num_objects >= area->vnum + area->max_mv)
    return;
  
  if ((cast = FNV (person, VAL_CAST)) != NULL &&
      nr (1,3) == 2)
    {
      add_reset (person, GEM_RANDPOP_VNUM, MAX(1, 10-person->level/15), 1, 
1);
    }
  
      /* Add standard armor/wpn pops. */
  
  if (nr (1,3) == 2)
    add_reset (person, WEAPON_RANDPOP_VNUM, 25, 1, 1);
  if (nr (1,3) == 2)
    add_reset (person, ARMOR_RANDPOP_VNUM, 20, 2, 1);
  if (nr (1,3) == 2)
    add_reset (person, PROVISION_RANDPOP_VNUM, 20, 1, 1);
  
  /* Sometimes there are no objects. */
  
  if (nr (1,5) == 2)
    return;
  
  num_objects = nr (1,2);
  if (nr (1,6) == 2)
    num_objects += nr (1,2);
  
   /* If this is a "pk world" -- autogenerated -- then the mobs get
      more objects. */
  
  if (IS_SET (server_flags, SERVER_AUTO_WORLDGEN) &&
      IS_SET (server_flags, SERVER_WORLDGEN))
    {
      num_objects += nr (num_objects/2, num_objects);
    }
  name[0] = UC(name[0]);
  sprintf (buf, "owner %s", name);
  for (i = 0; i < num_objects; i++)
    {
      /* Generate objects, and use the person's name 1/5 of the time
	 and if they're high enough level (or pass the check). */
      if ((obj = objectgen (NULL, 0,  nr (person->level*3/4,person->level*3/2),
			    person->vnum+i+1,
			    (nr (1,5) == 2 &&
			     nr (1,300) >= LEVEL(person) ? buf : NULL))) != NULL)
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
  THING *area, *proto_area, *proto, *mob, *mob2;

  int start_vnum, curr_vnum, end_vnum;
  int weak_mobs = 0, strong_mobs = 0;
  int randmobs_from_this_proto, num_prefix_names, real_num_names = 0;
  char buf[STD_LEN];
  int i, name_tries;
  EDESC *edesc;
  /* 2 passes: LEVEL < STRONG_RANDPOP_MOB_MINLEVEL are "lower" mobs, 
     >= STRONG_RANDPOP_MOB_MINLEVEL  are "higher" mobs. */
  int pass; 
  bool found_same_name;
  
  if (th && (LEVEL (th) < MAX_LEVEL || !IS_PC (th)))
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
      combination of words (up to some limit) 
      the proto has, generate a mob. */
   
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
	   
	   
	   /* Find the number of long names and short names since we
	      only want a few kinds of mobs with each name. */
	   
	   num_prefix_names = 1;
	   if ((edesc = find_edesc_thing (proto, "klong", TRUE)) != NULL)
	     num_prefix_names *= MAX(1, find_num_words (edesc->desc));
	   if ((edesc = find_edesc_thing (proto, "kshort", TRUE)) != NULL)
	     num_prefix_names *= MAX(1, find_num_words (edesc->desc));
	   if ((edesc = find_edesc_thing (proto, "kname", TRUE)) != NULL)
	     real_num_names = MAX(1, find_num_words (edesc->desc));
	   
	   
	   if (num_prefix_names > 15)
	     num_prefix_names = 15;
	   
	   /* The number of mobs you gen for this proto is the max of
	      15, total_num_names/5. This is APPROXIMATE since what
	      really happens is nr (1, total_num_names) is checked vs
	      mobs_genned_this_proto... to have more randomness. */
	   
	   randmobs_from_this_proto = num_prefix_names*real_num_names;
	   
	   /* Cycle through the names. */
	   
	   for (i = 0; i < randmobs_from_this_proto; i++)
	     {
	       /* Make sure the vnum is ok before continuing. */
	   
	       if (curr_vnum < start_vnum || curr_vnum > end_vnum)
		 break;
	       
	       /* Set up the mob. */
	       
	       if ((mob = generate_randpop_mob (area, proto, curr_vnum)) == NULL)
		 continue;
	       
	       /* Check to see if this mob shares a name with another.
		  Take up to 10 tries to try to fix this problem. 
		  This is expensive, but worth it to avoid mobs
		  with the same exact names...*/
	       found_same_name = TRUE;
	       for (name_tries = 0; name_tries < 10 &&
		      found_same_name; name_tries++)
		 {
		   found_same_name = FALSE;
		   for (mob2 = area->cont; mob2; mob2 = mob2->next_cont)
		     {
		       if (mob2 == mob ||
			   !mob2->name || !*mob2->name ||
			   str_cmp (mob2->name, mob->name))
			 continue;
		       
		       found_same_name = TRUE;
		       break;
		     }
		   if (found_same_name)
		     generate_detail_name (proto, mob);
		 }
	       
	       if (pass == 0)
		 weak_mobs++;
	       else
		 strong_mobs++;	 
	       curr_vnum++; 
	       if (curr_vnum - 1 > top_vnum)
		 top_vnum = curr_vnum;	       		       
	     }
	 }
       if (pass == 0)
	 {
	   curr_vnum += 100;
	   weak_mobs += 100;
	 }
     }
   
   
   /* Not quite correct since if there are more strong mobs than
      weak mobs, we miss some of the strong, but it's more important to
      have no strong mobs loaded when weak ones are wanted. */
      
   setup_mob_randpop_item (start_vnum, weak_mobs);
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
generate_randpop_mob (THING *area, THING *proto, int curr_vnum)
{
  THING *mob;
  int prot_flags = 0, i;

  if (!area || !proto || !curr_vnum)
    return NULL;

   
  
   /* Now create the mob and set its base stats. */

   mob = new_thing();
   
   mob->thing_flags = MOB_SETUP_FLAGS | TH_NO_TALK;
   copy_thing (proto, mob);
   mob->type = new_str (proto->type);
   mob->vnum = curr_vnum; 
  
   thing_to (mob, area);
   add_thing_to_list (mob);
   
   generate_detail_name (proto, mob);
   
   /* Fix the room flags for "Arctic" creatures. */
   
   if (proto->level < STRONG_RANDPOP_MOB_MINLEVEL &&
       (strstr (mob->name, "arctic") ||
	strstr (mob->name, "ice") ||
	strstr (mob->name, "snow") ||
	strstr (mob->name, "white") ||
	strstr (mob->name, "polar")))
     {
       remove_flagval (mob, FLAG_ROOM1,
		       (flagbits (mob->flags, FLAG_ROOM1) &
			ROOM_SECTOR_FLAGS));
			   add_flagval (mob, FLAG_ROOM1, ROOM_SNOW);
     }
   
   /* Modify the height and weight and level of the
      mob somewhat. */
   
   /* First modify on size. */
   
   if (strstr (mob->name, "small") ||
       strstr (mob->name, "little") ||
       strstr (mob->name, "tiny"))
     {
       mob->level = mob->level *3/4;
       mob->height = mob->height *3/4;
       mob->weight = mob->weight * 3/4;
     }
   else if (strstr (mob->name, "large"))
     {
       mob->level = mob->level *5/4;
       mob->height = mob->height *5/4;
       mob->weight = mob->weight *5/4;
     }
   else if (strstr (mob->name, "giant"))
     {
       mob->level = mob->level*3/2;
       mob->height=  mob->height*3/2;
       mob->weight = mob->height*3/2; 
     }
   else if (strstr (mob->name, "huge"))
     {
       mob->level *= 2;
       mob->height *= 2;
       mob->weight *= 2;
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



/* This tells if an area has a free mobject vnum in it or not. */

int
find_free_mobject_vnum (THING *area)
{
  int vnum;
  THING *mobject;
  
  if (!area || !IS_AREA (area))
    return 0;

  for (vnum = area->vnum + area->mv + 1; vnum < area->vnum + area->max_mv; vnum++)
    {
      if ((mobject = find_thing_num (vnum)) == NULL)
	return vnum;
    }
  return 0;
}

/* This returns the random name of a regular animal randpop mob,
   and not too wimpy either. The parameter tells if we have the a_an
   on the front of the return or not. (essentially return name vs
   short desc) */

char *
find_randpop_mob_name (int flags)
{
  int count, num_choices = 0, num_chose = 0;
  
  static char buf[STD_LEN];

  THING *area, *mob;
  
  if ((area = find_thing_num (MOBGEN_LOAD_AREA_VNUM)) == NULL)
    {
      if (IS_SET (flags, RANDMOB_NAME_A_AN))
	return "a monster";
      else
	return "monster";
    }
  
  for (count = 0; count < 2; count++)
    {
      for (mob = area->cont; mob; mob = mob->next_cont)
	{
	  if (IS_ROOM (mob) ||
	      !CAN_FIGHT (mob) || !CAN_MOVE (mob))
	    continue;
	  
	  if (LEVEL (mob) < 10)
	    continue;
	  /* The mob can only be above the strong mob minlevel if
	     we asked for this feature. */
	  if (LEVEL (mob) >= STRONG_RANDPOP_MOB_MINLEVEL &&
	      !IS_SET (flags, RANDMOB_NAME_STRONG))
	    continue;
	  
	  if (LEVEL (mob) < STRONG_RANDPOP_MOB_MINLEVEL &&
	      IS_SET (flags, RANDMOB_NAME_STRONG))
	    continue;

	  
	  if (!mob->name || !*mob->name ||
	      !mob->short_desc || !*mob->short_desc)
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    {
	      if (IS_SET (flags, RANDMOB_NAME_A_AN))
		return "a monster";
	      
	      return "monster";
	    }
	  num_chose = nr (1, num_choices);
	}
    }
  
  if (mob)
    {
      if (IS_SET (flags, RANDMOB_NAME_FULL))
	{
	  if (IS_SET (flags, RANDMOB_NAME_A_AN) 
	      && mob->short_desc && *mob->short_desc)
	    {
	      strcpy (buf, mob->short_desc);
	      return buf;
	    }
	  if (!IS_SET (flags, RANDMOB_NAME_A_AN)
	      && mob->name && *mob->name)
	    {
	      strcpy (buf, mob->name);
	      return buf;
	    }
	}
      /* Use two words...instead of one. */
      else if (IS_SET (flags, RANDMOB_NAME_TWO_WORDS))
	{
	  int num_words = find_num_words (mob->name);
	  char *wd;
	  char buf1[STD_LEN];
	  wd = mob->name;
	  num_words -= 2;
	  while (num_words-- > 0)
	    wd = f_word (wd, buf1);
	  
	  strcpy (buf1, wd);
	  if (*buf1)
	    {
	      if (IS_SET (flags, RANDMOB_NAME_A_AN))
		{
		  sprintf (buf, "%s %s", a_an(buf1), buf1);
		  return buf;
		}
	      else
		{
		  strcpy (buf, buf1);
		  return buf;
		}
	    }
	}
      else
	{
	  char *wd;
	  char buf2[STD_LEN];
	  
	  buf2[0] = '\0';
	  wd = mob->name;
	  
	  while (wd && *wd)
	    wd = f_word (wd, buf2);

	  if (*buf2)
	    {
	      if (IS_SET (flags, RANDMOB_NAME_A_AN))
		{
		  sprintf (buf, "%s %s", a_an(buf2), buf2);
		  return buf;
		}
	      else
		{
		  strcpy (buf, buf2);
		  return buf;
		}
	    }
	  
	}
      
      
    }
  if (IS_SET (flags, RANDMOB_NAME_A_AN))
    return "a monster";  
  return "monster";
}
