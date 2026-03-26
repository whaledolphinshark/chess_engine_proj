#ifndef     SEARCH_H
#define     SEARCH_H

#include "chess_types.h"

#define DEFAULT_ALPHA INT_MIN + 1
#define DEFAULT_BETA INT_MAX

typedef struct _tt_search_entry{
    int eval;
    int depth;
    _move best_move;
} _tt_search_entry;

_move se_search(_board *board, int depth, int alpha, int beta, _transposition_table *tt_table);

#endif