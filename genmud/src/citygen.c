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
#include "mobgen.h"
#include "citygen.h"



/* The actual city rooms are stored in this grid. */

THING *city_grid[CITY_SIZE][CITY_SIZE][CITY_HEIGHT];


/* This is a specialized type of "areagen". */

void
do_citygen (THING *th, char *arg)
{
  if (!th || LEVEL(th) < MAX_LEVEL)
    return;
  citygen (th, arg);
  
  return;
}

void 
citygen (THING *th, char *arg)
{
  int start, size, level;
  char arg1[STD_LEN];
  THING *area, *citygen_area, *thg;
  EDESC *districts;
  int times;
  char *wd;
  char word[STD_LEN];
  int x, y;
  THING *room1, *room2;
  if (!arg || !*arg)
    return;

  if (!str_cmp (arg, "show"))
    {
      show_city_map (th);
      return;
    }

  arg = f_word (arg, arg1);
  if ((start = atoi (arg1)) < 1)
    {
      stt ("citygen <start> <size> [<level>]\n\r", th);
      return;
    }
  
  arg = f_word (arg, arg1);
  if ((size = atoi (arg1)) < 100 || size > 2000)
    {
      stt ("The size of the city must be from 100-2000 things.\n\r", th);
      return;
    }
  
  if (!check_area_vnums (NULL, start, start+size))
    {
      stt ("This city site overlaps another area.\n\r", th);
      return;
    }

  arg = f_word (arg, arg1);
  
  if ((level = atoi (arg1)) == 0)
    level = 100;

  if ((citygen_area = find_thing_num (CITYGEN_AREA_VNUM)) == NULL)
    {
      stt ("The citygen area doesn't exist. Bailing out.\n\r", th);
      return;
    }

  if ((districts = find_edesc_thing (citygen_area, "districts", TRUE)) == NULL)
    {
      stt ("The citygen area doesn't have a list of districts to set up inside of the city.\n\r", th);
      return;
    }

  /* Create the area and set it up. */
  
  if ((area = new_thing ()) == NULL)
    return;
  
  area->vnum = start;
  top_vnum = MAX (top_vnum, start);
  add_thing_to_list (area);
  set_up_new_area(area, size);
  area->mv = size*2/3;


  /* This generates a base city grid that uses up about 3/4 of the
     available rooms. It makes a grid of partially overlapping 
     squares. */
  
  generate_base_city_grid (area, area->vnum+1);
  

  /* Now add each of the districts. Find the district in the citygen
     area and plop it into the city. */
  wd = districts->desc;
  do
    {
      wd = f_word (wd, word);
      if ((thg = find_thing_thing (citygen_area, citygen_area, word, FALSE)) == NULL)
	continue;
  
      /* Get the number of times we try to set this thing into the
	 city. */
      times = MAX(1, thg->max_mv*area->mv/(CITYGEN_MAX_SIZE/4));
      if (times > 4)
	times = 4;
      while (--times >= 0)
	{
	  citygen_add_detail (thg, area, NULL, 1);
	}
    }
  while (wd && *wd);
  
  /* Now connect all rooms at depth 1 on the road level. */

  for (x = 0; x < CITY_SIZE; x++)
    {
      for (y = 0; y < CITY_SIZE; y++)
	{
	  if (city_coord_is_ok (x,y,CITY_STREET_LEVEL) &&
	      (room1 = city_grid[x][y][CITY_STREET_LEVEL]) != NULL &&
	      room1->mv == 0)
	    {
	      if (city_coord_is_ok (x+1, y, CITY_STREET_LEVEL) &&
		  (room2 = city_grid[x+1][y][CITY_STREET_LEVEL]) != NULL &&
		  room2->mv == 0)
		{
		  room_add_exit (room1, DIR_EAST, room2->vnum);
		  room_add_exit (room2, DIR_WEST, room1->vnum);
		}
	      if (city_coord_is_ok (x, y+1, CITY_STREET_LEVEL) &&
		  (room2 = city_grid[x][y+1][CITY_STREET_LEVEL]) != NULL &&
		  room2->mv == 0)
		{
		  room_add_exit (room1, DIR_NORTH, room2->vnum);
		  room_add_exit (room2, DIR_SOUTH, room1->vnum);
		}
	    }
	}
    }

  clear_city_grid(FALSE);
  return;
}


  
/* This clears the city grid of all objects...and may delete some of
   them if you specify that it should do so. */
  
