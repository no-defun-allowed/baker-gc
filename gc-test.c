#include "copy.h"
#include <stdio.h>

cons_t bar() {
  cons_t a = cons(NULL, NULL);
  allocate_page();
  cons_t b = cons(NULL, a);
  allocate_page();           /* put everything else on another page */
  return b;
}
cons_t foo() {
  cons_t b = cons(NULL, bar());
  printf("b = %p, cddr = %p\n", b, b->cdr->cdr);
  scan_stack(start_of_stack, print_cons);
  /* do a GC */
  gc_setup();
  gc_work(128);
  /* did anything move? */
  printf("b = %p, cddr = %p\n", b, b->cdr->cdr);
  return b;
}

int main() {
  char start = 0;
  start_of_stack = &start;
  
  foo();
  return 0;
}
