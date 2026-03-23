#include <limits.h>
#include <stdio.h>

#include "chess_engine/search.h"
#include "chess_engine/board.h"
#include "chess_engine/evaluate.h"

static int search(_board *board, int depth, int alpha, int beta){
    if (depth == 0){
        return ev_evaluate(board);
    }

    int best = INT_MIN;
    for (int i = 0; i < board->move_count; i++){
        cb_make_move(board, board->move_pool[i]);
        int score = -search(board, depth - 1, -beta, -alpha);
        cb_undo_move(board);

        if (score > best){
            best = score;
            if (score > alpha){
                alpha = score;
            }
        }
        if (score >= beta){
            return best;
        }
    }

    return best;
}

_move se_search(_board *board, int depth, int alpha, int beta){
    int best_score = INT_MIN;
    _move best_move = board->move_pool[0];
    for (int i = 0; i < board->move_count; i++){
        _move move = board->move_pool[i];
        cb_make_move(board, move);
        int score = -search(board, depth - 1, -beta, -alpha);
        cb_undo_move(board);

        if (score > best_score){
            best_score = score;
            best_move = move;
            if (score > alpha){
                alpha = score;
            }
        }
    }

    return best_move;
}