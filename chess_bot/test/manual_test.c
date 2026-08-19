#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#include "chess_engine/init_chess_engine.h"
#include "chess_engine/chess_types.h"
#include "chess_engine/moves.h"
#include "chess_engine/board.h"
#include "utils/bitboard_util.h"
#include "chess_engine/transposition_table.h"

int validate_board_move(_board *board, _move move){
    _move moves[MAX_MOVES];
    int move_count;
    mv_generate_moves(board, moves, &move_count);
    for (int i = 0; i < move_count; i++){
        if (mv_moves_equal(moves[i], move)){
            return 1;
        }
    }

    return 0;
}

void print_game_state(_board *board){
    char fen[MAX_FEN_LENGTH];

    cb_board_to_fen(board, fen);
    printf("fen: %s\n", fen);
    cb_display_fen(fen);
    printf("zobrist hash: %llu\n", board->zobrist_hash);
    printf("plies: %d\n", board->plies);

    switch (board->game_state){
            case W_WIN:
                printf("white wins\n");
                break;
            case B_WIN:
                printf("black wins\n");
                break;
            case DRAW:
                printf("draw\n");
                break;
            case ONGOING:
                if (board->in_check == 1){
                    printf("in check\n");
                }
                mv_print_moves(board);
                printf("\n");
                break;
        }
}

void undo_move_on_board(_board *board, int num_moves){
    if (num_moves == 0){
        printf("cannot undo move\n");
    }
    else{
        cb_undo_move(board);
        print_game_state(board);
    }
}

void make_move_on_board(_board *board, char *input){
    _move move = mv_uci_to_move(input, board);
    if (board->game_state == ONGOING && validate_board_move(board, move) == 1){
        cb_make_move(board, move);
        print_game_state(board);
    }
    else{
        printf("invalid move\n");
    }
}

int main(void){
    init_chess_engine();
    _board *board = cb_create_board();

    // display board
    cb_fen_to_board(board, "rnq3k1/pRp1p2p/6P1/3p4/3Q4/2P5/P1P2r2/2B1K2R w K - 0 15");
    print_game_state(board);

    char input[MAX_MOVE_BUFFER_SIZE];
    int num_moves = 0;
    while(scanf("%s", input)){
        if (strcmp(input, "stop") == 0){
            printf("terminating\n");
            break;
        }
        else if (strcmp(input, "undo") == 0){
            undo_move_on_board(board, num_moves);
            num_moves--;
        }
        else{
            make_move_on_board(board, input);
            num_moves++;
        }
    }

    cb_destroy_board(board);
    return 0;
}
