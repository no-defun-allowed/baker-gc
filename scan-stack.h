#ifndef SCAN_STACK_H
#define SCAN_STACK_H

#include "pages.h"

typedef void (*obj_consumer_t)(obj_t, page_t);

void print_cons(obj_t, page_t);
void scan_stack(char*, obj_consumer_t);

extern char* start_of_stack;

#endif
