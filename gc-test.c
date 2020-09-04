#include "copy.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void print_list(cons_t c) {
  for (cons_t this = c; this != NULL; this = cdr(this))
    printf("%p -> ", this);
  printf("\n");
}

cons_t make_list(int count) {
  cons_t list = NULL;
  for (int i = 0; i < count; i++) {
    cons_t this_cons = cons(NULL, list);
    if (rand() % 256 == 1)
      list = this_cons;
  }
  return list;
}

cons_t bar() {
  cons_t b = make_list(32768);
  allocate_page();
  return b;
}
cons_t test1() {
  cons_t b = bar();
  print_list(b);
  gc_work(131072);
  print_list(b);
  return b;
}

cons_t test2() {
  cons_t a = cons(NULL, NULL);
  cons_t b = cons(a, a);
  gc_work(131072);
  printf("(%p . %p)\n", car(b), cdr(b));
  return b;
}

int main() {
  char start = 0;
  start_of_stack = &start;
  srand(time(NULL));
  
  test1();
  test2();
  return 0;
}
