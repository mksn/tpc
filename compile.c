#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "op-codes.h"

enum
{
  TK_IDENTIFIER=256,
  TK_NUM,
  TK_STRING,
  TK_ASSIGN,
  TK_NEQ,
  TK_LEQ,
  TK_GEQ,
  TK_DOTS,
  TK_EOF,

  KW_AND, KW_ARRAY, KW_BEGIN, KW_CASE, KW_CONST, KW_DIV, KW_DO,
  KW_DOWNTO, KW_ELSE, KW_END, KW_FILE, KW_FOR, KW_FUNCTION, KW_GOTO,
  KW_IF, KW_IN, KW_LABEL, KW_MOD, KW_NIL, KW_NOT, KW_OF, KW_OR,
  KW_PACKED, KW_PROCEDURE, KW_PROGRAM, KW_RECORD, KW_REPEAT, KW_SET,
  KW_THEN, KW_TO, KW_TYPE, KW_UNTIL, KW_VAR, KW_WHILE, KW_WITH,

  TK_END
};

char *keywords[] = {
  "and", "array", "begin", "case", "const", "div", "do", "downto",
  "else", "end", "file", "for", "function", "goto", "if", "in", "label",
  "mod", "nil", "not", "of", "or", "packed", "procedure", "program",
  "record", "repeat", "set", "then", "to", "type", "until", "var",
  "while", "with",
};

char  *token_names[] = {
  "Identifier",
  "Number",
  "String",
  ":=",
  "<>",
  "<=",
  ">=",
  "..",
  "[EOF]",
};

/*
 * Global variables.
 *
 */
FILE *input;
FILE *output;
int cur_tok;
char *cur_file;
char cur_char;
char cur_text[512];
char command_buffer[512];
int  cur_text_len;
int cur_line;
int cur_col;

void print_sym_tab ();
/*
 * Error handling.
 *
 */
__attribute__((noreturn)) void die (const char *fmt, ...)
{
  va_list ap;
  fprintf (stderr, "%s@%d:%d: ", cur_file, cur_line, cur_col);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr,"\n");
  print_sym_tab ();
  exit (1);
}

/**
 * Utilities
 *
 */
void emit (int code)
{
  fwrite (&code, sizeof (int), 1, output);
}

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
  char *name;
  char *return_type;
  int  no_args;
  int  no_vars;
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
  
struct sym_tab *table;
struct sym_tab *cur_tab;
struct sym_var *cur_var;
void expression ();

