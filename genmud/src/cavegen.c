#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "serv.h"
#include "areagen.h"
#include "cavegen.h"


/* This is the cavegen grid...it gets used for bits at first, then
   it's set with the room numbers we will use. */

static int cave_grid[CAVEGEN_MAX*2+1][CAVEGEN_MAX*2+1][CAVEGEN_MAX];

#define USED_CAVE(x,y,z) (IS_SET(cave_grid[(x)][(y)][(z)],CAVEGEN_USED))

void 
do_cavegen (THING *th, char *arg)
{
  cavegen (th, arg);
  return;
}


bool
cavegen (THING *th, char *arg)
{
  /* Dimensions of the cave. */
  int dx = 0, dy = 0, dz = 0;
  int total_rooms = 0;
  int x, y, z;
  int count = 0;
  /* Edges of the cave. */
  int min_x, min_y, min_z, max_x, max_y, max_z;
  char arg1[STD_LEN];
  int total_dist, val;
  int tries = 0, i;
  if (!arg || !*arg)
    {
      stt ("Cavegen <rooms|size|num> <size> or cavegen <dx> [<dy>] [<dz>] or cavegen show or cavegen generate vnum\n\r", th);
      return FALSE;
    }


  arg = f_word (arg, arg1);

  if (!str_cmp (arg1, "show"))
    {
      cavegen_show (th);
      return FALSE;
    }
  
  if (!str_cmp (arg1, "generate"))
    {
      cavegen_generate (th, atoi(arg));
      return FALSE;
    }

  
  if (named_in (arg1, "room size num rooms"))
    {
      total_rooms = atoi (arg);
      if (total_rooms < CAVEGEN_MIN_ROOMS)
	{ 
	  stt ("Cavegen <rooms|size|num> <size >= 100> or cavegen <dx> [<dy>] [<dz>]\n\r", th);
	  return FALSE;
	}
      for (dx = 1; dx*dx*dx < total_rooms/9; dx++);
      dy = dx;
      dz = MAX(1,dx*2/5);
      {
	char buf[STD_LEN];
	sprintf (buf, "%d %d %d\n\r", dx, dy, dz);
	echo (buf);
      }
    }
  else
    {
      dx = atoi (arg1);
      if (dx < 5)
	dx = 5;
      arg = f_word (arg, arg1);
      dy = atoi (arg1);
      if (dy < 1)
	dy = dx;
      else if (dy < 5)
	dy = 5;
      dz = atoi (arg);
      if (dz < 1)
	dz = dx*2/5;
    }
  
  /* Make sure that the dimensions will fit in the space allotted. */
  if (dx >CAVEGEN_MAX*2-2)
    dx = CAVEGEN_MAX*2-2;
  if (dy > CAVEGEN_MAX*2-2)
    dy = CAVEGEN_MAX*2-2;
  if (dz >= CAVEGEN_MAX -2)
    dz = CAVEGEN_MAX-2;
  
  /* Now set the min/max x/y/z. */

  min_x = CAVEGEN_MAX-(dx-1)/2;
  max_x = CAVEGEN_MAX+dx/2;
  min_y = CAVEGEN_MAX-(dy-1)/2;
  max_y = CAVEGEN_MAX+dy/2;
  min_z = CAVEGEN_MAX/2-(dz-1)/2;
  max_z = CAVEGEN_MAX/2+dz/2;
  
  /* Now clear the grid. and seed some rooms. */
 cavegen_clear_bit (~0);
 
  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {	
	      total_dist = 0;
	      val = ABS(x-(max_x+min_x)/2);
	      total_dist += val*val;
	      val = ABS(y-(max_y+min_y)/2);
	      total_dist += val * val; 
	      val = 2*ABS(z-(max_z+min_z)/2);
	      total_dist += val*val;
	      if (total_dist <= (dx*dx + dy*dy + dz*dz)/7)
		SBIT (cave_grid[x][y][z], CAVEGEN_ALLOWED);
	    }
	}
    }
  
  
  /* Now keep checking if the cave is connected or not and adding
     more rooms until it's done. */
  
  for (tries = 0; tries < 100; tries++)
    {
      /* Clear used flags and seed the rooms. The reason that I 
	 keep reseeding if the numbers get big is that I've noticed
	 that it either generates a cave really quickly, or it thrashes
	 for a long time, so if it gets a bad seed, I will just start over
	 with a new seed. */
      cavegen_clear_bit (CAVEGEN_USED);
      cavegen_seed_rooms (dx, dy, dz);      
      count = 0;
      while (!cavegen_is_connected () && ++count < 100)
	{
	  /* Do 10 add rooms before checking connected status. */
	  for (i = 0; i < 10; i++)
	    cavegen_add_rooms(dx, dy, dz, (count >= 50));
	}
      if (cavegen_is_connected())
	break;
    }
  
  if (!cavegen_is_connected ())
    {
      stt ("Failed to generate a cave!\n\r", th);
      return FALSE;
    }
  stt ("Cave generated. Use cavegen generate <number> to make the map.\n\r", th);
  return TRUE;
}


