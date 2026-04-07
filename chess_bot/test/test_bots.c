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

int main(){
    init_chess_engine();
    _search_context context_1;
    _search_context context_2;
    _search_stats stats_1 = {0, 0, 0, 0, 0};
    _search_stats stats_2 = {0, 0, 0, 0, 0};
    _board *board = cb_create_board();
    char *openings[30] = {START_FEN, 
                            "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3", 
                            "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
                            "rnbqkbnr/pppp1ppp/4p3/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
                            "rnbqkbnr/pp1ppppp/2p5/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
                            "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 2 3",
                            "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2",
                            "rnbqkb1r/ppp1pppp/3p1n2/8/3PP3/8/PPP2PPP/RNBQKBNR w KQkq - 1 3",
                            "rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2",
                            "rnbqkbnr/pppp1ppp/8/4p3/4PP2/8/PPPP2PP/RNBQKBNR b KQkq f3 0 2",
                            "r1bqkbnr/pppp1ppp/2n5/4p3/3PP3/5N2/PPP2PPP/RNBQKB1R b KQkq d3 0 3",
                            "rnbqkbnr/pppp1ppp/8/4p3/4P3/2N5/PPPP1PPP/R1BQKBNR b KQkq - 1 2",
                            "rnbqkbnr/ppp1pppp/8/3p4/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2",
                            "rnbqkbnr/pp2pppp/2p5/3p4/2PP4/8/PP2PPPP/RNBQKBNR w KQkq - 0 3",
                            "rnbqkb1r/pppppp1p/5np1/8/2PP4/8/PP2PPPP/RNBQKBNR w KQkq - 0 3",
                            "rnbqk2r/pppp1ppp/4pn2/8/1bPP4/2N5/PP2PPPP/R1BQKBNR w KQkq - 2 4",
                            "rnbqkb1r/p1pp1ppp/1p2pn2/8/2PP4/5N2/PP2PPPP/RNBQKB1R w KQkq - 0 4",
                            "rnbqkb1r/pppp1ppp/4pn2/8/2PP4/6P1/PP2PP1P/RNBQKBNR b KQkq - 0 3",
                            "rnbqk2r/pppp1ppp/4pn2/8/1bPP4/5N2/PP2PPPP/RNBQKB1R w KQkq - 2 4",
                            "rnbqkb1r/ppp1pp1p/5np1/3p4/2PP4/2N5/PP2PPPP/R1BQKBNR w KQkq d6 0 4",
                            "rnbqkbnr/ppppp1pp/8/5p2/3P4/8/PPP1PPPP/RNBQKBNR w KQkq f6 0 2",
                            "rnbqkb1r/pppppppp/5n2/6B1/3P4/8/PPP1PPPP/RN1QKBNR b KQkq - 2 2",
                            "rnbqkb1r/p2ppppp/5n2/1ppP4/2P5/8/PP2PPPP/RNBQKBNR w KQkq b6 0 4",
                            "rnbqkb1r/ppp1pppp/5n2/3p4/3P1B2/5N2/PPP1PPPP/RN1QKB1R b KQkq - 3 3",
                            "rnbqkb1r/pp1p1ppp/4pn2/2pP4/2P5/8/PP2PPPP/RNBQKBNR w KQkq - 0 4",
                            "rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1",
                            "rnbqkbnr/pppppppp/8/8/2P5/8/PP1PPPPP/RNBQKBNR b KQkq c3 0 1",
                            "rnbqkbnr/pppppppp/8/8/5P2/8/PPPPP1PP/RNBQKBNR b KQkq f3 0 1",
                            "rnbqkbnr/pppppppp/8/8/8/5P2/PPPPP1PP/RNBQKBNR b KQkq - 0 1",
                            "rnbqkbnr/pppppppp/8/8/8/1P6/P1PPPPPP/RNBQKBNR b KQkq - 0 1"};
    char fen[MAX_FEN_LENGTH];

    int original_wins = 0;
    int alt_wins = 0;
    int draws = 0;
    int total_games = 0;
    for (int i = 0; i < 30; i++){
        for (int j = 0; j < 2; j++){
            total_games++;
            se_init_search_context(&context_1);
            se_init_search_context(&context_2);
            cb_fen_to_board(board, openings[i]);
            cb_board_to_fen(board, fen);
            printf("%s\n", fen);
            _color original_side;
            if ((board->turn == WHITE && j == 0) || (board->turn == BLACK && j == 1)){
                original_side = WHITE;
            }
            else{
                original_side = BLACK;
            }

            while (board->game_state == ONGOING){
                _move move;
                if (j == 0){
                    move = se_search(board, DEPTH, &context_1, &stats_1);
                    cb_make_move(board, move);
                    if (board->game_state == ONGOING){
                        move = alt_search(board, DEPTH, &context_2, &stats_2);
                        cb_make_move(board, move);
                    }
                    else{
                        break;
                    }
                }
                else{
                    move = alt_search(board, DEPTH, &context_1, &stats_1);
                    cb_make_move(board, move);
                    if (board->game_state == ONGOING){
                        move = se_search(board, DEPTH, &context_2, &stats_2);
                        cb_make_move(board, move);
                    }
                    else{
                        break;
                    }
                }

                cb_board_to_fen(board, fen);
                printf("%s\n", fen);
            }

            if ((board->game_state == W_WIN && original_side == WHITE) || (board->game_state == B_WIN && original_side == BLACK)){
                original_wins++;
            }
            else if (board->game_state != DRAW){
                alt_wins++;
            }
            else{
                draws++;
            }
            se_destroy_search_context(&context_1);
            se_destroy_search_context(&context_2);

            printf("\nposition: %s, total games: %d, original wins: %d, alt wins: %d, draws: %d\n\n", openings[i], total_games, original_wins, alt_wins, draws);
        }
    }

    printf("\ntotal games: %d, original wins: %d, alt wins: %d, draws: %d\n\n", total_games, original_wins, alt_wins, draws);

    cb_destroy_board(board);

    return 0;
}