struct var_loc find_var (struct sym_tab *tptr,
                          char *name,
                          int level)
{
  struct sym_var *tmp = tptr->vars;
  int i;
  for (i=0;tmp != NULL; tmp = tmp->next, i++)
  {
    if (strcmp (tmp->name, name) == 0)
      return (struct var_loc){level, i, tmp};
  }
  if (tptr->parent) {
    return find_var (tptr->parent, name, level + 1);
  } else {
    die ("Undefined variable %s", name);
  }
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

struct sym_tab *find_proc (struct sym_tab *t,
                           char *name)
{
  struct sym_tab *tmp = t->child;
  for (;tmp != NULL; tmp = tmp->next)
  {
    if (strcmp (tmp->name, name) == 0)
      return tmp;
  }
  if (t->parent != NULL)
    return find_proc (t->parent, name);
  return NULL;
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
  printf ("Name: %s", v->name);
  if (v->type) 
  {
    printf (", type: %s", v->type);
  }
  printf ("\n");
}

void print_proc (struct sym_tab *t)
{
  struct sym_var *v = t->vars;
  printf ("[proc: %s, no_vars: %d, no_args: %d\n", t->name, t->no_vars, t->no_args);
  for (;v != NULL; v = v->next)
  {
    print_var (v);
  }
  if (t->child)
    print_proc (t->child);
  printf ("]\n");
}
void print_sym_tab ()
{
  struct sym_tab *t = table;
  for (;t!=NULL; t=t->next) 
  {
    print_proc (t);
  }
}

int get_char (FILE *stream)
{
  int rc = getc (stream);
  if (rc == '\n') {
    cur_col = 1;
    cur_line++;
  } else {
    cur_col++;
  }
  return rc;
}

/*
 * Parser functions.
 *
 */
char *string_from_token (char *buf, int tkn)
{
  if (tkn > 255 && tkn < KW_AND) 
    return token_names [tkn-256];
  else if (tkn >= KW_AND)
    return keywords [tkn-KW_AND];
  else {
    buf [0] = '\'';
    buf [1] = tkn;
    buf [2] = '\'';
    buf [3] = 0;
    return buf;
  }
}

#define NUM(x) (sizeof (x)/sizeof (x[0]))

int lookup_keyword ()
{
  unsigned int i=0;
  for (;i < NUM(keywords); i++) {
    if (strcmp (keywords[i], cur_text) == 0)
      return i + KW_AND;
  }
  return TK_IDENTIFIER;
}

int next_tok ()
{
  while (isspace (cur_char))
  {
    cur_char = get_char (input);
  }

  if (isalpha (cur_char))
  { 
    cur_text_len = 0;
    cur_text [cur_text_len++] = cur_char;
    cur_char = get_char (input);

    while (isalnum (cur_char))
    {
      cur_text [cur_text_len++] = cur_char;
      cur_char = get_char (input);
    }

    cur_text [cur_text_len] = 0;
    return lookup_keyword ();
  }

  if (isdigit (cur_char))
  {
    cur_text_len = 0;
    cur_text [cur_text_len++] = cur_char;
    cur_char = get_char (input);
    while (isdigit (cur_char))
    {
      cur_text [cur_text_len++] = cur_char;
      cur_char = get_char (input);
    }
    cur_text [cur_text_len] = 0;
    return TK_NUM;
  }

  if (cur_char == EOF)
    return TK_EOF;

  if (strchr ("+-*/=[],;^()", cur_char))
  {
    char rc = cur_char;
    cur_char = get_char (input);
    return rc;
  }

  if (cur_char == '.')
  {
    cur_char = get_char (input);
    if (cur_char == '.')
    {
      cur_char = get_char (input);
      return TK_DOTS;
    }
    return '.';    
  }
  
  if (cur_char == ':')
  {
    cur_char = get_char (input);
    if (cur_char == '=')
    {
      cur_char = get_char (input);
      return TK_ASSIGN;
    }
    return ':';
  }

  if (cur_char == '<')
  {
    cur_char = get_char (input);
    if (cur_char == '=')
    {
      cur_char = get_char (input);
      return TK_LEQ;
    }
    if (cur_char == '>')
    {
      cur_char = get_char (input);
      return TK_NEQ;
    }
    return '<';
  }

  if (cur_char == '>')
  {
    cur_char = get_char (input);
    if (cur_char == '=')
    {
      cur_char = get_char (input);
      return TK_GEQ;
    }
    return '>';
  }

  die ("next_tok: Unexpected character = %c", cur_char);
}


void next ()
{
  cur_tok = next_tok ();
}

void expect (int tkn)
{
  char buf[4];
  char buf2[4];
  if (cur_tok != tkn)
    die ("expect: unexpected token! Expected:%s, got:%s\n",
         string_from_token(buf, tkn),
         string_from_token(buf2, cur_tok));
  if (cur_tok == TK_IDENTIFIER ||
      cur_tok == TK_STRING ||
      cur_tok == TK_NUM)
    strcpy (command_buffer, cur_text);
  next ();
}

int accept (int tkn)
{
  if (tkn == cur_tok)
  {
    if (cur_tok == TK_IDENTIFIER ||
        cur_tok == TK_STRING ||
        cur_tok == TK_NUM)
      strcpy (command_buffer, cur_text);
    next ();
    return 1;
  }
  return 0;
}

int arg_list ()
{
  int n = 1;
  expression ();
  while (accept (','))
  {
    expression ();
    n += 1;
  }
  expect (')');
  return n;
}

void factor ()
{
  char *ident;

  if (accept(TK_NUM))
  {
    printf ("NUMBER %g\n", atof(command_buffer));
    emit (OP_NUM);
    emit (atoi (command_buffer));
  }
  else if (accept (TK_STRING))
  {
    printf ("STRING %s\n", command_buffer);
  }
  else if (accept(KW_NIL))
  {
    printf ("NIL!!!!\n");
  }
  else if (accept (TK_IDENTIFIER))
  {
    ident = strdup (command_buffer);
    if (accept ('('))
    {
      int n = arg_list ();
      struct sym_tab *f = find_proc (cur_tab, ident);
      if (!f) {
        die ("Undeclared function: %s", ident);
      } else {
        if (f->no_args != n) {
          die ("Wrong number of arguments to function: %s", ident);
        } else {
          printf ("CALL %s %d\n", ident, n);
        }
      }
    }
    else
    {
      struct var_loc v;
      printf ("LOAD %s\n", ident);
      v = find_var (cur_tab, ident, 0);
      emit (OP_LD);
      emit (v.lvl);
      emit (v.idx);
    }
  }
  else if (accept ('('))
  {
    expression ();
    expect (')');
  }
  else if (accept (KW_NOT))
  {
    factor ();
    printf ("NOT \n");
    emit (OP_NOT);
  }
  else
    die ("Syntax error! in factor");
}


void term ()
{
  factor ();
  while (1) 
  {
    if (accept ('*')) {
      factor();
      printf ("MULT\n");
      emit (OP_MUL);
    }
    else if (accept ('/')){
      factor ();
      printf ("DIV\n");
      emit (OP_DIV);
    }
    else if (accept (KW_MOD)) {
      factor ();
      printf ("MOD\n");
      emit (OP_MOD);
    }
    else if (accept (KW_AND)) {
      factor ();
      printf ("AND\n");
      emit (OP_AND);
    }
    else
      break;
  }
}


void simple_expression ()
{
  if (accept ('+')) {
    term ();
  }
  else if (accept ('-')) {
    term ();
    printf ("NEG\n");
    emit (OP_NEG);
  }
  else 
    term();

  while (1) 
  {
    if (accept ('-')) {
      term();
      printf ("SUB\n");
      emit (OP_SUB);
    }
    else if (accept ('+')){
      term ();
      printf ("ADD\n");
      emit (OP_ADD);
    }
    else if (accept (KW_OR)) {
      term ();
      printf ("OR\n");
      emit (OP_OR);
    }
    else
      break;
  }
}

void expression ()
{
  simple_expression ();
  while (1) 
  {
    if (accept ('<')) {
      simple_expression ();
      printf ("LT\n");
      emit (OP_LT);
    }
    else if (accept ('>')){
      term ();
      printf ("GT\n");
      emit (OP_GT);
    }
    else if (accept (TK_NEQ)) {
      term ();
      printf ("NEQ\n");
      emit (OP_NEQ);
    }
    else if (accept ('=')) {
      term ();
      printf ("EQ\n");
      emit (OP_EQ);
    }
    else if (accept (TK_LEQ)) {
      term ();
      printf ("LEQ\n");
      emit (OP_LEQ);
    }
    else if (accept (TK_GEQ)) {
      term ();
      printf ("GEQ\n");
      emit (OP_GEQ);
    }
    else
      break;
  }
}

void assign_stmt (char *ident)
{
  struct var_loc v;
  expression ();
  printf ("STORE %s\n", ident);
  v = find_var (cur_tab, ident, 0);
  emit (OP_ST);
  emit (v.lvl);
  emit (v.idx);
}

void procedure_stmt (char *ident)
{
  int n = 0;
  if (accept ('('))
  {
    n = arg_list ();
  }
  struct sym_tab *p = find_proc (cur_tab, ident);
  if (!p) {
    die ("Undeclared procedure: %s", ident);
  } else {
    if (p->no_args != n) {
      die ("Wrong number of arguments to procedure: %s", ident);
    } else {
      printf ("PCALL %s %d\n", ident, n);
    }
  }
}

void stmt ();
void stmt_list ();

void if_stmt ()
{
  expression ();
  expect(KW_THEN);
  printf ("IF\n");
  stmt ();
  if (accept (KW_ELSE))
  {
    printf ("ELSE\n");
    stmt ();
  }
  printf ("ENDIF\n");
}

void stmt ()
{
  if (accept (TK_IDENTIFIER))
  {
    char *ident = strdup (command_buffer);

    if (accept (TK_ASSIGN))
    {
      assign_stmt (ident);
    }
    else
    {
      procedure_stmt (ident);
    }
  }
  else if (accept (KW_IF))
  {
    if_stmt ();
  }
  else if (accept (KW_BEGIN))
  {
    stmt_list ();
    expect (KW_END);
  }
}

void stmt_list ()
{
  stmt ();
  while (accept (';'))
  {
    stmt ();
  }
}

void var_decls ()
{
  expect (TK_IDENTIFIER);
  char *ident = strdup (command_buffer);
  add_var (ident);
  while (accept (','))
  {
    expect (TK_IDENTIFIER);
    ident = strdup (command_buffer);
    add_var (ident);
  }
  expect (';');
}

void block ();

void arg_decls ()
{
  do 
  {
    accept (KW_VAR);
    expect (TK_IDENTIFIER);
    char *name = strdup (command_buffer);
    add_arg (name);
  } while (accept (','));
  expect (')');
}

void fun_decl ()
{
  expect (TK_IDENTIFIER);
  char *name = strdup (command_buffer);
  add_proc (name);
  expect ('(');
  arg_decls ();
  expect (':');
  expect (TK_IDENTIFIER);
  char *type = strdup (command_buffer);
  cur_tab->return_type = type;
  expect (';');
  add_var (name);
  block ();
  expect (';');
  pop_proc ();
}


void proc_decl ()
{
  expect (TK_IDENTIFIER);
  char *name = strdup (command_buffer);
  add_proc (name);
  expect ('(');
  arg_decls ();
  expect (';');
  block ();
  expect (';');
  pop_proc ();
}

void block ()
{  
  if (accept (KW_VAR))
  {
    var_decls ();
  }
  while (1) {
    if (accept (KW_FUNCTION)) {
      fun_decl ();
    } else if (accept (KW_PROCEDURE)) {
      proc_decl ();
    } else {
      break;
    }
  }
  expect (KW_BEGIN);
  stmt_list ();
  expect (KW_END);
}

void program ()
{
  expect (KW_PROGRAM);
  expect (TK_IDENTIFIER);
  char *ident = strdup (command_buffer);
  printf ("PROG %s\n", command_buffer);
  add_proc (ident);
  expect (';');
  block ();
  expect ('.');
  pop_proc ();
}

void compile (void)
{
  table = malloc (sizeof (struct sym_tab));
  memset (table, 0, sizeof (struct sym_tab));
  cur_tab = NULL;
  cur_line = 1;
  cur_col = 1;
  cur_char = get_char (input);
  next ();
  program ();
  expect (TK_EOF);
  print_sym_tab ();
}

int main(int argc, char **argv)
{
  if (argc != 3)
    die ("Nothing to work with, eejit!");
  input = fopen (argv[1], "r");
  output = fopen (argv[2], "w");
  cur_file = strdup (argv[1]);
  if (input == NULL)
  {
    die ("Can not open file %s", cur_file);
  }
  compile ();
  free (cur_file);
  fclose (input);
}

