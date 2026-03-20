#include <limits.h>

#include "chess_engine/search.h"
#include "chess_engine/board.h"

int search(_board *board, int depth, int alpha, int beta){
    if (depth == 0){
        // return evaluation of position
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