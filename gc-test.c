#include "copy.h"
#include <stdio.h>

void print_list(cons_t c) {
  printf("%p = (", c);
  for (cons_t this = c; this != NULL; this = cdr(this))
    printf("x ");
  printf(")\n");
}

cons_t make_list(int count) {
  cons_t list = NULL;
  for (int i = 0; i < count; i++) {
    list = make_cons(NULL, list);
    allocate_page();
  }
  return list;
}

cons_t bar() {
  cons_t b = make_list(32);
  allocate_page();
  return b;
}
cons_t foo() {
  cons_t b = bar();
  print_list(cdr(cdr(b)));
  /* do a GC */
  gc_setup();
  gc_work(128);
  /* did anything move? */
  print_list(cdr(cdr(b)));
  return b;
}

int main() {
  char start = 0;
  start_of_stack = &start;
  
  foo();
  return 0;
}
