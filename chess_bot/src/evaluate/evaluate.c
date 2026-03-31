#include <limits.h>

#include "chess_engine/evaluate.h"
#include "evaluate_internal.h"
#include "chess_engine/moves.h"
#include "utils/bitboard_util.h"

int ev_evaluate(_board *board){
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

    while (white_pieces != 0){
        int square = bb_pop_lsb(&white_pieces) - 1;
        _piece piece = board->piece_array[square];
        white_score += white_piece_values[piece];
        
        switch(piece){
            case W_PAWN:
                white_score += 3 * (square / 8) + pawn_table[square];
                break;
            case W_QUEEN:
                white_score += queen_table[square];
                break;
            case W_ROOK:
                white_score += rook_table[square];
                break;
            case W_BISHOP:
                white_score += bishop_table[square];
                break;
            case W_KNIGHT:
                white_score += knight_table[square];
                break;
            case W_KING:
                white_score += king_mid_table[square];
                break;
            default:
                break;
        }
    }

    while (black_pieces != 0){
        int square = bb_pop_lsb(&black_pieces) - 1;
        _piece piece = board->piece_array[square];
        black_score += black_piece_values[piece];
        
        switch(piece){
            case B_PAWN:
                black_score += 3 * (7 - (square / 8)) + pawn_table[63 - square];
                break;
            case B_QUEEN:
                black_score += queen_table[63 - square];
                break;
            case B_ROOK:
                black_score += rook_table[63 - square];
                break;
            case B_BISHOP:
                black_score += bishop_table[63 - square];
                break;
            case B_KNIGHT:
                black_score += knight_table[63 - square];
                break;
            case B_KING:
                black_score += king_mid_table[63 - square];
                break;
            default:
                break;
        }
    }

    int score = board->turn == WHITE ? white_score - black_score : black_score - white_score;
    _move moves[MAX_MOVES];
    int move_count;
    mv_generate_moves(board, moves, &move_count);
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