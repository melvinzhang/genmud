#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<sys/time.h>
#include<sys/types.h>
#include<dirent.h>
#include "serv.h"
#include "society.h"
#include "track.h"
#include "areagen.h"
#include "worldgen.h"
#include "mobgen.h"
#include "questgen.h"

/* Are we allowed to have areas in these places? */
static int worldgen_allowed[WORLDGEN_MAX][WORLDGEN_MAX];

/* What vnums for areas are in these places? */
static int worldgen_areas[WORLDGEN_MAX][WORLDGEN_MAX];

/* What area types those areas are. */
static int worldgen_sectors[WORLDGEN_MAX][WORLDGEN_MAX];

/* What the levels of these worldgen areas are. */

static int worldgen_levels[WORLDGEN_MAX][WORLDGEN_MAX];

/* What the underground areas are. The underground areas get linked
s   up to the upper level areas, and to each other. */
static int worldgen_underworld[WORLDGEN_MAX][WORLDGEN_MAX];

static int worldgen_checked[WORLDGEN_MAX][WORLDGEN_MAX];

static int worldgen_attached[WORLDGEN_MAX][WORLDGEN_MAX];

static bool have_generated_sectors = FALSE;

/* Kind of a hack but these are the current align outpost room
   entrances so they get added into the world when it's set up. 
   These rooms get attached to areas in the southern part of the 
   world map. */


/* This tells how likely an area is to pick an area of a certain
   sector type next to it. */

/* Rou For Fie Swa Des Sno Wat Und */

/* Rows represent "If the current area has sector type of this row,
   what are the chances we want for the area type next door" */
static int sector_adjacency[SECTOR_PATCHTYPE_MAX][SECTOR_PATCHTYPE_MAX] =
  {
    /* Each row (Except the last) should have a total of 16 in it. 
       This is so that when multiple areas compete to see what
       area gets put next to them, they all get the same number of
       votes to start with. */
    {5, 3, 3, 1, 3, 1, 0, 0},  /* Rou */
    {2, 8, 5, 1, 0, 0, 0, 0},  /* For */
    {2, 3, 8, 3, 0, 0, 0, 0},  /* Fie */
    {2, 4, 4, 4, 0, 0, 2, 0},  /* Swa */
    {9, 0, 0, 0, 7, 0, 0, 0},  /* Des */
    {6, 1, 0, 0, 0, 9, 0, 0},  /* Sno */
    {0, 0, 0, 9, 0, 0, 7, 0},  /* Wat */
    {0, 0, 0, 0, 0, 0, 0, 1},  /* Und */
  };
    

/* Simple worldgen command. */

void
do_worldgen (THING *th, char *arg)
{
  if (LEVEL (th) < MAX_LEVEL || !IS_PC (th))
    return;
  
  worldgen (th, arg);
}

/* The worldgen command takes 3 arguments: The central sector type,
   the x size and the y size. The x and y sizes must be between
   5 and WORLDGEN_MAX */

