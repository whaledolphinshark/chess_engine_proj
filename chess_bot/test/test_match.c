#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <bits/getopt_core.h>
#include <time.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/init_chess_engine.h"
#include "chess_engine/search.h"
#include "chess_engine/board.h"
#include "chess_engine/moves.h"
#include "chess_engine/transposition_table.h"

#define ERROR_MSG "Usage: test_match [-s] [-d]\n-s: which color the bot play, 0 for white, 1 for black\n-d: depth the bot should search to\n"

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
            if (board->turn == WHITE){
                printf("white to move\n");
            }
            else{
                printf("black to move\n");
            }
            break;
    }

    printf("\n");
}

int undo_board_move(_board *board, int num_moves){
    if (num_moves == 0){
        return 0;
    }

    if (board->game_state == ONGOING){
        cb_undo_move(board);
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

int bot_move(_board *board, _search_context *context, int depth){
    _search_stats stats = {0, 0, 0, 0};
    clock_t start = clock();
    _move move = se_search(board, depth, DEFAULT_ALPHA, DEFAULT_BETA, context, &stats);
    clock_t end = clock();
    if (validate_board_move(board, move) == 1){
        cb_make_move(board, move);
        printf("bot played: ");
        mv_print_move(move, 0);
        printf(", time taken: %f seconds, depth: %d\n", ((double) (end - start)) / CLOCKS_PER_SEC, depth);
        printf("re-searches: %d, evaluation: %d, nodes visited: %d, quiescent nodes visited: %d\n", stats.researches, stats.eval, stats.nodes_visited, stats.quiescent_nodes_visited);
        print_game_state(board);
        return 1;
    }

    return 0;
}

void read_args(int argc, char *argv[], char **arg_s, char **arg_d){
    int opt;
    while ((opt = getopt(argc, argv, "s:d:")) != -1){
        switch (opt){
            case 's':
                *arg_s = optarg;
                break;
            case 'd':
                *arg_d = optarg;
                break;
            case '?':
            default:
                fprintf(stderr, "Error: unexpected argument '%s'\n", argv[optind]);
                fprintf(stderr, ERROR_MSG);
                exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        fprintf(stderr, "Error: unexpected argument '%s'\n", argv[optind]);
        fprintf(stderr, ERROR_MSG);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]){
    _color side = BLACK;
    int depth = 5;
    char *arg_s = NULL;
    char *arg_d = NULL;
    read_args(argc, argv, &arg_s, &arg_d);
    if (arg_s != NULL){
        char *end_ptr;
        unsigned long s = strtoul(arg_s, &end_ptr, 10);
        if (*end_ptr != '\0' || *arg_s == '-' || (s != 0 && s != 1)){
            fprintf(stderr, "Error: invalid argument '%s'\n", arg_s);
            fprintf(stderr, ERROR_MSG);
            exit(EXIT_FAILURE);
        }
        side = s == 0 ? WHITE : BLACK;
    }
    if (arg_d != NULL){
        char *end_ptr;
        unsigned long d = strtoul(arg_d, &end_ptr, 10);
        if (*end_ptr != '\0' || *arg_d == '-' || d == 0){
            fprintf(stderr, "Error: invalid argument '%s'\n", arg_d);
            fprintf(stderr, ERROR_MSG);
            exit(EXIT_FAILURE);
        }
        depth = d;
    }

    init_chess_engine();
    _search_context context;
    se_init_search_context(&context);
    _board *board = cb_create_board();
    cb_fen_to_board(board, START_FEN);
    print_game_state(board);

    if (side == WHITE){
        bot_move(board, &context, depth);
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

        if (board->game_state == ONGOING){
            int success = bot_move(board, &context, depth);
            if (success == 0){
                fprintf(stderr, "error: invalid move played\n");
                cb_destroy_board(board);
                return 1;
            }
        }
    }

    se_destroy_search_context(&context);
    cb_destroy_board(board);
    return 0;
}