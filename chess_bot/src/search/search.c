#include <limits.h>
#include <stdlib.h>
#include <time.h>

#include "chess_engine/search.h"
#include "chess_engine/board.h"
#include "chess_engine/moves.h"
#include "chess_engine/evaluate.h"
#include "chess_engine/transposition_table.h"
#include "utils/error_handling.h"

#define MAX_HISTORY 65536

static void update_history(const int bonus, const int index, _move moves[restrict MAX_MOVES], _search_context *restrict context){
    _move move = moves[index];
    context->history[move.piece][move.from][move.to] += bonus - (context->history[move.piece][move.from][move.to] * bonus) / MAX_HISTORY;
    for (int j = 0; j < index; j++){
        _move temp = moves[j];
        if (temp.capture == NONE){
            context->history[temp.piece][temp.from][temp.to] -= bonus - (context->history[temp.piece][temp.from][temp.to] * bonus) / MAX_HISTORY;
        }
    }
}

static void order_moves(_board *restrict board, _move moves[restrict MAX_MOVES], int values[restrict MAX_MOVES], const int move_count, _search_context *restrict context){
    const uint64_t hash = board->zobrist_hash;
    _move best_move = {0, 0, NONE, NONE, NONE, NORMAL};
    if (tt_contains_key(context->table, hash) == 1){
        best_move = ((_tt_search_entry *)tt_get_item(context->table, hash))->best_move;
    }
    
    // int values[move_count];
    for (int i = 0; i < move_count; i++){
        values[i] = 0;
        _move move = moves[i];
        if (mv_moves_equal(best_move, move) == 1){
            values[i] += 60000;
        }
        if (move.capture != NONE){
            values[i] += piece_values[move.capture] - piece_values[move.piece] + 10000;
        }
        if (move.special_move == PROMOTION){
            values[i] += 5000;
        }
        if (move.capture == NONE){
            values[i] += context->history[move.piece][move.from][move.to];
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

static int quiescence_search(_board *board, int alpha, int beta, _search_context *context, _search_stats *stats){
    stats->quiescent_nodes_visited++;
    int best_score = ev_evaluate(board);
    if (board->game_state != ONGOING || (board->in_check == 0 && best_score >= beta)){
        return best_score;
    }
    if (best_score > alpha){
        alpha = best_score;
    }

    _move moves[MAX_MOVES];
    int values[MAX_MOVES];
    int move_count;
    mv_generate_moves(board, moves, &move_count);
    order_moves(board, moves, values, move_count, context);
    int score = best_score;
    for (int i = 0; i < move_count; i++){
        _move move = moves[i];
        if (board->in_check == 0 && move.capture == NONE && move.special_move != PROMOTION){
            continue;
        }

        cb_make_move(board, move);
        score = -quiescence_search(board, -beta, -alpha, context, stats);
        cb_undo_move(board);

        if (score > best_score){
            best_score = score;
            if (score >= beta){
                break;
            }
            if (score > alpha){
                alpha = score;
            }
        }
    }

    return best_score;
}

static int search(_board *board, int depth, int alpha, int beta, int pv, _search_context *context, _search_stats *stats){
    stats->nodes_visited++;
    if (board->game_state != ONGOING){
        return ev_evaluate(board);
    }
    if (depth <= 0){
        return quiescence_search(board, alpha, beta, context, stats);
    }

    if (tt_contains_key(context->table, board->zobrist_hash) == 1){
        _tt_search_entry *entry = (_tt_search_entry *)tt_get_item(context->table, board->zobrist_hash);
        if (entry->depth >= depth && (entry->flag == EXACT || (entry->flag == LOWER_BOUND && entry->eval >= beta) || (entry->flag == UPPER_BOUND && entry->eval <= alpha))){
            return entry->eval;
        }
    }
    
    _move moves[MAX_MOVES];
    int values[MAX_MOVES];
    int move_count;
    mv_generate_moves(board, moves, &move_count);
    order_moves(board, moves, values, move_count, context);
    int best_score = INT_MIN + 1;
    _move best_move = moves[0];
    _tt_flag flag = UPPER_BOUND;
    for (int i = 0; i < move_count; i++){
        int score;
        _move move = moves[i];
        int depth_reduction = 1;
        if (depth > 2){
            if (values[i] <= 0){
                depth_reduction++;
            }
            if (2 * i > move_count){
                depth_reduction++;
            }
        }

        cb_make_move(board, move);
        if (i == 0 && pv == 1){
            score = -search(board, depth - 1, -beta, -alpha, 1, context, stats);
        }
        else{
            score = -search(board, depth - depth_reduction, -alpha - 1, -alpha, 0, context, stats);
            if (pv == 1 && score > alpha){
                stats->researches++;
                score = -search(board, depth - 1, -beta, -alpha, 1, context, stats);
                if (score <= alpha){
                    stats->research_fail_low++;
                }
            }
        }
        cb_undo_move(board);

        if (score > best_score){
            best_score = score;
            best_move = move;
            if (score >= beta){
                flag = LOWER_BOUND;
                if (move.capture == NONE){
                    const int bonus = depth * depth < MAX_HISTORY ? depth * depth : MAX_HISTORY;
                    update_history(bonus, i, moves, context);
                }
                break;
            }
            if (score > alpha){
                flag = EXACT;
                alpha = score;
            }
        }
    }

    if (tt_contains_key(context->table, board->zobrist_hash) == 0 || ((_tt_search_entry *)tt_get_item(context->table, board->zobrist_hash))->depth < depth){
        _tt_search_entry entry = {best_score, depth, best_move, flag};
        tt_insert_item(context->table, board->zobrist_hash, &entry);
    }

    return best_score;
}

_move se_search(_board *board, int depth, double time, _search_context *context, _search_stats *stats){
    if (board == NULL || stats == NULL){
        eh_die("passed in null pointer");
    }
    if (context->table == NULL){
        eh_die("context not initialized");
    }
    if (tt_get_item_size(context->table) != sizeof(_tt_search_entry)){
        eh_die("context initialized incorrectly");
    }

    int i = 1;
    clock_t start = clock();
    while (i <= depth){
        search(board, i, DEFAULT_ALPHA, DEFAULT_BETA, 1, context, stats);
        i++;
    }

    while ((double)(clock() - start) / CLOCKS_PER_SEC < time){
        search(board, i, DEFAULT_ALPHA, DEFAULT_BETA, 1, context, stats);
        i++;
    }

    for (int i = 0; i < 12; i++){
        for (int j = 0; j < 64; j++){
            for (int k = 0; k < 64; k++){
                context->history[i][j][k] /= 2;
            }
        }
    }

    _tt_search_entry *entry = (_tt_search_entry *)tt_get_item(context->table, board->zobrist_hash);
    stats->eval = entry->eval;
    return entry->best_move;
}

_search_context *se_init_search_context(){
    _search_context *context = malloc(sizeof(_search_context));
    if (context == NULL){
        eh_die("malloc() failed");
    }
    context->table = tt_create_transposition_table(sizeof(_tt_search_entry));
    for (int i = 0; i < 12; i++){
        for (int j = 0; j < 64; j++){
            for (int k = 0; k < 64; k++){
                context->history[i][j][k] = 0;
            }
        }
    }

    return context;
}

void se_destroy_search_context(_search_context *context){
    tt_destroy_transposition_table(context->table);
    free(context);
}