void
worldgen (THING *th, char *arg)
{
  int x, y;
  char arg1[STD_LEN];
  char buf[STD_LEN];
  int x_size, y_size;
  int min_x, max_x, min_y, max_y;
  int times;
  int area_size;
  int total_size;
  int curr_size;
  
  if (th && IS_PC (th) && LEVEL(th) < MAX_LEVEL)
    return;
  
  
  


  if (!str_cmp (arg, "clear yes"))
    {
      stt ("The worldgen areas will be gone after the next reboot.\n\r", th);
      do_purge (th, "all");
      do_society (th, "clearall");
      clear_base_randpop_mobs (th);
      worldgen_clear_quests (th);
      worldgen_clear();
      worldgen_clear_player_items();
      return;
    }
  
  if (!str_cmp (arg, "show"))
    {
      if (!have_generated_sectors)
	{
	  stt ("You must first type worldgen <x_size> <y_size> to generate some sectors.\n\r", th);
	  return;
	}
      /* These get set as we move through the sector setup. */
      
      worldgen_show_sectors(th);
      return;
    }
  
 
  if (!worldgen_check_vnums())
    {
      sprintf (buf, "Vnums from %d to %d must be open and the difference between the WORLDGEN_START_VNUM and WORLDGEN_UNDERWORLD_START_VNUM must be at least WORLDGEN_VNUM_SIZE of %d\n\r",
	       WORLDGEN_START_VNUM, WORLDGEN_END_VNUM, WORLDGEN_VNUM_SIZE);
      stt (buf, th);
      return;
    } 
  
  arg = f_word (arg, arg1);
  if (!str_cmp (arg1, "generate"))
    {
      int start_memory, end_memory, diff_memory;
      int start_time, end_time, diff_time;
      int cpu_per_byte;
      struct timeval worldgen_time;
      
      start_memory = find_total_memory_used();
      start_time = current_time;
      
      if ((area_size = atoi (arg)) < 20)
	area_size = WORLDGEN_AREA_SIZE;
      
      if (!have_generated_sectors)
	{
	  stt ("You must first type worldgen <x_size> <y_size> to generate some sectors.\n\r", th);
	  return;
	}
      SBIT (server_flags, SERVER_WORLDGEN);
      do_society (th, "generate");
      generate_randpop_mobs (th);
      worldgen_generate_areas(area_size);
      worldgen_link_areas();
      worldgen_add_catwalks_between_areas();
      worldgen_society_seed();
      worldgen_link_align_outposts();
      worldgen_generate_quests();
      
      do_purge (th, "all");
      set_up_teachers();
      set_up_map(NULL);
      reset_world();
      RBIT (server_flags, SERVER_WORLDGEN);
      end_memory = find_total_memory_used();
      gettimeofday (&worldgen_time, NULL);
      end_time = worldgen_time.tv_sec;
      diff_memory = (end_memory-start_memory)/1000;
      diff_time = end_time - start_time;
      if (diff_memory == 0)
	diff_memory = 1;
      /* Assuming a 1ghz machine...make this 1000000 into speed/1000 for
	 your machine. */
      cpu_per_byte = 1000000*diff_time/diff_memory;
      sprintf (buf, "Memory went from %d to %d B in %d seconds, giving an average of %d cpu cycles per byte.\n\r",
	       start_memory, end_memory, diff_time, cpu_per_byte);
      stt (buf, th);
      return;
    }
  
  if ((x_size = atoi(arg1)) < 5 ||
       x_size >= WORLDGEN_MAX)
    {
      sprintf (buf, "The worldgen x size must be between 5 and %d\n\r",
	       WORLDGEN_MAX-1);
      stt (buf, th);
      stt ("worldgen <x_size> <y_size> \n\r", th);
      return;
    }
  
  arg = f_word (arg, arg1);
  if ((y_size = atoi(arg1)) < 5 ||
      y_size >= WORLDGEN_MAX)
    {
      sprintf (buf, "The worldgen y size must be between 5 and %d\n\r",
	       WORLDGEN_MAX-1);
      stt (buf, th);
      stt ("worldgen <x_size> <y_size> \n\r", th);
      return;
    }
  
  if ((area_size = atoi (arg)) < 20)
    area_size = WORLDGEN_AREA_SIZE;
  
  
  /* Clear data. */
  
  for (x = 0; x < WORLDGEN_MAX; x++)
    {
      for (y = 0; y < WORLDGEN_MAX; y++)
	{
	  worldgen_allowed[x][y] = 0;
	  worldgen_areas[x][y] = 0;
	  worldgen_sectors[x][y] = 0;
	  worldgen_underworld[x][y] = 0;
	  worldgen_checked[x][y] = 0;
	  worldgen_levels[x][y] = 0;
	}
    }
  
  /* Now find the min/max x/y values where we will be working. */
  
  min_x = (WORLDGEN_MAX/2-(x_size-1)/2);
  max_x = (WORLDGEN_MAX/2+(x_size)/2);
  min_y = (WORLDGEN_MAX/2-(y_size-1)/2);
  max_y = (WORLDGEN_MAX/2+(y_size)/2);
  
 
  
  /* Now set all of the areas that can be built in the world grid. */
  
  /* This is used to round off the world a bit. */
  total_size = (x_size*x_size+y_size*y_size)/4;
  
  /* Should this be changed to a circular/elliptic world?? hmmm... */
  for (x = 0; x < WORLDGEN_MAX; x++)
    {
      for (y = 0; y < WORLDGEN_MAX; y++)
	{ 
	  /* Smooth out some of the "corners" of the world...by not
	     letting the dx + dy get too big. */
	  curr_size = (x-WORLDGEN_MAX/2)*(x-WORLDGEN_MAX/2) +
	    (y-WORLDGEN_MAX/2)*(y-WORLDGEN_MAX/2);
	  
	  if (x >= min_x && x <= max_x &&
	      y >= min_y && y <= max_y 
	      && curr_size < total_size*2/5
	     
	     
	      /* && ((x % 2) == 0 || (y % 2) == 0) */ 
	      ) 
	    
	    {
	      
	      worldgen_allowed[x][y] = 1;
	      /* If the map is large enough, give it some shape. */
	      if (nr (1,(x_size+y_size)/2) == 2)
		{
		  
		  if (max_y - y <= y_size/5)
		    {
		      if (max_y - y <= y_size/10)
			worldgen_sectors[x][y] = ROOM_SNOW;
		      else
			worldgen_sectors[x][y] = ROOM_ROUGH;
		    }
		  else if (y-min_y <= y_size/5)
		    {
		      if (x - min_x <= x_size/5)
			worldgen_sectors[x][y] = ROOM_FIELD;
		      else if (max_x - x <= x_size/3)
			worldgen_sectors[x][y] = ROOM_FOREST;
		      else if (ABS(WORLDGEN_MAX/2-x) <= x_size/8)
			worldgen_sectors[x][y] = ROOM_SWAMP;
		    }
		  else if (ABS(WORLDGEN_MAX/2-y) <= y_size/10)
		    {
		      if (x-min_x <= x_size/4)
			worldgen_sectors[x][y] = ROOM_FIELD;
		      else if (max_x-x <= x_size/4)
			worldgen_sectors[x][y] = ROOM_DESERT;
		      else if (ABS(WORLDGEN_MAX/2-x) <= x_size/8 &&
			       nr (1,4) == 2)
			worldgen_sectors[x][y] = ROOM_WATERY;
		    }
		}
	    }
	  else
	    worldgen_allowed[x][y] = 0;
	}
    }

  
  
  /* Set up the initial positions. Always a field area in the middle... */
  
  worldgen_sectors[WORLDGEN_MAX/2][WORLDGEN_MAX/2] = ROOM_FIELD;
  
  for (times = 1; times < 100; times++)    
    {          
      for (x = 0; x < WORLDGEN_MAX; x++)
	{
	  for (y = 0; y < WORLDGEN_MAX; y++)
	    {
	      worldgen_checked[x][y] = FALSE;
	    }
	}
      for (x = 0; x < WORLDGEN_MAX; x++)
	{
	  for (y = 0; y < WORLDGEN_MAX; y++)
	    {
	      worldgen_add_sector (x, y);
	    }
	}
      if (worldgen_sectors_attached ())
	break; 
    }
  
 
  
  /* Now draw the map again. */
  
  worldgen_show_sectors (th);
  
 
  /* This puts the levels that will be used when making areas into
     the worldgen_levels[][] array. */
  
  worldgen_generate_area_levels();
  have_generated_sectors = TRUE;
  
  return;
}

/* This checks if the worldgen vnums are ok or not. */

bool
worldgen_check_vnums (void)
{
  THING *area;
  int start_vnum = WORLDGEN_START_VNUM;
  int end_vnum = WORLDGEN_END_VNUM;
  
  if (WORLDGEN_UNDERWORLD_START_VNUM-WORLDGEN_START_VNUM < WORLDGEN_VNUM_SIZE)
    return FALSE;
  
  
  for (area = the_world->cont; area; area = area->next_cont)
    {
      /* Don't need full check here since the area can't have
	 more than 32k rooms... */
      if ((area->vnum >= start_vnum && area->vnum < end_vnum) ||
	  (area->vnum + area->max_mv >= start_vnum &&
	   (area->vnum + area->max_mv < end_vnum)))
	return FALSE;
    }
  return TRUE;
}


void
worldgen_clear (void)
{
  THING *area;
  int start_vnum = WORLDGEN_START_VNUM;
  int end_vnum = WORLDGEN_END_VNUM;
  char buf[STD_LEN];
  for (area = the_world->cont; area; area = area->next_cont)
    {
      /* Don't need full check here since the area can't have
	 more than 32k rooms... */
      if ((area->vnum >= start_vnum && area->vnum < end_vnum) ||
	  (area->vnum + area->max_mv >= start_vnum &&
	   (area->vnum + area->max_mv < end_vnum)))
	SBIT (area->thing_flags, TH_NUKE);
    }
  sprintf (buf, "\\rm %s*qz.are", WLD_DIR);
  system (buf);
  return;
}

/* The way the worldgen sectors are calculated is by always starting at
   the central location, then searching out as far as possible until
   we get to area spots that don't have sectors. Then attempt to find a 
   decent sector for those areas. */

