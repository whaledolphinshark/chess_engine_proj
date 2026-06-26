#ifndef     EVALUATE_H
#define     EVALUATE_H

#include "chess_types.h"

#define TOTAL_PHASE_VALUE 24
#define MATE_SCORE -5000000

extern const int piece_phase_value[12];
extern const int piece_values[12];
extern const int white_piece_values[12];
extern const int black_piece_values[12];
extern const int mobility_values[12];
extern const int pawn_table[64];
extern const int queen_table[64];
extern const int rook_table[64];
extern const int bishop_table[64];
extern const int knight_table[64];
extern const int king_mid_table[64];
extern const int king_end_table[64];

int ev_non_pawn_material(_board *board);
int ev_evaluate(_board *board);

#endif