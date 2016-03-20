#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

enum
{
  TK_IDENTIFIER=256,
  TK_PROCEDURE,
  TK_FUNCTION,
  TK_PROGRAM,
  TK_BEGIN,
  TK_END,
  TK_ASSIGN,
  TK_EOF,
  TK_NUM,
  TK_STRING,
  TK_NIL,
  TK_NOT,
  TK_NEQ,
  TK_LEQ,
  TK_GEQ,
  TK_AND,
  TK_OR,
  TK_MOD,
  TK_VAR
};

char  *token_names[] = {
  "Identifier",
  "Procedure",
  "Function",
  "Program",
  "Begin",
  "End",
  ":=",
  "End-of-file",
  "Number",
  "String",
  "Nil",
  "Not",
  "<>",
  "<=",
  ">=",
  "And",
  "Or",
  "Mod",
  "Var"
};

FILE *input;
int cur_tok;
char cur_char;
char cur_text[512];
int  cur_text_len;

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
  struct sym_var *args;
  struct sym_var *var_offset;
  struct sym_tab *next;
};

struct sym_tab *table;
struct sym_tab *cur_tab;
struct sym_var *cur_var;
void expression ();

__attribute__((noreturn)) void die (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr,"\n");
  exit (1);
}

struct sym_var *find_var (struct sym_tab *tptr,
                          char *name)
{
  struct sym_var *tmp = tptr->args;
  for (;tmp != NULL; tmp = tmp->next)
  {
    if (strcmp (tmp->name, name) == 0)
      return tmp;
  }
  return NULL;
}

struct sym_tab *find_proc (char *name)
{
  struct sym_tab *tmp = table;
  for (;tmp != NULL; tmp = tmp->next)
  {
    if (strcmp (tmp->name, name) == 0)
      return tmp;
  }
  return NULL;
}

struct sym_tab *new_proc ()
{
  struct sym_tab *rc = malloc (sizeof (struct sym_tab));
  memset (rc, 0, sizeof (struct sym_tab));
  return rc;
}

void add_proc (char *name)
{
  if (cur_tab != NULL)
  {
    cur_tab->next = new_proc ();
    cur_tab = cur_tab->next;
  }
  else
  {
    cur_tab = table;
  }

  cur_var = cur_tab->args;
  cur_tab->name = name;
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
    cur_tab->args = new_var ();
    cur_var = cur_tab->args;
  }

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
  if (cur_tab->var_offset == NULL) 
  {
    cur_tab->var_offset = cur_var;
  }
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
  struct sym_var *v = t->args;
  for (;v != NULL; v = v->next)
  {
    print_var (v);
  }
  printf ("proc: %s, no_args: %d\n", t->name, t->no_args);
}
void print_sym_tab ()
{
  struct sym_tab *t = table;
  for (;t!=NULL; t=t->next) 
  {
    print_proc (t);
  }
}

char *string_from_token (char *buf, int tkn)
{
  if (tkn > 255) 
    return token_names [tkn-256];

  buf [0] = '\'';
  buf [1] = tkn;
  buf [2] = '\'';
  buf [3] = 0;
  return buf;
}

int lookup_keyword ()
{
  if (strcmp (cur_text, "procedure") == 0) return TK_PROCEDURE;
  if (strcmp (cur_text, "begin") == 0) return TK_BEGIN;
  if (strcmp (cur_text, "end") == 0) return TK_END;
  if (strcmp (cur_text, "nil") == 0) return TK_NIL;
  if (strcmp (cur_text, "and") == 0) return TK_AND;
  if (strcmp (cur_text, "or") == 0) return TK_OR;
  if (strcmp (cur_text, "not") == 0) return TK_NOT;
  if (strcmp (cur_text, "mod") == 0) return TK_MOD;
  if (strcmp (cur_text, "var") == 0) return TK_VAR;
  return TK_IDENTIFIER;
}

int next_tok ()
{
  while (isspace (cur_char))
  {
    cur_char = getc (input);
  }
  
  if (isalpha (cur_char))
  {
    cur_text_len = 0;
    cur_text [cur_text_len++] = cur_char;
    cur_char = getc (input);
    while (isalnum (cur_char))
    {
      cur_text [cur_text_len++] = cur_char;
      cur_char = getc (input);
    }
    cur_text [cur_text_len] = 0;
    return lookup_keyword ();
  }
  if (isdigit (cur_char))
  {
    cur_text_len = 0;
    cur_text [cur_text_len++] = cur_char;
    cur_char = getc (input);
    while (isdigit (cur_char))
    {
      cur_text [cur_text_len++] = cur_char;
      cur_char = getc (input);
    }
    cur_text [cur_text_len] = 0;
    return TK_NUM;
  }
  if (cur_char == EOF)
    return TK_EOF;
  if (strchr ("();+-*/,=", cur_char))
  {
    char rc = cur_char;
    cur_char = getc (input);
    return rc;
  }
  if (cur_char == ':')
  {
    cur_char = getc (input);
    if (cur_char == '=')
    {
      cur_char = getc (input);
      return TK_ASSIGN;
    }
    return ':';
  }
  if (cur_char == '<')
  {
    cur_char = getc (input);
    if (cur_char == '=')
    {
      cur_char = getc (input);
      return TK_LEQ;
    }
    if (cur_char == '>')
    {
      cur_char = getc (input);
      return TK_NEQ;
    }
    return '<';
  }
  if (cur_char == '>')
  {
    cur_char = getc (input);
    if (cur_char == '=')
    {
      cur_char = getc (input);
      return TK_GEQ;
    }
    return '>';
  }
  
  die ("Unexpected character = %c", cur_char);
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
    die ("unexpected token! Expected:%s, got:%s\n",
         string_from_token(buf, tkn),
         string_from_token(buf2, cur_tok));
  next ();
}

