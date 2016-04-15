default: fib.out

tpc: compile.c
	$(CC) -Wall -Wextra -ggdb -o $@ $<

tpr: run.c
	$(CC) -Wall -Wextra -ggdb -o $@ $<

test: tpc test.p fail.p
	./tpc test.p
	./tpc fail.p

fib: tpr fib.out
	./tpr fib.out

fib.out: fib.p tpc
	./tpc fib.p fib.out
