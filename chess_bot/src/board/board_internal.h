#ifndef BOARD_INTERNAL_H
#define BOARD_INTERNAL_H

#include <stdint.h>

extern uint64_t piece_keys[64][12];
extern uint64_t black_turn_key;
extern uint64_t castling_keys[16];
extern uint64_t en_passant_keys[8];

extern int cb_ready;

void cb_init_board();

#endif