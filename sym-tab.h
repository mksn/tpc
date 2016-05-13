#ifndef __mksn_tpc_sym_tab_h__
#define __mksn_tpc_sym_tab_h__

/**
 * Symbol table handling.
 *
 */
struct sym_var {
  char *name;
  char *type;
  struct sym_var *next;
};

struct sym_tab {
  char           *name;
  char           *return_type;
  int            addr;
  int            no_args;
  int            no_vars;
  int            is_std;
  struct sym_var *vars;

  struct sym_tab *child;
  struct sym_tab *parent;
  struct sym_tab *next;
};

struct var_loc
{
  int lvl;
  int idx;
  struct sym_var *var;
};

struct proc_loc
{
  int lvl;
  int addr;
  struct sym_tab *proc;
};

extern struct sym_tab *table;
extern struct sym_tab *cur_tab;
extern struct sym_var *cur_var;

extern void prepare_symbol_table ();

extern struct var_loc find_var (struct sym_tab *tptr,
                                char *name,
                                int level);

extern struct proc_loc find_proc (struct sym_tab *t,
                                  char *name,
                                  int level);

extern int st_find_proc (struct sym_tab *it,
                         char *name,
                         int *level,
                         int *addr,
                         struct sym_tab **ot);

extern int st_find_var (struct sym_tab *t,
                        char *name,
                        int *level,
                        int *addr,
                        struct sym_var **v);

extern void add_inner_proc (char *name);
extern void add_proc       (char *name);
extern void pop_proc       ();
extern void add_var        (char *name);
extern void add_arg        (char *name);
extern void print_var      (struct sym_var *v);
extern void print_proc     (struct sym_tab *t);
extern void print_sym_tab  ();

#endif

