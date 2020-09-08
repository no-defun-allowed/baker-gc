#include "allocation-vector.h"
#include <stddef.h>
#define CEIL(x, y) (((x) + (y) - 1) / (y))
allocations_t page_allocation_sizes(uint32_t data_words) {
  /* The allocation bitmap is 1/word-size times the size of the data region. */
  uint32_t bitmap_size = CEIL(data_words, WORD_BITS);
  uint32_t data_size   = data_words - bitmap_size;
  return (allocations_t){bitmap_size, data_size};
}

/* 
   We encode the start of objects in a vector of words, with starts marked as
   set bits, and non-starts as cleared bits. The most-significant bit has the
   lowest address, and the first word has the lowest address.
   So, to decode an address A (relative to the start of the page):
   - we take the word position to be floor(A / word-size), and
   - we take the bit position to be word-size - (A % word-size)
     noting that we have to "reverse" the bit position to get the lowest
     address associated with the most significant bit.
 */
void set_allocation_bit(allocation_bitmap_t bitmap, char* address, char* base) {
  intptr_t offset = (address - base) / WORD_BYTES;
  uint32_t word_offset = offset / WORD_BITS;
  uint32_t bit_offset  = WORD_BITS - 1 - (offset % WORD_BITS);
  bitmap[word_offset] |= (intptr_t)(1) << bit_offset;
}

#define BIT_TEST(word, bit) ((word & ((intptr_t)1 << (bit))) != 0)
/* Find the address of the object start with greatest address represented in a
   word.*/
char* decode_word(char* base, intptr_t word) {
  for (int n = 0; n < WORD_BITS; n++)
    if (BIT_TEST(word, n))
      return base + (WORD_BITS - 1 - n) * WORD_BYTES;
  return NULL;
}

char* closest_previous_bit(allocation_bitmap_t bitmap, char* address, char* base) {
  /* Is it in this word? */
  intptr_t offset = (address - base) / WORD_BYTES;
  uint32_t word_offset = offset / WORD_BITS;
  uint32_t bit_offset  = WORD_BITS - 1 - (offset % WORD_BITS);
  intptr_t this_word = bitmap[word_offset] >> bit_offset;
  if (this_word != 0)
    return decode_word(base + word_offset * WORD_BYTES * WORD_BITS, this_word << bit_offset);
  /* Keep trying bits until we would go past the base word */
  char* this_base; uint32_t this_word_offset;
  for (this_base = base + word_offset - WORD_BYTES,
       this_word_offset = word_offset - 1;
       this_base >= base;
       this_base -= WORD_BYTES, this_word_offset -= 1)
    /* Does an object start in this word? */
    if (bitmap[this_word_offset] != 0)
      return decode_word(this_base, bitmap[this_word_offset]);
  return NULL;
}
