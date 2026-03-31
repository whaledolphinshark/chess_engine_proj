#ifndef     EVALUATE_INTERNAL_H
#define     EVALUATE_INTERNAL_H

#include "chess_engine/evaluate.h"

const int piece_values[12] = {10000, 100, 500, 300, 300, 900, 10000, 100, 500, 300, 300, 900};
const int white_piece_values[12] = {10000, 100, 500, 300, 300, 900, -10000, -100, -500, -300, -300, -900};
const int black_piece_values[12] = {-10000, -100, -500, -300, -300, -900, 10000, 100, 500, 300, 300, 900};
const int mobility_values[12] = {0, 0, 15, 15, 15, 0, 0, 0, 15, 15, 15, 0};

int ev_evaluate(_board *board);

#endif