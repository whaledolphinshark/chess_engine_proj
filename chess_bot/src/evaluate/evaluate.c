#include <limits.h>

#include "chess_engine/evaluate.h"
#include "chess_engine/moves.h"
#include "utils/bitboard_util.h"

const int white_piece_values[12] = {10000, 100, 500, 300, 300, 900, -10000, -100, -500, -300, -300, -900};
const int black_piece_values[12] = {-10000, -100, -500, -300, -300, -900, 10000, 100, 500, 300, 300, 900};
const int mobility_values[12] = {0, 0, 15, 15, 15, 0, 0, 0, 15, 15, 15, 0};

int ev_evaluate(_board *board){
    if (board->game_state == DRAW){
        return 0;
    }
    if (board->game_state != ONGOING){
        return INT_MIN + 1;
    }

    int score = 0;
    const int *piece_values = board->turn == WHITE ? white_piece_values : black_piece_values;

    uint64_t pieces = board->board;
    while (pieces != 0){
        int square = bb_pop_lsb(&pieces) - 1;
        _piece piece = board->piece_array[square];
        score += piece_values[piece];
        
        if (piece == W_PAWN || piece == B_PAWN){
            score += 3 * (piece == W_PAWN ? square / 8 : 7 - (square / 8));
        }
    }

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