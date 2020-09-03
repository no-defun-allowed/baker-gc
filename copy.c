#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "pages.h"
#include "copy.h"

extern cons_t next_cons;
cons_t next_to_copy = NULL;
page_t first_page = NULL;

cons_t copy(cons_t c) {
  if (c == NULL)
    return c;
  if (!in_heap(c))
    return c;                   /* don't move cons in newspace */
  if (pinned(c)) {
    /* don't move pinned cons, copy its slots */
    c->car = copy(c->car);
    c->cdr = copy(c->cdr);
    return c;
  }
  if (forwarded(c))
    return c->forward;          /* don't duplicate already moved cons */
  cons_t new_cons = make_cons(c->car, c->cdr);
  c->forward = new_cons;
  return new_cons;
}

void scan_cons(cons_t c) {
  page(c)->pinned = true;
  c->car = copy(c->car);
  c->cdr = copy(c->cdr);
}

void gc_setup(void) {
  flip();
  make_page();
  first_page = last_page;
  next_cons = next_to_copy = &first_page->data[0];
  scan_stack((char*)start_of_stack, scan_cons);
}

void gc_work(int steps) {
  for (int x = 0; x < steps; x++) {
    if (next_to_copy == next_cons) {
      gc_setup();
      return;
    }
    page_t this_page = page(next_to_copy);
    if (!in_page(next_to_copy, this_page))
      next_to_copy = &first_page->next_page->data[0];
    next_to_copy->car = copy(next_to_copy->car);
    next_to_copy->cdr = copy(next_to_copy->cdr);
    next_to_copy++;
  }
}

int steps_per_cons = 2;
cons_t cons(cons_t car, cons_t cdr) {
  gc_work(steps_per_cons);
  return make_cons(car, cdr);
}
