#include "chess_engine/chess_types.h"
#include "chess_engine/moves.h"

int helper_get_board_move_count(_board *board){
    return board->move_count;
}

int helper_find_move(_board *board, char *move_uci){
    _move move_to_check = mv_uci_to_move(move_uci, board);
    for (int i = 0; i < board->move_count; i++){
        _move move = board->move_pool[i];
        if (mv_moves_equal(board->move_pool[i], move_to_check)){
            return i;
        }
    }

    return -1;
}

_move helper_get_move(_board *board, int index){
    _move move = {0, 0, NONE, NONE, NONE, NORMAL};
    if (index >= board->move_count || index < 0){
        return move;
    }
    else{
        return board->move_pool[index];
    }
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