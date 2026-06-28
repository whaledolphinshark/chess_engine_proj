#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "chess_engine/search.h"
#include "chess_engine/board.h"
#include "chess_engine/moves.h"
#include "chess_engine/evaluate.h"
#include "chess_engine/transposition_table.h"
#include "utils/error_handling.h"

#define MAX_HISTORY 65536

typedef struct _timer{
    const clock_t start;
    const double max_time;
    int current_depth;
    const int min_depth;
}_timer;

typedef struct _node_info{
    int pv;
    int null_move;
}_node_info;

static inline double time_elapsed(_timer timer){
    return (double)(clock() - timer.start) / CLOCKS_PER_SEC;
}

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
    _move hash_move = {0, 0, NONE, NONE, NONE, NORMAL};
    if (tt_contains_key(context->table, hash) == 1){
        hash_move = ((_tt_search_entry *)tt_get_item(context->table, hash))->best_move;
    }
    
    int temp_val;
    _move temp_move;
    for (int i = 0; i < move_count; i++){
        values[i] = 0;
        _move move = moves[i];
        values[i] += (mv_moves_equal(hash_move, move) == 1) * 60000;
        if (move.capture != NONE){
            values[i] += piece_values[move.capture] - piece_values[move.piece] + 10000;
        }
        else{
            values[i] += context->history[move.piece][move.from][move.to];
        }
        values[i] += (move.special_move == PROMOTION) * 5000;

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

static int quiescence_search(_board *board, int alpha, int beta, _search_context *context, _search_stats *stats, _timer timer){
    stats->quiescent_nodes_visited++;
    int best_score = ev_evaluate(board);
    if ((timer.current_depth > timer.min_depth && time_elapsed(timer) > timer.max_time) || board->game_state != ONGOING || (board->in_check == 0 && best_score >= beta)){
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
        score = -quiescence_search(board, -beta, -alpha, context, stats, timer);
        cb_undo_move(board);

        if (timer.current_depth > timer.min_depth && time_elapsed(timer) > timer.max_time){
            break;
        }

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

static int search(_board *board, const int depth, int alpha, const int beta, const _node_info info, _search_context *context, _search_stats *stats, const _timer timer){
    stats->nodes_visited++;
    if ((timer.current_depth > timer.min_depth && time_elapsed(timer) > timer.max_time) || board->game_state != ONGOING){
        return ev_evaluate(board);
    }
    if (depth <= 0){
        return quiescence_search(board, alpha, beta, context, stats, timer);
    }

    if (info.pv == 0 && tt_contains_key(context->table, board->zobrist_hash) == 1){
        _tt_search_entry *entry = (_tt_search_entry *)tt_get_item(context->table, board->zobrist_hash);
        if (entry->depth >= depth && (entry->flag == EXACT || (entry->flag == LOWER_BOUND && entry->eval >= beta) || (entry->flag == UPPER_BOUND && entry->eval <= alpha))){
            return entry->eval;
        }
    }

    // null move pruning
    const int endgame = ev_non_pawn_material(board) <= 2 * piece_values[W_ROOK] + piece_values[W_BISHOP] ? 1 : 0;
    const int static_score = ev_evaluate(board);
    if (static_score >= beta && depth > 3 && board->in_check == 0 && endgame == 0 && info.pv == 0 && info.null_move == 1){
        // make null move
        cb_make_move(board, (_move)NULL_MOVE);
        const int score = -search(board, depth - 3, -beta, -beta + 1, (_node_info){0, 0}, context, stats, timer);
        // undo null move
        cb_undo_move(board);
        if (score >= beta){
            return score;
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
        if (i == 0 && info.pv == 1){
            score = -search(board, depth - 1, -beta, -alpha, (_node_info){1, 1}, context, stats, timer);
        }
        else{
            score = -search(board, depth - depth_reduction, -alpha - 1, -alpha, (_node_info){0, 1}, context, stats, timer);
            if (info.pv == 1 && score > alpha){
                stats->researches++;
                score = -search(board, depth - 1, -beta, -alpha, (_node_info){1, 1}, context, stats, timer);
                if (score <= alpha){
                    stats->research_fail_low++;
                }
            }
        }
        cb_undo_move(board);

        if (timer.current_depth > timer.min_depth && time_elapsed(timer) > timer.max_time){
            break;
        }

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

    // only save entry if we still have time remaining
    // else we may end up saving a bad entry
    if ((timer.current_depth <= timer.min_depth || time_elapsed(timer) < timer.max_time) && 
            (tt_contains_key(context->table, board->zobrist_hash) == 0 || ((_tt_search_entry *)tt_get_item(context->table, board->zobrist_hash))->depth < depth)){
        _tt_search_entry entry = {best_score, depth, best_move, flag};
        tt_insert_item(context->table, board->zobrist_hash, &entry);
    }

    return best_score;
}

_move se_search(_board *board, int depth, double time, _search_context *context, _search_stats *stats){
    if (board == NULL || context == NULL){
        eh_die("passed in null pointer");
    }
    if (tt_get_item_size(context->table) != sizeof(_tt_search_entry)){
        eh_die("context initialized incorrectly");
    }

    _search_stats placeholder = {0, 0, 0, 0, 0};
    if (stats == NULL){
        stats = &placeholder;
    }

    FILE *fptr = fopen("output.txt", "a");
    char temp_buffer[MAX_FEN_LENGTH];
    cb_board_to_fen(board, temp_buffer);
    fprintf(fptr, "%s\n", temp_buffer);
    fflush(fptr);

    int current_depth = 1;
    clock_t start = clock();
    _timer timer = {start, time, current_depth, depth};
    _node_info info = {1, 1};
    while (current_depth <= depth || time_elapsed(timer) < time){
        search(board, current_depth, DEFAULT_ALPHA, DEFAULT_BETA, info, context, stats, timer);
        _tt_search_entry *entry = (_tt_search_entry *)tt_get_item(context->table, board->zobrist_hash);
        fprintf(fptr, "depth: %d, time: %f, game state: %d, min depth: %d, max time: %f, nodes visited: %d, entry depth: %d, entry eval: %d\n", current_depth, time_elapsed(timer), board->game_state, timer.min_depth, timer.max_time, stats->nodes_visited, entry->depth, entry->eval);
        fflush(fptr);
        stats->nodes_visited = 0;
        current_depth++;
        timer.current_depth = current_depth;
    }

    fclose(fptr);

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