/* This clears some bit from the entire cavegen grid -- not too efficient
   but who cares. */

void
cavegen_clear_bit (int bit)
{
  int x, y, z;
  
  for (x = 0; x < CAVEGEN_MAX*2+1; x++)
    {
      for (y = 0; y < CAVEGEN_MAX*2+1; y++)
	{
	  for (z = 0; z < CAVEGEN_MAX; z++)
	    {
	      RBIT(cave_grid[x][y][z], bit);
	    }
	}
    }
  return;
}

/* This seeds the map with some rooms on the edges of the map so that they
   have to grow toward the middle of the map. */

void
cavegen_seed_rooms (int dx, int dy, int dz)
{  
  int min_x, max_x, min_y, max_y, min_z, max_z, z;
  int i;
  if (dx >CAVEGEN_MAX*2-2)
    dx = CAVEGEN_MAX*2-2;
  if (dy > CAVEGEN_MAX*2-2)
    dy = CAVEGEN_MAX*2-2;
  if (dz >= CAVEGEN_MAX -2)
    dz = CAVEGEN_MAX-2;
  min_x = CAVEGEN_MAX-(dx-1)/2;
  max_x = CAVEGEN_MAX+dx/2;
  min_y = CAVEGEN_MAX-(dy-1)/2;
  max_y = CAVEGEN_MAX+dy/2;
  min_z = CAVEGEN_MAX/2-(dz-1)/2;
  max_z = CAVEGEN_MAX/2+dz/2;
  /* Used for seeding random spots along the edges of the map. */  
  
  /* Sanity check the variables. */

  if (max_x < min_x)
    max_x = min_x;
  if (max_y < min_y)
    max_y = min_y;
  if (max_z < min_z)
    max_z = min_z;
  
  if (min_x < 1 || min_x >= CAVEGEN_MAX*2)
    min_x = CAVEGEN_MAX;
  if (max_x < 1 || max_x >= CAVEGEN_MAX*2)
    max_x = CAVEGEN_MAX;
  if (min_y < 1 || min_y >= CAVEGEN_MAX*2)
    min_y = CAVEGEN_MAX;
  if (max_y < 1 || max_y >= CAVEGEN_MAX*2)
    max_y = CAVEGEN_MAX;
  if (min_z < 1 || min_z >= CAVEGEN_MAX-1)
    min_z = CAVEGEN_MAX/2;
  if (max_z < 1 || max_z >= CAVEGEN_MAX-1)
    max_z = CAVEGEN_MAX/2;
  
  
  
  /* Now go to the edges and seed new rooms. */

  for (z = min_z; z <= max_z; z++)
    {
      int times = 1;
      if (z > min_z && z < max_z && nr (1,3) != 2)
	times++;
      for (i = 0; i < times; i++)
	cavegen_seed_room (0, 0, z);
    }
  /*
  cavegen_seed_room (0, 0, min_z);
  cavegen_seed_room (0, 0, max_z);
  cavegen_seed_room (0, min_y, 0);
  cavegen_seed_room (0, max_y, 0);
  cavegen_seed_room (min_x, 0, 0);
  cavegen_seed_room (max_x, 0, 0);
  
  for (i = 0; i < 4; i++)
    cavegen_seed_room (0, 0, 0);
 
  */
  return;
}

/* This seeds specific cave rooms. It assumes that there are already
   certain rooms set as valid rooms and that we just need to 
   set them as used. */


