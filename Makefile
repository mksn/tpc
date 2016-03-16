tpc: main.c
	$(CC) -Wall -Wextra -ggdb -o $@ $<
	./tpc test.p
	./tpc fail.p
