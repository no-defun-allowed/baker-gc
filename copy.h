#ifndef COPY_H
#define COPY_H

#include <stddef.h>
#include <stdbool.h>
#include "pages.h"
#include "scan-stack.h"

extern _Bool disable_gc;
cons_t copy(cons_t);

void gc_setup(void);
void gc_work(int);
cons_t cons(cons_t, cons_t);
extern int steps_per_cons;
#endif
