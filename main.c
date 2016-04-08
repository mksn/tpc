#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

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
int cur_tok;
char *cur_file;
char cur_char;
char cur_text[512];
char command_buffer[512];
int  cur_text_len;
int cur_line;

void print_sym_tab ();
/*
 * Error handling.
 *
 */
__attribute__((noreturn)) void die (const char *fmt, ...)
{
  va_list ap;
  fprintf (stderr, "%s:%d: ", cur_file, cur_line);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr,"\n");
  print_sym_tab ();
  exit (1);
}

/*
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
  struct sym_tab *parent;
  struct sym_tab *next;
  struct sym_tab *child;
};

struct sym_tab *table;
struct sym_tab *cur_tab;
struct sym_var *cur_var;
void expression ();

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
    cur_tab->next->parent = cur_tab;
    cur_tab = cur_tab->next;
  }
  else
  {
    cur_tab = table;
  }

  cur_var = cur_tab->args;
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

int get_char (FILE *stream)
{
  int rc = getc (stream);
  if (rc == '\n') cur_line++;
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

  if (strchr ("();+-*/,=", cur_char))
  {
    char rc = cur_char;
    cur_char = get_char (input);
    return rc;
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

  if (accept(TK_NUM))
  {
    printf ("NUMBER %g\n", atof(cur_text));
  }
  else if (accept (TK_STRING))
  {
    printf ("STRING %s\n", cur_text);
  }
  else if (accept(KW_NIL))
  {
    printf ("NIL!!!!\n");
  }
  else if (accept (TK_IDENTIFIER))
  {
    ident = strdup (cur_text);
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
  else if (accept (KW_NOT))
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
  while (1) 
  {
    if (accept ('*')) {
      factor();
      printf ("MULT\n");
    }
    else if (accept ('/')){
      factor ();
      printf ("DIV\n");
    }
    else if (accept (KW_MOD)) {
      factor ();
      printf ("MOD\n");
    }
    else if (accept (KW_AND)) {
      factor ();
      printf ("AND\n");
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
  }
  else 
    term();
    
  while (1) 
  {
    if (accept ('-')) {
      term();
      printf ("SUB\n");
    }
    else if (accept ('+')){
      term ();
      printf ("ADD\n");
    }
    else if (accept (KW_OR)) {
      term ();
      printf ("OR\n");
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
      printf ("LE\n");
    }
    else if (accept ('>')){
      term ();
      printf ("GE\n");
    }
    else if (accept (TK_NEQ)) {
      term ();
      printf ("NEQ\n");
    }
    else if (accept ('=')) {
      term ();
      printf ("EQ\n");
    }
    else if (accept (TK_LEQ)) {
      term ();
      printf ("LEQ\n");
    }
    else if (accept (TK_GEQ)) {
      term ();
      printf ("GEQ\n");
    }
    else
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
  if (accept (TK_IDENTIFIER))
  {
    char *ident = strdup (command_buffer);

    if (!find_var (cur_tab, ident))
      die ("Undeclared variable: %s\n", ident);
    
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


void block ()
{
  if (accept (KW_VAR))
  {
    var_decls ();
  }
  expect (KW_BEGIN);
  stmt_list ();
  expect (KW_END);
}

void procedure (void)
{
  expect (KW_PROCEDURE);
  expect (TK_IDENTIFIER);

  char *ident = strdup (command_buffer);
  fprintf (stderr, "Adding procedure %s\n", ident);
  add_proc (ident);

  if (accept ('('))
  {
    parameter_list ();
  }
  expect (';');
  block ();
}

void var ()
{
}

void constants ()
{
}

void mainproc ()
{
}

void program ()
{
  expect (KW_PROGRAM);
  expect (TK_IDENTIFIER);  
  printf ("PROG %s\n", command_buffer);
  add_proc (command_buffer);
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
  cur_char = get_char (input);
  next ();
  //procedure ();
  program ();
  expect (TK_EOF);
  print_sym_tab ();
}

int main(int argc, char **argv)
{
  int i;
  for (i=1;i<argc;i++)
  {
    input = fopen (argv[i], "r");
    cur_file = strdup (argv[i]);
    if (input == NULL)
    {
      die ("Can not open file %s", argv[i]);
    }
    compile ();
    free (cur_file);
    fclose (input);
  }
}

