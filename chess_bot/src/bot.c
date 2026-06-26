#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/init_chess_engine.h"
#include "chess_engine/search.h"
#include "chess_engine/board.h"
#include "chess_engine/moves.h"

#define BUFFER_SIZE 50

int main(int argc, char *argv[]){
    if (argc != 3){
        fprintf(stderr, "Usage: %s <minimum depth> <maximum search time (seconds)>\n", argv[0]);
        return 1;
    }
    // get minimum depth and max search time
    char *end_ptr;
    const int min_depth = (int)strtol(argv[1], &end_ptr, 10);
    if (end_ptr == argv[1] || *end_ptr != '\0'){
        fprintf(stderr, "Usage: %s <minimum depth> <maximum search time (seconds)>\n", argv[0]);
        return 1;
    }
    const int max_seconds = (int)strtol(argv[2], &end_ptr, 10);
    if (end_ptr == argv[1] || *end_ptr != '\0'){
        fprintf(stderr, "Usage: %s <minimum depth> <maximum search time (seconds)>\n", argv[0]);
        return 1;
    }

    init_chess_engine();
    _board *board = cb_create_board();
    _search_context *context = se_init_search_context();
    // listen for stdin
    // (move,seconds) make move and print with response
    // (null,seconds) just print response
    // (stop) stop program
    // else crash
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), stdin) != NULL){
        buffer[strcspn(buffer, "\n")] = '\0';
        // if stop then stop program
        if (strcmp(buffer, "stop") == 0){
            break;
        }

        // find comma
        char *comma_ptr = strchr(buffer, ',');
        const int comma_index = comma_ptr - buffer;
        if (comma_ptr == NULL || comma_index > 5){
            printf("invalid input\n");
            fflush(stdout);
            continue;
        }

        // get move
        char move_uci[MAX_MOVE_BUFFER_SIZE];
        for (int i = 0; i < comma_index; i++){
            move_uci[i] = buffer[i];
        }
        move_uci[comma_index] = '\0';

        // get time
        char *end_ptr;
        const int seconds = (int)strtol(buffer + comma_index + 1, &end_ptr, 10);
        if (end_ptr == (buffer + comma_index + 1) || *end_ptr != '\0'){
            printf("invalid input\n");
            fflush(stdout);
            continue;
        }
        const int search_time = max_seconds < seconds ? max_seconds : seconds;


        if (strcmp(move_uci, "null") == 0){
            const _move move = se_search(board, min_depth, search_time, context, NULL);
            cb_make_move(board, move);
            mv_move_to_uci(move, move_uci);
            printf("%s\n", move_uci);
            fflush(stdout);
        }
        else{
            const _move move = mv_uci_to_move(move_uci, board);
            if (mv_is_invalid_move(move) == 1){
                printf("invalid input\n");
                fflush(stdout);
                continue;
            }

            _move moves[MAX_MOVES];
            int move_count;
            int valid = 0;
            mv_generate_moves(board, moves, &move_count);
            for (int i = 0; i < move_count; i++){
                if (mv_moves_equal(moves[i], move)){
                    valid = 1;
                }
            }
            if (valid == 0){
                printf("invalid input\n");
                fflush(stdout);
                continue;
            }

            cb_make_move(board, move);
            const _move response = se_search(board, min_depth, search_time, context, NULL);
            cb_make_move(board, response);
            mv_move_to_uci(response, move_uci);
            printf("%s\n", move_uci);
            fflush(stdout);
        }
    }

    cb_destroy_board(board);
    se_destroy_search_context(context);

    return 0;
}