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
_Bool gc_running = false;
_Bool disable_gc = false;
int threshold_pages = 3;

cons_t copy(cons_t c) {
  if (c == NULL)
    return c;
  if (!in_heap(c))
    return c;                   /* don't move cons in newspace */
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

#ifdef GC_REPORT_STATUS
#include <sys/time.h>
long microseconds(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}
#endif

void gc_setup(void) {
  gc_running = true;
#ifdef GC_REPORT_STATUS
  long start_time = microseconds();
#endif
  flip();
  first_page = last_page;
  next_cons = next_to_copy = &first_page->data[0];
  scan_stack((char*)start_of_stack, scan_cons);
#ifdef GC_REPORT_STATUS
  long end_time = microseconds();
  printf("gc: STW took %ld microseconds\n", end_time - start_time);
#endif
}

void gc_stop(void) {
  first_page = NULL;
  gc_running = false;
  room_t the_room = room();
  float freed_ratio = (float)(the_room.freed_pages) /
                      (float)(the_room.freed_pages + the_room.newspace_pages + the_room.pinned_pages);
#ifdef GC_REPORT_STATUS
  printf("gc: freed %.2f percent of the heap\n", freed_ratio * 100);
#endif
  if (freed_ratio < 0.6)
    threshold_pages *= 2;
  else if (freed_ratio > 0.9)
    threshold_pages /= 2;
  if (threshold_pages < 3)
    threshold_pages = 3;
#ifdef GC_REPORT_STATUS
  printf("gc: threshold is now %d pages\n", threshold_pages);
#endif
}

void gc_work(int steps) {
  if (!gc_running && new_pages < threshold_pages)
    /* Don't start unless we've consed enough pages */
    return;
  if (first_page == NULL)
    gc_setup();
  for (int x = 0; x < steps; x++) {
    if (next_to_copy == next_cons) {
      gc_stop();
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
      gc_stop();
      return;
    }
    next_to_copy->car = copy(next_to_copy->car);
    next_to_copy->cdr = copy(next_to_copy->cdr);
    next_to_copy++;
  }
}

int steps_per_cons = 2;
cons_t cons(cons_t car, cons_t cdr) {
  if (!disable_gc)
    gc_work(steps_per_cons);
  return make_cons(car, cdr);
}
