#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/init_chess_engine.h"
#include "chess_engine/search.h"
#include "chess_engine/board.h"
#include "chess_engine/moves.h"
#include "chess_engine/transposition_table.h"

#define DEPTH 5

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
    printf("%s\n", fen);
    cb_display_fen(fen);

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
            break;
    }

    if (board->turn == WHITE){
        printf("white to move\n");
    }
    else{
        printf("black to move\n");
    }

    printf("\n");
}

int undo_board_move(_board *board, int num_moves){
    if (num_moves == 0){
        return 0;
    }

    cb_undo_move(board);
    print_game_state(board);

    return 1;
}

int player_move(_board *board, char *input){
    _move move = mv_uci_to_move(input, board);
    if (board->game_state == ONGOING && validate_board_move(board, move) == 1){
        cb_make_move(board, move);
        printf("you played: ");
        mv_print_move(move, 1);
        print_game_state(board);
        return 1;
    }

    return 0;
}

int bot_move(_board *board, _transposition_table *transposition_table){
    clock_t start = clock();
    _move move = se_search(board, DEPTH, DEFAULT_ALPHA, DEFAULT_BETA, transposition_table);
    clock_t end = clock();
    if (validate_board_move(board, move) == 1){
        cb_make_move(board, move);
        printf("bot played: ");
        mv_print_move(move, 0);
        printf(", time taken: %f seconds, depth: %d\n", ((double) (end - start)) / CLOCKS_PER_SEC, DEPTH);
        print_game_state(board);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]){
    _color side;
    if (argc != 2){
        fprintf(stderr, "Usage: test_match side\nside: which color the bot plays as, 0 for white, 1 for black\n");
        return 1;
    }
    else{
        char *endptr;
        int arg1 = strtol(argv[1], &endptr, 10);
        if (*endptr != '\0' || (arg1 != 0 && arg1 != 1)){
            fprintf(stderr, "Usage: test_match side\nside: which color the bot plays as, 0 for white, 1 for black\n");
            return 1;
        }

        if (arg1 == 0){
            side = WHITE;
        }
        else if(arg1 == 1){
            side = BLACK;
        }
    }

    init_chess_engine();
    _transposition_table *transposition_table = tt_create_transposition_table(sizeof(_tt_search_entry));
    _board *board = cb_create_board();
    cb_fen_to_board(board, START_FEN);
    print_game_state(board);

    if (side == WHITE){
        bot_move(board, transposition_table);
    }

    char input[MAX_MOVE_BUFFER_SIZE];
    int num_moves = 0;
    while(scanf("%s", input)){
        if (strcmp(input, "stop") == 0){
            printf("terminating\n");
            break;
        }
        else if (strcmp(input, "undo") == 0){
            int success = undo_board_move(board, num_moves);
            if (success == 0){
                printf("cannot undo move\n");
                continue;
            }
            num_moves--;
        }
        else{
            int success = player_move(board, input);
            if (success == 0){
                printf("invalid move\n");
                continue;
            }
            num_moves++;
        }

        int success = bot_move(board, transposition_table);
        if (success == 0){
            fprintf(stderr, "error: invalid move played\n");
            cb_destroy_board(board);
            return 1;
        }
    }

    cb_destroy_board(board);
    return 0;
}