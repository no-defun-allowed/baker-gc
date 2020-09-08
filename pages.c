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
obj_t next_cons = NULL;
int new_pages = 0;

void allocate_page(void) {
  page_t new_page = memalign(PAGE_SIZE, PAGE_SIZE);
  if (!new_page) {
    perror("Failed to allocate a page");
    abort();
  }
  /* Initialize allocation markers */
  new_page->data = (obj_t)new_page->rest;
  new_page->allocated = (obj_t)new_page->rest;
  new_page->size = PAGE_SIZE;
  new_page->pinned = false;
  new_page->newspace = true;
  /* Link this page into the list of pages. */
  new_page->next_page = NULL;
  new_page->previous_page = last_page;
  if (last_page != NULL)
    last_page->next_page = new_page;
  last_page = new_page;
  /* Set up global information */
  next_cons = new_page->data;
  new_pages++;
}

obj_t end_of_page(page_t page) {
  return (obj_t)((intptr_t)page + page->size);
}

_Bool in_page(obj_t pointer, page_t page) {
  return pointer >= page->data &&
         pointer < page->allocated;
}

// Lower and upper bounds for oldspace pages
void* low = NULL;
void* high = NULL;

// A cache of frequently retrieved pages, keyed by page
struct page_entry {
  void* page;
};
struct page_entry page_cache[CACHE_SIZE];
// A cache of bogus pointers, keyed by pointer >> 3
struct page_entry negative_cache[CACHE_SIZE];
void clear_page_cache(void) {
  for (int i = 0; i < CACHE_SIZE; i++) {
    struct page_entry entry = {NULL};
    page_cache[i] = entry;
    negative_cache[i] = entry;
  }
}

void set_interval(void) {
  low =  (obj_t)((intptr_t)~1);
  high = (obj_t)((intptr_t)0);
  for (page_t page = oldspace; page != NULL; page = page->previous_page) {
    void* start = page->data;
    low = start < low ? start : low;
    void* end = page->allocated;
    high = end > high ? end : high;
  }
}

obj_t car(cons_t c) { return c->forward->car; }
obj_t cdr(cons_t c) { return c->forward->cdr; }
page_t page(obj_t c) { return (page_t)((intptr_t)(c) / PAGE_SIZE * PAGE_SIZE); }
_Bool forwarded(cons_t c) { return c->forward != c; }
_Bool pinned(obj_t c) { return page(c)->pinned; }

size_t hash_page(void* x) { return ((intptr_t)x / PAGE_SIZE) % CACHE_SIZE; }
size_t hash_pointer(void* x) { return ((intptr_t)(x) >> 3) % CACHE_SIZE; }
_Bool in_heap(obj_t pointer) {
  /* Is this pointer even in bounds? */
  if ((void*)pointer > high || (void*)pointer < low)
    return false;               // Quite obviously not in a page.
#ifndef GC_MOLASSES_SIMULATOR
  /* Try the cached page */
  struct page_entry cached_page = page_cache[hash_page(pointer)];
  if (cached_page.page != NULL && in_page(pointer, cached_page.page))
    return true;
  /* Test if this pointer is certainly out */
  cached_page = negative_cache[hash_pointer(pointer)];
  if (cached_page.page == (void*)((intptr_t)pointer >> 3))
    return false;
#endif
  /* Now scan the pages. */
  for (page_t the_page = oldspace; the_page != NULL; the_page = the_page->previous_page)
    if (pointer >= (obj_t)the_page->data && pointer < end_of_page(the_page)) {
      /* If we have a hit, put it in the cache for later. */
      struct page_entry entry = {the_page};
      page_cache[hash_page(the_page)] = entry;
      if (in_page(pointer, the_page))
        return true;
      else
        /* It won't be in another page then. */
        break;
    }
  struct page_entry entry = {(void*)((intptr_t)pointer >> 3)};
  negative_cache[hash_pointer(pointer)] = entry;
  return false;
}

cons_t make_cons(obj_t car, obj_t cdr) {
 again:
  if (last_page != NULL &&
      (next_cons + sizeof(struct cons)) < end_of_page(last_page)) {
    cons_t this = (cons_t)next_cons;
    next_cons += sizeof(struct cons);
    this->car = car;
    this->cdr = cdr;
    this->forward = this;
    last_page->allocated += sizeof(struct cons);
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
  clear_page_cache();
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
      page->newspace = true;
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
  for (page_t the_page = oldspace; the_page != NULL; the_page = the_page->previous_page)
    the_page->newspace = false;
  last_page = NULL;
  last_freed = freed_pages;
  last_pinned = pinned_pages;
  new_pages = 0;
  allocate_page();
  set_interval();
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
    intptr_t this_size = (intptr_t)page->allocated - (intptr_t)page->data;
    if (this_size > PAGE_SIZE)
      printf("Oddly sized page: %p\n", page);
    newspace_bytes += this_size;
  }
  room_t the_room = {oldspace_pages, oldspace_bytes,
                     newspace_pages, newspace_bytes,
                     last_freed, last_pinned};
  return the_room;
}
