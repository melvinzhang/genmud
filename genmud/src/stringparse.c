#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include "serv.h"



  /* This strips off the first "word" in a string. Had to add code
   for multiple words in one..like c 'magic missile' blah. */

char *
f_word (char *ol, char *nw)
{
  if (!ol)
    {
      *nw = '\0';
      return nonstr;
    }
  
  while (isspace (*ol))
    ol++;
  if (*ol == '\'')
    {
      ol++;
      while (*ol && *ol != '\'')
	{
	  *nw = LC(*ol);
	  nw++;
	  ol++;
	}
      if (*ol == '\'')
	ol++;
    }
  else if (*ol == '\"')
    {
      ol++;
      while (*ol && *ol != '\"')
	{
	  *nw = LC(*ol);
	  nw++;
	  ol++;
	}
      if (*ol == '\"')
	ol++;
    }
  else
    {
      while (!isspace (*ol) && *ol)
	{
	  *nw = LC(*ol);
	  nw++;
	  ol++;
	}
    }
  *nw = '\0';

  while (isspace (*ol))
    ol++;
  return ol;
}  

/* This checks if the small string (a) is the same as the beginning of
   the big string (b). If the big string (b) ends too soon, then it
   returns as not the same, but if the small string ends first then
   the strings are considered different. This makes it so case matters.
   This is an optimization (BOO! HISS!) because believe it or not the
   biggest drain on CPU for this game is name parsing. :P */

bool 
str_case_prefix (const char *a, const char *b)
{
  if (!a || !b)
    return TRUE;

  while (*a && *b)
    {
      if (*a != *b)
	return TRUE;
      a++;
      b++;
    }
  if (*a && !*b)
    return TRUE;
  return FALSE;
}
/* This checks if the small string (a) is the same as the beginning of
   the big string (b). If the big string (b) ends too soon, then it
   returns as not the same, but if the small string ends first then
   the strings are considered different. */

bool 
str_prefix (const char *a, const char *b)
{
  if (!a || !b)
    return TRUE;

  while (*a && *b)
    {
      if (LC (*a) != LC (*b))
	return TRUE;
      a++;
      b++;
    }
  while (*a && *b);
  if (*a && !*b)
    return TRUE;
  return FALSE;
}

/* This checks if the end of the string (a)...which must be longer than
   (b) is the same or not. It goes from the back to the front of (a)
   and checks each character vs those in (b). If (b) is shorter or 
   if any character is different, it returns TRUE, else it returns FALSE. */

bool 
str_suffix (char *a, char *b)
{
  char *as = a, *bs = b;
  if (!a || !b || !*b || !*a)
    return TRUE;
  while (*a)
    a++;
  while (*b) 
    b++;
  do
    {
      a--;
      b--;
      if (LC(*b) != LC(*a))
	return TRUE;
    }
  while (b > bs && a > as);  
  if (a > as && b <= bs)
    return TRUE;
  return FALSE;
}

/* I think this is roughly what strcmp does, but it's just my implementation
   where empty strings are always set to not be the same as anything... and 
   it's not case sensitive. */

bool
str_cmp (const char *a, const char *b)
{
  if (!a || !*a || !b || !*b)
    return TRUE;
  do
    {
      if (LC (*a) != LC (*b))      
	return TRUE;
      a++;
      b++;
    }
  while (*a && *b);
  if (*a || *b)
    return TRUE;
  return FALSE;
}
	

/* This function takes a search_in string (big string) and starts
   to strip off words from it until we either find the 
   same word as we are looking for, or until the end of search_in
   at which point we give up and set search_in back to where it
   was. */


bool 
named_in (char *search_in, char *look_for)
{
  char search_in_word[STD_LEN*2], look_for_word[STD_LEN*2];
  if (!search_in || !look_for || !*search_in || !*look_for)
    return FALSE;
  
  /* Strip off the first word in the list of names to look for
     and look for it. */
  
  look_for = f_word (look_for, look_for_word);
 
  do
    {
      search_in = f_word (search_in, search_in_word);
      
      if (!str_case_prefix (look_for_word, search_in_word))
	return TRUE;
    }
  while (*search_in);
  return FALSE;
}

