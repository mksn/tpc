#include "std-procs.h"
#include <string.h>

struct std_proc std_procs[] = {
  {"WriteLn", NULL,     1, 1},
  {"Abs",     "int",    1, 1},
  {"ReadLn",  NULL,     1, 1},
  {"Sin",     "double", 1, 1},
  {"Cos",     "double", 1, 1},
  {NULL, NULL, 0, 0}
};

  
void
WriteLn (int i)
{
  printf ("%d\n", i);
}

int
ReadLn ()
{
  int rc;
  scanf ("%d", &rc);
  return rc;
}

     
