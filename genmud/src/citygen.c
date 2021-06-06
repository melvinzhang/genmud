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
#include "citygen.h"



THING *city_grid[CITY_SIZE][CITY_SIZE][CITY_HEIGHT];


/* This is a specialized type of "areagen". */

void
do_citygen (THING *th, char *arg)
{
  if (!th || LEVEL(th) < MAX_LEVEL)
    return;
  citygen (th, arg);
  show_city_map (th);
  return;
}

void 
citygen (THING *th, char *arg)
{
  int start, size, level;
  char arg1[STD_LEN];
  THING *area;
  
  if (!arg || !*arg)
    return;

  arg = f_word (arg, arg1);
  if ((start = atoi (arg1)) < 1)
    {
      stt ("citygen <start> <size> [<level>]\n\r", th);
      return;
    }
  
  arg = f_word (arg, arg1);
  if ((size = atoi (arg1)) < 200 || size > 2000)
    {
      stt ("The size of the city must be from 200-2000 things.\n\r", th);
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

  

  /* Create the area and set it up. */
  
  if ((area = new_thing ()) == NULL)
    return;
  
  area->vnum = start;
  top_vnum = MAX (top_vnum, start);
  add_thing_to_list (area);
  set_up_new_area(area, size);
  area->mv = size*2/3;

  echo ("Moo");

  /* This generates a base city grid that uses up about 3/4 of the
     available rooms. It makes a grid of partially overlapping 
     squares. */
  
  generate_base_city_grid (area, area->vnum+1);
  
  
  
  
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
     to our target size. If it's a single building, just make a 
     single block based on the VAL_DIMENSION data given. */
  
  echo ("moo2\n");
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
  
  echo ("moo3\n");
  if (num_rooms < 1 || num_rooms > 2000)
    return;

  echo ("moo4\n");
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
		  if (curr_vnum >= start_vnum && curr_vnum <= max_vnum)
		    {
		      city_grid[x][y][z] = new_room(curr_vnum);
		      curr_vnum++;
		    }
		}
	    }
	}
      return;
    }
  
  echo ("moo5\n");
     
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

      echo ("moo6\n");
      
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
      
 
