default: fib.out

tpc: compile.c
	$(CC) -Wall -Wextra -ggdb -o $@ $<

tpr: run.c
	$(CC) -Wall -Wextra -ggdb -o $@ $<

tpd: disassembler.c
	$(CC) -Wall -Wextra -ggdb -o $@ $<

tools: tpd tpc tpr

test: tools test.p fail.p
	./tpc test.p test.out
	./tpc fail.p fail.out

fib: tools fib.out
	./tpr fib.out

fib.out: fib.p tools
	./tpc fib.p fib.out

test.out: test.p tools
	./tpc test.p test.out

dump: tools test.out
	./tpd test.out

