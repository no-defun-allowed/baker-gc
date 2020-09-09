#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include "scan-stack.h"

void print_cons(obj_t obj, page_t p) {
  cons_t c = (cons_t)obj;
  printf("%p = (%p . %p) ⇒ %p on page %p \n",
         c, car(c), cdr(c), forwarding(c), p);
}

void scan_stack2(char* start, char* end, obj_consumer_t k) {
  for (char* position = start; position < end; position++) {
    object_location_t start_of_object = in_heap(*(obj_t*)position);
    if (start_of_object.object != NULL) {
      k(start_of_object.object, start_of_object.page);
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
