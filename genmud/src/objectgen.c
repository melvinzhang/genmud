#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "serv.h"
#include "society.h"
#include "areagen.h"
#include "worldgen.h"
#include "objectgen.h"

/* These words used to make up objects are stored in area 108000, it's
   the one referenced by OBJECTGEN_AREA_VNUM in serv.h. */


/* These are the levels you need to attain to get certain kinds of
   affects of certain ranks. The ranks are shown in const.c
   just below where the affects are defined. */

static int aff_rank_levels[AFF_RANK_MAX] = 
  {
    30,
    60,
    90,
    120,
  };


/* This creates an object of a certain type and of a certain level of
   power. If the wear_loc is not a valid location, then it picks
   a random value. */
static const char *objectgen_part_names[OBJECTGEN_NAME_MAX] =
  {
    "owner", /*0 */
    "a_an",   /* Only set if owner isn't. */
    "prefix",
    "color",
    "gem",
    
    "metal", /* 5 */
    "nonmetal", /* Other things besides metals that an object can
		   be made of. */
    "animal", /* Things like skin, bones, feathers etc... */
    "society", 
    "type",   /* 8th slot, array[7]. Important to keep distinct. */
    "suffix",
  };


THING *
objectgen (THING *area, int wear_loc, int level, int curr_vnum, char *society_name, char *owner_name)
{
  THING *obj, *in_area;
  int weapon_type = -1;
  char name[OBJECTGEN_NAME_MAX][STD_LEN];
  char color[OBJECTGEN_NAME_MAX][STD_LEN];
  
  if (wear_loc <= ITEM_WEAR_NONE || wear_loc >= ITEM_WEAR_MAX ||
      wear_loc == ITEM_WEAR_BELT)
    {
      wear_loc = objectgen_find_wear_location();
      if (wear_loc <= ITEM_WEAR_NONE || wear_loc >= ITEM_WEAR_MAX ||
	  wear_loc == ITEM_WEAR_BELT)
	{
	  log_it ("objectgen_find_wear_location FAILED!\n");
	  return NULL;
	}
    }
  if (wear_loc == ITEM_WEAR_WIELD)
    weapon_type = generate_weapon_type ();
  
  
  /* See if we have an open vnum slot. */
  if (curr_vnum > 0)
    {
      if ((in_area = find_area_in (curr_vnum)) == NULL ||
	  (area && in_area != area) ||
	  curr_vnum <= in_area->vnum + in_area->mv ||
	  curr_vnum >= in_area->vnum + in_area->max_mv ||
	  (obj = find_thing_num (curr_vnum)) != NULL)
	return NULL;
    }
  else if (area != NULL)
    {
      in_area = area;
      for (curr_vnum = area->vnum + area->mv + 1; 
	   curr_vnum < area->vnum + area->max_mv; curr_vnum++)
	{
	  if ((obj = find_thing_num (curr_vnum)) == NULL)
	    break;
	}
      if (curr_vnum < area->vnum + area->mv + 1 ||
	  curr_vnum >= area->vnum + area->max_mv)
	return NULL;
    }
  else /* No curr_vnum and no area. */
    return NULL;
  /* Get the descriptions of the object. */
  
  objectgen_generate_names (name, color, wear_loc, society_name, 
			    owner_name, weapon_type, level);
  
  
  if ((obj = new_thing()) == NULL)
    return NULL;
  obj->vnum = curr_vnum;
  if (curr_vnum > top_vnum)
    top_vnum = curr_vnum;
  obj->thing_flags = OBJ_SETUP_FLAGS;
  obj->wear_pos = wear_loc;
  obj->level = level;
  thing_to (obj, in_area);
  add_thing_to_list (obj);
  /* Set up the name, short_desc, long_desc for the object. Maybe
     desc at some way way future date. */

  objectgen_setup_names (obj, name, color);
  
  /* This puts the stats and bonuses onto the object. */
  
  objectgen_setup_stats (obj, weapon_type);
  
  
  return obj;
}

/* This gives the type of weapon you will make...it's weighted toward
   making swords. */

int
generate_weapon_type (void)
{
  int weapon_type;
  if (nr (1,12) == 3)
    weapon_type = WPN_DAM_WHIP;
  else if (nr (1,9) == 2)
    weapon_type = WPN_DAM_CONCUSSION;
  else if (nr (1,4) == 1)
    weapon_type = WPN_DAM_PIERCE;
  else
    weapon_type = WPN_DAM_SLASH;
  return weapon_type;
}


/* This sets up the names for the object by looking at the list of names. */

