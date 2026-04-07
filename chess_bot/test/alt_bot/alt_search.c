#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "alt_bot.h"
#include "chess_engine/board.h"
#include "chess_engine/moves.h"
#include "chess_engine/transposition_table.h"
#include "utils/bitboard_util.h"
#include "utils/error_handling.h"

#define MAX_HISTORY 65536
#define TOTAL_PHASE_VALUE 24

static const int piece_phase_value[12] = {0, 0, 2, 1, 1, 4, 0, 0, 2, 1, 1, 4};
static const int piece_values[12] = {10000, 100, 500, 320, 330, 900, 10000, 100, 500, 320, 330, 900};
static const int white_piece_values[12] = {10000, 100, 500, 320, 330, 900, -10000, -100, -500, -320, -330, -900};
static const int black_piece_values[12] = {-10000, -100, -500, -320, -330, -900, 10000, 100, 500, 320, 330, 900};
static const int mobility_values[12] = {0, 0, 5, 5, 5, 0, 0, 0, 5, 5, 5, 0};
// static const int mobility_values[12] = {0, 0, 3, 5, 5, 0, 0, 0, 3, 5, 5, 0};
static const int pawn_table[64] = {0, 0, 0, 0, 0, 0, 0, 0, 5, 10, 10, -20, -20, 10, 10, 5, 5, -5, -10, 0, 0, -10, -5, 5, 0, 0, 0, 20, 20, 0, 0, 0, 5, 5, 10, 25, 25, 10, 5, 5, 10, 10, 20, 30, 30, 20, 10, 10, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0};
static const int queen_table[64] = {-20, -10, -10, -5, -5, -10, -10, -20, -10, 0, 5, 0, 0, 0, 0, -10, -10, 5, 5, 5, 5, 5, 0, -10, 0, 0, 5, 5, 5, 5, 0, -5, -5, 0, 5, 5, 5, 5, 0, -5, -10, 0, 5, 5, 5, 5, 0, -10, -10, 0, 0, 0, 0, 0, 0, -10, -20, -10, -10, -5, -5, -10, -10, -20};
static const int rook_table[64] = {0, 0, 0, 5, 5, 0, 0, 0, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, 5, 10, 10, 10, 10, 10, 10, 5, 0, 0, 0, 0, 0, 0, 0, 0};
static const int bishop_table[64] = {-20, -10, -10, -10, -10, -10, -10, -20, -10, 5, 0, 0, 0, 0, 5, -10, -10, 10, 10, 10, 10, 10, 10, -10, -10, 0, 10, 10, 10, 10, 0, -10, -10, 5, 5, 10, 10, 5, 5, -10, -10, 0, 5, 10, 10, 5, 0, -10, -10, 0, 0, 0, 0, 0, 0, -10, -20, -10, -10, -10, -10, -10, -10, -20};
static const int knight_table[64] = {-50, -40, -30, -30,- 30, -30, -40, -50, -40, -20, 0, 5, 5, 0, -20, -40, -30, 5, 10, 15, 15, 10, 5, -30, -30, 0, 15, 20, 20, 15, 0, -30, -30, 5, 15, 20, 20, 15, 5, -30, -30, 0, 10, 15, 15, 10, 0, -30, -40, -20, 0, 0, 0, 0, -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};
static const int king_mid_table[64] = {20, 30, 10, 0, 0, 10, 30, 20, 20, 20, 0, 0, 0, 0, 20, 20, -10, -20, -20, -20, -20, -20, -20, -10, -20, -30, -30, -40, -40, -30, -30, -20, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30};
static const int king_end_table[64] = {-50, -30, -30, -30, -30, -30, -30, -50, -30, -30, 0, 0, 0, 0, -30, -30, -30, -10, 20, 30, 30, 20, -10, -30, -30, -10, 30, 40, 40, 30, -10, -30, -30, -10, 30, 40, 40, 30, -10, -30, -30, -10, 20, 30, 30, 20, -10, -30, -30, -20, -10, 0, 0, -10, -20, -30, -50, -40, -30, -20, -20, -30, -40, -50};

