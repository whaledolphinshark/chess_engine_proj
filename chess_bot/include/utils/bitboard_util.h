#ifndef     BITBOARD_UTIL_H
#define     BITBOARD_UTIL_H

#include <stdint.h>

int bb_pop_lsb(uint64_t *restrict mask);
int bb_pop_msb(uint64_t *restrict mask);
int bb_get_lsb(uint64_t mask);
int bb_get_msb(uint64_t mask);
int bb_get_bits_set(uint64_t mask);
void bb_print_bitboard(uint64_t bitboard);

#endif