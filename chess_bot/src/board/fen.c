#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chess_engine/board.h"
#include "board_internal.h"
#include "chess_engine/moves.h"
#include "chess_engine/transposition_table.h"
#include "utils/bitboard_util.h"
#include "utils/error_handling.h"

static _piece character_to_piece(char character){
    switch (character){
        case 'r':
            return B_ROOK;
        case 'n':
            return B_KNIGHT;
        case 'b':
            return B_BISHOP;
        case 'q':
            return B_QUEEN;
        case 'k':
            return B_KING;
        case 'p':
            return B_PAWN;
        case 'R':
            return W_ROOK;
        case 'N':
            return W_KNIGHT;
        case 'B':
            return W_BISHOP;
        case 'Q':
            return W_QUEEN;
        case 'K':
            return W_KING;
        case 'P':
            return W_PAWN;
    }

    return NONE;
}

// assumes fen is correct, breaks if fen is not correct
void cb_fen_to_board(_board *board, char *fen){
    int i = 0;
    int array_index = 56;
    char character;
    for (int i = 0; i < 12; i++){
        board->bitboards[i] = 0UL;
    }

    // pieces
    while (fen[i] != ' '){
        character = fen[i];
        if (character >= '1' && character <= '8'){
            int empty_spaces = character - '0';
            for (int i = 0; i < empty_spaces; i++){
                board->piece_array[array_index] = NONE;
                array_index++;
            }
        }
        else if (character != '/'){
            _piece piece = character_to_piece(character);
            board->piece_array[array_index] = piece;
            board->bitboards[piece] |= 1UL << array_index;
            array_index++;
        }
        else{
            array_index -= 16;
        }

        i++;
    }
    board->white_pieces = board->bitboards[W_KING] | board->bitboards[W_PAWN] | board->bitboards[W_ROOK] | board->bitboards[W_BISHOP] | board->bitboards[W_KNIGHT] | board->bitboards[W_QUEEN];
    board->black_pieces = board->bitboards[B_KING] | board->bitboards[B_PAWN] | board->bitboards[B_ROOK] | board->bitboards[B_BISHOP] | board->bitboards[B_KNIGHT] | board->bitboards[B_QUEEN];
    board->board = board->white_pieces | board->black_pieces;

    i++;
    // turn
    board->turn = fen[i] == 'w' ? WHITE : BLACK;

    i += 2;
    board->castling_rights = 0;
    // castling rights
    while (fen[i] != ' '){
        switch (fen[i]){
            case 'K':
                board->castling_rights |= 2UL;
                break;
            case 'Q':
                board->castling_rights |= 1UL;
                break;
            case 'k':
                board->castling_rights |= 8UL;
                break;
            case 'q':
                board->castling_rights |= 4UL;
                break;
        }
        i++;
    }

    i++;
    int en_passant_square = 0;
    // en passant
    while (fen[i] != ' '){
        char character = fen[i];
        if (character >= 'a' && character <= 'h'){
            en_passant_square += character - 'a';
        }
        else if (character >= '0' && character <= '9'){
            en_passant_square += 8 * (character - '0' - 1);
        }
        else{
            en_passant_square = 0;
        }
        i++;
    }

    board->en_passant_square = en_passant_square;

    i++;
    char *ptr;
    // clocks
    board->halfmove_clock = strtol(fen + sizeof(char) * i, &ptr, 10);
    board->fullmove_clock = strtol(ptr + sizeof(char), NULL, 10);

    // plies
    // w-b  w-b  w-b  w-b  w-b
    // 0-1, 2-3, 4-5, 6-7, 8-9
    // 1    2    3    4    5
    board->plies = 2 * board->fullmove_clock + (board->turn == WHITE ? -2 : -1);

    // check
    int king_square = bb_get_lsb(board->turn == WHITE ? board->bitboards[W_KING] : board->bitboards[B_KING]) - 1;
    board->in_check = mv_is_square_attacked(king_square, board, board->board, board->turn) == 1;

    // moves
    mv_generate_moves(board);

    // zobrist hash and history
    board->zobrist_hash = cb_hash_board(board);
    tt_clear_items(board->history);
    int one = 1;
    tt_insert_item(board->history, board->zobrist_hash, &one);

    // game state
    cb_calculate_game_state(board);
}

