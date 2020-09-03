test: gc-test.o copy.o scan-stack.o pages.o
	gcc $^ -o $@

%.o: %.c
	gcc -c $^ -o $@

clean:
	rm -f *.o
	rm -f test