void
objectgen_setup_names (THING *obj, char name[OBJECTGEN_NAME_MAX][STD_LEN],
		       char colorname[OBJECTGEN_NAME_MAX][STD_LEN])
{
  int i;
  char fullname[STD_LEN*2];
  char realfullname[STD_LEN * 2];
  char longname[STD_LEN*3];
  char *t;
  if (!obj || !name || !colorname || obj->wear_pos <= ITEM_WEAR_NONE ||
      obj->wear_pos >= ITEM_WEAR_MAX || obj->wear_pos == ITEM_WEAR_BELT)
    return;
  
  /* Set up the name name. */
  fullname[0] = '\0';
  for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
    {
      if (i == OBJECTGEN_NAME_A_AN ||
	  !*name[i])
	continue;
      if (*fullname)
	strcat (fullname, " ");
      if (!*colorname[i] && *name[i])
	strcpy (colorname[i], name[i]);
      for (t = name[i]; *t; t++)
	{
	  if (*t == '\'' || *t == '\"')
	    *t = '\0';
	}
      strcat (fullname, name[i]);
    }
  
  free_str (obj->name);
  obj->name = new_str (fullname);
  
  
  /* Set up the short and long descs. */
  fullname[0] = '\0';
  for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
    {
      if (!*name[i])
	continue;
      /*      for (t = name[i]; *t; t++)
	{
	  if (t == name[i] || *(t-1) == ' ')
	    *t = UC(*t);
	    } */
      
      if (i == OBJECTGEN_NAME_METAL && *name[OBJECTGEN_NAME_GEM])
	{
	  if (obj->wear_pos == ITEM_WEAR_WIELD)
	    strcat (fullname, "$7-encrusted ");
	  else if (wear_data[obj->wear_pos].how_many_worn == 1)
	    strcat (fullname, "$7-studded ");
	  else
	    { /* Jewelry is gem and metal jewelry...*/
	      if (*name[OBJECTGEN_NAME_METAL])
		{
		  if (nr (1,2) == 2)
		    strcat (fullname, "$7-encrusted ");
		  else
		    strcat (fullname, " $7and ");
		}
	      else
		strcat (fullname, " ");	      
	    }
	}  
     
      else if (*fullname)
	strcat (fullname, " ");
      
      
      if (i == OBJECTGEN_NAME_MAX-1)
	strcat (fullname, "of ");
      
      if (*colorname[i])
	strcat (fullname, colorname[i]);
      else
	strcat (fullname, name[i]);
      
      /* Cloaks/leather/cloth/hide armor are metal-woven armor,
	 not metal armor. Also add the woven look to a small percentage
	 of nonweapons. */
      if (i == OBJECTGEN_NAME_METAL &&
	  *name[OBJECTGEN_NAME_METAL] &&
	  obj->wear_pos != ITEM_WEAR_WIELD &&
	  ((obj->wear_pos == ITEM_WEAR_ABOUT ||
	    strstr (name[OBJECTGEN_NAME_TYPE], "leather") ||
	    strstr (name[OBJECTGEN_NAME_TYPE], "cloth") ||
	    strstr (name[OBJECTGEN_NAME_TYPE], "woven") ||
	    strstr (name[OBJECTGEN_NAME_TYPE], "hide") ||
	    nr (1,52) == 3)))
	{
	  strcat (fullname, find_gen_word (OBJECTGEN_AREA_VNUM, "metal_weaves", NULL));
	}
    }
  
  strcpy (realfullname, add_color (fullname));
  
  free_str (obj->short_desc);
  obj->short_desc = new_str (realfullname);
  
  /* Put a prefix here. */
  
  longname[0] = '\0';
  if (!*name[OBJECTGEN_NAME_OWNER])
    {
      realfullname[0] = LC(realfullname[0]);
      longname[0] = '\0';
      switch (nr (1,8))
	{
	  case 1:
	  case 2:
	  default:
	    break;
	  case 5:
	    sprintf (longname, "You notice ");
	    break;
	  case 6:
	    sprintf (longname, "You see ");
	    break;
	  case 7:
	    sprintf (longname, "You see that ");
	    break;
	  case 8:
	    sprintf (longname, "You notice that ");
	    break;
	}
    }
  strcat (longname, realfullname);
  strcat (longname, " ");
  
  switch (nr (1,8))
    {
      case 1:
      case 2:
	strcat (longname, "is here.");
	break;
      case 3:
	strcat (longname, "has been left here.");
	break;
      case 4:
	strcat (longname, "is lying here.");
	break;
      case 5:
	strcat (longname, "was left here by somebody.");
	break;
      case 6:
	strcat (longname, "is on the ground.");
	break;
      case 7:
	strcat (longname, "was left here.");
	break;
      case 8:
	strcat (longname, "has been abandoned here.");
	break;
    }
  
  free_str (obj->long_desc);
  longname[0] = UC(longname[0]);
  obj->long_desc = new_str (longname);
  
  
  
  return;
}

/* This generates the objectgen names. */

