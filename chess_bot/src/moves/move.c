#include <stdlib.h>
#include <stdio.h>

#include "chess_engine/moves.h"
#include "moves_internal.h"
#include "utils/error_handling.h"

int mv_moves_equal(const _move left, const _move right){
    return left.from == right.from && left.to == right.to && left.piece == right.piece && left.promotion == right.promotion && left.capture == right.capture && left.special_move == right.special_move;
}

_move mv_init_move(_board *board, int from, int to, _piece promotion){
    _move move;
    move.to = to;
    move.from = from;
    move.piece = board->piece_array[from];
    move.promotion = promotion;
    move.capture = board->piece_array[to];
    if (promotion != NONE){
        move.special_move = PROMOTION;
    }
    else if ((move.piece == W_KING || move.piece == B_KING) && abs(from - to) == 2){
        move.special_move = CASTLE;
    }
    else if ((move.piece == W_PAWN || move.piece == B_PAWN) && to == board->en_passant_square){
        move.special_move = EN_PASSANT;
        move.capture = board->turn == WHITE ? B_PAWN : W_PAWN;
    }
    else{
        move.special_move = NORMAL;
    }

    return move;
}

_move mv_uci_to_move(char *move_uci, _board *board){
    int string_length = 0;
    _piece promotion = NONE;
    int from = 0;
    int to = 0;
    while (string_length < 6 && move_uci[string_length] != '\0'){
        string_length++;
    }

    if (string_length != 4 && string_length != 5){
        _move none = {0, 0, NONE, NONE, NONE, NORMAL};
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

    _move move = mv_init_move(board, from, to, promotion);

    return move;
}

void mv_move_to_uci(_move move, char *buffer){
    char files[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    char ranks[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    buffer[0] = files[move.from % 8];
    buffer[1] = ranks[move.from / 8];
    buffer[2] = files[move.to % 8];
    buffer[3] = ranks[move.to / 8];
    switch (move.promotion){
        case W_ROOK:
        case B_ROOK:
            buffer[4] = 'r';
            buffer[5] = '\0';
            break;
        case W_KNIGHT:
        case B_KNIGHT:
            buffer[4] = 'n';
            buffer[5] = '\0';
            break;
        case W_BISHOP:
        case B_BISHOP:
            buffer[4] = 'b';
            buffer[5] = '\0';
            break;
        case W_QUEEN:
        case B_QUEEN:
            buffer[4] = 'q';
            buffer[5] = '\0';
            break;
        default:
            buffer[4] = '\0';
    }
}

void mv_print_move(_move move, int new_line){
    char files[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    printf("%c%d%c%d", files[move.from % 8], move.from / 8 + 1, files[move.to % 8], move.to / 8 + 1);
    if (move.special_move == PROMOTION){
        switch (move.promotion){
            case W_ROOK:
            case B_ROOK:
                printf("%c", 'r');
                break;
            case W_KNIGHT:
            case B_KNIGHT:
                printf("%c", 'n');
                break;
            case W_BISHOP:
            case B_BISHOP:
                printf("%c", 'b');
                break;
            case W_QUEEN:
            case B_QUEEN:
                printf("%c", 'q');
                break;
            default:
                eh_die("invalid promotion");
        }
    }

    if (new_line == 1){
        printf("\n");
    }
}

void mv_print_moves(_board *board){
    _move moves[MAX_MOVES];
    int move_count;
    mv_generate_moves(board, moves, &move_count);
    printf("number of moves: %d\n", move_count);
    for (int i = 0; i < move_count; i++){
        mv_print_move(moves[i], 0);
        if (i != move_count - 1){
            printf(", ");
        }
        else{
            printf("\n");
        }
    }
}