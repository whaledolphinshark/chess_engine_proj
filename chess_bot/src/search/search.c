#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "chess_engine/search.h"
#include "chess_engine/board.h"
#include "chess_engine/moves.h"
#include "chess_engine/evaluate.h"
#include "chess_engine/transposition_table.h"
#include "utils/error_handling.h"

static void order_moves(_board *restrict board, _move moves[restrict MAX_MOVES], const int move_count, _transposition_table *restrict transposition_table){
    const uint64_t hash = board->zobrist_hash;
    _move best_move = {0, 0, NONE, NONE, NONE, NORMAL};
    if (transposition_table != NULL && tt_is_key_in_table(transposition_table, hash) == 1){
        best_move = ((_tt_search_entry *)tt_get_item(transposition_table, hash))->best_move;
    }
    
    int values[move_count];
    for (int i = 0; i < move_count; i++){
        values[i] = 0;
        _move move = moves[i];
        if (mv_moves_equal(best_move, move) == 1){
            values[i] += 60000;
        }
        if (move.capture != NONE){
            values[i] += piece_values[move.capture] - piece_values[move.piece] + 10000;
        }
    }

    int temp_val;
    _move temp_move;
    for (int i = 0; i < move_count; i++){
        int j = i;
        while (j > 0 && values[j] > values[j - 1]){
            temp_val = values[j - 1];
            values[j - 1] = values[j];
            values[j] = temp_val;
            temp_move = moves[j - 1];
            moves[j - 1] = moves[j];
            moves[j] = temp_move;
            j--;
        }
    }
}

static int quiescence_search(_board *board, int alpha, int beta){
    int best_score = ev_evaluate(board);
    if (board->game_state != ONGOING || best_score >= beta){
        return best_score;
    }

    _move moves[MAX_MOVES];
    int move_count;
    mv_generate_moves(board, moves, &move_count);
    order_moves(board, moves, move_count, NULL);
    int score = best_score;
    for (int i = 0; i < move_count; i++){
        _move move = moves[i];
        if (move.capture == NONE && move.special_move != PROMOTION){
            continue;
        }

        cb_make_move(board, move);
        score = -quiescence_search(board, -beta, -alpha);
        cb_undo_move(board);

        if (score > best_score){
            best_score = score;
            if (score > alpha){
                alpha = score;
            }
        }

        if (alpha >= beta){
            break;
        }
    }

    return best_score;
}

static int search(_board *board, int depth, int alpha, int beta, _transposition_table *transposition_table){
    if (board->game_state != ONGOING){
        return ev_evaluate(board);
    }
    if (depth == 0){
        return quiescence_search(board, alpha, beta);
    }

    if (tt_is_key_in_table(transposition_table, board->zobrist_hash) == 1){
        _tt_search_entry *entry = (_tt_search_entry *)tt_get_item(transposition_table, board->zobrist_hash);
        if (entry->depth >= depth){
            return entry->eval;
        }
    }

    _move moves[MAX_MOVES];
    int move_count;
    mv_generate_moves(board, moves, &move_count);
    order_moves(board, moves, move_count, transposition_table);
    int best_score = INT_MIN + 1;
    _move best_move = moves[0];
    for (int i = 0; i < move_count; i++){
        _move move = moves[i];
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
        // maybe i can somehow put this in the first if condition?
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
    if (board == NULL || transposition_table == NULL){
        eh_die("passed in null pointer");
    }
    if (tt_get_item_size(transposition_table) != sizeof(_tt_search_entry)){
        eh_die("passed in invalid transposition table");
    }

    search(board, depth, alpha, beta, transposition_table);

    return ((_tt_search_entry *)tt_get_item(transposition_table, board->zobrist_hash))->best_move;
}