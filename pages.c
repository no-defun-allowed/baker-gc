#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pages.h"

// Oldspace
page_t oldspace = NULL;
// Newspace
page_t last_page = NULL;
cons_t next_cons = NULL;

void push_onto_list(page_t page, page_t* list) {
  page->previous_page = *list;
  if (*list != NULL) {
    (*list)->next_page = page;
  }
  *list = page;
}

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
  push_onto_list(new_page, &last_page);
  next_cons = new_page->data;
}

cons_t end_of_page(page_t page) {
  return (cons_t)((intptr_t)page + page->size);
}

_Bool in_page(cons_t pointer, page_t page) {
  return pointer >= page->data &&
         pointer < page->allocated;
}

_Bool in_heap(cons_t pointer) {
  for (page_t page = oldspace; page != NULL; page = page->previous_page)
    if (in_page(pointer, page)) return true;
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

cons_t car(cons_t c) { return c->forward->car; }
/* remove the secret pinned-with-copied-slots tag */
cons_t cdr(cons_t c) { return c->forward->cdr; }
page_t page(cons_t c) { return (page_t)((intptr_t)(c) / PAGE_SIZE * PAGE_SIZE); }
_Bool forwarded(cons_t c) { return c->forward != c; }
_Bool pinned(cons_t c) { return page(c)->pinned; }

void flip(void) {
  puts("gc: Flipping...");
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
      last_page = page;
      pinned_pages++;
    } else {
      free(page);
      freed_pages++;
    }
    page = next_page;
  }
  printf("gc: %d pages freed, %d pages still pinned\n", freed_pages, pinned_pages);
  /* set up the next oldspace and newspace */
  oldspace = last_page;
  last_page = NULL;
  allocate_page();
}