void
clear_city_grid (bool nuke_things)
{
  int x, y, z;
  
  for (x = 0; x < CITY_SIZE; x++)
    {
      for (y = 0; y < CITY_SIZE; y++)
	{
	  for (z = 0; z < CITY_SIZE; z++)
	    {
	      if (nuke_things && city_grid[x][y][z])
		{
		  /* Get rid of room thing_flag so it will nuke the thing. */
		  city_grid[x][y][z]->thing_flags = 0;
		  free_thing (city_grid[x][y][z]);
		}
	      city_grid[x][y][z] = NULL;
	    }
	}
    }
  return;
}

/* This tells if a certain x, y, z triple is within the city grid
   or not. */

bool
city_coord_is_ok (int x, int y, int z)
{
  if (x < 0 || x >= CITY_SIZE ||
      y < 0 || y >= CITY_SIZE ||
      z < 0 || z >= CITY_HEIGHT)
    return FALSE;
  return TRUE;
}


/* This generates the base city shape as a grid of partially overlapping
   squares. */

void
generate_base_city_grid (THING *obj, int start_vnum)
{
  /* These are the curr_vnum we're on and the max vnum available for
     generating this grid of rooms. */
  int curr_vnum, max_vnum = 0;
  int dim_x = 0, dim_y = 0, dim_z;
  int start_x, start_y, start_z;
  int x, y, z;
  int num_rooms, max_num_rooms, max_num_rooms_sqrt;
  int num_blocks; /* Number of blocks of rooms we will use to make the 
		     city or building. */
  int block_times = 0; /* Number of times we've added blocks. */
  VALUE *dim;
  THING *area, *room;
  int vnum;
  bool rooms_overlap;
  
  if (!obj || !start_vnum)
    return;

  if (IS_AREA (obj))
    area = obj;
  else if ((area = find_area_in (start_vnum)) == NULL)
    return;
  
  max_vnum = area->vnum + area->mv-1;
  /* Clean up city grid before doing this. */
  clear_city_grid (TRUE);
  
  /* If this is a city, we make a bunch of blocks until we finally get
     to our target size. If it's a single building or room, just make a 
     single block based on the VAL_DIMENSION data given. */
  
  if ((dim = FNV (obj, VAL_DIMENSION)) != NULL)
    {
      num_blocks = 1;
      dim_x = nr (dim->val[0], dim->val[1]);
      dim_y = nr (dim->val[2], dim->val[3]);
      dim_z = nr (dim->val[4], dim->val[5]);
      if (dim_x < 1)
	dim_x = 1;
      if (dim_y < 1)
	dim_y = 1;
      if (dim_z < 1)
	dim_z = 1;
      num_rooms = dim_x*dim_y*dim_z;
      curr_vnum = start_vnum;
    }
  else if (!IS_AREA (obj))
    {
      dim_x = 1;
      dim_y = 1;
      dim_z = 1;
      num_blocks = 1;
      num_rooms = dim_x*dim_y*dim_z;
      curr_vnum = start_vnum;
    }
  else /* For areas, add unlimited numbers of blocks. */
    {
      num_blocks = 0; /* Unlimited blocks to be added. */
      num_rooms = (area->vnum + area->mv-start_vnum)*2/3;
      dim_z = 1;
      dim_x = 0;
      dim_y = 0;
      curr_vnum = start_vnum;
    }
  
  if (num_rooms < 1 || num_rooms > 2000)
    return;

  /* Check to make sure that all of these rooms are open. */
  for (vnum = start_vnum; vnum <= start_vnum + num_rooms; vnum++)
    {
      if ((room = find_thing_num (vnum)) != NULL)
	return;
    }
  
  if (num_blocks == 1)
    {
      start_x = (CITY_SIZE-dim_x)/2;
      start_y = (CITY_SIZE-dim_y)/2;
      start_z = 1;
      
      for (z = 1; z < start_z + dim_z; z++)
	{
	  for (x = start_x; x < start_x + dim_x; x++)
	    {
	      for (y = start_y; y < start_y + dim_y; y++)
		{
		  if (!city_coord_is_ok (x, y, z))
		    continue;
		  if (curr_vnum >= start_vnum && curr_vnum <= max_vnum &&
		      !city_grid[x][y][z])
		    {
		      city_grid[x][y][z] = new_room(curr_vnum);
		      curr_vnum++;
		      num_rooms++;
		    }
		}
	    }
	}
      return;
    }
  
     
  /* Add the blocks of rooms to the area. */
  
  num_rooms = 0;
  max_num_rooms = (max_vnum-start_vnum)*2/3;

  max_num_rooms_sqrt = 0;
  while (max_num_rooms_sqrt*max_num_rooms_sqrt < max_num_rooms)
    max_num_rooms_sqrt++;
  if (max_num_rooms_sqrt < 9)
    max_num_rooms_sqrt = 9;
  /* Do this while we haven't used at least 2/3 of the alotted rooms. */
  while (num_rooms < max_num_rooms && ++block_times < 100)
    {
      dim_x = nr (max_num_rooms_sqrt/3,max_num_rooms_sqrt*2/3);
      dim_y = nr (max_num_rooms_sqrt/3,max_num_rooms_sqrt*2/3);
      dim_z = 1;

      
      if (num_rooms == 0)
	{
	  start_x = CITY_SIZE/2 - dim_x/2;
	  start_y = CITY_SIZE/2 - dim_y/2;
	  start_z = CITY_STREET_LEVEL;
	}
      else
	{
	  start_x = CITY_SIZE/2 + nr (0, max_num_rooms_sqrt/2) - nr (0, max_num_rooms_sqrt/2);
	  start_y = CITY_SIZE/2 + nr (0, max_num_rooms_sqrt/2) - nr (0, max_num_rooms_sqrt/2);
	  start_z = CITY_STREET_LEVEL;
	  
	  /* Now check if the rooms overlap. */
	  rooms_overlap = FALSE;
	  
	  for (x = start_x; x < start_x + dim_x && !rooms_overlap; x++)
	    {
	      for (y = start_y; y < start_y + dim_y && !rooms_overlap; y++)
		{
		  for (z = start_z; z < start_z + dim_z; z++)
		    {
		      if (!city_coord_is_ok (x, y, z))
			continue;
		      
		      if (city_grid[x][y][z])
			{
			  rooms_overlap = TRUE;
			  break;
			}
		    }
		}
	    }
	  if (!rooms_overlap)
	    continue;
	}
      
      
      for (x = start_x; x < start_x + dim_x; x++)
	{
	  for (y = start_y; y < start_y + dim_y; y++)
	    {
	      for (z = start_z; z < start_z + dim_z; z++)
		{
		  if (!city_coord_is_ok (x, y, z))
		    continue;
		  
		  if (curr_vnum >= max_vnum)
		    break;
		  
		  if (!city_grid[x][y][z])
		    {
		      city_grid[x][y][z] = new_room(curr_vnum);
		      add_flagval (city_grid[x][y][z], FLAG_ROOM1, ROOM_EASYMOVE);
		      curr_vnum++;
		      num_rooms++;
		    }
		}
	    }
	}
    }  
  
  
  
  return;
}
      
      
      
