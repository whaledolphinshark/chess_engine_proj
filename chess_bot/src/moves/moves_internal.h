#ifndef MOVES_INTERNAL_H
#define MOVES_INTERNAL_H

#include <stdint.h>

#include "chess_engine/moves.h"

void mv_init_moves();
_move mv_init_move(_board *board, int from, int to, _piece promotion);

#endif