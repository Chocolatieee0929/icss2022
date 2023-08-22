CC=gcc
CFLAGS=-Wall -Werror -ggdb3 -std=gnu99 -pedantic $(OTHERFLAGS)
test:
	gcc -o test main.c $(CFLAGS) $^
clean:
	rm -rf *.o test
