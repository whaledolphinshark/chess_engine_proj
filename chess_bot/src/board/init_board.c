#include <stdlib.h>
#include <time.h>

#include "chess_engine/board.h"
#include "board_internal.h"

uint64_t piece_keys[64][12];
uint64_t black_turn_key;
uint64_t castling_keys[16];
uint64_t en_passant_keys[8];

int cb_ready = 0;

static uint64_t generate_random_uint64(){
    int int_bits = sizeof(int) * 8;
    int calls_needed = 64 / int_bits;

    uint64_t result = 0;
    for (int i = 0; i < calls_needed; i++){
        result |= ((uint64_t) rand()) << (i * int_bits);
    }

    return result;
}

void cb_init_board(){
    if (cb_ready == 1){
        return;
    }

    srand(time(NULL));
    for (int i = 0; i < 64; i++){
        for (int j = 0; j < 12; j++){
            piece_keys[i][j] = generate_random_uint64();
        }
    }

    black_turn_key = generate_random_uint64();

    for (int i = 0; i < 16; i++){
        castling_keys[i] = generate_random_uint64();
    }

    for (int i = 0; i < 8; i++){
        en_passant_keys[i] = generate_random_uint64();
    }

    cb_ready = 1;
}