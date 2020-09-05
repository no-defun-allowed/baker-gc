CFLAGS=-O3 -g
CC=gcc

test: gc-test.o copy.o scan-stack.o pages.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o
	rm -f test
