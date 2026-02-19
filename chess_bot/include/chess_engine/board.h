#ifndef     BOARD_H
#define     BOARD_H

#include "chess_types.h"

_board *cb_create_board();
void cb_make_move(_board *board, _move move);
void cb_undo_move(_move move, _board *board);
void cb_print_board(_board *board);
int cb_is_in_check(_board *board);
uint64_t cb_hash_board(_board *board);
void cb_destroy_board(_board *board);

#endif