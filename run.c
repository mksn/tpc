#include <stdio.h>
#include <stdlib.h>
#include "op-codes.h"

int mem[65536];

void run (int *code)
{
  int pc = 0;
  int sp = 0;
  int mp = 0;

  while (1)
  {
    printf ("%3d: ", pc);
    switch (code [pc++]) {
      case OP_MST:
      {
        int level = code [pc++];
        mem [sp + 0] = 0; // return value
        mem [sp + 1] = mem [mp + 1]; // static link
        mem [sp + 2] = mp; // dynamic link
        mem [sp + 3] = 0; // return addr;
        sp += 4;
        printf ("MST %3d\n", level);
      }
        break;
      case OP_CUP:
      {
        int no_args = code [pc++];
        int addr = code [pc++];
        mem [sp-no_args-1] = pc;
        mp = sp - no_args - 4;
        pc = addr;
        printf ("CUP %3d %3d\n", no_args, addr);
      }
        break;
      case OP_ENT:
      {
        int no_vars = code [pc++];
        sp = mp + no_vars;
        printf ("ENT %3d\n", no_vars);
      }
        break;
      case OP_RET:
        sp = mp;
        pc = mem [mp + 3];
        mp = mem [mp + 2];
        printf ("RET\n");
        break;
      case OP_RC:
        sp = mp + 1;
        pc = mem [mp + 3];
        mp = mem [mp + 2];
        printf ("RC\n");
        break;
      case OP_LD:
      {
        int level = code [pc++];
        int addr = code [pc++];
        mem [sp++] = mem [mp + addr];
        printf ("LD  %3d %3d\n", level, addr);
      }
        break;
      case OP_ST:
      {
        int level = code [pc++];
        int addr = code [pc++];
        mem [mp + addr] = mem [--sp];
        printf ("ST  %3d %3d\n", level, addr);
      }
        break;
      case OP_ADD:
        mem [sp - 2] = mem [sp - 2] + mem [sp - 1];
        sp -= 1;
        printf ("ADD\n");
        break;
      case OP_SUB:
        mem [sp - 2] = mem [sp - 2] - mem [sp - 1];
        sp -= 1;
        printf ("SUB\n");
        break;
      case OP_MUL:
        mem [sp - 2] = mem [sp - 2] * mem [sp - 1];
        sp -= 1;
        printf ("MUL\n");
        break;
      case OP_DIV:
        printf ("DIV\n");
        mem [sp - 2] = mem [sp - 2] / mem [sp - 1];
        sp -= 1;
        break;
      case OP_MOD:
        mem [sp - 2] = mem [sp - 2] % mem [sp - 1];
        sp -= 1;
        printf ("MOD\n");
        break;
      case OP_AND:
        mem [sp - 2] = mem [sp - 2] && mem [sp - 1];
        sp -= 1;
        printf ("AND\n");
        break;
      case OP_OR:
        mem [sp - 2] = mem [sp - 2] || mem [sp - 1];
        sp -= 1;
        printf ("OR\n");
        break;
      case OP_LT:
        mem [sp - 2] = mem [sp - 2] < mem [sp - 1];
        sp -= 1;
        printf ("LT\n");
        break;
      case OP_GT:
        mem [sp - 2] = mem [sp - 2] > mem [sp - 1];
        sp -= 1;
        printf ("GT\n");
        break;
      case OP_EQ:
        mem [sp - 2] = mem [sp - 2] == mem [sp - 1];
        sp -= 1;
        printf ("EQ\n");
        break;
      case OP_NEQ:
        mem [sp - 2] = mem [sp - 2] != mem [sp - 1];
        sp -= 1;
        printf ("NEQ\n");
        break;
      case OP_GEQ:
        mem [sp - 2] = mem [sp - 2] >= mem [sp - 1];
        sp -= 1;
        printf ("GEQ\n");
        break;
      case OP_LEQ:
        mem [sp - 2] = mem [sp - 2] <= mem [sp - 1];
        sp -= 1; 
        printf ("LEQ\n");
        break;
      case OP_NOT:
        mem [sp - 1] = !mem [sp - 1];
        printf ("NOT\n");
        break;
      case OP_JMP:
      {
        int addr = code [pc++];
        pc = addr;
        printf ("JMP %3d\n", addr);
      }
        break;
      case OP_JZ:
      {
        int addr = code [pc++];
        int v = mem [sp - 1];
        sp -= 1;
        if (!v) pc = addr;
        printf ("JZ  %3d\n", addr);
      }
        break;
      case OP_NUM:
      {
        int no = code [pc++];
        mem [sp++] = no;
        printf ("NUM %3d\n", no);
      }
        break;
      case OP_NEG:
        mem [sp - 1] = -mem [sp - 1];
        printf ("NEG\n");
        break;
      case OP_HLT:
        printf ("HLT\n");
        return;

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
  run (code);
  return 0;
}
