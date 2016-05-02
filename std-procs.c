#include "std-procs.h"

struct std_proc std_procs[] = {
  {"WriteLn", NULL,     1, 1},
  {"Abs",     "int",    1, 1},
  {"ReadLn",  NULL,     1, 1},
  {"Sin",     "double", 1, 1},
  {"Cos",     "double", 1, 1}
};

void insert_standard_procedures (struct sym_tab *t)
{
  int i;
  int nelems = sizeof (std_procs)/sizeof(std_procs[0]);
  for (i=0; i<nelems; i++)
  {
    add_proc (std_procs [i].name);
    if (std_procs [i].return_type)
      cur_tab->return_type = strdup(std_procs [i].return_type);
    cur_tab->no_args = std_procs [i].no_args;
    cur_tab->no_vars = std_procs [i].no_vars;
  }
}
      
