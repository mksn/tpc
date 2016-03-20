tpc: main.c
	$(CC) -Wall -Wextra -ggdb -o $@ $<
ifneq ($(UNAME), Darwin)
	./tpc test.p
	./tpc fail.p
endif
