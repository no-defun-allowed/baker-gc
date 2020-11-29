#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "pages.h"
#include "copy.h"

extern obj_t next_cons;
obj_t next_to_copy = NULL;
page_t this_page;
enum gc_state current_state = STOPPED;
_Bool disable_gc = false;
int threshold_pages = 10;

#include <sys/time.h>
long microseconds(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

obj_t copy(obj_t cobj) {
  cons_t c = (cons_t)cobj;
  if (c == NULL)
    return cobj;
  if (in_newspace(c))
    return cobj;                /* don't move cons in newspace */
  if (forwarded(c))
    return (obj_t)forwarding(c); /* don't duplicate already moved cons */
  /* okay, actually copy the cons */
  cons_t new_cons = make_cons(c->car, c->cdr);
  c->forward = new_cons;
  set_in_newspace(c, false);
  return (obj_t)new_cons;
}

void scan_cons(obj_t cobj, page_t page) {
  cons_t c = (cons_t)cobj;
  page->pinned = true;
  copy(cobj);
}

/* The maximum duration (in microseconds) that we want to keep pauses under. */
#define PAUSE_TARGET 3000

long last_newspace_size = PAGE_SIZE * 10;
long last_pause_time = 0;

void set_threshold(void) {
  room_t the_room = room();
  if (the_room.oldspace_bytes > 0) {
    float size_ratio = (float)(the_room.newspace_bytes) /
                       (float)(last_newspace_size);
#ifdef GC_REPORT_STATUS
    printf("gc: Finished collecting; heap is now %.2fx its last size\n", size_ratio);
#endif
    /* First of all, try to keep pause times down. */
    if (last_pause_time > PAUSE_TARGET) {
      float time_ratio = (float)(last_pause_time) / (float)(PAUSE_TARGET);
      /* Assuming that there is a linear relationship between pause time and
       threshold, then we should be able to scale down by how much we went
      over the target to keep under the target.*/
      threshold_pages = (int)((float)threshold_pages / time_ratio);
    /* Try to keep the heap between 0.5x and 1.3x its last size. */
    } else if (size_ratio < 0.5)
      threshold_pages = threshold_pages * 3 / 4;
    else if (size_ratio > 1.3)
      threshold_pages = threshold_pages * 4 / 3;
    /* And don't ever schedule for sooner than 10 pages. */
    if (threshold_pages < 10)
      threshold_pages = 10;
  } else {
#ifdef GC_REPORT_STATUS
    printf("gc: Finished collecting; freed 0 pages\n");
#endif    
  }
#ifdef GC_REPORT_STATUS
  printf("gc: threshold is now %d pages\n", threshold_pages);
#endif
  last_newspace_size = the_room.newspace_bytes;
}

void gc_setup(void) {
  if (last_page == NULL)
    return;                     /* Nothing to GC! */
  current_state = COPYING;
  long start_time = microseconds();
  flip();
  next_to_copy = last_page->data;
  this_page = last_page;
  scan_stack((char*)start_of_stack, scan_cons);
  long end_time = microseconds();
#ifdef GC_REPORT_STATUS  
  printf("gc: STW took %ld microseconds\n", end_time - start_time);
#endif
  last_pause_time = end_time - start_time;
}

void gc_stop(void) {
  next_to_copy = NULL;
  this_page = NULL;
  current_state = STOPPED;
  set_threshold();
}

void gc_work(int steps) {
  if (current_state == STOPPED && new_pages < threshold_pages)
    /* Don't start unless we've consed enough pages */
    return;
  if (next_to_copy == NULL)
    gc_setup();
  for (int x = 0; x < steps; x++) {
    if (next_to_copy == next_cons) {
      gc_stop();
      return;
    }
    /* There appears to be nothing else to copy onto this page. 
       Move to the next one. */
    if (!in_page(next_to_copy, this_page)) {
      next_to_copy = this_page->next_page->data;
      this_page = this_page->next_page;
    }
    /* next_cons could be at the start of a page with no conses if the user
       calls allocate_page() directly, so we could also run out of conses to 
       copy. */
    if (next_to_copy == next_cons) {
      gc_stop();
      return;
    }
    cons_t this_cons = (cons_t)next_to_copy;
    this_cons->car = copy(this_cons->car);
    this_cons->cdr = copy(this_cons->cdr);
    next_to_copy += sizeof(struct cons);
  }
}

int steps_per_cons = 3;
cons_t cons(obj_t car, obj_t cdr) {
  car = (car == NULL) ? NULL : (obj_t)forwarding((cons_t)car);
  cdr = (cdr == NULL) ? NULL : (obj_t)forwarding((cons_t)cdr);
  cons_t the_cons = make_cons(car, cdr);
  if (!disable_gc)
    gc_work(steps_per_cons);
  return the_cons;
}
