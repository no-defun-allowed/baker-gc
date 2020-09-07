#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include "scan-stack.h"

void print_cons(obj_t obj) {
  cons_t c = (cons_t)obj;
  printf("%p = (%p . %p) on page %p \n", c, car(c), cdr(c), page(obj));
}

void scan_stack2(char* start, char* end, obj_consumer_t k) {
  for (char* position = start; position < end; position++) {
    if (in_heap(*(obj_t*)position)) {
      // Push this pointer back to the start of the cons.
      obj_t  the_obj  = *(obj_t*)position;
      page_t the_page = page(the_obj);
      size_t offset = (char*)the_obj - (char*)the_page->data;
      offset = (offset / sizeof(struct cons)) * sizeof(struct cons);
      k((obj_t)((char*)the_page->data + offset));
    }
  }
}

void scan_stack(char* start, obj_consumer_t k) {
  jmp_buf j;
  setjmp(j);
  char* end = ((char*)(&j) + sizeof(j));
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
