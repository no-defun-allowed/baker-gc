#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
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
    if ((intptr_t)c->cdr & 1 == 1)  /* secret bit telling us if we copied already */
      return c;
    /* don't move pinned cons, copy its slots */
    c->car = copy(c->car);
    c->cdr = (cons_t)((intptr_t)copy(c->cdr) | 1);
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
  allocate_page();
  first_page = last_page;
  next_cons = next_to_copy = &first_page->data[0];
  scan_stack((char*)start_of_stack, scan_cons);
}

void gc_work(int steps) {
  if (first_page == NULL)
    gc_setup();
  for (int x = 0; x < steps; x++) {
    if (next_to_copy == next_cons) {
      gc_setup();
      return;
    }
    page_t this_page = page(next_to_copy);
    /* There appears to be nothing else to copy onto this page. 
       Move to the next one. */
    if (!in_page(next_to_copy, this_page))
      next_to_copy = &first_page->next_page->data[0];
    /* next_cons could be at the start of a page with no conses if the user
       calls allocate_page() directly, so we could also run out of conses to 
       copy. */
    if (next_to_copy == next_cons) {
      gc_setup();
      return;
    }
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
