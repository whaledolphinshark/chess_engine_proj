#include <stdlib.h>
#include <stdio.h>

#include "chess_engine/moves.h"
#include "moves_internal.h"
#include "utils/error_handling.h"

static int moves_equal(_move left, _move right){
    return left.from == right.from && left.to == right.to && left.piece == right.piece && left.promotion == right.promotion && left.capture == right.capture && left.info == right.info;
}

_move init_move(_board *board, int from, int to, _piece promotion){
    _move move;
    move.to = to;
    move.from = from;
    move.piece = board->piece_array[from];
    move.promotion = promotion;
    move.info = 0;
    // promotion
    move.info |= promotion == NONE ? 0 : 2UL;
    // castling
    move.info |= (move.piece == W_KING || move.piece == B_KING) && abs(from - to) == 2 ? 8UL : 0;
    // en passant
    if ((move.piece == W_PAWN || move.piece == B_PAWN) && to == board->pawn_jump){
        move.capture = board->turn == WHITE ? B_PAWN : W_PAWN;
        // is en passant and is capture
        move.info |= 5UL;
    }
    else{
        move.capture = board->piece_array[to];
        move.info |= move.capture == NONE ? 0 : 1UL;
    }

    return move;
}

void mv_add_move(_board *board, int from, int to, _piece promotion){
    board->move_pool[board->move_count] = init_move(board, from, to, promotion);
    board->move_count++;
}

void mv_clear_moves(_board *board){
    board->move_count = 0;
}

_move mv_uci_to_move(char *move_uci, _board *board, int validate, int *valid){
    int string_length = 0;
    _piece promotion = NONE;
    int from = 0;
    int to = 0;
    *valid = 0;
    while (string_length < 7 && move_uci[string_length] != '\0'){
        string_length++;
    }

    if (string_length != 4 && string_length != 5){
        _move none = {0, 0, NONE, NONE, NONE, 0};
        return none;
    }

    if (move_uci[0] >= 'a' && move_uci[0] <= 'h'){
        from += move_uci[0] - 'a';
    }
    if (move_uci[1] >= '1' && move_uci[1] <= '8'){
        from += 8 * (move_uci[1] - '1');
    }
    if (move_uci[2] >= 'a' && move_uci[2] <= 'h'){
        to += move_uci[2] - 'a';
    }
    if (move_uci[3] >= '1' && move_uci[3] <= '8'){
        to += 8 * (move_uci[3] - '1');
    }

    if (string_length == 5){
        switch (move_uci[4]){
            case 'q':
                promotion = board->turn == WHITE ? W_QUEEN : B_QUEEN;
                break;
            case 'r':
                promotion = board->turn == WHITE ? W_ROOK : B_ROOK;
                break;
            case 'b':
                promotion = board->turn == WHITE ? W_BISHOP : B_BISHOP;
                break;
            case 'n':
                promotion = board->turn == WHITE ? W_KNIGHT : B_KNIGHT;
                break;
        }
    }

    _move move = init_move(board, from, to, promotion);

    if (validate == 1){
        for (int i = 0; i < board->move_count; i++){
            if (moves_equal(move, board->move_pool[i]) == 1){
                *valid = 1;
            }
        }
    }
    return move;
}

void mv_print_move(_move move, int new_line){
    char files[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    char promotions[4] = {'r', 'n', 'b', 'q'};
    printf("%c%d%c%d", files[move.from % 8], move.from / 8 + 1, files[move.to % 8], move.to / 8 + 1);
    if ((move.info & 2UL) != 0){
        int promotion = NONE;
        switch (move.promotion){
            case W_ROOK:
            case B_ROOK:
                promotion = 0;
                break;
            case W_KNIGHT:
            case B_KNIGHT:
                promotion = 1;
                break;
            case W_BISHOP:
            case B_BISHOP:
                promotion = 2;
                break;
            case W_QUEEN:
            case B_QUEEN:
                promotion = 3;
                break;
            default:
                eh_die("invalid promotion");
        }
        printf("%c", promotions[promotion]);
    }

    if (new_line == 1){
        printf("\n");
    }
}

void mv_print_moves(_board *board){
    int length = board->move_count;
    printf("number of moves: %d\n", length);
    for (int i = 0; i < length; i++){
        mv_print_move(board->move_pool[i], 0);
        if (i != length - 1){
            printf(", ");
        }
        else{
            printf("\n");
        }
    }
}