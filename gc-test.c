#include "copy.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

unsigned int g_seed = 0;
int fastrand() { 
  g_seed = (214013*g_seed+2531011); 
  return (g_seed>>16)&0x7FFF; 
} 

void print_list(cons_t c) {
  for (cons_t this = c; this != NULL; this = (cons_t)cdr(this))
    printf("%p (%s) -> ", this, in_newspace(c) ? "new" : "old");
  printf("\n");
}

void print_room(room_t r) {
  printf("Oldspace: %ld bytes, %ld pages\n",
         r.oldspace_bytes,
         r.oldspace_pages);
  printf("Newspace: %ld bytes, %ld pages\n",
         r.newspace_bytes,
         r.newspace_pages);
  printf("Last freed %ld pages, pinned %ld pages\n",
         r.freed_pages, r.pinned_pages);
  printf("We will start collection after %d pages are allocated.\n",
         threshold_pages);
}

cons_t make_garbage_list(int count) {
  cons_t list = NULL;
  for (int i = 0; i < count; i++) {
    cons_t this_cons = cons(NULL, (obj_t)list);
    if (fastrand() % 256 == 1)
      list = this_cons;
  }
  return list;
}

cons_t make_list(int count) {
  cons_t list = NULL;
  for (int i = 0; i < count; i++) {
    list = cons(NULL, (obj_t)list);
  }
  return list;
}

cons_t make_tree(int depth) {
  if (depth == 0)
    return NULL;
  cons_t a = make_tree(depth - 1);
  cons_t b = make_tree(depth - 1);
  switch (fastrand() % 4) {
  case 0:
    return NULL;
  case 1:
    return a;
  case 2:
    return b;
  case 3:
    return cons((obj_t)a, (obj_t)b);
  }
}

cons_t bar() {
  cons_t b = make_garbage_list(2048);
  allocate_page();
  return b;
}
cons_t test1() {
  puts("Test: copy a list. The following lists should be of the same length, ");
  puts("      but some parts may be copied.");
  cons_t b = bar();
  print_list(b);
  threshold_pages = 0;
  gc_work(131072);
  print_list(b);
  print_room(room());
  return b;
}

cons_t test2() {
  puts("Test: copy a cons with (eq (car it) (cdr it)), which should still ");
  puts("      hold after copying.");
  cons_t a = cons(NULL, NULL);
  cons_t b = cons((obj_t)a, (obj_t)a);
  threshold_pages = 0;
  gc_work(131072);
  printf("(%p . %p)\n", car(b), cdr(b));
  return b;
}

cons_t test3() {
  puts("Test: generate a large tree while producing a lot of garbage, to ");
  puts("      increase the collection threshold.");
  cons_t t = make_tree(27);
  print_room(room());
  return t;
}

cons_t test4() {
  puts("Test: generate less garbage to decrease the collection threshold.");
  /* Will the target decrease if we have fewer still-live pages? */
  for (int i = 0; i < 4096; i++)
    make_garbage_list(128);
}

cons_t test5() {
  puts("Test: Will scanning be noticeably slower with more pages?");
  cons_t the_list = make_list(1<<20);
  gc_work(INT_MAX);
  print_room(room());
  return the_list;
}

int main() {
  char start = 0;
  start_of_stack = &start;
  g_seed = time(NULL);
  
  test1();
  test2();
  test3();
  test4();
  test5();
  threshold_pages = 0;
  gc_work(INT_MAX);
  threshold_pages = 0;
  gc_work(INT_MAX);
  print_room(room());
  return 0;
}
