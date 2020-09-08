#include <stdio.h>
#include <stdlib.h>
#include <time.h>

unsigned int g_seed = 0;
int fastrand() { 
  g_seed = (214013*g_seed+2531011); 
  return (g_seed>>16)&0x7FFF; 
}

struct cons {
  struct cons* car;
  struct cons* cdr;
};
typedef struct cons* cons_t;

cons_t cons(cons_t car, cons_t cdr) {
  cons_t the_cons = malloc(sizeof(struct cons));
  the_cons->car = car;
  the_cons->cdr = cdr;
  return the_cons;
}

void free_cons(cons_t c) {
  cons_t cons_to_free = c;
  while (cons_to_free != NULL) {
    free_cons(cons_to_free->car);
    cons_t this_cons = cons_to_free;
    cons_to_free = cons_to_free->cdr;
    free(this_cons);
  }
}

void print_list(cons_t c) {
  for (cons_t this = c; this != NULL; this = this->cdr)
    printf("%p -> ", this);
  printf("\n");
}

cons_t make_garbage_list(int count) {
  cons_t list = NULL;
  for (int i = 0; i < count; i++) {
    cons_t this_cons = cons(NULL, list);
    if (fastrand() % 256 == 1)
      list = this_cons;
    else
      free(this_cons);          /* just free this cons, not the list */
  }
  return list;
}


cons_t make_list(int count) {
  cons_t list = NULL;
  for (int i = 0; i < count; i++)
    list = cons(NULL, list);
  return list;
}

cons_t make_tree(int depth) {
  if (depth == 0)
    return NULL;
  cons_t a = make_tree(depth - 1);
  cons_t b = make_tree(depth - 1);
  switch (fastrand() % 4) {
  case 0:
    free_cons(a);
    free_cons(b);
    return NULL;
  case 1:
    free_cons(b);
    return a;
  case 2:
    free_cons(a);
    return b;
  case 3:
    return cons(a, b);
  }
}

cons_t bar() {
  cons_t b = make_garbage_list(2048);
  /* allocate_page(); */
  return b;
}
cons_t test1() {
  puts("Test: copy a list. The following lists should be of the same length, ");
  puts("      but some parts may be copied.");
  cons_t b = bar();
  print_list(b);
  print_list(b);
  return b;
}

cons_t test3() {
  puts("Test: generate a large tree while producing a lot of garbage, to ");
  puts("      increase the collection threshold.");
  cons_t t = make_tree(27);
  return t;
}

void test4() {
  puts("Test: generate less garbage to decrease the collection threshold.");
  /* Will the target decrease if we have fewer still-live pages? */
  for (int i = 0; i < 4096; i++)
    free_cons(make_garbage_list(128));
}

cons_t test5() {
  puts("Test: Will scanning be noticeably slower with more pages?");
  cons_t the_list = make_list(1<<20);
  return the_list;
}

int main() {
  g_seed = time(NULL);
  
  free_cons(test1());
  /* test2 creates two references to the same cons; we can't free it easily. */
  /* test2(); */
  free_cons(test3());
  test4();
  free_cons(test5());
  return 0;
}
