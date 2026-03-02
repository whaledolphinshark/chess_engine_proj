#include "chess_engine/chess_types.h"
#include "chess_engine/moves.h"

int helper_get_board_move_count(_board *board){
    return board->move_count;
}

int helper_has_move(_board *board, char *move_uci){
    int *filler;
    char test = move_uci[4];
    // _move move = mv_uci_to_move(move_uci, board, 0, filler);
    // for (int i = 0; i < board->move_count; i++){
    //     if (mv_moves_equal(board->move_pool[i], move)){
    //         return 1;
    //     }
    // }

    return 0;
}