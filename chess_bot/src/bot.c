#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/init_chess_engine.h"
#include "chess_engine/search.h"
#include "chess_engine/board.h"
#include "chess_engine/moves.h"

#define BUFFER_SIZE 50

int main(){
    // get minimum depth and max search time

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
        const int search_time = 6 < seconds ? 6 : seconds;


        if (strcmp(move_uci, "null") == 0){
            const _move move = se_search(board, 6, search_time, context, NULL);
            cb_make_move(board, move);
            mv_move_to_uci(move, move_uci);
            printf("%s\n", move_uci);
            fflush(stdout);
        }
        else{
            const _move move = mv_uci_to_move(move_uci, board);
            if (mv_is_null_move(move) == 1){
                printf("invalid input\n");
                fflush(stdout);
                continue;
            }
            // validate move

            cb_make_move(board, move);
            const _move response = se_search(board, 6, search_time, context, NULL);
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