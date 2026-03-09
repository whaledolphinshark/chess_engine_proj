#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "chess_engine/init_chess_engine.h"
#include "chess_engine/chess_types.h"
#include "chess_engine/moves.h"
#include "chess_engine/board.h"
#include "utils/bitboard_util.h"
#include "chess_engine/transposition_table.h"

#define SETUP_FEN "7k/8/8/2B5/3p2b1/8/8/4K3 w - - 0 1"

void print_game_state(_board *board){
    char fen[MAX_FEN_LENGTH];

    cb_board_to_fen(board, fen);
    printf("fen: %s\n", fen);
    cb_display_fen(fen);
    printf("zobrist hash: %lu\n", board->zobrist_hash);
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
    if (board->game_state == ONGOING){
        cb_make_move(board, move);
        print_game_state(board);
    }
    else{
        printf("invalid move\n");
    }
}

int main(void){
    init_chess_engine();
    printf("initialized engine\n");
    _board *board = cb_create_board();

    // display board
    cb_fen_to_board(board, SETUP_FEN);
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