void
objectgen_generate_names (char name[OBJECTGEN_NAME_MAX][STD_LEN], 
			  char colorname[OBJECTGEN_NAME_MAX][STD_LEN],
			  int wear_loc, char *society_name, 
			  char *owner_name, int weapon_type, int level)
{
  int i;
  char *t;
  int num_names = 1;
  int names_used, names_left, num_names_used, choice, name_choices;
  
  
  if (!name || !colorname)
    return;
  
  if (level < 0)
    level = 0;
  
  /* Generate the names. */
  for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
    {
      name[i][0] = '\0';
      colorname[i][0] = '\0';
      
      if (i == OBJECTGEN_NAME_METAL && nr (1,8) == 2)
	strcpy (name[i], find_gen_word 
		(OBJECTGEN_AREA_VNUM,
		 "metal_gen_names", colorname[i]));
      else if (i == OBJECTGEN_NAME_ANIMAL)
	strcpy (name[i], objectgen_animal_name (wear_loc, weapon_type));
      else if (i != OBJECTGEN_NAME_TYPE)
	strcpy (name[i], find_gen_word 
		(OBJECTGEN_AREA_VNUM,
		 (char *) objectgen_part_names[i], colorname[i]));
      else
	strcpy (name[i], objectgen_find_typename (wear_loc, weapon_type));
    }
  
  /* Choose metal or nonmetal or animal genned. 

  Metal is for all. nonmetal is for armor, animal is for all. 
  
  Breakdown: 70pct metal, 20pct animal, 10pct nonmetal. */
  
  
  if ((nr (1,10) <= 7 ||
       (!*name[OBJECTGEN_NAME_NONMETAL] &&
	!*name[OBJECTGEN_NAME_ANIMAL])) && 
      *name[OBJECTGEN_NAME_METAL])
    {
      *name[OBJECTGEN_NAME_NONMETAL] = '\0';
      *name[OBJECTGEN_NAME_ANIMAL] = '\0'; 
      *colorname[OBJECTGEN_NAME_NONMETAL] = '\0';
      *colorname[OBJECTGEN_NAME_ANIMAL] = '\0';
    }      
  /* Allow animal names */ 
  else if (*name[OBJECTGEN_NAME_ANIMAL] &&
	   (nr (1,3) != 2 || 
	    !*name[OBJECTGEN_NAME_NONMETAL] ||
	    wear_loc <= ITEM_WEAR_NONE || wear_loc >= ITEM_WEAR_MAX ||
	    wear_loc == ITEM_WEAR_BELT))
    {
      *name[OBJECTGEN_NAME_NONMETAL] = '\0';
      *name[OBJECTGEN_NAME_METAL] = '\0'; 
      *colorname[OBJECTGEN_NAME_NONMETAL] = '\0';
      *colorname[OBJECTGEN_NAME_METAL] = '\0';
    }
  else
    {
      *name[OBJECTGEN_NAME_METAL] = '\0';
      *name[OBJECTGEN_NAME_ANIMAL] = '\0';
      *colorname[OBJECTGEN_NAME_METAL] = '\0';
      *colorname[OBJECTGEN_NAME_ANIMAL] = '\0';
    }
      
  
      
  /* Add the society name, and if no society name exists, add the owner
     name. */
  
  if (society_name && *society_name)
    {
      strcpy (name[OBJECTGEN_NAME_SOCIETY], society_name);
      num_names--;
    }
  else if (owner_name && *owner_name)
    {
      f_word (owner_name, name[OBJECTGEN_NAME_OWNER]);
      capitalize_all_words (name[OBJECTGEN_NAME_OWNER]);
      num_names--;
    }
  
  /* The typename really has to exist... */
  
  
  if (!*name[OBJECTGEN_NAME_TYPE])
    {
      if (wear_loc == ITEM_WEAR_WIELD)
	strcpy (name[OBJECTGEN_NAME_TYPE], "weapon");
      else
	strcpy (name[OBJECTGEN_NAME_TYPE], "armor");
    }
  
  
  /* Set names_used to have bits set for all nonnull names that won't
     be fixed. */

  names_used = ((1 << OBJECTGEN_NAME_TYPE) | (1 << OBJECTGEN_NAME_A_AN) |
		(1 << OBJECTGEN_NAME_OWNER) | (1 << OBJECTGEN_NAME_SOCIETY));
  
  
  if (*name[OBJECTGEN_NAME_OWNER])
    {
      /* Most of the time keep the name on the front, otherwise move
	 it to the back where the suffix is. */
      if (nr (1,6) != 2)
	*name[OBJECTGEN_NAME_A_AN] = '\0';
      else
	{
	  RBIT (names_used, (1 << OBJECTGEN_NAME_OWNER));
	  SBIT (names_used, (1 << OBJECTGEN_NAME_SUFFIX));
	  strcpy (name[OBJECTGEN_NAME_SUFFIX],
		  name[OBJECTGEN_NAME_OWNER]);
	  strcpy (colorname[OBJECTGEN_NAME_SUFFIX],
		  colorname[OBJECTGEN_NAME_OWNER]);
	  *name[OBJECTGEN_NAME_OWNER] = '\0';
	  *colorname[OBJECTGEN_NAME_OWNER] = '\0';
	  strcpy (name[OBJECTGEN_NAME_A_AN], "The");
	  strcpy (colorname[OBJECTGEN_NAME_A_AN], "The");
	}
    } 
  /* If there's a suffix, usually change the "A/An" into a "The" */
  else if (*name[OBJECTGEN_NAME_SUFFIX] && nr (1,5) == 3)
    {
      strcpy (name[OBJECTGEN_NAME_A_AN], "The");
      strcpy (colorname[OBJECTGEN_NAME_A_AN], "The");
    }
  num_names += nr (2, level/100+2);
  if (num_names > 4)
    num_names = 4;
  
  num_names_used = 0;
  for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
    {
      if (IS_SET (names_used, (1 << i)))
	num_names_used++;
    }
  names_left = num_names;
  num_names += num_names_used;
  
  /* Name choices is set to 6 since there are 6 things you can pick
     from besides the type, the a_an and the society/owner name.

     Those are: prefix color gem metal suffix. */
  for (name_choices = OBJECTGEN_NAME_EXTRA; name_choices > 0; name_choices--)
    {
      choice = nr (1, name_choices);
      
      for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
	{
	  if (!IS_SET (names_used, (1 << i)) && 
	      *name[i] && --choice < 1)
	    break;
	}
      SBIT (names_used, (1 << i));
      if (--names_left < 1)
	break;
    }
  
  for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
    {
      if (!IS_SET (names_used, (1 << i)))
	*name[i] = '\0';
    }
  
  /* Find the a_an at the start of the name. */
  
  for (t = name[OBJECTGEN_NAME_TYPE]; *t; t++);
  t--;
  
  /* If the owner_name exists, the a_an gets set to empty. */
  
  
  *name[OBJECTGEN_NAME_A_AN] = '\0';
  if (!*name[OBJECTGEN_NAME_OWNER])
    {
      if (LC (*t) == 's'  &&
	  str_cmp (name[OBJECTGEN_NAME_TYPE], "cutlass") &&
	  str_cmp (name[OBJECTGEN_NAME_TYPE], "cuirass"))
	{
	  if ((wear_loc == ITEM_WEAR_BODY &&
	       strstr (name[OBJECTGEN_NAME_TYPE], "mail")) || nr (1,3) != 2)  
	    strcpy (name[OBJECTGEN_NAME_A_AN], "some");
	  else
	    strcpy (name[OBJECTGEN_NAME_A_AN], "a pair of");
	}
      else
	{
	  for (i = 0; i < OBJECTGEN_NAME_MAX-1; i++)
	    {
	      if (*name[i])
		{
		  strcpy (name[OBJECTGEN_NAME_A_AN], a_an(name[i]));
		  break;
		}
	    }	 
	}
      
    }
  else
    {
      possessive_form (name[OBJECTGEN_NAME_OWNER]);
    }
  return;
}
  
 




