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
  TK_GEQ
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
  ">="
};

FILE *input;
int cur_tok;
char cur_char;
char cur_text[512];
int  cur_text_len;

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
    }
  }
}


void simple_expression ()
{
  int sign;
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
    }
  }
}

void expression ()
{
  simple_expression ();
  
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

void block ()
{
  expect (TK_BEGIN);
  stmt_list ();
  expect (TK_END);
}

void procedure (void)
{
  expect (TK_PROCEDURE);
  expect (TK_IDENTIFIER);
  if (accept ('('))
  {
    parameter_list ();
  }
  expect (';');
  block ();
}


void compile (void)
{
  cur_char = getc (input);
  next ();
  procedure ();
  expect (TK_EOF);
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

