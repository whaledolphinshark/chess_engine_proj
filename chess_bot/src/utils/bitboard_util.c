#include <string.h>
#include <stdio.h>

#include "utils/bitboard_util.h"

int bb_pop_lsb(uint64_t * restrict mask){
    if (*mask == 0){
        return 0;
    }

    int lsb = __builtin_ffsl(*mask);
    *mask &= *mask - 1;
    return lsb;
}

int bb_pop_msb(uint64_t * restrict mask){
    if (*mask == 0){
        return 0;
    }

    int msb = 64 - __builtin_clzl(*mask);
    *mask &= ~(1UL << (msb - 1));
    return msb;
}

int bb_get_lsb(uint64_t mask){
    return __builtin_ffsl(mask);
}

int bb_get_msb(uint64_t mask){
    return mask == 0 ? 0 : 64 - __builtin_clzl(mask);
}

int bb_get_bits_set(uint64_t mask){
    return __builtin_popcountl(mask);
}

void bb_print_bitboard(uint64_t bitboard){
    for (int i = 56; i >= 0; i -= 8){
        for (int j = 0; j < 8; j++){
            if ((bitboard & (1UL << (i + j))) != 0){
                printf("1");
            }
            else{
                printf("0");
            }
        }
        printf("\n");
    }
    printf("\n\n");
}