/* This finds a wear location at random. It has a 25% chance of picking
   a wielded item (Weapon) a 60% chance of picking a large armor
   (worn only one location) and a 15% chance of picking small armor
   (jewelry and such). */

int
objectgen_find_wear_location (void)
{
  int num_single_choices = 0, num_multi_choices = 0;
  int num_chose;
  int wearloc;
  int obj_type; /* Wpn = 0 big armor = 1 jewelry = 2 */

  switch (nr(1,10))
    {
      case 3:
      case 6:
      case 2:
	obj_type = 0; /* 30pct */
	break;
      case 9:
      case 4:
	obj_type = 2; /* 20pct */
	break;
      default:
	obj_type = 1; /* 50pct */
	break;
    }

  for (wearloc = 0; wear_data[wearloc].flagval < ITEM_WEAR_MAX; wearloc++)
    {
      if (wearloc == ITEM_WEAR_WIELD)
	continue;
      if (wear_data[wearloc].how_many_worn == 1)
	num_single_choices++;
      else if (wear_data[wearloc].how_many_worn > 1)
	num_multi_choices++;
    }

  if (obj_type == 0) /* Weapon */
    wearloc = ITEM_WEAR_WIELD;
  else if (obj_type == 1) /* Armor */
    {
      if (num_single_choices < 1)
	return ITEM_WEAR_NONE;
      num_chose = nr (1, num_single_choices);
      
      for (wearloc = 0; wear_data[wearloc].flagval < ITEM_WEAR_MAX; wearloc++)
	{
	  if (wearloc == ITEM_WEAR_WIELD)
	    continue;
	  if (wear_data[wearloc].how_many_worn == 1 &&
	      --num_chose < 1)
	    break;
	}
    }
  else if (obj_type == 2)
    {
      if (num_multi_choices < 1)
	return ITEM_WEAR_NONE;
      num_chose = nr (1, num_multi_choices);
      
      for (wearloc = 0; wear_data[wearloc].flagval < ITEM_WEAR_MAX; wearloc++)
	{
	  if (wearloc == ITEM_WEAR_WIELD)
	    continue;
	  if (wear_data[wearloc].how_many_worn > 1 &&
	      --num_chose < 1)
	    break;
	}
    }
  
  if (wearloc <= ITEM_WEAR_NONE || wearloc >= ITEM_WEAR_MAX ||
      wearloc == ITEM_WEAR_BELT)
    return ITEM_WEAR_NONE;
  return wearloc;
}


