#include "chess_engine/evaluate.h"
#include "utils/bitboard_util.h"

const int white_piece_values[12] = {10000, 100, 500, 300, 300, 900, -10000, -100, -500, -300, -300, -900};
const int black_piece_values[12] = {-10000, -100, -500, -300, -300, -900, 10000, 100, 500, 300, 300, 900};
const int white_mobility_values[12] = {0, 0, 15, 15, 15, 0, 0, 0, -15, -15, -15, 0};
const int black_mobility_values[12] = {0, 0, -15, -15, -15, 0, 0, 0, 15, 15, 15, 0};

int ev_evaluate(_board *board){
    int score = 0;
    const int *piece_values;
    const int *mobility_values;
    if (board->turn == WHITE){
        piece_values = white_piece_values;
        mobility_values = white_mobility_values;
    }
    else{
        piece_values = black_piece_values;
        mobility_values = black_mobility_values;
    }

    uint64_t pieces = board->board;
    while (pieces != 0){
        _piece piece = board->piece_array[bb_pop_lsb(&pieces) - 1];
        score += piece_values[piece];
    }

    for (int i = 0; i < board->move_count; i++){
        _move move = board->move_pool[i];
        score += mobility_values[move.piece];
        if (move.capture != NONE){
            score++;
        }
    }

    return score;
}