void
cavegen_seed_room (int start_x, int start_y, int start_z)
{
  int x, y, z;
  int times, i;
  int min_x = 1, max_x = CAVEGEN_MAX*2, min_y = 1, max_y = CAVEGEN_MAX*2, min_z = 1, max_z = CAVEGEN_MAX-1;
  int tries;
  /* Get min/max x, y, z values. */
  
  for (x = CAVEGEN_MAX*2; x >= 0; x--)
    {
      if (cave_grid[x][CAVEGEN_MAX][CAVEGEN_MAX/2])
	{
	  max_x = x;
	  break;
	}
    }
  for (x = 0; x <= CAVEGEN_MAX*2; x++)
    {
      if (cave_grid[x][CAVEGEN_MAX][CAVEGEN_MAX/2])
	{
	  min_x = x;
	  break;
	}
    }
  for (y = CAVEGEN_MAX*2; y >= 0; y--)
    {
      if (cave_grid[CAVEGEN_MAX][y][CAVEGEN_MAX/2])
	{
	  max_y = y;
	  break;
	}
    }
  for (y = 0; y <= CAVEGEN_MAX*2; y++)
    {
      if (cave_grid[CAVEGEN_MAX][y][CAVEGEN_MAX/2])
	{
	  min_y = y;
	  break;
	}
    }
  for (z = CAVEGEN_MAX-1; z >= 0; z--)
    {
      if (cave_grid[CAVEGEN_MAX][CAVEGEN_MAX][z])
	{
	  max_z = z;
	  break;
	}
    }
  for (z = 0; z <= CAVEGEN_MAX-1; z++)
    {
      if (cave_grid[CAVEGEN_MAX][CAVEGEN_MAX][z])
	{
	  min_z = z;
	  break;
	}
    }

  times = 1;
  
  for (i = 0; i < times; i++)
    {  
      for (tries = 0; tries < 20; tries++)
	{
	  if (start_x == 0)
	    x = nr (min_x, max_x);
	  else 
	    x = MID (min_x, start_x, max_x);
	  if (start_y == 0)
	    y = nr (min_y, max_y);
	  else 
	    y = MID (min_y, start_y, max_y);
	  if (start_z == 0)
	    z = nr (min_z, max_z);
	  else 
	    z = MID (min_z, start_z, max_z);
	  
	  
	  if (IS_SET (cave_grid[x][y][z], CAVEGEN_ALLOWED))
	    {
	      cave_grid[x][y][z] |= CAVEGEN_USED;
	      break;
	    }
	}
    }
  return;
}


/* This adds new rooms to the cave by looking at rooms adjacent to
   1 or more current cave rooms (You should do this only after
   seeding some rooms.) and adding the room into the allowed set. */


