#include <stdio.h>
#include <time.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/board.h"
#include "chess_engine/init_chess_engine.h"

void perft(_board *board, int ply, int collect_stats, int stats[5]){
    if (ply <= 1){
        stats[0] += board->move_count;
        if (collect_stats == 1){
            for (int i = 0; i < board->move_count; i++){
                _move move = board->move_pool[i];
                if (move.capture != NONE){
                    stats[1]++;
                }
                
                switch (move.special_move){
                    case EN_PASSANT:
                        stats[2]++;
                        break;
                    case CASTLE:
                        stats[3]++;
                        break;
                    case PROMOTION:
                        stats[4]++;
                        break;
                    default:
                        break;
                }
            }
        }
        
        return;
    }

    for (int i = 0; i < board->move_count; i++){
        cb_make_move(board, board->move_pool[i]);
        perft(board, ply - 1, collect_stats, stats);
        cb_undo_move(board);
    }
}

void test_pos(int collect_stats, char *fen, int *expected_pos, int depth){
    int stats[5] = {0, 0, 0, 0, 0};
    _board *board = cb_create_board();
    cb_fen_to_board(board, fen);
    printf("position: %s\n", fen);
    for (int i = 1; i <= depth; i++){
        clock_t start = clock();
        perft(board, i, collect_stats, stats);
        clock_t end = clock();
        if (expected_pos[i - 1] != stats[0]){
            printf("test not passed %d != %d\n", expected_pos[i - 1], stats[0]);
        }
        printf("depth: %d, seconds: %f, nodes: %d", i, ((double) (end - start)) / CLOCKS_PER_SEC, stats[0]);
        if (collect_stats == 1){
            printf(", captures: %d, en passants: %d, castles %d, promotions: %d", stats[1], stats[2], stats[3], stats[4]);
        }
        printf("\n\n");
        stats[0] = stats[1] = stats[2] = stats[3] = stats[4] = 0;
    }
    cb_destroy_board(board);
}

int main(int argc, char *argv[]){
    init_chess_engine();

    int collect_stats = 0;
    if (argc == 2){
        char *flag = "-s";
        for (int i = 0; i < 3; i++){
            if (flag[i] != argv[1][i]){
                fprintf(stderr, "Usage %s [-s]\n-s: Enable statistics tracking\n", argv[0]);
                return 1;
            }
        }
        collect_stats = 1;
    }
    else if (argc > 2){
        fprintf(stderr, "Usage %s [-s]\n-s: Enable statistics tracking\n", argv[0]);
        return 1;
    }

    int expected_pos_1[5] = {20, 400, 8902, 197281, 4865609};
    test_pos(collect_stats, START_FEN, expected_pos_1, 5);

    int expected_pos_2[4] = {48, 2039, 97862, 4085603};
    test_pos(collect_stats, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", expected_pos_2, 4);
    
    int expected_pos_3[5] = {14, 191, 2812, 43238, 674624};
    test_pos(collect_stats, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", expected_pos_3, 5);

    int expected_pos_4[4] = {6, 264, 9467, 422333};
    test_pos(collect_stats, "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", expected_pos_4, 4);

    int expected_pos_5[3] = {44, 1486, 62379};
    test_pos(collect_stats, "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", expected_pos_5, 3);

    int expected_pos_6[4] = {24, 496, 9483, 182838};
    test_pos(collect_stats, "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", expected_pos_6, 4);

    int expected_pos_7[3] = {60, 2608, 113742};
    test_pos(collect_stats, "r3k2r/1bp2pP1/5n2/1P1Q4/1pPq4/5N2/1B1P2p1/R3K2R b KQkq c3 0 1", expected_pos_7, 3);

    int expected_pos_8[5] = {6, 162, 937, 24947, 139087};
    test_pos(collect_stats, "8/K7/8/8/2Q1Pp1k/8/8/8 b - e3 0 1", expected_pos_8, 5);

    return 0;
}