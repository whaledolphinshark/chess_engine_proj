#include "chess_engine/evaluate.h"
#include "chess_engine/moves.h"
#include "utils/bitboard_util.h"

const int white_piece_values[12] = {10000, 100, 500, 300, 300, 900, -10000, -100, -500, -300, -300, -900};
const int black_piece_values[12] = {-10000, -100, -500, -300, -300, -900, 10000, 100, 500, 300, 300, 900};
const int mobility_values[12] = {0, 0, 15, 15, 15, 0, 0, 0, 15, 15, 15, 0};

int ev_evaluate(_board *board){
    int score = 0;
    const int *piece_values = board->turn == WHITE ? white_piece_values : black_piece_values;

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
    _move enemy_moves[MAX_MOVES];
    int move_count = 0;
    mv_generate_enemy_moves(board, enemy_moves, &move_count);
    for (int i = 0; i < move_count; i++){
        _move move = enemy_moves[i];
        score -= mobility_values[move.piece];
        if (move.capture != NONE){
            score--;
        }
    }

    return score;
}