void
cavegen_add_rooms (int dx, int dy, int dz, bool force_it)
{ 
  int x, y, z;
  int min_x, max_x, min_y, max_y, min_z, max_z;
  /* Number of adjacent rooms. */
  int num_adj_rooms;   /* Number of rooms directly adjacent to this NEWS. */
  int num_above_below_rooms; /* How many rooms are above and below this. */
  int num_corner_rooms; /* How many rooms are diagonal from this in the 
			   NEWS plane. */
  int chance_to_add_room;
  
  if (dx >CAVEGEN_MAX*2-2)
    dx = CAVEGEN_MAX*2-2;
  if (dy > CAVEGEN_MAX*2-2)
    dy = CAVEGEN_MAX*2-2;
  if (dz >= CAVEGEN_MAX -2)
    dz = CAVEGEN_MAX-2;
  min_x = CAVEGEN_MAX-(dx-1)/2;
  max_x = CAVEGEN_MAX+dx/2;
  min_y = CAVEGEN_MAX-(dy-1)/2;
  max_y = CAVEGEN_MAX+dy/2;
  min_z = CAVEGEN_MAX/2-(dz-1)/2;
  max_z = CAVEGEN_MAX/2+dz/2;

  cavegen_clear_bit(CAVEGEN_ADDED_ADJACENT);
  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {
	      if (USED_CAVE (x,y,z) ||
		  !IS_SET (cave_grid[x][y][z], CAVEGEN_ALLOWED) ||
		  IS_SET (cave_grid[x][y][z], CAVEGEN_ADDED_ADJACENT))
		continue;

	      /* Don't usually do rooms on edges. */
	      
	      
	      
	      num_adj_rooms = 0;
	      num_above_below_rooms = 0;
	      num_corner_rooms = 0;
	      if (USED_CAVE (x+1,y,z))
		num_adj_rooms++;
	      if (USED_CAVE (x-1,y,z))
		num_adj_rooms++;
	      if (USED_CAVE (x,y+1,z))
		num_adj_rooms++;
	      if (USED_CAVE (x,y-1,z))
		num_adj_rooms++;
	      if (USED_CAVE (x+1, y+1, z))
		num_corner_rooms++;
	      if (USED_CAVE (x-1, y+1, z))
		num_corner_rooms++;
	      if (USED_CAVE (x-1, y-1, z))
		num_corner_rooms++;
	      if (USED_CAVE (x+1, y-1, z))
		num_corner_rooms++;
	      
	      if (USED_CAVE (x,y,z+1))
		num_above_below_rooms++;
	      if (USED_CAVE (x,y,z-1))
		num_above_below_rooms++;
	      
	      /* Don't connect if there's too many rooms on the
		 same level. */
	      if (num_adj_rooms < 1 || num_adj_rooms > 2)
		continue;
	      
	      /* Don't connect 3 levels at once. */
	      if (num_above_below_rooms >= 2)
		continue;
	      
	      /* Dont' connect if there are lots of adjacent rooms and
		 an above/below room. */
	      if (num_adj_rooms > 1 && num_above_below_rooms > 0 &&
		  (!force_it || nr (1,100) == 2))
		continue;
	      
	      /* Don't allow too many corner rooms near this. */
	      if (num_adj_rooms > 1 && num_corner_rooms > 1 &&
		  nr (1,4) != 2)
		continue;
	      
	      /* Don't allow too many corner rooms near this. */
	      if (num_corner_rooms > 2)
		continue;
	      
	      if (num_corner_rooms > 1 && nr (1, 10) != 2)
		continue;
	      
	      /* Don't connect most of the time if there are adj and
		 above/below rooms. */
	      if (num_adj_rooms > 0 && num_above_below_rooms > 0 &&
		  nr (1,200) != 2)
		continue;
	      
	      /* The chance to add a room drops off like the fourth
		 power of the number of rooms already attached!
		 This should make the map have a lot of tunnels
		 but not many caverns. */
	      
	      if (num_adj_rooms == 1)
		chance_to_add_room = 2;
	      else if (num_adj_rooms == 2)
		chance_to_add_room = 200;
	      
	      if (nr (0,chance_to_add_room) != chance_to_add_room/2)
		continue;
	      
	      cave_grid[x][y][z] |= CAVEGEN_USED;
	      
	      /* Now mark all nearby as added adjacent... */
	      cave_grid[x+1][y][z] |= CAVEGEN_ADDED_ADJACENT;
	      cave_grid[x-1][y][z] |= CAVEGEN_ADDED_ADJACENT;
	      cave_grid[x][y+1][z] |= CAVEGEN_ADDED_ADJACENT;
	      cave_grid[x][y-1][z] |= CAVEGEN_ADDED_ADJACENT;
	      cave_grid[x][y][z+1] |= CAVEGEN_ADDED_ADJACENT;
	      cave_grid[x][y][z-1] |= CAVEGEN_ADDED_ADJACENT;
	      
	    }
	}
    }
  return;
}

/* This checks if the cave is connected or not. */

bool
cavegen_is_connected (void)
{
  int x, y, z;
  bool found = FALSE;
  /* Find a room in the cave. */
  for (x = 1; x < CAVEGEN_MAX*2; x++)
    {
      for (y = 1; y < CAVEGEN_MAX*2; y++)
	{
	  for (z = 1; z < CAVEGEN_MAX-1; z++)
	    {
	      if (USED_CAVE(x,y,z))
		{
		  found = TRUE;
		  break;
		}
	    }
	  if (found)
	    break;
	}
      if (found)
	break;
    }
  
  /* Clear all connected bits. */
  cavegen_clear_bit (CAVEGEN_CONNECTED);
  
  /* Do a dfs to see what this room is connected to. */
  cavegen_check_connected (x, y, z);
  
  /* Loop through all rooms and if any are used and not connected, then
     the cave is not connected. */

  for (x = 1; x < CAVEGEN_MAX*2; x++)
    {
      for (y = 1; y < CAVEGEN_MAX*2; y++)
	{
	  for (z = 1; z < CAVEGEN_MAX-1; z++)
	    {
	      if (USED_CAVE(x,y,z))
		{
		  if (!IS_SET (cave_grid[x][y][z], CAVEGEN_CONNECTED))
		    {
		      return FALSE;
		    }
		}
	    }
	}
    }
  return TRUE;
}


/* This marks a certain room as connected and also as checked so we
   don't go back to it again. */

