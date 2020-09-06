#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pages.h"

// Oldspace
page_t oldspace = NULL;
int last_freed = 0;
int last_pinned = 0;
// Newspace
page_t last_page = NULL;
cons_t next_cons = NULL;
int new_pages = 0;

void allocate_page(void) {
  page_t new_page = memalign(PAGE_SIZE, PAGE_SIZE);
  if (!new_page) {
    perror("Failed to allocate a page");
    abort();
  }
  /* Link this page into the list of pages. */
  new_page->size = PAGE_SIZE;
  new_page->next_page = NULL;
  new_page->allocated = new_page->data;
  new_page->pinned = false;
  new_page->previous_page = last_page;
  if (last_page != NULL)
    last_page->next_page = new_page;
  last_page = new_page;
  next_cons = new_page->data;
  new_pages++;
}

cons_t end_of_page(page_t page) {
  return (cons_t)((intptr_t)page + page->size);
}

_Bool in_page(cons_t pointer, page_t page) {
  return pointer >= page->data &&
         pointer < page->allocated;
}

// Lower and upper bounds for oldspace pages
void* low = NULL;
void* high = NULL;

// A cache of frequently retrieved pages, keyed by page
struct page_entry {
  page_t page;
  _Bool present;
};
struct page_entry page_cache[CACHE_SIZE];
void clear_page_cache(void) {
  for (int i = 0; i < CACHE_SIZE; i++) {
    struct page_entry entry = {NULL, false};
    page_cache[i] = entry;
  }
}

void set_interval(void) {
  low =  (void*)((intptr_t)~1);
  high = (void*)((intptr_t)0);
  for (page_t page = oldspace; page != NULL; page = page->previous_page) {
    low = (void*)page < low ? page : low;
    void* end = end_of_page(page);
    high = end > high ? end : high;
  }
}

cons_t car(cons_t c) { return c->forward->car; }
cons_t cdr(cons_t c) { return c->forward->cdr; }
page_t page(cons_t c) { return (page_t)((intptr_t)(c) / PAGE_SIZE * PAGE_SIZE); }
_Bool forwarded(cons_t c) { return c->forward != c; }
_Bool pinned(cons_t c) { return page(c)->pinned; }

#define HASH_POINTER(x) (((intptr_t)x / PAGE_SIZE) % CACHE_SIZE)
_Bool in_heap(cons_t pointer) {
  /* Is this pointer even in bounds? */
  if (!((void*)pointer < high && (void*)pointer > low))
    return false;               // Quite obviously not in a page.
  /* Try the cached page */
  struct page_entry cached_page = page_cache[HASH_POINTER(pointer)];
  if (cached_page.page == page(pointer) && in_page(pointer, cached_page.page))
    return cached_page.present;
  /* Now scan the pages. */
  for (page_t the_page = oldspace; the_page != NULL; the_page = the_page->previous_page)
    if (in_page(pointer, the_page)) {
      /* If we have a hit, put it in the cache for later. */
      struct page_entry entry = {the_page, true};
      page_cache[HASH_POINTER(the_page)] = entry;
      return true;
    }
  struct page_entry entry = {page(pointer), false};
  page_cache[HASH_POINTER(pointer)] = entry;
  return false;
}

cons_t make_cons(cons_t car, cons_t cdr) {
 again:
  if (last_page != NULL &&
      (next_cons + sizeof(struct cons)) < end_of_page(last_page)) {
    cons_t this = next_cons++;
    this->car = car;
    this->cdr = cdr;
    this->forward = this;
    last_page->allocated++;
    return this;
  } else {
    allocate_page();
    goto again;
  }
}

void flip(void) {
#ifdef GC_REPORT_STATUS
  puts("gc: Flipping...");
#endif
  /* clear up oldspace */
  int freed_pages = 0, pinned_pages = 0;
  page_t page = oldspace;
  while (page != NULL) {
    page_t next_page;
    next_page = page->previous_page;
    if (page->pinned) {
      /* Move this page to newspace. */
      page->next_page = NULL;
      page->previous_page = last_page;
      page->pinned = false;
      last_page->next_page = page;
      last_page = page;
      pinned_pages++;
    } else {
      free(page);
      freed_pages++;
    }
    page = next_page;
  }
#ifdef GC_REPORT_STATUS
  printf("gc: %d pages freed, %d pages still pinned, %d pages to evacuate\n", freed_pages, pinned_pages, new_pages);
#endif
  /* set up the next oldspace and newspace */
  oldspace = last_page;
  last_page = NULL;
  last_freed = freed_pages;
  last_pinned = pinned_pages;
  new_pages = 0;
  clear_page_cache();
  set_interval();
  allocate_page();
}

// Get the room (memory usage)
room_t room(void) {
  long oldspace_pages = 0, oldspace_bytes = 0,
       newspace_pages = 0, newspace_bytes = 0;
  for (page_t page = oldspace; page != NULL; page = page->previous_page) {
    oldspace_pages++;
    oldspace_bytes += (intptr_t)page->allocated - (intptr_t)page->data;
  }
  for (page_t page = last_page; page != NULL; page = page->previous_page) {
    newspace_pages++;
    newspace_bytes += (intptr_t)page->allocated - (intptr_t)page->data;
  }
  room_t the_room = {oldspace_pages, oldspace_bytes,
                     newspace_pages, newspace_bytes,
                     last_freed, last_pinned};
  return the_room;
}