/* This finds the typename for objects with certain wear locations. 
   There's some special coding to handle weapons. */

char *
objectgen_find_typename (int wear_loc, int weapon_type)
{
  static char retbuf[STD_LEN];
  char typebuf[STD_LEN];
  char color = '7';
  
  retbuf[0] = '\0';
  typebuf[0] = '\0';
  if (wear_loc <= ITEM_WEAR_NONE || wear_loc >= ITEM_WEAR_MAX ||
      wear_loc == ITEM_WEAR_BELT || 
      (wear_loc == ITEM_WEAR_WIELD &&
       (weapon_type < 0 || weapon_type >= WPN_DAM_MAX)))
    return retbuf;
  
  if (wear_loc == ITEM_WEAR_WIELD)
    strcpy (typebuf, weapon_damage_types[weapon_type]);
  else
    strcpy (typebuf, wear_data[wear_loc].name);
  
  strcpy (retbuf, find_gen_word (OBJECTGEN_AREA_VNUM, typebuf, &color));
  return retbuf;
}


/* This sets up the stats for the object. It is assumed that the 
   wear_pos of the object is already set. */

void
objectgen_setup_stats (THING *obj, int weapon_type)
{
  FLAG *flg;
  VALUE *val;
  int total_flags, curr_total_flags, rank_choice;
  int lev_sqrt;  /* Needed since item power is proportional to the
		    sqrt of its level. */
  int main_part; /* Main part that the armor protects. */
  int i, rank, times, gen_tries;
  int obj_flags = 0;
  
  /* Number of flags of each affect rank type. */
  
  int num_flags[AFF_RANK_MAX];
  /* Actual flags used in each rank. */
  int aff_used[MAX_AFF_PER_RANK];
  if (!obj ||  IS_AREA (obj) || IS_ROOM (obj) ||
      !obj->in || !IS_AREA (obj->in))
    return;
  
  
  if (obj->wear_pos <= ITEM_WEAR_NONE || obj->wear_pos >= ITEM_WEAR_MAX ||
      obj->wear_pos == ITEM_WEAR_BELT)
    return;

  if (obj->level < 10)
    obj->level = 10;
  
  /* if (obj->level > 300)
     obj->level = 300; */
  
  obj->level += nr (0, obj->level/3);
  obj->level -= nr (0, obj->level/4);
  
  
  
  for (lev_sqrt = 1; lev_sqrt*lev_sqrt <= obj->level; lev_sqrt++);
  
  obj->cost = obj->level*obj->level;
  
  if (obj->wear_pos == ITEM_WEAR_WIELD)
    {
      if ((val = FNV (obj, VAL_WEAPON)) == NULL)
	{
	  val = new_value();
	  add_value (obj, val);
	}

      obj->height = nr (20,40);
      obj->weight = MAX(30, obj->level+30);
      
      val->type = VAL_WEAPON;
      
      val->val[0] = nr (lev_sqrt/3,lev_sqrt/2);
      val->val[1] = nr(lev_sqrt*2/3,lev_sqrt*3/2);
      
      if (weapon_type < 0 || 
	  weapon_type >= WPN_DAM_MAX)
	weapon_type = WPN_DAM_SLASH;
      
      val->val[2] = weapon_type;
      
      /* Adjust damage based on wpn type. */

      if (weapon_type == WPN_DAM_PIERCE)
	{
	  val->val[0] -= nr (0, val->val[0]/8);
	  val->val[1] -= nr (0, val->val[1]/8);
	}
      if (weapon_type == WPN_DAM_WHIP ||
	  weapon_type == WPN_DAM_CONCUSSION)
	{
	  val->val[0] += nr (0, val->val[0]/10);
	  val->val[1] += nr (0, val->val[1]/10);
	}
      
      /* Add extra attacks and weaken the object if it's too powerful. */
      
      if (nr (1,3) == 2 &&
	  (weapon_type == WPN_DAM_WHIP ||
	   weapon_type == WPN_DAM_CONCUSSION))
	val->val[3] += nr (0,obj->level)/120;
      
      if (val->val[3] > 0)
	{
	  val->val[0] -= nr (0, val->val[0]/5);
	  val->val[1] -= nr (0, val->val[1]/5);
	}
      
   
    }
  else /* Armors */
    {
      if ((val = FNV (obj, VAL_ARMOR)) == NULL)
	{
	  val = new_value();
	  add_value (obj, val);
	}
      val->type = VAL_ARMOR;
      
      main_part = wear_data[obj->wear_pos].part_of_thing;
      val->val[main_part] = lev_sqrt;
      
      /* Add the warmth. Can be + or - */
      
      val->val[6] = nr (0, lev_sqrt/2) - nr (0, lev_sqrt/2);
      if (wear_data[obj->wear_pos].how_many_worn > 1)
	{
	  obj->weight = nr (2,10);
	  obj->height = nr (2,10);
	  obj->cost *= nr (3,5);
	  val->val[main_part] = val->val[main_part]*2/3;
	}
      else
	{
	  obj->height = nr (10, 30);
	  if (obj->wear_pos != ITEM_WEAR_BODY)
	    obj->weight = 10+obj->level/10;
	  else
	    obj->weight = 40+obj->level/5;
	}
		
      
      if (obj->wear_pos == ITEM_WEAR_ABOUT ||
	  obj->wear_pos== ITEM_WEAR_SHIELD)
	{
	  for (i = 0; i < PARTS_MAX; i++)
	    {
	      if (i != main_part)
		val->val[i] = val->val[main_part]/2;
	    }
	}
    }
  
  /* Now set up the flags on the object. */
  
  total_flags = nr (obj->level/50, obj->level/25);  
  if (total_flags > 6)
    total_flags = 6;
  if (total_flags == 0 && nr (1,3) == 2)
    total_flags = 1;
  
  curr_total_flags = 0;
  for (i = 0; i < AFF_RANK_MAX; i++)
    {
      num_flags[i] = MIN(MAX_AFF_PER_RANK, obj->level/aff_rank_levels[i]);
      curr_total_flags += num_flags[i];
    }
  
  
  /* If we want more flags than we can put on the thing...
     remove something. */

  if (curr_total_flags < total_flags)
    total_flags = curr_total_flags;
  else if (curr_total_flags > total_flags)
    {
      while (curr_total_flags > total_flags)
	{
	  rank_choice = nr (0, AFF_RANK_MAX-1);
	  
	  if (num_flags[rank_choice] > 0)
	    {
	      num_flags[rank_choice]--;
	      curr_total_flags--;
	    }
	}
    }

  /* Loop through all ranks and find flags from each rank. */
  
  for (rank = 0; rank < AFF_RANK_MAX; rank++)
    {
      for (times = 0; times < MIN (MAX_AFF_PER_RANK, num_flags[rank]); times++)
	{
	  aff_used[times] = 0;
	  gen_tries = 0;
	  do 
	    {
	      aff_used[times] = find_aff_with_rank (rank);
	      
	      for (i = 0; i < times; i++)
		{
		  if (aff_used[i] == aff_used[times])
		    {
		      aff_used[times] = 0;
		      break;
		    }
		}
	    }
	  while (aff_used[times] == 0 && gen_tries++ < 20);
	  
	  
	  /* Now add all of the flags. */
	  if (aff_used[times] >= AFF_START &&
	      aff_used[times] < AFF_MAX)
	    {
	      flg = new_flag();
	      flg->from = 0;
	      flg->type = aff_used[times];
	      flg->timer = 0;
	      

	      /* These are the powers of the items based on the
		 level_sqrt number. */
	      
	      if (rank == AFF_RANK_POOR)		
		flg->val = nr (1, lev_sqrt*3/2);
	      else if (rank == AFF_RANK_FAIR)
		flg->val = nr (1, lev_sqrt*2/3);
	      else if (rank == AFF_RANK_GOOD)
		flg->val = nr (1, lev_sqrt/3);
	      else 
		flg->val = nr (1, lev_sqrt/10);
	      if (obj->wear_pos != ITEM_WEAR_WIELD &&
		  wear_data[obj->wear_pos].how_many_worn > 1)
		flg->val = flg->val*2/3;
	      if (flg->val < 1)
		flg->val = 1;
	      flg->next = obj->flags;
	      obj->flags = flg;
	    }
	}
    }

  /* Now add special object flags like glow/hum/power etc... so that
     weapons can hit better creatures. */

  if (nr (1,1000) <= obj->level)
    obj_flags |= OBJ_MAGICAL;
  
  if (LEVEL(obj) > 150 || (num_flags[AFF_RANK_EXCELLENT] > 0))
    obj_flags |= OBJ_NOSTORE;

  if (obj->wear_pos != ITEM_WEAR_WIELD)
    {
      if (nr (1,500) <= obj->level)
	obj_flags |= OBJ_GLOW;
      if (nr (1,500) <= obj->level)
	obj_flags |= OBJ_HUM;
    }
  else
    {
      for (i = 0; obj1_flags[i].flagval != 0 &&
	     obj1_flags[i].flagval <= OBJ_EARTH; i++)
	{
	  if (IS_SET (obj1_flags[i].flagval, OBJ_GLOW | OBJ_HUM))
	    {
	      if (nr (1,250) < obj->level)
		obj_flags |= obj1_flags[i].flagval;
	    }
	  else if (obj->level >= 100 && nr (1,500) < obj->level)
	    {
	      obj_flags |= obj1_flags[i].flagval;
	    }
	}
    }

  if (obj_flags)
    add_flagval (obj, FLAG_OBJ1, obj_flags);
  

  return;
}

