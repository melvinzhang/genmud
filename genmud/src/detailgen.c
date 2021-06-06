#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "track.h"
#include "areagen.h"
#include "objectgen.h"
#include "detailgen.h"


/* Global society name used because I don't feel like passing it
   around to all of the functions. */

static char detail_society_name[STD_LEN];

void
detailgen (THING *area)
{
  THING *start_room = NULL, *area_in, *room;
  int sector_type;
  int num_details;
  int i, times,  j;
  int used[DETAIL_MAX];
  
  if (!area || !IS_AREA (area) || 
      (sector_type = flagbits (area->flags, FLAG_ROOM1)) == 0 ||
      !IS_SET (sector_type, ROOM_SECTOR_FLAGS))
    return;
  
  
  
  /* Get the number of details: */
  
  for (i = 0; i < DETAIL_MAX; i++)
    used[i] = 0;
  
  num_details = nr (1, 1 + area->mv/DETAIL_DENSITY);
  
  /* Try to add a detail for each detail listed. */

  for (times = 0; times < num_details; times++)
    {
      if ((start_room = find_detail_type (area, used)) != NULL &&
	  IS_ROOM (start_room) &&
	  (area_in = find_area_in (start_room->vnum)) != NULL &&
	  area_in->vnum == DETAILGEN_AREA_VNUM &&
	  start_room->name && *start_room->name)
	{
	  /* The weight in the detail room lets you determine
	     how many times the detail can be added to an area. */
	  for (j = 0; j < MAX(1, start_room->weight); j++)
	    {
	      if (j == 0 || nr(1,2) == 1)
		{
		  detail_society_name[0] = '\0';
		  add_detail (area, area, start_room, 0); 
		  for (room = area->cont; room; room = room->next_cont)
		    {
		      RBIT (room->thing_flags, TH_MARKED);
		    }
		  detail_society_name[0] = '\0';
		}
	    }
	}
    }

 
  return;
  
}

/* This finds a detail from the details area and tries to get it
   set up within the area. The used array is used so that we don't
   put two details of the same type in an area. */

