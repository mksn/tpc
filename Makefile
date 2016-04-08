default: test

tpc: main.c
	$(CC) -Wall -Wextra -ggdb -o $@ $<

test: tpc test.p fail.p
	./tpc test.p
	./tpc fail.p
