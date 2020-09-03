#ifndef SCAN_STACK_H
#define SCAN_STACK_H

#include "pages.h"

typedef void (*cons_consumer_t)(cons_t);

void print_cons(cons_t);
void scan_stack(char*, cons_consumer_t);

extern char* start_of_stack;

#endif
