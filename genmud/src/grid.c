#include<stdio.h>
#include<sys/select.h>
#include "serv.h"
#include "wilderness.h"

/* This little program converts a .grd file generated by the terraform 
   program into the wilderness formula used by the game.

   Create the program by typing gcc -o gridmaker grid.c fileio.c
  
   There must be a .grd file called wld.grd in your ../wld directory
   that was made by the terraform program. 

   The size of the map made must be WILDERNESS_SIZE X WILDERNESS_SIZE.

   Also, you can't make a smaller (say 400x400) map then scale it up
   using terraform since I tried that and it scaled a 500x500 map into
   1997x1997. :P And that won't work. So you have to really make the
   terraform map the correct size to start with.

*/

int main (int argc, char **argv)
{
  FILE *ol, *nw;
  
  int i, x, y;
  char c;
  int number, oldnumber;
  int badcount;
  if ((ol = wfopen ("wld.grd", "r")) == NULL)
    {
      printf ("Couldn't open wld.grd.\n");
      exit (0);
    }
  
  if ((nw = wfopen ("wilderness.dat", "w")) == NULL)
    {
      printf ("Couldn't open wilderness.dat for writing.\n");
      fclose (ol);
      exit (0);
    }

  // Now eat up the 6 numbers at the start of the file.
  
  for (i = 0; i < 6; i++)
    read_number (ol);
  
  // Now loop through the entire file and read numbers in one by one 
  // and convert them to our acceptable values. This is heavily dependent
  // on the PIXMAP_ constants in serv.h

  for (y = 0; y < WILDERNESS_SIZE; y++)
    {
      for (x = 0; x < WILDERNESS_SIZE; x++)
	{
	  // Read up to a period.
	  badcount = 0;
	  while ((c = getc(ol)) != '.' && ++badcount < 100);
	  
	  if (badcount >= 100)
	    {
	      fclose (ol);
	      fclose (nw);
	      printf ("Bad format on .grd file. Bailing out.\n");
	      exit (1);
	    }
	  
	  number = read_number (ol);
	  number /= 100000;
	  
	  if (number < 2)
	    number = 2;
	  if (number > 8)
	    number = 8;
	  
	 
	  
	  fputc ((number+'a'), nw);
	}
    }
  fclose (ol);
  fclose (nw);
  exit (0);
}
	  
  
  
  
