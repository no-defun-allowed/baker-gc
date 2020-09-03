#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include "scan-stack.h"

void print_cons(cons_t c) {
  printf("%p = (%p . %p) on page %p \n", c, car(c), cdr(c), page(c));
}

void scan_stack2(char* start, char* end, cons_consumer_t k) {
  for (char* position = start; position < end; position++) {
    if (in_heap(*(cons_t*)position)) {
      // Push this pointer back to the start of the cons.
      cons_t the_cons = *(cons_t*)position;
      page_t the_page = page(the_cons);
      size_t offset = (char*)the_cons - (char*)the_page;
      offset = (offset / sizeof(struct cons)) * sizeof(struct cons);
      k((cons_t)((char*)the_page + offset));
    }
  }
}

void scan_stack(char* start, cons_consumer_t k) {
  jmp_buf j;
  setjmp(j);
  char* end = (char*)j;
  if (start > end)
    scan_stack2(end, start, k);
  else if (end > start)
    scan_stack2(start, end, k);
  else {
    puts("start = end?");
    abort();
  }
}

char* start_of_stack;
