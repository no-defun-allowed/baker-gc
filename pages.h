#ifndef PAGES_H
#define PAGES_H

#define PAGE_SIZE (1 << 13)
#define CACHE_SIZE 16384

#include <stdint.h>
#include <stdbool.h>
#include "allocation-vector.h"

typedef char* obj_t;

struct cons {
  struct cons* forward;
  obj_t car;
  obj_t cdr;
};
typedef struct cons* cons_t;
struct page {
  struct page* previous_page;
  struct page* next_page;
  _Bool pinned : 1;
  _Bool newspace : 1;
  uint32_t size;
  obj_t allocated;
  obj_t data;
  char rest[0];
};
typedef struct page* page_t;

struct object_location {
  obj_t object;
  page_t page;
};
typedef struct object_location object_location_t;

extern page_t last_page;
extern obj_t next_cons;
extern int new_pages;

void allocate_page(void);
_Bool in_page(obj_t pointer, page_t page);
struct object_location in_heap(obj_t pointer);
cons_t make_cons(obj_t, obj_t);

obj_t car(cons_t);
obj_t cdr(cons_t);
_Bool forwarded(cons_t);
cons_t forwarding(cons_t);
_Bool in_newspace(cons_t);
void set_in_newspace(cons_t, _Bool);

void flip(void);

struct room_data {
  long oldspace_pages;
  long oldspace_bytes;
  long newspace_pages;
  long newspace_bytes;
  long freed_pages;
  long pinned_pages;
};
typedef struct room_data room_t;
room_t room(void);
#endif