void
cavegen_check_connected (int x, int y, int z)
{
  if (x < 0 || x >= CAVEGEN_MAX*2 ||
      y < 0 || y >= CAVEGEN_MAX*2 ||
      z < 0 || z >= CAVEGEN_MAX)
    return;
  
  if (!USED_CAVE (x, y, z))
    return;
  
  if (IS_SET (cave_grid[x][y][z], CAVEGEN_CONNECTED))
    return;
  SBIT (cave_grid[x][y][z], CAVEGEN_CONNECTED);
  
  cavegen_check_connected (x+1, y, z);
  cavegen_check_connected (x-1, y, z);
  cavegen_check_connected (x, y+1, z);
  cavegen_check_connected (x, y-1, z);
  cavegen_check_connected (x, y, z+1);
  cavegen_check_connected (x, y, z-1);
  return;
}

/* Now I want to actually generate the rooms to go somewhere. */

bool
cavegen_generate (THING *th, int start_vnum)
{
  int x, y, z;
  int end_vnum, num_rooms = 0, vnum;
  THING *area, *area2, *room;
  char buf[STD_LEN];
  int min_x = CAVEGEN_MAX*2, max_x = 0,  min_y = CAVEGEN_MAX*2, max_y = 0, min_z = CAVEGEN_MAX, max_z = 0;
  /* Used to start doing the DFS to mark rooms so we don't have
     many U/D connections in our map. */
  
  THING *first_room = NULL;
  int first_x, first_y, first_z;
  if ((area = find_area_in (start_vnum)) == NULL)
    {
      stt ("This start vnum isn't in an area!\n\r", th);
      return FALSE;
    }
  
  
  
  for (x = 0; x < CAVEGEN_MAX*2; x++)
    {
      for (y = 0; y < CAVEGEN_MAX*2; y++)
	{
	  for (z = 0; z < CAVEGEN_MAX-1; z++)
	    {
	      if (USED_CAVE(x,y,z))
		{
		  if (x > max_x)
		    max_x = x;
		  if (x < min_x)
		    min_x = x;
		  if (y > max_y)
		    max_y = y;
		  if (y < min_y)
		    min_y = y;
		  if (z > max_z)
		    max_z = z;
		  if (z < min_z)
		    min_z = z;
		  num_rooms++;
		}
	    }
	}
    }
  
  if (num_rooms < 1)
    {
      stt ("You have to make a cave before you generate the rooms!\n\r", th);
      cavegen_clear_bit (~0);
      return FALSE;
    }

  if (!cavegen_is_connected ())
    {
      stt ("This cave isn't connected! Clearing map!\n\r", th);
      cavegen_clear_bit (~0);
      return FALSE;
    }
  
  /* Ok this is off by 1 but I don't want to stretch it. */
  end_vnum = start_vnum+num_rooms;
  area2 = find_area_in (start_vnum+num_rooms);
  
  if (area2 != area)
    {
      sprintf (buf, "Your cave has %d rooms and it's too big to fit in that area!\n\r", num_rooms);
      stt (buf, th);
      return FALSE;
    }
  
  if (end_vnum >= area->vnum + area->mv)
    {
      sprintf (buf, "The cave is too big to fit in the room space you've allotted for your area. Increase the number of rooms to at least %d.\n\r", num_rooms+(start_vnum-area->vnum));
      stt (buf, th);
      return FALSE;
    }
  
  for (vnum = start_vnum; vnum <= start_vnum+num_rooms;vnum++)
    {
      if ((room = find_thing_num (vnum)) != NULL)
	{
	  sprintf (buf, "You need vnum range %d-%d to be open, and object %d exists.\n\r", start_vnum, end_vnum, vnum);
	  stt (buf, th);
	  return FALSE;
	}
    }
  
  /* At this point we know that the target area exists and that it has
     enough space for the rooms and nothing is in the target vnum range,
     so start to add the rooms in. Use a loop. */

  vnum = start_vnum;
  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {
	      if (USED_CAVE(x,y,z))
		cave_grid[x][y][z] = vnum++;
	      else /* Set unused rooms to 0. */
		cave_grid[x][y][z] = 0;
	    }
	}
    }
  
  /* Now all rooms have been marked with increasing vnums. So, 
     generate the actual rooms and link them up. */
  
  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {	      
	      if (cave_grid[x][y][z])
		{
		  if ((room = new_thing()) == NULL)
		    return FALSE;
		  
		  room->vnum = cave_grid[x][y][z];
		  room->thing_flags = ROOM_SETUP_FLAGS;
		  thing_to (room, area);
		  add_thing_to_list (room);
		}
	    }
	}
    }
  
		  
  /* Now add exits. Do this in 2 loops because adding exits doesn't
     work unless the target room exists. ONLY ADD NEWS EXITS NOW. 
     THERE SHOULD BE VERY FEW UD EXITS AND THOSE GET ADDED NEXT. */
  
  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {
	      if (cave_grid[x][y][z] &&
		  (room = find_thing_num (cave_grid[x][y][z])) != NULL)
		{	  
		  if (first_room == NULL)
		    {
		      first_room = room;
		      first_x = x; 
		      first_y = y;
		      first_z = z;
		    }
		  
		  room_add_exit (room, DIR_EAST, cave_grid[x+1][y][z]);
		  room_add_exit (room, DIR_WEST, cave_grid[x-1][y][z]);
		  room_add_exit (room, DIR_NORTH, cave_grid[x][y+1][z]);
		  room_add_exit (room, DIR_SOUTH, cave_grid[x][y-1][z]);
		}
	    }
	}
    }
  
  undo_marked (area);
  
  
  /* Now add up/down connections only as needed! */

  


  /* Now mark the rooms on the area edges. */

  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {
	      if (USED_CAVE (x,y,z) &&
		  (room = find_thing_num (cave_grid[x][y][z])) != NULL &&
		  IS_ROOM (room))
		{
		  if (x == min_x)
		    append_name (room, "west_edge");
		  if (x == max_x)
		    append_name (room, "east_edge");
		  if (y == max_y)
		    append_name (room, "north_edge");
		  if (y == min_y) 
		    append_name (room, "south_edge");
		  if (y == min_z)
		    append_name (room, "down_edge");
		  if (y == max_z)
		    append_name (room, "up_edge");
		}
	    }
	}
    }
  SBIT (area->thing_flags, TH_CHANGED);
  return TRUE;
}
  
