#include "copy.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void print_list(cons_t c) {
  printf("%p = (", c);
  for (cons_t this = c; this != NULL; this = cdr(this))
    printf("x ");
  printf(")\n");
}

cons_t make_list(int count) {
  cons_t list = NULL;
  for (int i = 0; i < count; i++) {
    cons_t this_cons = make_cons(NULL, list);
    if (rand() % 16 == 1)
      list = this_cons;
  }
  return list;
}

cons_t bar() {
  cons_t b = make_list(2048);
  allocate_page();
  return b;
}
cons_t foo() {
  cons_t b = bar();
  print_list(cdr(cdr(b)));
  /* do a GC */
  gc_setup();
  gc_work(4096);
  /* did anything move? */
  print_list(cdr(cdr(b)));
  return b;
}

int main() {
  char start = 0;
  start_of_stack = &start;
  srand(time(NULL));
  
  foo();
  return 0;
}