static int evaluate(_board *board){
    if (board->game_state == DRAW){
        return 0;
    }
    if (board->game_state != ONGOING){
        return INT_MIN + 1;
    }

    int white_score = 0;
    int black_score = 0;
    uint64_t white_pieces = board->white_pieces;
    uint64_t black_pieces = board->black_pieces;
    int phase_value = 0;
    int black_king_square = 0;
    int white_king_square = 0;
    uint64_t white_control = 0;
    uint64_t black_control = 0;

    while (white_pieces != 0){
        int square = bb_pop_lsb(&white_pieces) - 1;
        _piece piece = board->piece_array[square];
        white_score += white_piece_values[piece];
        phase_value += piece_phase_value[piece];
        
        switch(piece){
            case W_PAWN:
                white_score += 3 * (square / 8) + pawn_table[square];
                white_control |= pawn_attacks[WHITE][square];
                break;
            case W_QUEEN:
                white_score += queen_table[square];
                break;
            case W_ROOK:
                white_score += rook_table[square];
                white_control |= mv_get_semi_legal_rook_moves(board, square);
                break;
            case W_BISHOP:
                white_score += bishop_table[square];
                white_control |= mv_get_semi_legal_bishop_moves(board, square);
                break;
            case W_KNIGHT:
                white_score += knight_table[square];
                white_control |= knight_attacks[square];
                break;
            case W_KING:
                white_king_square = square;
                break;
            default:
                break;
        }
    }

    while (black_pieces != 0){
        int square = bb_pop_lsb(&black_pieces) - 1;
        _piece piece = board->piece_array[square];
        black_score += black_piece_values[piece];
        phase_value += piece_phase_value[piece];
        
        switch(piece){
            case B_PAWN:
                black_score += 3 * (7 - (square / 8)) + pawn_table[63 - square];
                black_control |= pawn_attacks[BLACK][square];
                break;
            case B_QUEEN:
                black_score += queen_table[63 - square];
                break;
            case B_ROOK:
                black_score += rook_table[63 - square];
                black_control |= mv_get_semi_legal_rook_moves(board, square);
                break;
            case B_BISHOP:
                black_score += bishop_table[63 - square];
                black_control |= mv_get_semi_legal_bishop_moves(board, square);
                break;
            case B_KNIGHT:
                black_score += knight_table[63 - square];
                black_control |= knight_attacks[square];
                break;
            case B_KING:
                black_king_square = square;
                break;
            default:
                break;
        }
    }

    white_score += bb_get_bits_set(white_control);
    black_score += bb_get_bits_set(black_control);

    int endgame_value = king_end_table[white_king_square];
    white_score += (king_mid_table[white_king_square] - endgame_value) * phase_value / TOTAL_PHASE_VALUE + endgame_value;
    endgame_value = king_end_table[63 - black_king_square];
    black_score += (king_mid_table[63 - black_king_square] - endgame_value) * phase_value / TOTAL_PHASE_VALUE + endgame_value;

    // int score = 10 + (board->turn == WHITE ? white_score - black_score : black_score - white_score);
    int score = board->turn == WHITE ? white_score - black_score : black_score - white_score;
    if (board->in_check == 1){
        score -= 30;
    }
    _move moves[MAX_MOVES];
    int move_count;
    mv_generate_semi_legal_moves(board, moves, &move_count);
    for (int i = 0; i < move_count; i++){
        _move move = moves[i];
        score += mobility_values[move.piece];
        if (move.capture != NONE){
            score++;
        }
    }
    
    mv_generate_enemy_moves(board, moves, &move_count);
    for (int i = 0; i < move_count; i++){
        _move move = moves[i];
        score -= mobility_values[move.piece];
        if (move.capture != NONE){
            score--;
        }
    }

    return score;
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
    int best_score = evaluate(board);
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
        return evaluate(board);
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
        if (depth > 2 && values[i] <= 0){
            depth_reduction++;
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

_move alt_search(_board *board, int depth, _search_context *context, _search_stats *stats){
    if (board == NULL || stats == NULL){
        eh_die("passed in null pointer");
    }
    if (context->table == NULL){
        eh_die("context not initialized");
    }
    if (tt_get_item_size(context->table) != sizeof(_tt_search_entry)){
        eh_die("context initialized incorrectly");
    }

    for (int i = 1; i <= depth; i++){
        search(board, i, DEFAULT_ALPHA, DEFAULT_BETA, 1, context, stats);
    }
    // for (int i = 2; i <= depth; i += 2){
    //     search(board, i, DEFAULT_ALPHA, DEFAULT_BETA, 1, context, stats);
    // }

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