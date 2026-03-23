#ifndef     SEARCH_H
#define     SEARCH_H

#include "chess_types.h"

#define DEFAULT_ALPHA INT_MIN + 1
#define DEFAULT_BETA INT_MAX

_move se_search(_board *board, int depth, int alpha, int beta);

#endif