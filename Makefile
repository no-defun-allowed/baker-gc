CFLAGS=-g -DGC_REPORT_STATUS -DGC_MOLASSES_SIMULATOR
CC=gcc

test: gc-test.o copy.o scan-stack.o pages.o allocation-vector.o
	$(CC) $(CFLAGS) $^ -o $@

malloc-test:
	$(CC) $(CFLAGS) malloc-test.c -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o
	rm -f test malloc-test