int accept (int tkn)
{
  if (tkn == cur_tok)
  {
    next ();
    return 1;
  }
  return 0;
}


void parameter_list ()
{
  expect (')');
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

  if (cur_tok == TK_NUM)
  {
    printf ("NUMBER %g\n", atof(cur_text));
    next ();
  }
  else if (cur_tok == TK_STRING)
  {
    printf ("STRING %s\n", cur_text);
  }
  else if (cur_tok == TK_NIL)
  {
    printf ("NIL!!!!\n");
  }
  else if (cur_tok == TK_IDENTIFIER)
  {
    ident = strdup (cur_text);
    next ();
    if (accept ('('))
    {
      int n = arg_list ();
      printf ("CALL %s %d\n", ident, n);
    }
    else
    {
      printf ("LOAD %s\n", ident);
    }
  }
  else if (accept ('('))
  {
    expression ();
    expect (')');
  }
  else if (accept (TK_NOT))
  {
    factor ();
    printf ("NOT \n");
  }
  else
    die ("Syntax error! in factor");
}


void term ()
{
  factor ();
  while (cur_tok == '*' || cur_tok == '/')
  {
    int op = cur_tok;
    next ();
    factor ();
    switch (op) {
      case '*':
        printf ("MULT\n");
        break;
      case '/':
        printf ("DIV\n");
        break;
      case TK_MOD:
        printf ("MOD\n");
        break;
      case TK_AND:
        printf ("AND\n");
        break;
    }
  }
}


void simple_expression ()
{
  int sign = 0;
  if (cur_tok == '+' || cur_tok == '-')
  {
    sign = cur_tok;
    next ();
  }
  term ();
  if (sign == '-')
    printf ("NEG\n");
  while (cur_tok == '+' || cur_tok == '-')
  {
    int op = cur_tok;
    next ();
    term ();
    switch (op) {
      case '+':
        printf ("ADD\n");
        break;
      case '-':
        printf ("SUB\n");
        break;
      case TK_OR:
        printf ("OR\n");
        break;
    }
  }
}

void expression ()
{
  simple_expression ();
  int op = cur_tok;
  switch (cur_tok)
  {
    case '<':
    case '>':
    case TK_NEQ:
    case TK_LEQ:
    case TK_GEQ:
    case '=':
      simple_expression ();
  }
  switch (op)
  {
    case '<':
      printf ("LE\n");
      break;
    case '>':
      printf ("GE\n");
      break;
    case TK_NEQ:
      printf ("NEQ\n");
      break;
    case TK_LEQ:
      printf ("LEQ");
      break;
    case TK_GEQ:
      printf ("GEQ\n");
      break;
  }
}

void assign_stmt (char *ident)
{
  expression ();
  printf ("STORE %s\n", ident);
}

void procedure_stmt (char *ident)
{
  int n = 0;
  if (accept ('('))
  {
    n = arg_list ();
  }
  printf ("PCALL %s %d\n", ident, n);
}

void stmt ()
{
  if (cur_tok == TK_IDENTIFIER)
  {
    char *ident = strdup (cur_text);
    add_var (ident);

    next ();
    if (accept (TK_ASSIGN))
    {
      assign_stmt (ident);
    }
    else
    {
      procedure_stmt (ident);
    }
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
  char *ident = strdup (cur_text);
  add_var (ident);
  while (accept (','))
  {
    expect (TK_IDENTIFIER);
    ident = strdup (cur_text);
    add_var (ident);
  }
}

void block ()
{
  if (accept (TK_VAR))
  {
    var_decls ();
  }
  expect (TK_BEGIN);
  stmt_list ();
  expect (TK_END);
}

void procedure (void)
{
  expect (TK_PROCEDURE);
  expect (TK_IDENTIFIER);

  char *ident = strdup (cur_text);
  add_proc (ident);

  if (accept ('('))
  {
    parameter_list ();
  }
  expect (';');
  block ();
}


void compile (void)
{
  table = malloc (sizeof (struct sym_tab));
  memset (table, 0, sizeof (struct sym_tab));
  cur_tab = NULL;
  cur_char = getc (input);
  next ();
  procedure ();
  expect (TK_EOF);
  print_sym_tab ();
}

int main(int argc, char **argv)
{
  int i;
  for (i=1;i<argc;i++)
  {
    input = fopen (argv[i], "r");
    if (input == NULL)
    {
      die ("Can not open file %s", argv[i]);
    }
    compile ();
    fclose (input);
  }
}

