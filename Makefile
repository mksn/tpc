default: chain

TPC_OBJS := compile.o \
						sym-tab.o \
						std-procs.o 

TPR_OBJS := run.o std-procs.o

TPD_OBJS := disassembler.o std-procs.o 

tpc: $(TPC_OBJS) 
	$(CC) -Wall -Wextra -ggdb -o $@ $^

tpr: $(TPR_OBJS)
	$(CC) -Wall -Wextra -ggdb -o $@ $^

tpd: $(TPD_OBJS)
	$(CC) -Wall -Wextra -ggdb -o $@ $^

.c.o: op-codes.h sym-tab.h std-procs.h
	$(CC) -Wall -Wextra -ggdb -c $<


tools: tpd tpc tpr

test: tools test.p fail.p
	./tpc test.p test.out
	./tpc fail.p fail.out

fib: tools fib.out
	./tpr fib.out

fib.out: fib.p tools
	./tpc fib.p fib.out

compile.o: compile.c sym-tab.h std-procs.h op-codes.h

sym-tab.o: sym-tab.c std-procs.h sym-tab.h

std-procs.o: std-procs.c std-procs.h sym-tab.h 

chain: tools fib.out fib.p

test.out: test.p tools
	./tpc test.p test.out

dump: tools test.out
	./tpd test.out

clean:
	$(RM) tpr
	$(RM) tpd
	$(RM) tpc
	$(RM) *.o{,~}

tags:
	etags -c *.c