/* This shows the cavegen map as layers. */

void
cavegen_show (THING *th)
{
  int min_x = CAVEGEN_MAX*2;
  int max_x = 0;
  int min_y = CAVEGEN_MAX*2;
  int max_y = 0;
  int min_z = CAVEGEN_MAX;
  int max_z = 0;
  int num_rooms = 0;
  int x, y, z;
  char buf[STD_LEN];
  char *t;
  
  if (!th)
    return;

  /* First calculate the min and max levels of the area. */
  
  for (x = 0; x < CAVEGEN_MAX*2+1; x++)
    {
      for (y = 0; y < CAVEGEN_MAX*2+1; y++)
	{
	  for (z = 0; z < CAVEGEN_MAX; z++)
	    {
	       if (USED_CAVE(x,y,z))
		{
		  if (x > max_x)
		    max_x = x;
		  if (x < min_x)
		    min_x = x;
		  if (y > max_y)
		    max_y = y;
		  if (y < min_y)
		    min_y = y;
		  if (z > max_z)
		    max_z = z;
		  if (z < min_z)
		    min_z = z;
		  num_rooms++;
		}
	    }
	}
    }
  
  /* Loop through the levels from highest down to lowest and show each
     level of the map. */

  for (z = max_z; z >= min_z; z--)
    {
      sprintf (buf, "Level %d ------------------\n\n\r", max_z-z+1);
      stt (buf, th);
      for (y = max_y; y >= min_y; y--)
	{
	  t = buf;
	  for (x = min_x; x <= max_x; x++)
	    {
	      
	      if (USED_CAVE(x,y,z))
		{
		  if (USED_CAVE (x,y,z-1))
		    {
		      if (!USED_CAVE (x,y,z+1))
			{
			  sprintf (t, "\x1b[1;31m#\x1b[0;37m");
			  while (*t)
			    t++;
			  t--;
			}
		      else
			{
			  sprintf (t, "\x1b[1;36m#\x1b[0;37m");
			  while (*t)
			    t++;
			  t--;
			}
		    }		  
		  else if (USED_CAVE (x,y,z+1))
		    { 
		      sprintf (t, "\x1b[1;32m#\x1b[0;37m");
		      while (*t)
			t++;
		      t--;
		    }
		  else
		    *t = '#';
		}
	      else
		*t = ' ';
	      t++;
	    }
	  *t++ = '\n';
	  *t++ = '\r';
	  *t = '\0';
	  stt (buf, th);
	}
    }
  
  sprintf (buf, "The total number of rooms is %d\n\r", num_rooms);
  stt (buf, th);
  return;
}

