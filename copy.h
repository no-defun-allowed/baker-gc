#ifndef COPY_H
#define COPY_H

#include <stddef.h>
#include <stdbool.h>
#include "pages.h"
#include "scan-stack.h"

enum gc_state { STOPPED, COPYING, CLEARING_NEWSPACE_BITS };

extern _Bool disable_gc;
obj_t copy(obj_t);

void gc_setup(void);
void gc_work(int);
cons_t cons(obj_t, obj_t);
extern int steps_per_cons;
extern int threshold_pages;
#endif
