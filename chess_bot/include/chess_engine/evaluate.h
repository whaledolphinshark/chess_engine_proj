#ifndef     EVALUATE_H
#define     EVALUATE_H

#include "chess_types.h"

extern const int piece_values[12];
extern const int white_piece_values[12];
extern const int black_piece_values[12];
extern const int mobility_values[12];

int ev_evaluate(_board *board);

#endif