void
show_city_map (THING *th)
{
  int min_x = CITY_SIZE, max_x = 0;
  int min_y = CITY_SIZE, max_y = 0;
  int x, y;
  
  char buf[STD_LEN], *t;
  
  if (!th)
    return;
  
  for (x = 0; x < CITY_SIZE; x++)
    {
      for (y = 0; y < CITY_SIZE; y++)
	{
	  if (city_grid[x][y][CITY_STREET_LEVEL])
	    {
	      if (x > max_x)
		max_x = x;
	      if (x < min_x)
		min_x = x;
	      if (y > max_y)
		max_y = y;
	      if (y < min_y)
		min_y = y;
	    }
	}
    }
  for (y = max_y; y >= min_y; y--)
    {
      buf[0] = '\0';
      t = buf;
      for (x = min_x; x <= max_x; x++)
	{
	  if (city_grid[x][y][CITY_STREET_LEVEL])
	    *t = '*';
	  else
	    *t = ' ';
	  t++;
	}
      *t++ = '\n';
      *t++ = '\r';
      *t = '\0';
      stt (buf, th);
    }
  return;
}



/* This adds a detail to an existing city. The obj argument is the
   thing describing the detail, the area is where the obj goes, 
  the dims are the xyz coords where this goes (NULL = randomly
  select from all rooms that exist) and the depth is how deeply nested
  this room is atm. */

