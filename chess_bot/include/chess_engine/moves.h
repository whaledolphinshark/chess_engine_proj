#ifndef     MOVES_H
#define     MOVES_H

#include "chess_types.h"

int mv_moves_equal(_move left, _move right);
void mv_generate_moves(_board *board);
int mv_is_square_attacked(int square, _board *board, uint64_t occupied, _color side);
_move mv_uci_to_move(char *move_uci, _board *board, int validate, int *valid);
void mv_print_move(_move move, int new_line);
void mv_print_moves(_board *board);

#endif
