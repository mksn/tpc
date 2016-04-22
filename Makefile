default: fib.out

tpc: compile.c
	$(CC) -Wall -Wextra -ggdb -o $@ $<

tpr: run.c
	$(CC) -Wall -Wextra -ggdb -o $@ $<

tpd: disassembler.c
	$(CC) -Wall -Wextra -ggdb -o $@ $<

test: tpc test.p fail.p
	./tpc test.p test.out
	./tpc fail.p fail.out

fib: tpr fib.out
	./tpr fib.out

fib.out: fib.p tpc
	./tpc fib.p fib.out

test.out: test.p tpc
	./tpc test.p test.out

dump: tpd test.out
	./tpd test.out

tout: tpd tpc tpr
