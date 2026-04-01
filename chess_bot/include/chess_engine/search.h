#ifndef     SEARCH_H
#define     SEARCH_H

#include "chess_types.h"

#define DEFAULT_ALPHA INT_MIN + 1
#define DEFAULT_BETA INT_MAX

typedef enum{
    EXACT = 0,
    LOWER_BOUND = 1,
    UPPER_BOUND = 2
}_tt_flag;

typedef struct _tt_search_entry{
    int eval;
    int depth;
    _move best_move;
    _tt_flag flag;
} _tt_search_entry;

_move se_search(_board *board, int depth, int alpha, int beta, _transposition_table *tt_table);

#endif