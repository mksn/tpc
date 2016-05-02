#ifndef __mksn_tpc_std_procs_h__
#define __mksn_tpc_std_procs_h__

#include <stdio.h>
#include "sym-tab.h"

struct std_proc
{
  char *name;
  char *return_type;
  int  no_args;
  int  no_vars;
};

extern struct std_proc std_procs [];

void write_line (const char *line);
void insert_standard_procedures (struct sym_tab *t);

#endif
