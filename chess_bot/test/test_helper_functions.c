#include "chess_engine/chess_types.h"
#include "chess_engine/moves.h"
#include "utils/error_handling.h"

int helper_find_move(_board *board, char *move_uci){
    _move move_to_check = mv_uci_to_move(move_uci, board);
    for (int i = 0; i < board->move_count; i++){
        if (mv_moves_equal(board->move_pool[i], move_to_check)){
            return i;
        }
    }

    return -1;
}

_move helper_get_move(_board *board, int index){
    if (index >= board->move_count || index < 0){
        eh_die("index out of bounds");
    }

    return board->move_pool[index];
}

int helper_are_fens_equal(char *left, char *right){
    int index = 0;
    while (left[index] != '\0' && right[index] != '\0'){
        if (left[index] != right[index]){
            return 0;
        }
        index++;
    }

    if (left[index] != right[index]){
        return 0;
    }
    
    return 1;
}

int helper_is_in_check(_board *board){
    return board->in_check;
}

int helper_get_game_state(_board *board){
    return board->game_state;
}

int helper_get_move_count(_board *board){
    return board->move_count;
}

int helper_get_previous_moves_count(_board *board){
    return gl_get_length(board->previous_moves);
}

uint64_t helper_get_zobrist_hash(_board *board){
    return board->zobrist_hash;
}

int helper_get_en_passant_square(_board *board){
    return board->en_passant_square;
}