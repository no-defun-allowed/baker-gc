#include "copy.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void print_list(cons_t c) {
  for (cons_t this = c; this != NULL; this = cdr(this))
    printf("%p -> ", this);
  printf("\n");
}

void print_room(room_t r) {
  printf("Oldspace: %ld bytes, %ld pages\n",
         r.oldspace_bytes,
         r.oldspace_pages);
  printf("Newspace: %ld bytes, %ld pages\n",
         r.newspace_bytes,
         r.newspace_pages);
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

cons_t make_tree(int depth) {
  if (depth == 0)
    return NULL;
  cons_t a = make_tree(depth - 1);
  cons_t b = make_tree(depth - 1);
  switch (rand() % 4) {
  case 0:
    return NULL;
  case 1:
    return a;
  case 2:
    return b;
  case 3:
    return cons(a, b);
  }
}

cons_t bar() {
  cons_t b = make_list(2048);
  allocate_page();
  return b;
}
cons_t test1() {
  puts("Test: copy a list. The following lists should be of the same length, ");
  puts("      but some parts may be copied.");
  cons_t b = bar();
  print_list(b);
  gc_work(131072);
  print_list(b);
  print_room(room());
  return b;
}

cons_t test2() {
  puts("Test: copy a cons with (eq (car it) (cdr it)), which should still ");
  puts("      hold after copying.");
  cons_t a = cons(NULL, NULL);
  cons_t b = cons(a, a);
  gc_work(131072);
  printf("(%p . %p)\n", car(b), cdr(b));
  return b;
}

cons_t test3() {
  puts("Test: generate a large tree while producing a lot of garbage, to ");
  puts("      increase the collection threshold.");
  cons_t t = make_tree(25);
  print_room(room());
  return t;
}

cons_t test4() {
  puts("Test: generate less garbage to decrease the collection threshold.");
  /* Will the target decrease if we have fewer still-live pages? */
  for (int i = 0; i < 1024; i++)
    make_list(64);
}

int main() {
  char start = 0;
  start_of_stack = &start;
  srand(time(NULL));
  
  test1();
  test2();
  test3();
  test4();
  gc_work(1000000);
  print_room(room());
  return 0;
}
