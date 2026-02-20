#include <stdint.h>
#include <time.h>
#include <stdio.h>

#include "chess_engine/init_chess_engine.h"
#include "init_chess_engine_internal.h"
#include "../moves/moves_internal.h"
#include "../board/board_internal.h"

int chess_engine_ready = 0;

void init_chess_engine(){
    if (chess_engine_ready == 1){
        printf("already initialized chess engine\n");
        return;
    }

    mv_init_moves();
    cb_init_board();
    chess_engine_ready = 1;
}

int is_chess_engine_ready(){
    return chess_engine_ready;
}