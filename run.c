#include <stdio.h>
#include <stdlib.h>
#include "op-codes.h"
#include "std-procs.h"

int mem[65536];

int debug  = 0;

void run (int *code)
{
  int pc = 0;
  int sp = 0;
  int mp = 0;

  while (1)
  {
    /*
      printf ("[sp: %d, mp: %d] ", sp, mp);
      printf ("%3d: ", pc);
    */
    switch (code [pc++])
    {
      case OP_MST:
        {
          int level = code [pc++];
          mem [sp + 0] = 0; // return value
          mem [sp + 1] = mem [mp + 1]; // static link
          mem [sp + 2] = mp; // dynamic link
          mem [sp + 3] = 0; // return addr;
          sp += 4;
          if (debug) 
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
          if (debug)
            printf ("CUP %3d %3d\n", no_args, addr);
        }
        break;
      case OP_CSP:
        {
          int no_args = code [pc++];
          int addr = code [pc++];
          mem [sp-no_args-1] = pc;
          switch (addr) {
            case 0:
              WriteLn (mem [sp - 1]);
              sp -= 1;
              break;
            case 2:
              {
                int val = ReadLn ();
                mem [sp++] = val;
              }
              break;
            default:
              
              break;
          }
          if (debug) 
            printf ("CSP %3d %3d\n", no_args, addr);
        }
        break;
      case OP_ENT:
        {
          int no_vars = code [pc++];
          sp = mp + no_vars;
          if (debug) 
            printf ("ENT %3d\n", no_vars);
        }
        break;
      case OP_RET:
        sp = mp;
        pc = mem [mp + 3];
        mp = mem [mp + 2];
        if (debug) 
          printf ("RET\n");
        break;
      case OP_RC:
        sp = mp + 1;
        pc = mem [mp + 3];
        mp = mem [mp + 2];
        if (debug) 
          printf ("RC\n");
        break;
      case OP_LD:
        {
          int level = code [pc++];
          int addr = code [pc++];
          mem [sp++] = mem [mp + addr];
          if (debug) 
            printf ("LD  %3d %3d\n", level, addr);
        }
        break;

      case OP_ST:
        {
          int level = code [pc++];
          int addr = code [pc++];
          mem [mp + addr] = mem [--sp];
          if (debug) 
            printf ("ST  %3d %3d\n", level, addr);
        }
        break;
      case OP_ADD:
        mem [sp - 2] = mem [sp - 2] + mem [sp - 1];
        sp -= 1;
        if (debug) 
          printf ("ADD\n");
        break;
      case OP_SUB:
        mem [sp - 2] = mem [sp - 2] - mem [sp - 1];
        sp -= 1;
        if (debug) 
          printf ("SUB\n");
        break;
      case OP_MUL:
        mem [sp - 2] = mem [sp - 2] * mem [sp - 1];
        sp -= 1;
        if (debug)  
          printf ("MUL\n");
        break;
      case OP_DIV:
        if (debug) 
          printf ("DIV\n");
        mem [sp - 2] = mem [sp - 2] / mem [sp - 1];
        sp -= 1;
        break;
      case OP_MOD:
        mem [sp - 2] = mem [sp - 2] % mem [sp - 1];
        sp -= 1;
        if (debug) 
          printf ("MOD\n");
        break;
      case OP_AND:
        mem [sp - 2] = mem [sp - 2] && mem [sp - 1];
        sp -= 1;
        if (debug) 
          printf ("AND\n");
        break;
      case OP_OR:
        mem [sp - 2] = mem [sp - 2] || mem [sp - 1];
        sp -= 1; 
        if (debug) 
          printf ("OR\n");
        break;
      case OP_LT:
        mem [sp - 2] = mem [sp - 2] < mem [sp - 1];
        sp -= 1; 
        if (debug)  
          printf ("LT\n");
        break;
      case OP_GT:
        mem [sp - 2] = mem [sp - 2] > mem [sp - 1];
        sp -= 1;
        if (debug) 
          printf ("GT\n");
        break;
      case OP_EQ:
        mem [sp - 2] = mem [sp - 2] == mem [sp - 1];
        sp -= 1;
        if (debug) 
          printf ("EQ\n");
        break;
      case OP_NEQ:
        mem [sp - 2] = mem [sp - 2] != mem [sp - 1];
        sp -= 1;
        if (debug) 
          printf ("NEQ\n");
        break;
      case OP_GEQ:
        mem [sp - 2] = mem [sp - 2] >= mem [sp - 1];
        sp -= 1;
        if (debug) 
          printf ("GEQ\n");
        break;
      case OP_LEQ:
        mem [sp - 2] = mem [sp - 2] <= mem [sp - 1];
        sp -= 1; 
        if (debug) 
          printf ("LEQ\n");
        break;
      case OP_NOT:
        mem [sp - 1] = !mem [sp - 1];
        if (debug) printf ("NOT\n");
        break;
      case OP_JMP:
        {
          int addr = code [pc++];
          pc = addr;
          if (debug) printf ("JMP %3d\n", addr);
        }
        break;
      case OP_JZ:
        {
          int addr = code [pc++];
          int v = mem [sp - 1];
          sp -= 1;
          if (v == 0) pc = addr;
          if (debug) printf ("JZ  %3d\n", addr);
        }
        break;
      case OP_NUM:
        {
          int no = code [pc++];
          mem [sp++] = no;
          if (debug) printf ("NUM %3d\n", no);
        }
        break;
      case OP_NEG:
        mem [sp - 1] = -mem [sp - 1];
        if (debug) printf ("NEG\n");
        break;
      case OP_HLT:
        if (debug) printf ("HLT\n");
        return;
      default:
        // some kind of exception, we're in deep sh-t man
        break;
    }
  }
  if (debug) printf ("[EOF]\n");
}

void usage (char *prog_name)
{
  printf ("usage: %s file.out\n", prog_name);
}

int main (int argc,
          char **argv)
{
  int *code;
  if (argc != 2)
    usage (argv[0]);

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
