#include <stdlib.h>
#include <string.h>
#include "sym-tab.h"
#include "std-procs.h"

struct sym_tab *table;
struct sym_tab *cur_tab;
struct sym_var *cur_var;

__attribute__((noreturn)) extern void die (const char *fmt, ...);

void insert_standard_procedures ()
{
  int i;

  for (i=0; std_procs [i].name != NULL; i++)
  {
    add_proc (std_procs [i].name);
    if (std_procs [i].return_type)
      cur_tab->return_type = strdup(std_procs [i].return_type);
    cur_tab->no_args = std_procs [i].no_args;
    cur_tab->no_vars = std_procs [i].no_vars;
    cur_tab->addr = i;
    cur_tab->is_std = 1;
    pop_proc ();
  }
}

void
prepare_symbol_table ()
{
  table = malloc (sizeof (struct sym_tab));
  memset (table, 0, sizeof (struct sym_tab));
  cur_tab = table;
  insert_standard_procedures (table);
}

struct var_loc find_var (struct sym_tab *tptr,
                         char *name,
                         int level)
{
  struct sym_var *tmp = tptr->vars;
  int i;
  for (i=0;tmp != NULL; tmp = tmp->next, i++)
  {
    if (strcmp (tmp->name, name) == 0)
    {
      if (strcmp (name, tptr->name) == 0)
      {
        return (struct var_loc){level, 0, tmp};
      }
      else
      {
        return (struct var_loc){level, i+4, tmp};
      }
    }
  }

  if (!tptr->parent)
  {
    die ("Undefined variable %s", name);
  }
  return find_var (tptr->parent, name, level + 1);
}

struct sym_tab *_find_proc (struct sym_tab *t,
                            char *name)
{
  struct sym_tab *tmp = t;
  for (;tmp != NULL; tmp = tmp->next)
  {
    if (strcmp (tmp->name, name) == 0)
      return tmp;
  }
  if (t->parent)
    return _find_proc (t->parent, name);
  return NULL;
}

struct proc_loc find_proc (struct sym_tab *t,
                           char *name,
                           int level)
{
  struct sym_tab *tmp = t->child;
  
  for (;tmp != NULL; tmp = tmp->next)
    if (strcmp (tmp->name, name) == 0)
      return (struct proc_loc){level, tmp->addr, tmp};
  
  if (t->parent != NULL)
    return find_proc (t->parent, name, level+1);
  else
    die ("Undefined procedure");
  
}

struct sym_tab *new_proc ()
{
  struct sym_tab *rc = malloc (sizeof (struct sym_tab));
  memset (rc, 0, sizeof (struct sym_tab));
  return rc;
}

void add_inner_proc (char *name)
{
  struct sym_tab *scope = new_proc ();
  scope->parent = cur_tab;
  cur_tab->child = scope;
  cur_tab = cur_tab->child;
  cur_var = cur_tab->vars;
  cur_tab->name = name;
}

void add_proc (char *name)
{
  if (cur_tab != NULL)
  {
    struct sym_tab *scope = new_proc ();
    scope->parent = cur_tab;
    scope->next = cur_tab->child;
    cur_tab->child = scope;
    cur_tab = scope;
  }
  else
  {
    cur_tab = table;
  }

  printf ("Adding proc: %s\n", name);
  cur_var = cur_tab->vars;
  cur_tab->name = name;
}

void pop_proc ()
{
  cur_tab = cur_tab->parent;
  cur_var = cur_tab->vars;
}

struct sym_var *new_var ()
{
  struct sym_var *rc = malloc (sizeof (struct sym_var));
  memset (rc, 0, sizeof (struct sym_var));
  return rc;
}


void _add_var (char *name)
{
  if (cur_var != NULL)
  {
    cur_var->next = new_var ();
    cur_var = cur_var->next;
  }
  else
  {
    cur_tab->vars = new_var ();
    cur_var = cur_tab->vars;
  }

  cur_tab->no_vars++;
  cur_var->name = name;
}

void add_arg (char *name) 
{
  cur_tab->no_args++;
  _add_var (name);
}

void add_var (char *name)
{
  _add_var (name);
  fprintf (stderr, "Adding variable %s\n", name);
}

void print_var (struct sym_var *v)
{
  printf (", Name: %s", v->name);
  if (v->type) 
  {
    printf (", type: %s", v->type);
  }
}

void print_proc (struct sym_tab *t)
{
  for (;t!=NULL; t=t->next)
  {
    struct sym_var *v = t->vars;
    printf ("\n[proc: %s, no_vars: %d, no_args: %d", t->name, t->no_vars, t->no_args);
    for (;v != NULL; v = v->next)
    {
      print_var (v);
    }
    if (t->child)
      print_proc (t->child);
    printf ("]");
  }
}

void print_sym_tab ()
{
  print_proc (table);
}
