#include "chess_engine/evaluate.h"
#include "utils/bitboard_util.h"

const int piece_values[12] = {10000, 100, 500, 300, 300, 900, 10000, 100, 500, 300, 300, 900};

int ev_evaluate(_board *board){
    int score = 0;
    uint64_t pieces = board->turn == WHITE ? board->white_pieces : board->black_pieces;

    while (pieces != 0){
        _piece piece = board->piece_array[bb_pop_lsb(&pieces) - 1];
        score += piece_values[piece];
    }

    for (int i = 0; i < board->move_count; i++){
        _move move = board->move_pool[i];
        switch (move.piece){
            case W_ROOK:
            case B_ROOK:
            case W_KNIGHT:
            case B_KNIGHT:
            case W_BISHOP:
            case B_BISHOP:
                score += 15;
            default:
                break;
        }

        if (move.capture != NONE){
            score++;
        }
    }

    return score;
}