#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "op-codes.h"
#include "sym-tab.h"
#include "std-procs.h"

#define UP 1
#define DOWN -1

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
  print_sym_tab (table);
  exit (1);
}


/**
 * Utilities
 *
 */
void prepare_parser_tracking ()
{
  cur_line = 1;
  cur_col = 1;
}

void emit (int code)
{
  fwrite (&code, sizeof (int), 1, output);
}

int here ()
{
  return (int)(ftell (output)/sizeof(int));;
}

void patch (int addr, int nv)
{
  fseek (output, addr*sizeof(int), SEEK_SET);
  emit (nv);
  fseek (output, 0, SEEK_END);
}

void expression ();

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
      emit (OP_MST);
      struct proc_loc f = find_proc (cur_tab, ident, 0);
      emit (f.lvl);
      int n = arg_list ();
      
      if (f.proc->no_args != n) {
        die ("Wrong number of arguments to function: %s", ident);
      } else {
        emit (f.proc->is_std ? OP_CSP : OP_CUP);
        emit (f.proc->no_args);
        emit (f.addr);
      }
    }
    else
    {
      struct var_loc v;
      // TODO: It might be a procedure call
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
      emit (OP_MUL);
    }
    else if (accept ('/')){
      factor ();
      emit (OP_DIV);
    }
    else if (accept (KW_MOD)) {
      factor ();
      emit (OP_MOD);
    }
    else if (accept (KW_AND)) {
      factor ();
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
    emit (OP_NEG);
  }
  else 
    term();

  while (1) 
  {
    if (accept ('-')) {
      term();
      emit (OP_SUB);
    }
    else if (accept ('+')){
      term ();
      emit (OP_ADD);
    }
    else if (accept (KW_OR)) {
      term ();
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
      emit (OP_LT);
    }
    else if (accept ('>')){
      term ();
      emit (OP_GT);
    }
    else if (accept (TK_NEQ)) {
      term ();
      emit (OP_NEQ);
    }
    else if (accept ('=')) {
      term ();
      emit (OP_EQ);
    }
    else if (accept (TK_LEQ)) {
      term ();
      emit (OP_LEQ);
    }
    else if (accept (TK_GEQ)) {
      term ();
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
  v = find_var (cur_tab, ident, 0);
  emit (OP_ST);
  emit (v.lvl);
  emit (v.idx);
}

void procedure_stmt (char *ident)
{
  int n = 0;
  emit (OP_MST);

  struct proc_loc p = find_proc (cur_tab, ident, 0);
  emit (p.lvl);

  if (accept ('('))
  {
    n = arg_list ();
  }

  if (p.proc->no_args != n)
  {
    die ("Wrong number of arguments to procedure: %s", ident);
  }
  else
  {
    emit (p.proc->is_std ? OP_CSP : OP_CUP);
    emit (p.proc->no_args);
    emit (p.addr);
  }
}

void stmt ();
void stmt_list ();

void if_stmt ()
{
  int out;
  expression ();
  expect(KW_THEN);
  emit (OP_JZ);
  int jmp = here ();
  emit (0);
  stmt ();
  if (accept (KW_ELSE))
  {
    emit (OP_JMP);
    out = here ();
    emit (0);
    patch (jmp, here ());
    stmt ();
    patch (out, here ());
  }
  else
  {
    patch (jmp, here ());
  }
}

void for_stmt ()
{
  int dir;
  expect (TK_IDENTIFIER);
  char *loop_var = strdup (command_buffer);
  add_var (loop_var);
  expect (TK_ASSIGN);
  expression ();
  
  if (accept (KW_TO))
  {
    dir = UP;
  }
  else if (accept (KW_DOWNTO))
  {
    dir = DOWN;
  }
  else
  {
    die ("Expected To or DownTo!");
  }
  
  expression ();
  expect (KW_DO);
  stmt ();
  if (dir) {;}
}

void repeat_stmt ()
{
  int loop = here ();
  stmt_list ();
  expect (KW_UNTIL);
  expression ();
  emit (OP_JZ);
  emit (loop);
}
    
void while_stmt ()
{
  int loop = here ();
  expression ();
  expect (KW_DO);
  emit (OP_JZ);
  int exit = here ();
  emit (0);
  stmt ();
  emit (OP_JMP);
  emit (loop);
  patch (exit, here());
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
  else if (accept (KW_FOR))
  {
    for_stmt ();
  }
  else if (accept (KW_WHILE))
  {
    while_stmt ();
  }
  else if (accept (KW_REPEAT))
  {
    repeat_stmt ();
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

int block ();

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
  cur_tab->addr = block ();
  expect (';');
  emit (OP_RC);
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
  cur_tab->addr = block ();
  expect (';');
  emit (OP_RET);
  pop_proc ();
}

int block ()
{  
  if (accept (KW_VAR))
  { 
   var_decls ();
  }
  while (1)
  {
    if (accept (KW_FUNCTION))
    {
      fun_decl ();
    }
    else if (accept (KW_PROCEDURE))
    {
      proc_decl ();
    }
    else
    {
      break;
    }
  }
  expect (KW_BEGIN);
  int start = here ();
  cur_tab->addr = start;
  emit (OP_ENT);
  emit (cur_tab->no_args+4);
  stmt_list ();
  expect (KW_END);
  printf ("block: %d\n", start);
  return start;
}

void program ()
{
  expect (KW_PROGRAM);
  expect (TK_IDENTIFIER);
  char *ident = strdup (command_buffer);
  add_proc (ident);
  expect (';');
  emit (OP_MST);
  emit (0);
  emit (OP_CUP);
  emit (0);
  int alku = here ();
  emit (0);
  emit (OP_HLT);
  int start = block ();
  patch (alku, start);
  emit (OP_RET);
  expect ('.');
  pop_proc ();
}

void compile (void)
{
  prepare_symbol_table ();
  prepare_parser_tracking ();
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

