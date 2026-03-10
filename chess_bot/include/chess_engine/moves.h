#ifndef     MOVES_H
#define     MOVES_H

#include "chess_types.h"

#define MAX_MOVE_BUFFER_SIZE 6

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

int mv_moves_equal(_move left, _move right);
void mv_generate_moves(_board *board);
_move mv_uci_to_move(char *move_uci, _board *board);
void mv_move_to_uci(_move move, char *buffer);
void mv_print_move(_move move, int new_line);
void mv_print_moves(_board *board);

#endif