void
do_metalgen (THING *th, char *arg)
{
  THING *obj;
  if (!IS_PC (th) || LEVEL (th) != MAX_LEVEL)
    return;
  
  generate_metal_names();
  if ((obj = find_gen_object (OBJECTGEN_AREA_VNUM, "metal_gen_names")) == NULL ||
      !obj->desc || !*obj->desc)
    {
      stt ("Couldn't generate metal names.\n\r", th);
      return;
    }
  stt ("The metal names:\n\n\r", th);
  stt (obj->desc, th);
  return;
}

/* This clears and generates a list of "fake metal" names for use when making
   objects...*/

void
generate_metal_names (void)
{
  THING *obj; /* The object where the metal names will be stored. */
  char buf[10*STD_LEN]; /* Where the complete string gets stored. */
  char name[STD_LEN];   /* Where we store the name. */
  char colorname[STD_LEN]; /* Where we store the color name. */
  char *t;
  int i;
  bool name_ok = FALSE;
  int num_generated; /* How many metal names to generate. */
  /* Keeping track of the colors. */
  int color[3], num_colors;
  int name_length; /* Used when setting up colors. */
  
  if ((obj = find_gen_object (OBJECTGEN_AREA_VNUM, "metal_gen_names")) == NULL)
    return;

  
 
  buf[0] = '\0';
  
  num_generated = nr (7,15) + 8;
  for (i = 0; i < num_generated; i++)
    {
      do
	{
	  name_ok = TRUE;
	  strcpy (name, create_society_name (NULL));
	  
	  /* Crop the name. */
	  name[nr(4,5)] = '\0';
	  
	  for (t = name; *t; t++)
	    /* Make sure that everything is a letter and not a 
	       weird letter. */
	    if (!isalpha(*t))
	      name_ok = FALSE;
	  
	  /* Move back to a consonant with a vowel before it. */
	 
	  t--;
	  while (t > name)
	    {
	      if (!isvowel (*t) &&
		  isvowel (*(t-1)))
		break;
	      t--;
	    }
	  *(t+1) = '\0';
	  
	  /* Make sure the name is long enough. */
	  if (t < name + 2)
	    name_ok = FALSE;
	}
      while (!name_ok);
      
      switch (nr (1,4))
	{
	  case 1:
	    strcat (name, "ite");
	    break;
	  case 2:
	    strcat (name, "ate");
	    break;
	  case 3:
	  case 4:
	  default:
	    strcat (name, "ium");
	    break;
	}
      
      name_length = strlen(name);
      if (nr (1,9) == 2)
	num_colors = 0;
      else if (nr (1,2) == 1)
	num_colors = 1;
      else if (nr (1,5) == 2)
	num_colors = 2;
      else 
	num_colors = 3;
      colorname[0] = '\0';
      if (num_colors > 0)
	{
	  sprintf (colorname, " & ");
	  /* Get the first colors all set to one thing. */
	  color[2] = color[1] = color[0] = nr (1,15);
	  
	  /* Make up the second color. */
	  while (color[1] == color[0])
	    color[1] = nr (1,15);
	  sprintf (colorname + strlen(colorname), "$%c", int_to_hex_digit (color[0]));
	  for (t = name; *t; t++)
	    {
	      if (t-name == name_length/num_colors)
		sprintf (colorname + strlen (colorname), "$%c",
			 int_to_hex_digit (color[1]));
	      if (t-name == 2*name_length/num_colors)
		sprintf (colorname + strlen (colorname), "$%c",
			 int_to_hex_digit (color[2]));
	      sprintf (colorname + strlen (colorname), "%c", *t);
	    }
	  strcat (colorname, "$7");

	}
      strcat (buf, name);
      strcat (buf, colorname);
      strcat (buf, "\n\r");
    }
  free_str (obj->desc);
  obj->desc = new_str (add_color(buf));

  return;
}

