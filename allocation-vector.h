#ifndef ALLOCATION_VECTOR_H
#define ALLOCATION_VECTOR_H

#include <stdint.h>
#define WORD_BITS (sizeof(intptr_t) * 8)
#define WORD_BYTES (sizeof(intptr_t))

struct allocation_sizes {
  uint32_t bitmap_size;
  uint32_t data_size;
};
typedef struct allocation_sizes allocations_t;
typedef intptr_t* allocation_bitmap_t;

allocations_t page_allocation_sizes(uint32_t);
void set_allocation_bit(allocation_bitmap_t, char*, char*);
char* closest_previous_bit(allocation_bitmap_t, char*, char*);

#endif
