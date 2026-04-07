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

extern int en_passant_squares[2][8];
// used for detection of when a pawn cannot en passant due to being pinned by a rook or queen
extern uint64_t en_passant_pinned_mask[2][8];

// from northwest clockwise
extern uint64_t rays[64][8];

extern uint64_t perimeter;
extern uint64_t bottom_border;
extern uint64_t top_border;
extern uint64_t left_border;
extern uint64_t right_border;

int mv_moves_equal(const _move left, const _move right);
uint64_t mv_get_semi_legal_rook_moves(_board *restrict board, const int square);
uint64_t mv_get_semi_legal_bishop_moves(_board *restrict board, const int square);
void mv_generate_moves(_board *restrict board, _move move_buffer[restrict MAX_MOVES], int *restrict move_count);
void mv_generate_semi_legal_moves(_board *restrict board, _move move_buffer[restrict MAX_MOVES], int *restrict move_count);
void mv_generate_enemy_moves(_board *restrict board, _move move_buffer[restrict MAX_MOVES], int *restrict move_count);
int mv_has_moves(_board *restrict board);
_move mv_uci_to_move(char *move_uci, _board *board);
void mv_move_to_uci(_move move, char *buffer);
void mv_print_move(_move move, int new_line);
void mv_print_moves(_board *board);

#endif
