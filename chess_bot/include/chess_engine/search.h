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

typedef struct _search_stats{
    int eval;
    int researches;
    int research_fail_low;
    int nodes_visited;
    int quiescent_nodes_visited;
} _search_stats;

typedef struct _search_context{
    _transposition_table *table;
    int history[12][64][64];
} _search_context;

_move se_search(_board *board, int depth, int alpha, int beta, _search_context *context, _search_stats *stats);
void se_init_search_context(_search_context *context);
void se_destroy_search_context(_search_context *context);

#endif