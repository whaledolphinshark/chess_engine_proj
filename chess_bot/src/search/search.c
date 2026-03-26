#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "chess_engine/search.h"
#include "chess_engine/board.h"
#include "chess_engine/evaluate.h"
#include "chess_engine/transposition_table.h"
#include "utils/error_handling.h"

static int search(_board *board, int depth, int alpha, int beta, _transposition_table *transposition_table){
    if (depth == 0){
        return ev_evaluate(board);
    }

    if (tt_is_key_in_table(transposition_table, board->zobrist_hash) == 1){
        _tt_search_entry *entry = (_tt_search_entry *)tt_get_item(transposition_table, board->zobrist_hash);
        if (entry->depth >= depth){
            return entry->eval;
        }
    }

    int best_score = INT_MIN + 1;
    _move best_move = board->move_pool[0];
    for (int i = 0; i < board->move_count; i++){
        _move move = board->move_pool[i];
        cb_make_move(board, move);
        int score = -search(board, depth - 1, -beta, -alpha, transposition_table);
        cb_undo_move(board);

        if (score > best_score){
            best_score = score;
            best_move = move;
            if (score > alpha){
                alpha = score;
            }
        }
        if (score >= beta){
            // maybe update transposition table?
            return best_score;
        }
    }

    if (tt_is_key_in_table(transposition_table, board->zobrist_hash) == 0 || ((_tt_search_entry *)tt_get_item(transposition_table, board->zobrist_hash))->depth < depth){
        _tt_search_entry entry = {best_score, depth, best_move};
        tt_insert_item(transposition_table, board->zobrist_hash, &entry);
    }

    return best_score;
}

_move se_search(_board *board, int depth, int alpha, int beta, _transposition_table *transposition_table){
    if (transposition_table == NULL || tt_get_item_size(transposition_table) != sizeof(_tt_search_entry)){
        eh_die("passed in invalid transposition table");
    }

    search(board, depth, alpha, beta, transposition_table);

    return ((_tt_search_entry *)tt_get_item(transposition_table, board->zobrist_hash))->best_move;
    // int best_score = INT_MIN;
    // _move best_move = board->move_pool[0];
    // for (int i = 0; i < board->move_count; i++){
    //     _move move = board->move_pool[i];
    //     cb_make_move(board, move);
    //     int score = -search(board, depth - 1, -beta, -alpha, transposition_table);
    //     cb_undo_move(board);

    //     if (score > best_score){
    //         best_score = score;
    //         best_move = move;
    //         if (score > alpha){
    //             alpha = score;
    //         }
    //     }
    // }
}