THING *
find_detail_type (THING *area, int used[DETAIL_MAX])
{
  THING *room, *detail_area, *start_room = NULL;
  int num_choices = 0, num_chose = 0, count;
  int num = 0;
  bool used_already = FALSE;
  if ((detail_area = find_thing_num (DETAILGEN_AREA_VNUM)) == NULL ||
      !IS_AREA (detail_area) || !area || !IS_AREA (area))
    return NULL;
  
  
  for (count = 0; count < 2; count++)
    {
      for (room = detail_area->cont; room; room = room->next_cont)
	{
	  /* We only search rooms in the first half of the detail area
	     room allocation, and then we only pick rooms with
	     names. */
	  if (!IS_ROOM (room) || !room->name || !*room->name ||
	      ((room->vnum - detail_area->vnum) >= detail_area->mv/2))
	    continue;
			
	  /* You can set levels on detail rooms so that they
	     don't show up in lower level areas. */
	  if (room->level > area->level)
	    continue;
	  
	  used_already = FALSE;
	  for (num = 0; num < DETAIL_MAX; num++)
	    {
	      if (used[num] == room->vnum)
		{
		  used_already = TRUE;
		  break;
		}
	    }
	  if (used_already)
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
  
  if (!room)
    return NULL;
  
  for (num = 0; num < DETAIL_MAX; num++)
    {
      if (used[num] == 0)
	{
	  used[num] = room->vnum;
	  start_room = room;
	  break;
	}
    }

  return start_room;
}

/* This adds a detail to a thing. It does it in the following
   ways .

   1. If to is an area, then if detail_thing is an area, quit. If
   detail_thing is not in the detail area, quit. If detail_thing is 
   a room within detail_area->mv/2 then setup the main block of rooms
   using add_main_detail_rooms. If the detail_thing is another room,
   add the block of rooms using start_other_detail_rooms, and then
   return one of the rooms that the detail was added to as the newth.
   If detail_thing is a mob or object, create it and add it as a reset
   to one of the marked rooms in the area.
   
   2. If to is a room, then if detail_thing is a room or area, quit. If
   detail_thing is a mob or object, create the mobject from the
   info given and add it as a reset to the room and call the thing
   made newth;
   
   If to is a mobject and detail_thing isn't an object, quit. Else,
   add a reset of the object onto to, and make the new object newth
   
   If newth doesn't exist, then bail out.
   
   Then, go down the list of resets in detail_thing. If they are resets
   for real items outside of the detail area, then copy the reset to newth
   directly. Otherwise, find the thing in the detail area that the
   reset represents and call it reset_detail_thing. Then call
   add_detail (newth, reset_detail_thing, depth+1); */

/* The reason for the had_society_name_first variable is that if I 
   didn't have such a name and I created a mobject and that mobject
   happened to add such a name, I want to dump the name when I get
   out of this mobject. */

void
add_detail (THING *area, THING *to, THING *detail_thing, int depth)
{
  THING *newth = NULL, *detail_area;
  bool had_society_name_first = FALSE;
  if (!to || !detail_thing || IS_AREA (detail_thing) || 
      depth >= DETAIL_MAX_DEPTH)
    return;
  
  if ((detail_area = detail_thing->in) == NULL ||
      !IS_AREA (detail_area) || 
      detail_area->vnum != DETAILGEN_AREA_VNUM)
    return;
  
  if (*detail_society_name)
    had_society_name_first = TRUE;

  if (IS_AREA (to))
    {      
      /* Little check here to make sure that the area we're adding the
	 detail to is the area we're adding the detail to.... */
      if (to != area)
	return;

      if (IS_ROOM (detail_thing))
	{
	  /* Don't add secondary detail rooms that are in the primary 
	     detail room area. */
	  if (detail_thing->vnum - detail_area->vnum <
	      detail_area->mv/2 && depth == 0)
	    {
	      start_main_detail_rooms (area, detail_thing);
	      newth = to;
	    }
	  else
	    {	      
	      newth = start_other_detail_rooms (area, detail_thing);
	    }
	}
      else
	newth = generate_detail_mobject (area, to, detail_thing);
    }
  else if (IS_ROOM (to))
    {
      if (IS_AREA (detail_thing) || IS_ROOM (detail_thing))
	return;
      
      newth = generate_detail_mobject (area, to, detail_thing);
    }
  else 
    {
      if (IS_AREA (detail_thing) || IS_ROOM (detail_thing) ||
	  CAN_MOVE (detail_thing) || CAN_FIGHT (detail_thing))
	return;
      newth = generate_detail_mobject (area, to, detail_thing);
    }
  
  if (newth)
    add_detail_resets (area, newth, detail_thing, depth);

  /* Now if we didn't have a society name to start with and we do 
     now and this is a mobject, then delete the society name. */
  
  if (!had_society_name_first && 
      (!newth || 
       (!IS_ROOM (newth) && !IS_AREA(newth))) &&
      *detail_society_name)
    *detail_society_name = '\0';
       
  return;
}
	  
/* This starts a detail by generating its name and then creating the
   detail. Then it looks for the resets on the detail to add more 
   details to the detail. */

void
start_main_detail_rooms (THING *area, THING *detail_room)
{
  THING *detail_area, *room, *start_room, *nroom;
  VALUE *exit;
  int area_sector_flags;
  /* Used for picking the room where the detail will start. */
  bool adjacent_rooms_ok;
  int num_choices = 0, num_chose = 0, dir, count;
  /* What kinds of rooms this detail can be placed into. */
  int detail_room_bits, extra_detail_room_bits = 0;
  VALUE *oval, *nval;
  char roomname[STD_LEN];
  char fullroomname[STD_LEN];
  char *t;
  if ((detail_area = find_thing_num (DETAILGEN_AREA_VNUM)) == NULL ||
      !IS_AREA (detail_area))
    return;

  if (!area || !IS_AREA (area))
    return;
  
  if (!IS_ROOM (detail_room) || !detail_room->name || !*detail_room->name ||
      ((detail_room->vnum - detail_area->vnum) >= detail_area->mv/2))
    return;
  
  detail_room_bits = flagbits (detail_room->flags, FLAG_ROOM1) & ROOM_SECTOR_FLAGS ;
  /* Inside can go anywhere. */
  extra_detail_room_bits = detail_room_bits & ROOM_INSIDE;
  RBIT (detail_room_bits, ROOM_INSIDE);
  area_sector_flags = flagbits (area->flags, FLAG_ROOM1);
  
  /* If the detail room has bits specified, then the area must have
     some of those bits or else the detail won't get made. The room
     where the detail gets started must also have some of those
     bits and it must be adjacent only to room with those bits. */
  if (IS_SET (detail_room_bits, AREAGEN_SECTOR_FLAGS) &&
      !(area_sector_flags & detail_room_bits & AREAGEN_SECTOR_FLAGS))
    return;
  
  for (count = 0; count < 2; count++)
    {
      for (room = area->cont; room; room = room->next_cont)
	{
	  /* The room must be a room and it must have the correct
	     bits set and it must not have resets. */
	  if (!IS_ROOM (room) || room->resets ||
	      IS_ROOM_SET (room, BADROOM_BITS | ROOM_EASYMOVE) ||
	      is_named (room, "detail") ||
	      (detail_room_bits &&
	       !IS_ROOM_SET (room, detail_room_bits)))
	    continue;
	  
	  adjacent_rooms_ok = TRUE;
	  
	  for (dir = 0; dir < REALDIR_MAX; dir++)
	    {
	      if ((exit = FNV (room, dir + 1)) != NULL &&
		  (nroom = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM (nroom) &&
		  (nroom->resets ||
		   IS_ROOM_SET (nroom, BADROOM_BITS | ROOM_EASYMOVE) ||
		   (detail_room_bits && 
		    (!IS_ROOM_SET (nroom, detail_room_bits)))))
		{
		  adjacent_rooms_ok = FALSE;
		  break;
		}
	    }
	  if (!adjacent_rooms_ok)
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return;
	  else
	    num_chose = nr (1, num_choices);
	}
      
    }
  
  if (room && IS_ROOM (room) && !room->resets &&
      !IS_ROOM_SET (room, BADROOM_BITS | ROOM_EASYMOVE) &&
      (!detail_room_bits || IS_ROOM_SET (room, detail_room_bits)))
    start_room = room;
  else
    return;
  
  /* Now get the name of this detail. */
  
  if (!str_cmp (detail_room->name, "previous_name") ||
      !str_cmp (detail_room->name, "same_name"))
    strcpy (roomname, room->short_desc);
  else
    strcpy (roomname, generate_detail_name (detail_room));
  
  if (!*roomname)
    return;
  detail_room_bits |= extra_detail_room_bits;
  
  sprintf (fullroomname, "%s %s", a_an (roomname), roomname);
  fullroomname[0] = UC (fullroomname[0]);
  

  for (t = fullroomname + 1; *t; t++)
    {
      if (*(t-1) == ' ')
	*t = UC(*t);
    }
  
  copy_flags (detail_room, start_room);
  remove_flagval (start_room, FLAG_ROOM1, 
		  flagbits(detail_room->flags, FLAG_ROOM1) & ROOM_SECTOR_FLAGS);
  add_flagval (start_room, FLAG_ROOM1, detail_room_bits);
  for (oval = detail_room->values; oval; oval = oval->next)
    {
      
      if (oval->type > REALDIR_MAX)
	{
	  nval = new_value();
	  copy_value (oval, nval);
	  add_value (start_room, nval);
	}
    }
  undo_marked (start_room);
  add_main_detail_room (start_room, fullroomname, MAX(1,detail_room->height),
			detail_room_bits);
  return;
}


/* This adds a detail room to the map. It adds the room's name. */

void
add_main_detail_room (THING *room, char *name, int depth_left, int room_bits)
{
  THING *nroom;
  VALUE *exit;
  int dir;
  char buf[STD_LEN];
  VALUE *oval, *nval;
  if (depth_left < 1 || !room || !IS_ROOM (room) || !name || !*name ||
      IS_ROOM_SET (room, BADROOM_BITS | ROOM_EASYMOVE) ||
      IS_MARKED (room))
    return;
  
  SBIT (room->thing_flags, TH_MARKED);
  if (room->name && *room->name)
    {
      strcpy (buf, room->name);
      strcat (buf, " ");
    }
  else
    buf[0] = '\0';
  strcat (buf, "detail");
  free_str (room->name);
  room->name = new_str (buf);
  free_str (room->short_desc);
  room->short_desc = new_str (name);
  /* Copy the flags and values over. */
  
  /* Now remove old bits except underground and add new bits. */
  
  

  if (room_bits)
    {
      remove_flagval (room, FLAG_ROOM1, AREAGEN_SECTOR_FLAGS &~ ROOM_UNDERGROUND);
      add_flagval (room, FLAG_ROOM1, room_bits);
    }
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      if ((exit = FNV (room, dir + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (room))
	{
	  copy_flags (room, nroom); 
	  for (oval = room->values; oval; oval = oval->next)
	    {	      
	      if (oval->type > REALDIR_MAX)
		{
		  nval = new_value();
		  copy_value (oval, nval);
		  add_value (nroom, nval);
		}
	    }
	  add_main_detail_room (nroom, name, depth_left - 1, room_bits);
	}
    }
  return;
}

/* This starts and adds other detail rooms besides the original ones. 
   It returns the room where the details were started. */

THING *
start_other_detail_rooms (THING *area, THING *detail_thing)
{
  THING *room, *nroom, *detail_area;
  int num_choices = 0, num_chose = 0, count;
  int dir;
  int room_flags = 0;
  VALUE *exit;
  char temproomname[STD_LEN];
  char roomname[STD_LEN];
  char *t;
  
  if (!area || !IS_AREA (area) || !detail_thing ||
      (detail_area = detail_thing->in) == NULL || 
      detail_area->vnum != DETAILGEN_AREA_VNUM ||
      !IS_ROOM (detail_thing) ||
      ((detail_thing->vnum-detail_area->vnum) < detail_area->mv/2))
    return NULL;
  
  /* Make sure we can get a detail name out of this. */

 
  strcpy (temproomname, generate_detail_name (detail_thing));
  if (!*temproomname)
    return NULL; 
  sprintf (roomname, "%s %s", a_an(temproomname), temproomname);
  
  /* Capitalize all words in roomname. */
  roomname[0] = UC(roomname[0]);
  for (t = roomname + 1; *t; t++)
    {
      if (*(t-1) == ' ')
	*t = UC(*t);
    }
      
  /* Get the room flags we will set. */

  room_flags = flagbits (detail_thing->flags, FLAG_ROOM1) & ~ROOM_MINERALS;

  /* We know we're in an area and the detail thing is a room in 
     the proper part of the detail area. */

  /* Find the room where the smaller detail chunk will start. */

  for (count = 0; count < 2; count++)
    {
      for (room = area->cont; room; room = room->next_cont)
	{
	  if (IS_ROOM (room) && IS_MARKED (room))
	    {
	      if (count == 0)
		num_choices++;
	      else if (--num_chose < 1)
		break;
	    }
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return NULL;
	  num_chose = nr (1, num_choices);
	}
    }
  
  
  if (!room)
    return NULL;
  
  /* Now replace the room and things nearby (if needed) */
  
  if (!str_cmp (detail_thing->name, "previous_name") ||
      !str_cmp (detail_thing->name, "same_name"))
    strcpy (roomname, room->short_desc);
  
  free_str (room->short_desc);
  room->short_desc = new_str (roomname);
  RBIT (room->thing_flags, TH_MARKED);
  if (room_flags)
    {
      remove_flagval (room, FLAG_ROOM1, 
		      flagbits (room->flags, FLAG_ROOM1));
      add_flagval (room, FLAG_ROOM1, room_flags);
      if (IS_ROOM_SET (area, ROOM_UNDERGROUND))
	add_flagval (room, FLAG_ROOM1, ROOM_UNDERGROUND);
    }
  
  /* Now check each dir to see if the exits need to be added. The way 
     to do it is to see if nr (1,4) > detail_room->height-1 or not. */

  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      if ((exit = FNV (room, dir+1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (nroom) && IS_MARKED(nroom) &&
	  nr (1,4) >= (detail_thing->height-1))
	{
	  free_str (nroom->short_desc);
	  nroom->short_desc = new_str (roomname);
	  RBIT (nroom->thing_flags, TH_MARKED);
	  if (room_flags)
	    { 
	      remove_flagval (nroom, FLAG_ROOM1, 
			      flagbits (nroom->flags, FLAG_ROOM1));
	      add_flagval (nroom, FLAG_ROOM1, room_flags);
	    }
	}
    }
  return room;
}


/* This generates a detail based on the data in a thing. 
   The basic format is to have strings of the form
   (long_desc) (short_desc) (name)
   based on the words found in the thing. It may not use all of the choices. */

char *
generate_detail_name (THING *proto)
{
  
  char name[STD_LEN];
  char shortdesc[STD_LEN];
  char longdesc[STD_LEN];
  static char retbuf[STD_LEN];
  retbuf[0] = '\0';
  if (!proto || !proto->name || !*proto->name)
    return retbuf;
  
  /* These attempt to create each name. The only difference is that
     if you encounter a "society_name" and if it's not set yet,
     then it sets detail_society_name to that name. If it is set
     and you encounter it, then it copies the same name into the 
     word. */
  
  strcpy (name, find_random_word 
	  (proto->name, detail_society_name));  
  strcpy (shortdesc, find_random_word 
	  (proto->short_desc, detail_society_name));
  strcpy (longdesc, find_random_word 
	  (proto->long_desc, detail_society_name));
 
  if (!*name)
    return retbuf; 
  
  sprintf (retbuf, "%s%s%s%s%s", longdesc, (*longdesc ? " " : ""),
	   shortdesc, (*shortdesc ? " " : ""), name);
  
  return retbuf;
}


/* This lets you create a mobject within an area and then add it to
   something as a reset. */

THING *
generate_detail_mobject (THING *area, THING *to, THING *detail_thing)
{
  THING *detail_area, *mobject, *thg, *new_to;
  int vnum;
  RESET *reset;
  char detailname[STD_LEN];
  char name[STD_LEN];
  char createdname[STD_LEN];
  bool proper_name_first = FALSE, proper_name_second = FALSE;
  
  if (!area || !IS_AREA (area) || !to || !detail_thing ||
      (IS_AREA (area) && IS_AREA(to) && to != area) ||
      (detail_area = detail_thing->in) == NULL ||
      detail_area->vnum != DETAILGEN_AREA_VNUM ||
      IS_ROOM (detail_thing) || IS_AREA (detail_thing))
    return NULL;
  
  /* Mobs can only go to areas/rooms. */
  
  if (!IS_ROOM (to) && !IS_AREA(to) &&
      (CAN_MOVE (detail_thing) || CAN_FIGHT (detail_thing)))
    return NULL;
  
  /* Find an open vnum. */
  
  for (vnum = area->vnum +area->mv+1; vnum < area->vnum + area->max_mv; vnum++)
    {
      if ((thg = find_thing_num (vnum)) == NULL)
	break;
    }
  
  if (vnum <= area->vnum + area->mv ||
      vnum >= area->vnum + area->max_mv)
    return NULL;
  
 
  
  /* Make sure that we have a name. */
  
  strcpy (detailname, generate_detail_name (detail_thing));
  
  if (!*detailname)
    return NULL;
  
 /* Find a created society name. */
  
  if (nr (1,5) == 2 && CAN_TALK (detail_thing) &&
      !strstr (detail_thing->name, "proper_name") &&
      !strstr (detail_thing->short_desc, "proper_name") &&
      !strstr (detail_thing->long_desc, "proper_name"))    
    strcpy (createdname, create_society_name(NULL));
  else
    {
      if (!str_cmp (detail_thing->long_desc, "proper_name"))
	proper_name_first = TRUE;
      if (!str_cmp (detail_thing->short_desc, "proper_name"))
	proper_name_second = TRUE;
      createdname[0] = '\0';
    }
  

  mobject = new_thing ();
  copy_thing (detail_thing, mobject);  
  mobject->vnum = vnum;
  if (CAN_TALK (mobject))
    mobject->level += nr (area->level/2, area->level);
  thing_to (mobject, area);
  add_thing_to_list (mobject);
  free_str (mobject->desc);
  
  if (createdname[0])
    sprintf (name, "%s %s", detailname, createdname);
  else
    strcpy (name, detailname);
  
  free_str (mobject->name);
  mobject->name = new_str (name);
  
  /* Create the full name and capitalize all words in it. */
  
  if (*createdname)
    sprintf (name, "%s the %s", createdname, detailname);
  else if (proper_name_first)
    {
      /* Split the name "Foo something something" 
	 into "Foo the something something". */
      char *arg, arg1[STD_LEN], arg2[STD_LEN];
      arg = detailname;
      arg = f_word (arg, arg1);
      *arg1 = UC(*arg1);
      if (!proper_name_second)
	sprintf (name, "%s the %s", arg1, arg);
      else
	{
	  arg = f_word (arg, arg2);
	  *arg2 = UC(*arg2);
	  sprintf (name, "%s %s the %s", arg1, arg2, arg);
	}
    }
  else
    sprintf (name, "%s %s",  a_an (detailname), detailname);  
  free_str (mobject->short_desc);
  mobject->short_desc = new_str (name);
  
  strcat (name, " is here.");
  name[0] = UC (name[0]);
  free_str (mobject->long_desc);  
  mobject->long_desc = new_str (name);
  
  /* Mobs get set to max 1 reset. */
  if (CAN_MOVE (mobject) || CAN_FIGHT (mobject))
    mobject->max_mv = 1;
  else
    mobject->max_mv = 0;
  mobject->mv = 0;

  /* Now add detection. */

  if (CAN_MOVE (mobject) || CAN_FIGHT (mobject))
    {
      if (LEVEL (mobject) >= 80)
	add_flagval (mobject, FLAG_DET, AFF_DARK);
      if (LEVEL (mobject) >= 120)
	add_flagval (mobject, FLAG_DET, AFF_INVIS);
      if (LEVEL (mobject) >= 200)
	add_flagval (mobject, FLAG_DET, AFF_CHAMELEON);
    }
  
  /* Now add the reset. */
  
  if (IS_AREA (to))
    {
      new_to = find_random_room (area, TRUE, 0, 0);
    }
  else
    new_to = to;
  
  if (!new_to)
    return NULL;
  
  reset = new_reset();
  reset->vnum = vnum;
  reset->max_num = 1;
  reset->nest = 1;
  if (CAN_MOVE(mobject) || CAN_FIGHT (mobject) ||
      IS_SET (mobject->thing_flags, TH_NO_TAKE_BY_OTHER | TH_NO_MOVE_BY_OTHER))
    reset->pct = 100;
  else
    reset->pct = MAX(2, 70 - mobject->level*2/3);
  
  reset->next = new_to->resets;
  new_to->resets = reset;
  return mobject;
  
}

  
  
/* This adds the details contained in detail_loc to the
   to_loc object. */

void
add_detail_resets (THING *area, THING *to, THING *detail_thing, int depth)
{
  RESET *reset, *newreset, *prev;
  int times;
  THING *detail_area, *reset_in_area, *new_detail_thing;
  THING *randpop_item, *room_to;
  VALUE *randpop;
  int randmob_sector_flags;
  if (!area || !IS_AREA (area) || !to || !detail_thing ||
      (IS_AREA (area) && IS_AREA(to) && to != area) ||
      (detail_area = detail_thing->in) == NULL ||
      detail_area->vnum != DETAILGEN_AREA_VNUM)
    return;
  
  for (reset = detail_thing->resets; reset; reset = reset->next)
    {
      /* If the reset is the objectgen area number, make random
	 objects for this thing. */
      
      if (reset->vnum == OBJECTGEN_AREA_VNUM)
	{
	  for (times = 0; times < MAX(1, reset->max_num); times++)
	    {
	      if (nr(1,100) <= reset->pct &&
		  (new_detail_thing = 
		   objectgen (area, ITEM_WEAR_NONE, 
			      nr (to->level*2/3, to->level*5/4), 
			      0, NULL, NULL)) != NULL)
		{
		  newreset = new_reset();
		  newreset->vnum = new_detail_thing->vnum;
		  newreset->max_num = 0;
		  newreset->pct = MAX (2, 70-new_detail_thing->vnum*2/3);
		  newreset->next = to->resets;
		  to->resets = newreset;
		}
	    }
	}
      else if ((reset_in_area = find_area_in (reset->vnum)) != detail_area)
	{
	  /* Deal with randpop mobs differently. In this case, pick
	     a randpop mob and have several resets of it in one
	     place. */

	  if (reset->vnum == MOB_RANDPOP_VNUM &&
	      (randpop_item = find_thing_num (MOB_RANDPOP_VNUM)) != NULL &&
	      (randpop = FNV (randpop_item, VAL_RANDPOP)) != NULL &&
	      (IS_AREA (to) || IS_ROOM (to)))
	    {
	      /* If to is an area, find one of the marked rooms. */
	      
	      
	      for (times = 0; times < 30; times++)
		{
		  if ((new_detail_thing = 
		       find_thing_num 
		       (calculate_randpop_vnum 
			(randpop, area->level))) != NULL && 
		      !IS_ROOM (new_detail_thing) && 
		      !IS_AREA (new_detail_thing) &&		      
		      CAN_MOVE (new_detail_thing) &&
		      CAN_FIGHT (new_detail_thing) &&
		      ((randmob_sector_flags = 
			flagbits (new_detail_thing->flags, FLAG_ROOM1)) == 0 ||
		       IS_ROOM_SET (area, randmob_sector_flags)) &&
		      ((IS_ROOM (to) && (room_to = to)) ||
		       (IS_AREA (to) &&
			(room_to = find_random_room (to, TRUE, 0, 0)) != NULL)))
		    {
		      if (!room_to)
			continue;
		     
		      newreset = new_reset ();
		      newreset->vnum = new_detail_thing->vnum;
		      newreset->pct = 50;
		      newreset->max_num = 50/(MAX(10, new_detail_thing->level));
		      newreset->max_num = MID (1, newreset->max_num, 3);
		      newreset->next = room_to->resets;
		      newreset->nest = 1;
		      room_to->resets = newreset;
		      break;
		    }
		}
	    }
	  else
	    {
	      newreset = new_reset();
	      copy_reset (reset, newreset);	      	      
	      if (!to->resets)
		{
		  newreset->next = to->resets;
		  to->resets = newreset;
		}
	      else
		{
		  for (prev = to->resets; prev; prev = prev->next)
		    {
		      if (!prev->next)
			{
			  prev->next = newreset;
			  newreset->next = NULL;
			  break;
			}
		    }
		}
	    }
	}
      else if ((new_detail_thing = find_thing_num (reset->vnum)) != NULL)
	{
	  for (times = 0; times < MAX(1, reset->max_num); times++)
	    {
	      if (nr(1,100) <= reset->pct)		
		add_detail (area, to, new_detail_thing, depth+1);
	    }
	}
    }
  
  return;
}

