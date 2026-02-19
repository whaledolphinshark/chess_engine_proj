#ifndef FEN_H
#define FEN_H

#include "chess_types.h"

#define MAX_FEN_LENGTH 90
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

void fn_board_to_fen(_board *board, char *buffer);
void fn_fen_to_board(_board *board, char *fen);
void fn_display_fen(char *fen);

#endif