void cb_board_to_fen(_board *board, char *buffer){
    char piece_symbols[12] = {'K', 'P', 'R', 'N', 'B', 'Q', 'k', 'p', 'r', 'n', 'b', 'q'};
    char empty_spaces = 0;
    char str[5];
    str[1] = '\0';
    buffer[0] = '\0';
    // pieces
    for (int i = 56; i >= 0; i -= 8){
        for (int j = 0; j < 8; j++){
            _piece piece = board->piece_array[i + j];
            if (piece == NONE){
                empty_spaces++;
            }
            else{
                if (empty_spaces > 0){
                    str[0] = '0' + empty_spaces;
                    strncat(buffer, str, 5);
                    empty_spaces = 0;
                }
                strncat(buffer, &(piece_symbols[piece]), 1);
            }
        }

        if (empty_spaces > 0){
            str[0] = '0' + empty_spaces;
            strncat(buffer, str, 5);
            empty_spaces = 0;
        }
        if (i != 0){
            strncat(buffer, "/", 1);
        }
    }

    // turn
    str[0] = ' ';
    str[2] = ' ';
    str[3] = '\0';
    if (board->turn == WHITE){
        str[1] = 'w';
    }
    else{
        str[1] = 'b';
    }
    strncat(buffer, str, 5);

    // castling rights
    int index = 0;
    if (board->castling_rights == 0){
        str[index] = '-';
        index++;
    }
    else{
        if ((board->castling_rights & 2UL) != 0){
            str[index] = 'K';
            index++;
        }
        if ((board->castling_rights & 1UL) != 0){
            str[index] = 'Q';
            index++;
        }
        if ((board->castling_rights & 8UL) != 0){
            str[index] = 'k';
            index++;
        }
        if ((board->castling_rights & 4UL) != 0){
            str[index] = 'q';
            index++;
        }
    }
    str[index] = '\0';
    strncat(buffer, str, 5);

    // en passant
    str[0] = ' ';
    char files[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    if (board->en_passant_square != 0){
        str[1] = files[board->en_passant_square% 8];
        str[2] = board->turn == WHITE ? '6' : '3';
        str[3] = ' ';
        str[4] = '\0';
        index = 4;
    }
    else{
        str[1] = '-';
        str[2] = ' ';
        str[3] = '\0';
        index = 3;
    }
    strncat(buffer, str, 5);

    // clock
    int value = snprintf(str, sizeof(str), "%d", board->halfmove_clock);
    if (value >= sizeof(str)){
        eh_die("value was truncated");
    }
    strncat(buffer, str, 5);
    strncat(buffer, " ", 1);
    value = snprintf(str, sizeof(str), "%d", board->fullmove_clock);
    if (value >= sizeof(str)){
        eh_die("value was truncated");
    }
    strncat(buffer, str, 5);
}

void cb_display_fen(char *fen){
    int i = 0;
    char character;
    printf("%-2c|", '8');
    char rank = '7';
    while (fen[i] != ' '){
        character = fen[i];
        if (character == '/'){
            printf("\n%-2c|", rank);
            rank--;
        }
        else if (character >= '1' && character <= '8'){
            int num = character - '0';
            for (int j = 0; j < num; j++){
                printf("%-2c", '#');
            }
        }
        else{
            printf("%-2c", character);
        }

        i++;
    }
    printf("\n%s\n   ", "  ----------------");

    for (int j = 0; j < 8; j++){
        printf("%-2c", 'a' + j);
    }
    printf("\n");
}