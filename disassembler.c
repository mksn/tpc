#include <stdio.h>
#include <stdlib.h>
#include "op-codes.h"

void dump (int *code,
           int n)
{
  int i;
  for (i = 0; i<n;)
  {
    printf ("%05d: ", i);
    int c = code [i++];
    switch (c)
    {
      case OP_MST:
        {
          int level = code [i++];
          printf ("MST %3d\n", level);
        }
        break;
      case OP_CUP:
        {
          int no_args = code [i++];
          int addr = code [i++];
          printf ("CUP %3d %3d\n", no_args, addr);
        }
        break;
      case OP_ENT:
        {
          int foo = code [i++];
          printf ("ENT %3d\n", foo);
        }
        break;
      case OP_RET:
        printf ("RET\n");
        break;
      case OP_LD:
        {
          int level = code [i++];
          int addr = code [i++];
          printf ("LD  %3d %3d\n", level, addr);
        }
        break;
      case OP_ST:
        {
          int level = code [i++];
          int addr = code [i++];
          printf ("ST  %3d %3d\n", level, addr);
        }
        break;
      case OP_ADD:
        printf ("ADD\n");
        break;
      case OP_SUB:
        printf ("SUB\n");
        break;
      case OP_MUL:
        printf ("MUL\n");
        break;
      case OP_DIV:
        printf ("DIV\n");
        break;
      case OP_MOD:
        printf ("MOD\n");
        break;
      case OP_AND:
        printf ("AND\n");
        break;
      case OP_OR:
        printf ("OR\n");
        break;
      case OP_LT:
        printf ("LT\n");
        break;
      case OP_GT:
        printf ("GT\n");
        break;
      case OP_EQ:
        printf ("EQ\n");
        break;
      case OP_NEQ:
        printf ("NEQ\n");
        break;
      case OP_GEQ:
        printf ("GEQ\n");
        break;
      case OP_LEQ:
        printf ("LEQ\n");
        break;
      case OP_NOT:
        printf ("NOT\n");
        break;
      case OP_JMP:
        {
          int addr = code [i++];
          printf ("JMP %3d\n", addr);
        }
        break;
      case OP_JZ:
        {
          int addr = code [i++];
          printf ("JZ  %3d\n", addr);
        }
        break;
      case OP_NUM:
        {
          int no = code [i++];
          printf ("NUM %3d\n", no);
        }
        break;
      case OP_NEG:
        printf ("NEG\n");
        break;
      case OP_RC:
        printf ("RC\n");
        break;
      case OP_HLT:
        printf ("HLT\n");
        break;
      default:
        printf ("FOO\n");
        break;
    }
  }
  printf ("[EOF]\n");
}

void usage ()
{
  printf ("usage: dump file.out\n");
}

int main (int argc,
          char **argv)
{
  int *code;
  if (argc != 2)
    usage ();

  FILE *fp = fopen (argv[1], "r");
  fseek (fp,  0, SEEK_END);
  int n = ftell (fp);
  fseek (fp, 0, SEEK_SET);
  code = malloc (n);
  fread (code, n, 1, fp);
  fclose (fp);
  dump (code, n/sizeof(int));
  return 0;
}
