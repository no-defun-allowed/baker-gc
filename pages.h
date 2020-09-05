#ifndef PAGES_H
#define PAGES_H
#define PAGE_SIZE (1 << 14)
#include <stdint.h>

struct cons {
  struct cons* forward;
  struct cons* car;
  struct cons* cdr;
};
typedef struct cons* cons_t;

struct page {
  struct page* previous_page;
  struct page* next_page;
  _Bool pinned : 1;
  uint32_t size : 31;
  struct cons* allocated;
  struct cons data[0];
};
typedef struct page* page_t;

extern page_t last_page;
extern cons_t next_cons;
extern int new_pages;
extern int last_pages;
extern int threshold_pages;

void allocate_page(void);
_Bool in_page(cons_t pointer, page_t page);
_Bool in_heap(cons_t pointer);
cons_t make_cons(cons_t, cons_t);

cons_t car(cons_t);
cons_t cdr(cons_t);
page_t page(cons_t);
_Bool forwarded(cons_t);
_Bool pinned(cons_t);

void flip(void);

struct room_data {
  long oldspace_pages;
  long oldspace_bytes;
  long newspace_pages;
  long newspace_bytes;
};
typedef struct room_data room_t;
room_t room(void);
#endif
