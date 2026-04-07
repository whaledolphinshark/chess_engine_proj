#include <stdio.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/init_chess_engine.h"
#include "chess_engine/search.h"
#include "chess_engine/board.h"
#include "chess_engine/moves.h"
#include "chess_engine/transposition_table.h"
#include "utils/error_handling.h"
#include "alt_bot/alt_bot.h"

#define DEPTH 6

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

int main(){
    init_chess_engine();
    _search_context context_1;
    _search_context context_2;
    _search_stats stats_1 = {0, 0, 0, 0, 0};
    _search_stats stats_2 = {0, 0, 0, 0, 0};
    se_init_search_context(&context_1);
    se_init_search_context(&context_2);
    _board *board = cb_create_board();
    char *openings[3] = {START_FEN, "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3", "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2"};
    char fen[MAX_FEN_LENGTH];

    int original_wins = 0;
    int alt_wins = 0;
    int draws = 0;
    int total_games = 0;
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < 2; j++){
            total_games++;
            cb_fen_to_board(board, openings[i]);
            cb_board_to_fen(board, fen);
            printf("%s\n", fen);
            while (board->game_state == ONGOING){
                _move move;
                if (board->turn == WHITE){
                    move = (j == 0 ? se_search(board, DEPTH, &context_1, &stats_1) : alt_search(board, DEPTH, &context_2, &stats_2));
                }
                else{
                    move = (j == 0 ? alt_search(board, DEPTH, &context_2, &stats_2) : se_search(board, DEPTH, &context_1, &stats_1));
                }

                if (validate_board_move(board, move)){
                    cb_make_move(board, move);
                }
                else{
                    eh_die("invalid move");
                }

                cb_board_to_fen(board, fen);
                printf("%s\n", fen);
            }

            if (board->game_state == W_WIN){
                if (j == 0){
                    original_wins++;
                }
                else{
                    alt_wins++;
                }
            }
            else if (board->game_state == B_WIN){
                if (j == 0){
                    alt_wins++;
                }
                else{
                    original_wins++;
                }
            }
            else{
                draws++;
            }
            printf("\nposition: %s, white: %s, total games: %d, original wins: %d, alt wins: %d, draws: %d\n\n", openings[i], j == 0 ? "original" : "alt", total_games, original_wins, alt_wins, draws);
        }
    }

    printf("\ntotal games: %d, original wins: %d, alt wins: %d, draws: %d\n\n", total_games, original_wins, alt_wins, draws);

    cb_destroy_board(board);
    se_destroy_search_context(&context_1);
    se_destroy_search_context(&context_2);

    return 0;
}