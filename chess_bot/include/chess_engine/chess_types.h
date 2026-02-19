#ifndef     CHESS_DEFINITIONS_H
#define     CHESS_DEFINITIONS_H

#include <stdint.h>

#define NUM_BITBOARDS 12
#define MAX_MOVES 218

typedef enum{
    B_WIN = -1,
    DRAW = 0,
    W_WIN = 1,
    ONGOING = 2
}_game_state;

typedef enum{
    WHITE = 1,
    BLACK = 0
}_color;

typedef enum{
    W_KING = 0,
    W_PAWN = 1,
    W_ROOK = 2,
    W_KNIGHT = 3,
    W_BISHOP = 4,
    W_QUEEN = 5,
    B_KING = 6,
    B_PAWN = 7,
    B_ROOK = 8,
    B_KNIGHT = 9,
    B_BISHOP = 10,
    B_QUEEN = 11, 
    NONE = 12
}_piece;

typedef struct _transposition_table _transposition_table;

typedef struct _move{
    int to;
    int from;
    _piece piece;
    _piece capture;
    _piece promotion;
    // 1st bit: is capture, 2nd bit: is promotion, 3rd bit: is en passant, 4th bit: is castle
    uint8_t info;
}_move;

typedef struct _board{
    // bitboards
    // king, pawns, rooks, knights, bishops, queen, white first black second
    uint64_t bitboards[NUM_BITBOARDS];
    _piece piece_array[64];
    uint64_t white_pieces;
    uint64_t black_pieces;
    uint64_t board;
    // stores the square where a pawn is vulnerable to en passant, 0 = none
    int pawn_jump;
    // stores castling rights, 1st bit: white left rook, 2nd bit: white right rook, 3rd bit: black left rook, 4th bit: black right rook
    uint8_t castling_rights;
    int plies;
    _color turn;
    int halfmove_clock;
    int fullmove_clock;
    int in_check;
    _game_state game_state;
    uint64_t zobrist_hash;
    _move move_pool[MAX_MOVES];
    int move_count;
    _transposition_table *history;
}_board;

#endif