/* This generates and object name using some kind of animal parts name. 
   It loops through all of the randmob generators that have edescs
   of the appropriate type: armor weapon jewelry, and then if that
   thing has the correct part type, it then counts the number of
   words in kname and (armor/jewelry/weapon) and multiplies them to
   get the total number of combinations possible. In the second pass, it
   of course then finds the correct word to use by subtracting these
   products until their choice is <= 0. The weapon types can only be
   slashing and piercing, or concussion if it has the word "bone" in its
   weapon location. */


char *
objectgen_animal_name (int wear_loc, int weapon_type)
{
  THING *proto, *area;
  
  char object_type[STD_LEN]; /* Armor/wpn/jewelry */
  /* Number of diff mob names and body part names for that thing. */
  int num_proto_names, num_object_names, total_names;
  
  int count, num_choices = 0, num_chose = 0;
  
  char animal_name[STD_LEN];
  char part_name[STD_LEN];
  static char whole_name[STD_LEN];
  char suffix_name[STD_LEN]; /* Used to put suffixes on items like
				-studded or -embedded...*/
  /* The extra descriptions with the object types allowed and the name
     of the thing. (Must use kname) */
  EDESC *typedesc = NULL, *namedesc = NULL;
  /* Make it more likely for things other than those with massive
     amounts of prefixes and item types to get a chance. */
  int square_total_names;
  
  if (wear_loc <= ITEM_WEAR_NONE ||
      wear_loc >= ITEM_WEAR_MAX ||
      wear_loc == ITEM_WEAR_BELT)
    return nonstr;
  
  if (wear_loc == ITEM_WEAR_WIELD && 
      (weapon_type < 0 || weapon_type >= WPN_DAM_MAX))
    return nonstr;
  
  if ((area = find_thing_num (MOBGEN_PROTO_AREA_VNUM)) == NULL)
    return nonstr;
  
  if (wear_loc == ITEM_WEAR_WIELD)
    strcpy (object_type, "weapon");
  else if (wear_data[wear_loc].how_many_worn <= 1)
    strcpy (object_type, "armor");
  else
    strcpy (object_type, "jewelry");
  
  whole_name[0] = '\0';
  for (count = 0; count < 2; count++)
    {
      for (proto = area->cont; proto; proto = proto->next_cont)
	{
	  /* Check if it has the proper edesc. */
	  if ((typedesc = find_edesc_thing (proto, object_type, TRUE)) == NULL ||
	      (namedesc = find_edesc_thing (proto, "kname", TRUE)) == NULL)	    
	    continue;
	  
	  num_proto_names = find_num_words (namedesc->desc);
	  num_object_names = find_num_words (typedesc->desc);
	  
	  square_total_names = num_proto_names*num_object_names;
	  
	  total_names = 0;
	  while (total_names*total_names < square_total_names)
	    total_names++;
	  
	  if (total_names < 1)
	    continue;
	  
	  if (count == 0)
	    num_choices += total_names;
	  else 
	    {
	      num_chose -= total_names;
	      if (num_chose < 1)
		break;
	    }
	}

      if (count == 0)
	{
	  if (num_choices < 1)
	    return nonstr;
	  num_chose = nr (1, num_choices);
	}
    }

  if (!proto || !namedesc || !typedesc)
    return nonstr;
  
  strcpy (animal_name, find_random_word (namedesc->desc, NULL));
  strcpy (part_name, find_random_word (typedesc->desc, NULL));
  suffix_name[0] = '\0';
  /* Animal weapon parts in weapons have to be handled more delicately.
     "bones" can go into piercing/slashing/conc. 
     
     teeth/fangs/claws/talons/pincers only go into slashing/piercing
     but can be embedded or spiked into conc/whipping. */

  if (wear_loc == ITEM_WEAR_WIELD)
    {
      if (weapon_type == WPN_DAM_WHIP)
	{
	  if (!named_in (part_name, "fang claw tooth"))
	    return nonstr;
	}
      else if (weapon_type == WPN_DAM_CONCUSSION)
	{
	  if (str_cmp (part_name, "bone"))
	    {
	      strcpy (suffix_name, find_random_word ("-studded -embedded -spiked", NULL));
	    }
	}
      else 
	suffix_name[0] = '\0';
    }
  /* Jewelry may have a -studded suffix for non feather things. */
  else if (!str_cmp (object_type, "jewelry"))
    {
      if (str_cmp (part_name, "feather") &&
	  str_cmp (part_name, "feathers") &&
	  nr (1,5) == 2)
	{
	  strcpy (suffix_name, find_random_word ("-studded", NULL));
	}
    }
  
  /* Now set up the name: animal_name(middle)part_name-suffix_name */
  
 
  /* No space in middle. */
  if (nr (1,5) != 3 &&
      named_in (part_name, "hide skin scale tooth claw fang bone"))
    sprintf (whole_name, "%s%s%s", animal_name, part_name, suffix_name);
  else if (nr (1,5) == 2 && !*suffix_name) /* Hyphen -- only if no suffix */
    sprintf (whole_name, "%s-%s", animal_name, part_name);
  else /* Space */
    {
      /* Check for possessive form. */
      if (nr (1,3) == 2)
	possessive_form (animal_name);
      sprintf (whole_name, "%s %s%s", animal_name, part_name, suffix_name);
    }
    
  return whole_name;
}
