#ifndef MOVES_INTERNAL_H
#define MOVES_INTERNAL_H

#include <stdint.h>

#include "chess_engine/moves.h"

extern uint64_t rook_masks[64];
extern uint64_t bishop_masks[64];
extern int rook_shifts[64];
extern int bishop_shifts[64];

extern uint64_t rook_magics[64];
extern uint64_t bishop_magics[64];

extern uint64_t pawn_attacks[2][64];
extern uint64_t rook_attacks[64][4096];
extern uint64_t bishop_attacks[64][512];
extern uint64_t knight_attacks[64];
extern uint64_t king_attacks[64];

extern uint64_t pawn_moves[2][64];
extern uint64_t castle_moves[2][4];

// from northwest clockwise
extern uint64_t rays[64][8];

extern uint64_t perimeter;
extern uint64_t bottom_border;
extern uint64_t top_border;
extern uint64_t left_border;
extern uint64_t right_border;

extern int mv_ready;

void mv_init_moves();
void mv_add_move(_board *board, int from, int to, _piece promotion);
void mv_clear_moves(_board *board);

#endif