void 
worldgen_add_sector (int x, int y)
{
  int sector_weights[SECTOR_PATCHTYPE_MAX];
  int total_weight, i, j, sectype, weight_chose, dir;
  int new_x, new_y;
  int other_x, other_y;
  int curr_exits = 0;
  
  /* Make sure that this is an area that's already been marked
     with a sector type so we can extend the sector outward. */
  if (x < 0 || x >= WORLDGEN_MAX || y < 0 || y >= WORLDGEN_MAX ||
      !worldgen_allowed[x][y] || worldgen_sectors[x][y] == 0 ||
      worldgen_checked[x][y])
    return;
  
  if (x > 0 && worldgen_sectors[x-1][y])
    curr_exits++;
  if (y > 0 && worldgen_sectors[x][y-1])
    curr_exits++;
  if (y < WORLDGEN_MAX -1 && worldgen_sectors[x][y+1])
    curr_exits++;
  if (x < WORLDGEN_MAX -1 && worldgen_sectors[x-1][y])
    curr_exits++;

  if (curr_exits == 1 && nr (1,3) != 2)
    return;
  if (curr_exits == 2 && nr (1,12) != 5)
    return;
  if (curr_exits >= 3)
    return;
  /*
  if (curr_exits == 3 && nr (1,43) != 2)
    return;
  if (curr_exits == 4)
    return; 
  */
  for (dir = 0; dir < 4; dir++)
    {
      new_x = x;
      new_y = y;
      
      if (nr (1,20) < 15)
	continue;
      
      if (dir == DIR_NORTH)
	new_y++;
      else if (dir == DIR_SOUTH)
	new_y--;
      else if (dir == DIR_EAST)
	new_x++;
      else if (dir == DIR_WEST)
	new_x--;
      else
	continue;
      
      if (new_x < 0 || new_y < 0 || 
	  new_x >= WORLDGEN_MAX || new_y >= WORLDGEN_MAX)
	continue;
      
      if (worldgen_sectors[new_x][new_y] || !worldgen_allowed[new_x][new_y])
	continue;
      
      /* Need to choose a sector for this area. */
      
      for (i = 0; i < SECTOR_PATCHTYPE_MAX; i++)
	sector_weights[i] = 0;
      
      /* Start by adding up all weights from what adjacent areas want. */
      
      for (i = 0; i < 4; i++)
	{
	  if (i < 2)
	    {
	      other_x = new_x - 1 + 2*i;
	      other_y = new_y;
	    }
	  else
	    {
	      other_x = new_x;
	      other_y = new_y - 5 + 2*i;
	    }
	  
	  /* Get the new x and y..odd way of doing x+/-1 y +/-1 */
	  
	  if (other_x >= 0 && other_x < WORLDGEN_MAX &&
	      other_y >= 0 && other_y < WORLDGEN_MAX &&
	      worldgen_sectors[other_x][other_y] != 0)
	    {
	      /* See what sector type this is from the list. */
	      for (sectype = 0; sectype < SECTOR_PATCHTYPE_MAX; sectype++)
		{
		  if (worldgen_sectors[other_x][other_y] == 
		      sector_patchtypes[sectype])
		    break;
		}
	      /* If the sector type was found, add those weights to the
		 choices for what sector to put here. */
	      
	      for (j = 0; j < SECTOR_PATCHTYPE_MAX; j++)
		sector_weights[j] += sector_adjacency[sectype][j];
	    }
	}
      
      /* Now add up the weights. */

      total_weight = 0;
      
      for (i = 0; i < SECTOR_PATCHTYPE_MAX; i++)
	total_weight += sector_weights[i];
      
      /* If there are no adjacent sectors, just pick any old thing.
	 This really shouldn't be possible, though... */
      
      if (total_weight < 1)
	{
	  worldgen_sectors[new_x][new_y] = 
	    sector_patchtypes[nr (0, SECTOR_PATCHTYPE_MAX-2)];
	}
      else
	{
	  /* Choose a weight from the choices and then set the
	     sector to be that. */
	  weight_chose = nr (1, total_weight);
	  
	  for (sectype = 0; sectype < SECTOR_PATCHTYPE_MAX; sectype++)
	    {
	      weight_chose -= sector_weights[sectype];
	      if (weight_chose < 0)
		break;
	    }
	  
	  /* If some error, just pick randomly. */
	  while (sectype < 0 || sectype >= SECTOR_PATCHTYPE_MAX ||
		 sector_patchtypes[sectype] == ROOM_UNDERGROUND)
	    sectype = nr (0, SECTOR_PATCHTYPE_MAX-1);
	  
	  worldgen_sectors[new_x][new_y] = sector_patchtypes[sectype];
	  /*worldgen_add_sector (new_x, new_y); */
	}
    }

  
  
  return;
}


/* This creates the actual areas for the world. */


void
worldgen_generate_areas (int area_size)
{
  int x, y, times, i;
  THING *area;
  int curr_top_vnum = WORLDGEN_START_VNUM;
  int curr_underworld_vnum = WORLDGEN_UNDERWORLD_START_VNUM;
  char buf[STD_LEN];
  int curr_size;
  /* This is a rough estimate of how far we are from the outposts. */
  int level_this_area = 0;
  if (area_size < 100 || area_size > 2000)
    area_size = WORLDGEN_AREA_SIZE;
  
  
  
  for (times = 0; times < 6; times++)
    {
      for (y = 0; y < WORLDGEN_MAX; y++)
	{
	  /* Get the level for the areas on this line...the level
	     increases as the lines go north since the outposts are
	     assumed to be on the south. If you make your outposts
	     appear in other parts of the map, you will have to
	     alter this code. */
	  
	  for (x = 0; x < WORLDGEN_MAX; x++)
	    {
	      if (!worldgen_allowed[x][y] ||
		  !worldgen_sectors[x][y])
		continue;
	      area = NULL;
	      level_this_area = worldgen_levels[x][y];
	      if (!worldgen_areas[x][y])
		{
		  for (i = 0; room1_flags[i].flagval != 0; i++)
		    {
		      if (room1_flags[i].flagval == worldgen_sectors[x][y])
			break;
		    }
		  /* Make sure that this area will still fit in the
		     alloted space. */
		  curr_size = nr (area_size/2, area_size *2);
		  curr_size = (curr_size/55+1)*50;
		  if (curr_top_vnum + area_size < 
		      WORLDGEN_UNDERWORLD_START_VNUM)
		    {
		      sprintf (buf, "%d %d %s %d",
			       curr_top_vnum,
			       curr_size,
			       room1_flags[i].name,
			       
			       nr (level_this_area*2/3,
				   level_this_area*5/4));
		      areagen (NULL, buf);
		      if ((area = find_thing_num (curr_top_vnum)) != NULL &&
			  IS_AREA (area))
			worldgen_areas[x][y] = area->vnum;
		      curr_top_vnum += curr_size;
		      if (area && !area->cont)
			{
			  sprintf (buf, "Failed to make area %d\n",
				   area->vnum);
			  echo (buf);
			}
		      if (area && worldgen_levels[x][y] == 10 &&
			  y > 0 &&
			  !worldgen_sectors[x][y-1])
			{
			  free_str (area->type);
			  area->type = new_str ("outpost_gate");
			}
		    }
		}
	      if (!worldgen_underworld[x][y])
		{
		  /* Make sure this area will still fit in the
		     range. */
		  curr_size = nr (area_size/2, area_size*2);
		  curr_size = (curr_size/55+1)*50;
		  if (((curr_underworld_vnum + area_size)-
		       WORLDGEN_UNDERWORLD_START_VNUM) < WORLDGEN_VNUM_SIZE)
		    {
		      sprintf (buf, "%d %d underground %d",
			       curr_underworld_vnum,
			       curr_size,
			       nr (level_this_area,
				   level_this_area*3/2));
		      areagen (NULL, buf);
		      if ((area = find_thing_num (curr_underworld_vnum)) != NULL &&
			  IS_AREA (area))
			worldgen_underworld[x][y] = area->vnum;
		      curr_underworld_vnum += curr_size;
		      if (area && !area->cont)
			{
			  sprintf (buf, "Failed to make area %d\n",
				   area->vnum);
			  echo (buf);
			}
		    }
		}
	    
	    }
	}
    }
  return;
}

