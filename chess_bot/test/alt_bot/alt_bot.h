#ifndef     ALT_H
#define     ALT_H

#include "chess_engine/chess_types.h"
#include "chess_engine/search.h"

_move alt_search(_board *board, int depth, _search_context *context, _search_stats *stats);

#endif