void
citygen_add_detail (THING *obj, THING *area, VALUE *start_dims, int depth)
{
 
  /* Actual xyz sizes for the object we will place here. */
  
  int o_dim_x, o_dim_y, o_dim_z;
  
  /* The starting dims where we're allowed to search. Gotten from start
     dims. If start_dims is null, search the whole city. (at ground level). */
  
  int s_min_x, s_min_y, s_min_z, s_max_x, s_max_y, s_max_z;
  

  /* Starting x, y, z when we actually find a start location. */

  int sx, sy, sz;

  int num_choices = 0, num_chose = 0, num_perfect_choices = 0, count;
  
  int x, y, z;  /* Outer loop to find starting area. */
  int x1, y1, z1;
  char word[STD_LEN];
  VALUE rst_obj_dims;  /* Allowed dimensions of this object. */
  bool bad_room = FALSE; 
  bool location_is_bad = FALSE;
  bool use_perfect_choices = FALSE; /* Go with perfect choices for room
				       placement if any are
				       available. */

  int vnum;
  VALUE *obj_dims;
  THING *rst_obj; /* Object being reset. */
  /* Number of good rooms in this particular region. */
  int good_rooms, good_rooms_base_level;
  int rooms_needed, base_rooms_needed;
  bool found_start_location = FALSE;
  
  /* Start and end vnum where we can put things. */
  int start_room_vnum = 0, end_room_vnum = 0;
  THING *room, *proto;
  
  /* Used for linking up/down rooms. */
  int stair_x, stair_y;
  THING *below_room, *above_room;
  /* Used to link rooms horizontally. */
  THING *room1, *room2;
  RESET *rst;
  int times, dir, i;
  if (!obj || !area)
    return;
  
  /* Get the vnum(s) where these objects will be stored in the area. */
  if (IS_ROOM (obj))
    {
      int max_room_vnum = area->vnum+1;
      for (room = area->cont; room; room = room->next_cont)
	{
	  if (IS_ROOM (room) &&
	      room->vnum > max_room_vnum)
	    max_room_vnum = room->vnum;
	}
      start_room_vnum = max_room_vnum+1;
      end_room_vnum = area->vnum+area->mv;
      if (start_room_vnum > end_room_vnum)
	{
	  start_room_vnum = end_room_vnum = 0;
	}
    }
  else
    {
      start_room_vnum = find_free_mobject_vnum (area);
      end_room_vnum = start_room_vnum;
    }
  
  if (start_room_vnum == 0)
    return;
  
  /* Get the size of the new object. */
  
  /* If this has no dimensions set, then make it 1x1x1. Same for
     nonrooms. */
  if ((obj_dims = FNV (obj, VAL_DIMENSION)) == NULL || !IS_ROOM (obj))
    {
      o_dim_x = o_dim_y = o_dim_z = 1;
    }
  else
    {
      o_dim_x = MAX (1, nr (obj_dims->val[0],obj_dims->val[1]));
      o_dim_y = MAX (1, nr (obj_dims->val[2],obj_dims->val[3]));
      o_dim_z = MAX (1, nr (obj_dims->val[4],obj_dims->val[5]));
    }

     
  /* Get the starting and ending places to look. */
  
  if (start_dims)
    {
      s_min_x = start_dims->val[0];
      s_max_x = start_dims->val[1];
      s_min_y = start_dims->val[2];
      s_max_y = start_dims->val[3];
      s_min_z = start_dims->val[4];
      s_max_z = start_dims->val[5];
    }
  else
    {
      s_min_x = s_min_y = 0;
      s_max_x = s_max_y = CITY_SIZE-1;
      s_min_z = s_max_z = CITY_STREET_LEVEL;
    }
  
  
  /* Now search for the proper place to go. Search between search_min
     and search_max in the x, y, z coords..with the rule that the
     "base" level of the search area must contain all rooms, and you
     can build up into blank rooms, but you can't add the rooms to the 
     base level of rooms. */
  
  {
    char buf[STD_LEN];
    sprintf (buf, "%d %d %d %d %d %d %d %d %d\n\r",
	     s_min_x, s_max_x, s_min_y, s_max_y, s_min_z, s_max_z,
	     o_dim_x, o_dim_y, o_dim_z);
    echo (buf);
    //return;
  }
  for (count = 0; count < 2; count++)
    {
      for (x = s_min_x; x <= s_max_x; x++)
	{
	  for (y = s_min_y; y <= s_max_y; y++)
	    {
	      for (z = s_min_z; z <= s_max_z; z++)
		{
		  /* At each possible starting point see if the rooms are
		     available for something of the size we need. */
		  
		  good_rooms = 0;
		  good_rooms_base_level = 0;
		  for (z1 = z; z1 < z + o_dim_z; z1++)
		    {
		      for (x1 = x; x1 < x + o_dim_x; x1++)
			{
			  for (y1 = y; y1 < y + o_dim_y; y1++)
			    { 
			      if (!city_coord_is_ok (x1, y1, z1))
				continue;
			      /* Set it so we haven't found a bad room. 
				 A bad room is not a killer. It just means
				 that we have to find enough good
				 rooms to justify putting this here. */
			      bad_room = FALSE;
			      /* The bottom level NEEDS all of the
				 rooms to be there. */
			      if (!city_grid[x1][y1][z1])
				{
				  if (z1 == z || !IS_ROOM(obj))
				    bad_room = TRUE;
				}
			      /* Now the city grid exists. If it's depth
				 is too high, ignore it. It's already
				 been detailed. */
			      
			      else if (city_grid[x1][y1][z1]->mv >= depth)
				bad_room = TRUE;
			      
			      if (!bad_room)
				good_rooms++;
			      if (z == z1)
				good_rooms_base_level++;

			    }
			}
		    }
		  /* Should be >= 1. */		  
		  rooms_needed = (o_dim_x*o_dim_y*o_dim_z);
		  base_rooms_needed = (o_dim_x*o_dim_y);
		  
		  /* Now allow some small errors to appear where
		     we don't require ALL rooms to be good. Just most
		     of them. */
		  location_is_bad = FALSE;
		  
		  /* If all rooms are available, we can go with a perfect
		     fit area.*/
		  if (good_rooms >= rooms_needed)
		    num_perfect_choices++;
		  else
		    {
		      /* Small areas require all rooms. */
		      if (rooms_needed <= 2 &&
			  good_rooms < rooms_needed)
			location_is_bad = TRUE;
		      
		      if (good_rooms <= rooms_needed*4/5)
			location_is_bad = TRUE;
		      
		      /* You REALLY need the base level rooms to be there. */
		      if (good_rooms_base_level < base_rooms_needed - 1)
			location_is_bad = TRUE;
		      
		    }
		  if (!location_is_bad)
		    {
		      if (count == 0)			
			num_choices++;
		      /* When we're choosing places to put the rooms,
			 if you have perfect choices to pick from, then
			 use them. Otherwise do this if it's any old ok
			 location. */
		      else if ((!use_perfect_choices ||
				(good_rooms >= rooms_needed)) &&
			       --num_chose < 1)
			{
			  found_start_location = TRUE;
			  break;
			}
		    }
		  if (found_start_location)
		    break;
		}
	      if (found_start_location)
		break;
	    }
	  if (found_start_location)
	    break;
	}
      if (count == 0)
	{
	  use_perfect_choices = FALSE;
	  if (num_perfect_choices > 0)
	    {
	      num_chose = nr (1, num_perfect_choices);
	      use_perfect_choices = TRUE;
	    }
	  else if (num_choices > 0)
	    num_chose = nr (1, num_choices);
	  else
	    {
	      found_start_location = FALSE;
	      break;	      
	    }
	}
      if (found_start_location)
	break;
    }
  if (!found_start_location)
    return;
  /* Get the starting coords  */
  sx = x;
  sy = y;
  sz = z;
/* Now fill in the rooms as needed. */
  
  if (IS_ROOM (obj))
    {
      vnum = start_room_vnum;
      for (x = sx; x < sx + o_dim_x; x++)
	{
	  for (y = sy; y < sy + o_dim_y; y++)
	    {
	      for (z = sz; z < sz + o_dim_z; z++)
		{
		  if (!city_coord_is_ok (x, y, z))
		    continue;
		  
		  if (!city_grid[x][y][z])
		    {
		      city_grid[x][y][z] = new_room(vnum);
		      remove_flagval (city_grid[x][y][z], FLAG_ROOM1, ~0);
		      add_flagval (city_grid[x][y][z], FLAG_ROOM1, flagbits (obj->flags, FLAG_ROOM1));
		      vnum++;
		    }
		  if (vnum >= end_room_vnum)
		    break;
		}
	      if (vnum >= end_room_vnum)
		break;
	    }
	  if (vnum >= end_room_vnum)
	    break;
	}
      free_str (city_grid[sx][sy][sz]->short_desc);
      city_grid[sx][sy][sz]->short_desc = nonstr;
      generate_detail_name (obj, city_grid[sx][sy][sz]);
      
      strcpy (word, city_grid[sx][sy][sz]->short_desc);
      /* Now copy the name. */
      
      for (x = sx; x < sx+o_dim_x; x++)
	{
	  for (y = sy; y < sy+o_dim_y; y++)
	    {
	      for (z = sz; z < sz + o_dim_z; z++)
		{
		  if (city_coord_is_ok (x,y,z) &&
		      (room = city_grid[x][y][z]) != NULL &&
		      room->mv < depth)
		    {
		      free_str (room->short_desc);
		      room->short_desc = nonstr;
		      room->short_desc = new_str (word);
		      room->mv = depth;
		    }
		}
	    }
	}

      /* ADD U/D links between rooms. */

      for (z = sz; z < sz + o_dim_z - 1; z++)
	{
	  stair_x = nr (sx, sx+o_dim_x-1);
	  stair_y = nr (sy, sy+o_dim_y-1);
	  
	  if (city_coord_is_ok(stair_x, stair_y, sz) &&
	      city_coord_is_ok(stair_x, stair_y, sz+1) &&
	      city_grid[stair_x][stair_y][sz] &&
	      (below_room = city_grid[stair_x][stair_y][sz]) != NULL &&
	      city_grid[stair_x][stair_y][sz+1] &&
	      (above_room = city_grid[stair_x][stair_y][sz+1]) != NULL)
	    {
	      room_add_exit (below_room, DIR_UP, above_room->vnum);
	      room_add_exit (above_room, DIR_DOWN, below_room->vnum);
	    }
	}
	    
     

    }
  else
    {
      proto = new_thing();
      copy_thing (obj, proto);
      proto->thing_flags = obj->thing_flags;
      proto->vnum = vnum;
      thing_to (proto, area);
      add_thing_to_list (proto);
      if (city_grid[sx][sy][sz])
	add_reset (city_grid[sx][sy][sz], vnum, MID(3,120-LEVEL(proto)*2/3,1),1, 1);
      generate_detail_name (obj, proto);
    }
  
  /* At this point the rooms (or proto) are created, and the names
     have been set. */

  /* Now set up the dims where the subobjects will be reset. */

  rst_obj_dims.type = VAL_DIMENSION;
  rst_obj_dims.val[0] = sx;
  rst_obj_dims.val[1] = sx + o_dim_x;
  rst_obj_dims.val[2] = sy;
  rst_obj_dims.val[3] = sy + o_dim_y;
  rst_obj_dims.val[4] = sz;
  rst_obj_dims.val[5] = sz + o_dim_z;
  
  
  for (rst = obj->resets; rst; rst = rst->next)
    {
      if ((rst_obj = find_thing_num (rst->vnum)) == NULL)
	continue;
      
      for (times = 0; times < MAX(1, rst->times); times++)
	{
	  if (nr(1,100) <= MAX(2, rst->pct))
	    {
	      citygen_add_detail (rst_obj, area, &rst_obj_dims, depth + 1);
	    }
	}
    }
  
  
  /* ADD NEWS links within the rooms here. We do this after the other
     resets since it will tend to make things */
  
  /* For each level....*/
  for (z = sz; z < sz + o_dim_z; z++)
    {
      /* Add n/e exits to all approprate rooms. */
      for (x = sx; x < sx + o_dim_x; x++)
	{
	  for (y = sy; y < sy + o_dim_y ; y++)
	    {
	      if (city_coord_is_ok(x,y,z) && 
		  (room1 = city_grid[x][y][z]) != NULL &&
		  room1->mv == depth)
		{
		  if (city_coord_is_ok (x+1, y, z) &&
		      (room2 = city_grid[x+1][y][z]) != NULL &&
		      room2->mv == depth &&
		      x < sx + o_dim_x -1)
		    {
		      room_add_exit (room1, DIR_EAST, room2->vnum);
		      room_add_exit (room2, DIR_WEST, room1->vnum);
		    }
		  if (city_coord_is_ok (x, y+1, z) &&
		      (room2 = city_grid[x][y+1][z]) != NULL &&
		      room2->mv == depth &&
		      y < sy + o_dim_y -1)
		    {
		      room_add_exit (room1, DIR_NORTH, room2->vnum);
		      room_add_exit (room2, DIR_SOUTH, room1->vnum);
		    }
		}
	    }
	}
    }
  
  /* Now add random links to other rooms on the same floor. */
  
  for (z = sz; z < sz + o_dim_z; z++)
    {
      for (x = sx + 1; x < sx + o_dim_x; x++)
	{
	  for (y = sy + 1; y < sy + o_dim_y; y++)
	    {
	      if (city_coord_is_ok(x,y,z) && 
		  (room1 = city_grid[x][y][z]) != NULL &&
		  room1->mv == depth)
		{
		  for (dir = 0; dir < FLATDIR_MAX; dir++)
		    connect_city_blocks (x, y, z, i);
		}
	    }
	}
    }
  
  /* Move the depths up a level in this room. */

  for (x = sx; x < sx+o_dim_x; x++)
    {
      for (y = sy; y < sy+o_dim_y; y++)
	{
	  for (z = sz; z < sz + o_dim_z; z++)
	    {
	      if (city_coord_is_ok (x,y,z) &&
		  (room = city_grid[x][y][z]) != NULL &&
		  room->mv >= depth)
		{
		  // Not doing this atm... need to keep this
		  // the same for linking rooms. 
		  //		  room->mv = depth-1;
		}
	    }
	}
    }
  
}

void
connect_city_blocks (int x, int y, int z, int dir)
{
  int nx, ny, nz;
  THING *room1, *room2;
  
  if (dir < 0 || dir >= FLATDIR_MAX)
    return;
  
  nx = x;
  ny = y;
  nz = z;
  if (dir == DIR_NORTH)
    ny = y+1;
  if (dir == DIR_SOUTH)
    ny = y-1;
  if (dir == DIR_EAST)
    nx = x+1;
  if (dir == DIR_WEST)
    nx = x-1;

  if (!city_coord_is_ok (x,y,z) ||
      !city_coord_is_ok (nx, ny, nz) ||
      (room1 = city_grid[x][y][z]) == NULL ||
      (room2 = city_grid[nx][ny][nz]) == NULL ||
      (room2->mv < room1->mv))
    return;
  
  room_add_exit (room1, dir, room2->vnum);
  room_add_exit (room2, RDIR(dir), room1->vnum);
  return;
}


  