/* The method is to mark the areas which have no S exit and call them
 the areas near the outposts and mark them as level 10, then
 do the searching to fill in the map. */

void
worldgen_generate_area_levels (void)
{
  int x, y, i, count, j;
  int times;
  int jumpsize;
  int num_aligns = 0, curr_align;
  int num_choices, num_chose;
  int num_areas_near, sum_area_levels;
  int min_x = WORLDGEN_MAX+1, max_x = -1;
  int min_y = WORLDGEN_MAX+1, max_y = -1;
  bool changed_area_level = FALSE;
  char buf[STD_LEN];
  RACE *align;
  THING *room;
  
  int gate_x[ALIGN_MAX];
  int gate_y[ALIGN_MAX];
  int my_x, my_y;
  
  
  /* Get the number of aligns that have outpost rooms set on them. */
  
  for (i = 1; i < ALIGN_MAX; i++)
    {
      if ((align = align_info[i]) != NULL && 
	  (room = find_thing_num (align->outpost_gate)) != NULL &&
	  IS_ROOM (room))
	num_aligns++;
    }

  /* Now get the number of usable areas. Those are areas that are
     level 0 (unused) and which are not badroom bit areas and which
     aren't too close to previously used outpost areas. */

  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      gate_x[i] = WORLDGEN_MAX*2;
      gate_y[i] = WORLDGEN_MAX*2;
    }
  
 
  
  for (x = 0; x < WORLDGEN_MAX; x++)
    {
      for (y = 0; y < WORLDGEN_MAX; y++)
	{
	  worldgen_levels[x][y] = 0;
	  if (worldgen_allowed[x][y])
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
  
  /* Loop through all aligns and for each one find an area that's
     not a BADROOM area, not level 10 (already an align gate) and
     not too close to another align gate. */

  for (curr_align = 1; curr_align <= num_aligns; curr_align++)
    {
      num_choices = 0;
      num_chose = 0;
      my_x = WORLDGEN_MAX;
      my_y = WORLDGEN_MAX;
       for (count = 0; count < 2; count++)
	 {
	   bool found_area = FALSE;
	   for (x = 0; x < WORLDGEN_MAX && !found_area; x++)
	     {
	       for (y = 0; y < WORLDGEN_MAX && !found_area; y++)
		 {
		   bool areas_too_close = FALSE;
		   if (!worldgen_sectors[x][y] ||
		       !worldgen_allowed[x][y] ||
		       IS_SET (worldgen_sectors[x][y], BADROOM_BITS) ||
		       (y > 0 && worldgen_sectors[x][y-1]) || 
		       worldgen_levels[x][y] != 0)
		     continue;
		   
		   
		   
		   for (j = 1; j < curr_align; j++)
		     {
		       if ((ABS(gate_x[j]-x) + ABS(gate_y[j]-y)) < 5)
			 {
			   areas_too_close = TRUE;
			   break;
			 }
		     }
		   if (areas_too_close)
		     continue;
		   

		   if (count == 0)
		     num_choices++;
		   else if (--num_chose < 1)
		     { 
		       my_x = x;
		       my_y = y;
		       found_area = TRUE;
		       break;
		     }
		 }
	     }
	   
	   if (count == 0)
	     {
	       if (num_choices < 1)
		 {
		   x = WORLDGEN_MAX;
		   y = WORLDGEN_MAX;
		   break;
		 }
	       else
		 num_chose = nr (1,num_choices);
	     }
	 }
       
       /* Now set the location to level 10 and update the curr align array. */
       
       if (my_x >= 0 && my_x < WORLDGEN_MAX &&
	   my_y >= 0 && my_y < WORLDGEN_MAX)
	 {
	   worldgen_levels[my_x][my_y] = 10;
	   gate_x[curr_align] = my_x;
	   gate_y[curr_align] = my_y;
	 }
     }
  
  
  if (max_x <= min_x)
    max_x = min_x + 2;
  if (max_y <= min_y)
    max_y = min_y + 2;;
  
  jumpsize = 250/((max_x-min_x)+(max_y-min_y));

  /* Now loop 100x or until all areas have levels. */
  
  for (times = 0; times < 100; times++)
    {
      changed_area_level = FALSE;
      
      for (x = min_x; x <= max_x; x++)
	{
	  for (y = min_y; y <= max_y; y++)
	    {
	      if (worldgen_levels[x][y] != 0 ||
		  !worldgen_allowed[x][y] ||
		  !worldgen_sectors[x][y])
		continue;
	      
	      num_areas_near = 0;
	      sum_area_levels = 0;
	      
	      /* First check south connection. */
	      if (y > 0 && worldgen_levels[x][y-1])
		{
		  num_areas_near++;
		  sum_area_levels += worldgen_levels[x][y-1];
		}
	      /* Then check north connection. */
	      else if (y < WORLDGEN_MAX - 1 && worldgen_levels[x][y+1])
		{
		  num_areas_near++;
		  sum_area_levels += worldgen_levels[x][y+1];
		}
	      else /* Then try EW connections. */
		{
		  if (x < WORLDGEN_MAX - 1 && worldgen_levels[x+1][y])
		    {
		      num_areas_near++;
		      sum_area_levels += worldgen_levels[x+1][y];
		    }
		  if (x > 0 && worldgen_levels[x-1][y])
		    {
		      num_areas_near++;
		      sum_area_levels += worldgen_levels[x-1][y];
		    }
		}
	      
	      if (num_areas_near < 1)
		continue;
	      /* Now set the value for this sector. */
	      
	      worldgen_levels[x][y] = sum_area_levels/num_areas_near + jumpsize;
	      changed_area_level = TRUE;
	    }
	}
      if (!changed_area_level)
	break;
    }

  for (y = max_y; y >= min_y; y--)
    {
      buf[0] = '\0';
      for (x = min_x; x <= max_x; x++)
	{
	  sprintf (buf + strlen(buf), "%4d", worldgen_levels[x][y]);
	}
      strcat (buf, "\n\r");
      echo (buf);
    }
	  
  
  return;
}

/* This links the pieces of the map together, preferably by using
   roads. Start at bottom left corner and link up and to the 
   right. */

void
worldgen_link_areas (void)
{
  int x, y;
  THING *area;
  
  for (x = 0; x < WORLDGEN_MAX; x++)
    {
      for (y = 0; y < WORLDGEN_MAX; y++)
	{
	  /* For each position, if there's an area here, 
	     link it to the other areas n and e of it. */
	  
	  if (worldgen_areas[x][y] &&
	      (area = find_thing_num (worldgen_areas[x][y])) != NULL &&
	      IS_AREA (area))
	    {
	      area->thing_flags |= TH_CHANGED;
	      worldgen_add_exit (area, DIR_EAST, x, y, FALSE);
	      worldgen_add_exit (area, DIR_NORTH, x, y, FALSE);
	    }
	  
	   if (worldgen_underworld[x][y] &&
	      (area = find_thing_num (worldgen_underworld[x][y])) != NULL &&
	      IS_AREA (area))
	     {
	       area->thing_flags |= TH_CHANGED;
	       worldgen_add_exit (area, DIR_EAST, x, y, TRUE);
	       worldgen_add_exit (area, DIR_NORTH, x, y, TRUE);
	     }

	   worldgen_link_top_to_underworld (x, y);
	}
    }
  return;
}

/* This links an area that's been worldgenned to another area 
   and the DIR_TO must be N or E. The underworld determines if
   we are linking the top world or the underworld. */

void
worldgen_add_exit (THING *area, int dir, int x, int y, bool underworld)
{
  THING *area2;
  THING *room, *room2, *nroom, *nroom2;
  VALUE *exit, *nexit, *nexit2;
  int vnum; /* Vnum of area 2 */
  int count; /* Used to find rooms needed to link. */
  int opp_dir, curr_dir;
  

  int area_sector_flags = ROOM_EASYMOVE;
  int area2_sector_flags = ROOM_EASYMOVE;
  if (!area || !IS_AREA (area)  || (dir != DIR_EAST && dir != DIR_NORTH) ||
      x < 0 || x >= WORLDGEN_MAX || y < 0 || y >= WORLDGEN_MAX ||
      (dir == DIR_EAST && x >= WORLDGEN_MAX -2) ||
      (dir == DIR_NORTH && y >= WORLDGEN_MAX-2))
    return;

  /* Get the acceptable sector flags. This is normally just EASYMOVE rooms
     but if the areas are WATERY areas, then that's ok to or else we 
     will never find the proper room. */

  if (IS_ROOM_SET (area, ROOM_WATERY))
    area_sector_flags |= ROOM_WATERY;
  
  /* Find which area we're linking to within the grid. */
  if (dir == DIR_EAST)
    {
      if (underworld)
	vnum = worldgen_underworld[x+1][y];
      else
	vnum = worldgen_areas[x+1][y];
    }
  else
    {
      if (underworld)
	vnum = worldgen_underworld[x][y+1];
      else
	vnum = worldgen_areas[x][y+1];
    }
  
  if ((area2 = find_thing_num (vnum)) == NULL ||
      !IS_AREA (area2))
    return;
  
  if (IS_ROOM_SET (area2, ROOM_WATERY))
    area2_sector_flags |= ROOM_WATERY;
  /* Now find the rooms..they REALLY should exist anyway, but if
     stuff happens, at least try a bunch of times. */
  count = 0;
  while ((room = find_room_on_area_edge (area, dir, area_sector_flags)) == NULL && ++count < 10);
  count = 0;
  while ((room2 = find_room_on_area_edge (area2, RDIR(dir), area2_sector_flags)) == NULL && ++count < 10);
  
  /* Make sure the rooms are ok. */
  
  if (!room || !room2 || !IS_ROOM (room) || !IS_ROOM (room2) ||
      room->in != area || room2->in != area2)
    return;
  
  /* Now add the exits. */
  if ((exit = FNV (room, dir + 1)) == NULL)
    {
      exit = new_value();
      exit->type = dir+1;
      add_value (room, exit);			  
    }  
  exit->val[0] = room2->vnum;
  
  if ((exit = FNV (room2, RDIR(dir) + 1)) == NULL)
    {
      exit = new_value();
      exit->type = RDIR(dir)+1;
      add_value (room2, exit);			  
    }
  exit->val[0] = room->vnum;		      

  /* Find the opposite dir we will search in to add exits next to the
     starting exit. */
  
  if (dir == DIR_NORTH || dir == DIR_SOUTH)
    opp_dir = DIR_EAST;
  else if (dir == DIR_EAST || dir == DIR_WEST)
    opp_dir = DIR_NORTH;
  else
    return;

  for (count = 0; count < 2; count++)
    {
      /* Get the direction we go in. */
      if (count == 0)
	curr_dir = opp_dir; 
      else
	curr_dir = RDIR(opp_dir);

      /* Scan out in this direction as long as there are rooms on both
	 sides and those rooms aren't already linked in the
	 directions that we want to deal with. */
      nroom = room;
      nroom2 = room2;
      while (1)
	{	
	  /* See if there are two more rooms in this direction
	     and make sure that they don't link in the direction
	     where we want to add new exits. */
	  
	  if ((nexit = FNV (nroom, curr_dir + 1)) == NULL ||
	      (nroom = find_thing_num (nexit->val[0])) == NULL ||
	      !IS_ROOM (nroom) ||
	      (nexit = FNV (nroom, dir + 1)) != NULL)
	    nroom = NULL;
	  
	  if ((nexit2 = FNV (nroom2, curr_dir + 1)) == NULL ||
	      (nroom2 = find_thing_num (nexit2->val[0])) == NULL ||
	      !IS_ROOM (nroom2) ||
	      (nexit2 = FNV (nroom2, RDIR(dir) + 1)) != NULL)
	    nroom2 = NULL;
	  
	  if (!nroom || !nroom2)
	    break;
	  
	  /* These values should not exist already. So if they do, it's
	     a major problem and I won't recheck to make sure
	     that they don't exist. */
	  
	  exit = new_value();
	  exit->type = dir+1;
	  add_value (nroom, exit);			  	  
	  exit->val[0] = nroom2->vnum;
	  
	  exit = new_value();
	  exit->type = RDIR(dir) +1;
	  add_value (nroom2, exit);			  	  
	  exit->val[0] = nroom->vnum;
	  
	}
    }
  return;
}



/* This links the area in the topworld to the area in the underworld
   below it. */

/* It tries to link underground rooms on top to only underground rooms
   on the bottom. If not possible, then it goes for nonwater nonroad
   rooms. */


void
worldgen_link_top_to_underworld (int x, int y)
{
  THING *top_area, *underworld_area;
  THING *top_room, *underworld_room = NULL;
  VALUE *exit;
  int count;

  
  if (x < 0 || x >= WORLDGEN_MAX || y <0 || y >= WORLDGEN_MAX ||
      !worldgen_areas[x][y] || !worldgen_underworld[x][y])
    return;
  
  if ((top_area = find_thing_num (worldgen_areas[x][y])) == NULL ||
      (underworld_area = find_thing_num (worldgen_underworld[x][y])) == NULL)
    return;
  
  if ((top_room = find_random_room 
       (top_area, FALSE, ROOM_UNDERGROUND, BADROOM_BITS)) == NULL)
    {
      for (count = 0; count < 30; count++)
	{
	  if ((top_room = find_random_room 
	       (top_area, FALSE, ROOM_UNDERGROUND, BADROOM_BITS)) != NULL &&
	      !IS_ROOM_SET (top_room, ROOM_EASYMOVE) &&
	      (exit = FNV (top_room, DIR_DOWN+1)) == NULL &&
	      (exit = FNV (top_room, DIR_UP+1)) == NULL)
	    break;
	}
      if (!top_room)
	return;
    }
  
  for (count = 0; count < 100; count++)
    {
      if ((underworld_room = find_random_room 
	   (underworld_area, FALSE, ROOM_UNDERGROUND, BADROOM_BITS)) != NULL &&
	  !IS_ROOM_SET (underworld_room, ROOM_EASYMOVE) &&
	  (exit = FNV (underworld_room, DIR_DOWN+1)) == NULL &&
	  (exit = FNV (underworld_room, DIR_UP+1)) == NULL)
	break;
    }
  if (!underworld_room)
    return;

  /* Now mark the areas for saving and link the rooms. */
  top_area->thing_flags |= TH_CHANGED;
  underworld_area->thing_flags |= TH_CHANGED;
  
  if ((exit = FNV (top_room, DIR_DOWN + 1)) == NULL)
    {
      exit = new_value();
      exit->type = DIR_DOWN + 1;
      add_value (top_room, exit);
    }
  exit->val[0] = underworld_room->vnum;

   if ((exit = FNV (underworld_room, DIR_UP + 1)) == NULL)
    {
      exit = new_value();
      exit->type = DIR_UP + 1;
      add_value (underworld_room, exit);
    }
   exit->val[0] = top_room->vnum;

   /* Maybe make this a secret door. */

   if (nr (1,7) == 3)
     place_secret_door (top_room, DIR_DOWN, generate_secret_door_name());

   return;
}
  
      
  
/* This shows a map of the current sectors to a player. */

void
worldgen_show_sectors (THING *th)
{
  int x, y;

  if (!th)
    return;
  
  for (y = WORLDGEN_MAX-1; y >= 0; y--)
    {
      for (x = 0; x < WORLDGEN_MAX; x++)
	{
	  if (!worldgen_allowed[x][y] ||
	      worldgen_sectors[x][y] == 0)
	    stt (" ", th);
	  else
	    {
	      switch (worldgen_sectors[x][y])
		{
		  case ROOM_ROUGH:
		    stt ("\x1b[0;35m^", th);
		    break;
		  case ROOM_FOREST:
		    stt ("\x1b[0;32m*", th);
		    break;
		  case ROOM_FIELD:
		    stt ("\x1b[1;32m,", th);
		    break;
		  case ROOM_DESERT:
		    stt ("\x1b[1;33m,", th);
		    break;
		  case ROOM_SWAMP:
		    stt ("\x1b[0;32m#", th);
		    break;
		  case ROOM_SNOW:
		    stt ("\x1b[1;37m*", th);
		    break;
		  case ROOM_UNDERGROUND:
		    stt ("\x1b[1;30m=", th);
		    break;
		  case ROOM_WATERY:
		    stt ("\x1b[1;34m#", th);
		    break;
		  default:
		    stt (" ", th);
		    break;
		} 
	    }
	}
      stt ("\x1b[0;37m\n\r", th);
    }
  return;
}


/* This seeds the societies into the world. */

void
worldgen_society_seed (void)
{
  SOCIETY *soc, *curr_soc;
  int count, num_choices, num_chose;
  int society_flags, i, al, area_count = 0;
  THING *area, *room;
  int room_count, total_times = 1, times, base_society_count = 0;
  
  unmark_areas();
  
  /* Find out how many areas there are to move into. */

  for (area = the_world->cont; area; area = area->next_cont)
    {
      if (!IS_AREA_SET (area, AREA_NOSETTLE))
	area_count++;
    }
  
  /* Clear align raws. */

  for (al = 0; al < ALIGN_MAX; al++)
    {
      if (align_info[al])
	{
	  for (i = 0; i < RAW_MAX; i++)
	    align_info[al]->raw_curr[i] = 0;
	}
    }

  /* Find the number of basic society templates. */
  
  for (soc = society_list; soc; soc = soc->next)
    {
      soc->room_start = 0;
      soc->room_end = 0;
      if (!IS_SET (soc->society_flags, SOCIETY_DESTRUCTIBLE) &&
	  (society_flags = flagbits (soc->flags, FLAG_ROOM1)) != 0)
	base_society_count++;

      for (i = 0; i < RAW_MAX; i++)
	soc->raw_curr[i] = 0;
    }
  
  if (base_society_count < 1)
    return;
  
  /* The times we do this is the number of areas over the number of
     base societies /2. This should fill up the world pretty well pretty
     quickly. */

  /* Have decided to start with 1 of each society only. */
  
  /*  total_times = (area_count/base_society_count)*2/3; */
  
  if (total_times < 1)
    total_times = 1;

  for (times = 0; times < total_times; times++)
    {
      for (soc = society_list; soc; soc = soc->next)
	{
	  if (IS_SET (soc->society_flags, SOCIETY_DESTRUCTIBLE))
	    continue;
	  
	  if ((society_flags = flagbits (soc->flags, FLAG_ROOM1)) == 0)
	    continue;
	  
	  
	  
	  num_choices = 0;
	  num_chose = 0;
	  
	  for (count = 0; count < 2; count++)
	    {
	      /* Pick all available areas. */
	      for (area = the_world->cont; area; area = area->next_cont)
		{
		  if (IS_AREA (area) &&
		      IS_ROOM_SET (area, society_flags) &&
		      !IS_MARKED(area) && area->level >= 
		      MAX(20, soc->level/2))
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
		    break;
		  else
		    num_chose = nr (1, num_choices);
		}
	    }
	  
	  if (!area)
	    continue;
	  
	  
	  num_choices = 0;
	  num_chose = 0;
	  
	  for (count = 0; count < 2; count++)
	    {
	      for (room = area->cont; room; room = room->next_cont)
		{
		  if (IS_ROOM (room) &&
		      IS_ROOM_SET (room, society_flags) &&
		      ((room->vnum - area->vnum) < area->mv/10))
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
		break;
	      else
		num_chose = nr (1, num_choices);
	    }
	}
      
      /* If this society hasn't been planted anywhere, then
	 plant it, otherwise copy it, and plant a new copy. */
	  
	  if (soc->room_start == 0)
	    curr_soc = soc;
	  else
	    {
	      curr_soc = new_society();
	      copy_society (soc, curr_soc);
	      curr_soc->room_start = 0;
	      curr_soc->room_end = 0;
	      SBIT (curr_soc->society_flags, SOCIETY_DESTRUCTIBLE);
	      soc_to_list (curr_soc);
	      curr_soc->recent_maxpop = 100;
	    }
	  
	  if (room && IS_ROOM (room) && room->in == area &&
	      ((room->vnum - area->vnum) < area->mv/4))
	    {
	      curr_soc->room_start = room->vnum;
	      
	      for (room_count = 0; room_count < 100; room_count++)
		{
		  if ((room = find_thing_num 
		       (nr (curr_soc->room_start + 50,
			    (area->vnum + area->mv-10)))) != NULL &&
		      IS_ROOM (room))
		    {
		      curr_soc->room_end = room->vnum;;
		      break;
		    }
		}
	      
	      if (!room)
		curr_soc->room_end = curr_soc->room_start +
		  area->vnum + area->mv*4/5;
	    }
	  SBIT (area->thing_flags, TH_MARKED);
	}  
    }
  unmark_areas();
  return;
}

/* This returns an area on the edge of the worldgen map if it exists. */

THING *
find_area_on_edge_of_world (int dir)
{
  THING *area;
  int x, y, i;  
  int count, num_choices = 0, num_chose = 0;
  int  extreme = WORLDGEN_MAX/2;
  int var1, var2;
  /* Only do news extremes. */
  if (dir < 0 || dir >= 4)
    return NULL;
  
  for (x = 0; x < WORLDGEN_MAX; x++)
    {
      for (y = 0; y < WORLDGEN_MAX; y++)
	{
	  if (worldgen_areas[x][y] > 0 &&
	      (area = find_thing_num (worldgen_areas[x][y])) != NULL &&
	      IS_AREA (area) &&
	      !IS_MARKED (area))
	    {
	      if (dir == DIR_NORTH && y > extreme)
		extreme = y;
	      if (dir == DIR_SOUTH && y < extreme)
		extreme = y;
	      if (dir == DIR_EAST && x > extreme)
		extreme = x;
	      if (dir == DIR_WEST && x < extreme)
		extreme = x;	      
	    }
	}
    }
  
  for (count = 0; count < 2; count++)
    {
      for (i = 0; i < WORLDGEN_MAX; i++)
	{ 

	  /* Determine which direction we're looping through. */
	  
	  if (dir == DIR_NORTH || dir == DIR_SOUTH)
	    {
	      var1 = i;
	      var2 = extreme;
	    }
	  else
	    {
	      var1 = extreme;
	      var2 = i;
	    }
	  
	  /* See if there's an unmarked area here or not. */
	  if ((area = find_thing_num (worldgen_areas[var1][var2])) != NULL &&
	      IS_AREA (area) && !IS_MARKED (area))
	    {
	      if (count == 0)
		num_choices++;
	      else if (--num_chose < 1)
		break;
	      area = NULL;
	    }
	}
      if (count == 0)
	{
	  if (num_choices < 1)
	    break;
	  else 
	    num_chose = nr (1, num_choices);
	}
    }
  return area;
}
    
/* This links the alignment outposts to the world. It links this room
   to the opp dir side of one of the map edges on one of the roadways. */

void
worldgen_link_align_outposts (void)
{
  THING *area;
  THING *world_room = NULL, *room;
  RACE *align;
  VALUE *exit, *exit2;
  int al;
  int num_choices, num_chose, count;
  unmark_areas();
  
  /* now mark all badroom areas so we don't try to link to those. */

  for (area = the_world->cont; area; area = area->next_cont)
    {
      if (IS_ROOM_SET (area, BADROOM_BITS))
	SBIT (area->thing_flags, TH_MARKED);
    }

  
  /* Now see if the align exists, and if it does if the required room
     exists, and see which dirs have rooms in opp direction from
     them so we can know what dirs are avail to move "Away" from the
     outpost. Assume that outposts link "north" from their outpost
     room. */
  
  
  for (al = 0; al < ALIGN_MAX; al++)
    {
      if ((align = align_info[al]) == NULL)
	continue;
      
      if ((room = find_thing_num (align->outpost_gate)) == NULL ||
	  !IS_ROOM (room))
	continue;
      
      num_choices = 0;
      num_chose = 0;
      for (count = 0; count < 2; count++)
	{
	  for (area = the_world->cont; area; area = area->next_cont)
	    {
	      if (!IS_AREA (area) || !area->type || !*area->type ||
		  IS_ROOM_SET (area, BADROOM_BITS) ||
		  str_cmp (area->type, "outpost_gate"))
		continue;
	      
	      if (count == 0)
		num_choices++;
	      else if (--num_chose < 1)
		break;
	    }
	  
	  if (count == 0)
	    {
	      if (num_choices < 1)
		break;
	      else
		num_chose = nr (1, num_choices);
	    }
	}

      if (!area)
	continue;
      
      if ((world_room = find_room_on_area_edge 
	   (area, DIR_SOUTH, ROOM_EASYMOVE)) == NULL)
	continue;

      /* Now world room and outpost room exist, so link them. */

      if ((exit = FNV (room, DIR_NORTH+1)) == NULL)
	{
	  exit = new_value();
	  exit->type = DIR_NORTH+1;
	  add_value (room, exit);
	}
      exit->val[0] = world_room->vnum;
      
      if ((exit2 = FNV (world_room, DIR_SOUTH+1)) == NULL)
	{
	  exit2 = new_value();
	  exit2->type = DIR_SOUTH+1;
	  add_value (world_room, exit2);
	}
      exit2->val[0] = room->vnum;
      free_str (area->type);
      area->type = nonstr;
    }
  
  unmark_areas();
  
  return;
}

/* This checks if worldgen sectors are attached or not. */

bool
worldgen_sectors_attached (void)
{
  int x, y;

  for (x = 0; x < WORLDGEN_MAX; x++)
    {
      for (y = 0; y < WORLDGEN_MAX; y++)
	{
	  worldgen_attached[x][y] = 0;
	  
	}
    }
  
  worldgen_check_sector_attachment (WORLDGEN_MAX/2, WORLDGEN_MAX/2);
  
  for (x = 0; x < WORLDGEN_MAX; x++)
    {
      for (y = 0; y < WORLDGEN_MAX; y++)
	{
	  if (worldgen_sectors[x][y] && !worldgen_attached[x][y])
	    return FALSE;
	}
    }
  return TRUE;
}

void
worldgen_check_sector_attachment (int x, int y)
{
  if (x < 0 || y < 0 || x >= WORLDGEN_MAX || y >= WORLDGEN_MAX ||
      worldgen_attached[x][y] || !worldgen_sectors[x][y] ||
      !worldgen_allowed[x][y])
    return;
  
    worldgen_attached[x][y] = TRUE;
    
    worldgen_check_sector_attachment (x + 1, y);
    worldgen_check_sector_attachment (x - 1, y);
    worldgen_check_sector_attachment (x, y + 1);
    worldgen_check_sector_attachment (x, y - 1);
    return;
}

/* This loops through all of the worldgen areas and links the ones
   that are appropriate areas to other ones with catwalks. */

/* You search each area and see if there are areas to the n and
   e of it and if so you add catwalks to those areas linking to
   the s and w parts of them. */

void
worldgen_add_catwalks_between_areas (void)
{
  THING *area1, *area2, *base1, *base2, *branch1, *branch2,
    *mid_room;
  
  VALUE *exit;
  int x, y, count, dir, areanum, dx, dy;
  
  for (x = 0; x < WORLDGEN_MAX; x++)
    {
      for (y = 0; y < WORLDGEN_MAX; y++)
	{
	  if ((area1 = find_thing_num (worldgen_areas[x][y])) == NULL ||
	      !IS_AREA (area1) ||
	      IS_ROOM_SET (area1, ROOM_WATERY | ROOM_UNDERGROUND | ROOM_DESERT))
	    continue;
	  
	  /* Add a catwalk to the east then the north. */

	  for (count = 0; count < 2; count++)
	    {
	      /* Do the east direction, then the north direction.
		 For each dir we need to make sure that the appropriate
		 area exists, and we need to set the direction we're
		 going and how far the dx and dy will be. */
	      if (count == 0)
		{
		  if (x >= WORLDGEN_MAX-1)
		    continue;
		  areanum = worldgen_areas[x+1][y];
		  dir = DIR_EAST;
		  dx = nr (5,9);
		  dy = nr (1,3);
		  if (nr (0,1) == 1)
		    dy = -dy;		  
		}
	      else
		{
		  if (y >= WORLDGEN_MAX-1)
		    continue;
		  areanum = worldgen_areas[y+1][x];
		  dir = DIR_NORTH; 
		  dy = nr (5,9);
		  dx = nr (1,3);
		  if (nr (0,1) == 1)
		    dx = -dx;
		}
	      
	      /* If the area nearby exists and it's of the proper type
		 and you can find trees and branches to link, then
		 link them. */

	      if ((area2 = find_thing_num (areanum)) != NULL &&
		  IS_AREA (area2) &&
		  !IS_ROOM_SET (area2, ROOM_WATERY | ROOM_UNDERGROUND | ROOM_DESERT) &&
		  (base1 = find_tree_area (area1, dir)) != NULL &&
		  (branch1 = find_tree_branch (base1, dir)) != NULL &&
		  (base2 = find_tree_area (area2, RDIR(dir))) != NULL &&
		  (branch2 = find_tree_branch (base2, RDIR(dir))))
		{
		  make_catwalk (branch1, branch2, dx, dy);		
		  /* Ok so now we connected the branches. We want
		     to have two halves of a catwalk so that
		     the catwalk doesn't go over the other area. */
		  
		  if ((exit = FNV (branch2, RDIR(dir) + 1)) != NULL &&
		      (mid_room = find_thing_num (exit->val[0])) != NULL &&
		      mid_room->in == area1)
		    {
		      remove_value (branch2, exit);
		      if ((exit = FNV (mid_room, dir + 1)) != NULL)
			remove_value (mid_room, exit);
		      
		      /* So now the two halves have been unattached.
			 Now, set up the dx and the dy and make another
			 catwalk. */
		      
		      if (count == 0)
			{
			  dx = nr (5,9);
			  dx = -dx;
			  dy = nr (1,3);
			  if (nr (0,1) == 1)
			    dy = -dy;					  
			}
		      else
			{
			  dy = nr (5,9);
			  dy = -dy;
			  dx = nr (1,3);
			  if (nr (0,1) == 1)
			    dx = -dx;
			}
		      make_catwalk (branch2, mid_room, dx, dy);
		    }
		}
	    }
	}
    }
  return;
}
		

/* This clears all of the worldgen items from the players' inventories
   and from their savefiles. */

void
worldgen_clear_player_items(void)
{
  DIR *dir_file; /* Directory pointer. */
  struct dirent *currentry; /* Ptr to current entry. */
  THING *thg, *thgn, *player;
  char name[STD_LEN];
  
  /* Now open the player directory and clean up all playerfiles. */
  
  if ((dir_file = opendir (PLR_DIR)) == NULL)
    {
      log_it ("Couldn't open player directory for worldgen object clear!\n\r");
      return;
    }
  
  while ((currentry = readdir (dir_file)) != NULL)
    {
      strcpy (name, currentry->d_name);
      for (thg = thing_hash[PLAYER_VNUM % HASH_SIZE]; thg; thg = thg->next)
	{
	  if (!str_cmp (NAME(thg), name))
	    break;
	}
      if (thg)
	continue;
      
      if ((player = read_playerfile (name)) != NULL)
	{
	  thing_to (player, find_thing_num (2));
	  worldgen_remove_object (player);
	  if (player->in)
	    {
	      write_playerfile (player);
	      thing_to (player, find_thing_num (player->align+100));
	      free_thing (player);
	    }
	}
    }
  closedir (dir_file); 
  
  
  /* Now clear all pc's that are online. */
  
  for (thg = thing_hash[PLAYER_VNUM % HASH_SIZE]; thg; thg = thgn)
    {
      thgn = thg->next;
      worldgen_remove_object (thg);
      if (thg->in)
	write_playerfile (thg);
    }
  
  return;
}
 

/* This (recursively) removes all worldgen objects from a thing. */

void
worldgen_remove_object (THING *th)
{
  THING *thg, *thgn;
  if (!th || IS_ROOM (th) || IS_AREA (th))
    return;


  /* Need to get rid of society eq somehow. */
  if ((th->vnum >= WORLDGEN_START_VNUM &&
       th->vnum <= WORLDGEN_END_VNUM) ||
      th->vnum >= GENERATOR_NOCREATE_VNUM_MIN)
    {
      free_thing (th);
      return;
    }
  
  for (thg = th->cont; thg; thg = thgn)
    {
      thgn = thg->next_cont;
      worldgen_remove_object (thg);
    }
  return;
}
  
  
