#ifndef     BOARD_H
#define     BOARD_H

#include "chess_types.h"

#define MAX_FEN_LENGTH 90
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

_board *cb_create_board();
void cb_make_move(_board *board, _move move);
void cb_undo_move(_board *board, _move move);
void cb_print_board(_board *board);
int cb_is_in_check(_board *board);
uint64_t cb_hash_board(_board *board);
void cb_destroy_board(_board *board);
void cb_board_to_fen(_board *board, char *buffer);
void cb_fen_to_board(_board *board, char *fen);
void cb_display_fen(char *fen);

#endif