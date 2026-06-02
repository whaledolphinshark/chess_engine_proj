#include <stdio.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/init_chess_engine.h"
#include "chess_engine/search.h"
#include "chess_engine/board.h"
#include "chess_engine/moves.h"

int main(){
    init_chess_engine();
    _board *board = cb_create_board();
    // listen for stdin
    // if move then make move and print with response
    // if null then just print response
    // if stop then stop
    // else crash

    return 0;
}