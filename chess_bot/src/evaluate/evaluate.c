#include <limits.h>

#include "chess_engine/evaluate.h"
#include "chess_engine/moves.h"
#include "utils/bitboard_util.h"

const int piece_phase_value[12] = {0, 0, 2, 1, 1, 4, 0, 0, 2, 1, 1, 4};
const int piece_values[12] = {10000, 100, 500, 320, 330, 900, 10000, 100, 500, 320, 330, 900};
const int white_piece_values[12] = {10000, 100, 500, 320, 330, 900, -10000, -100, -500, -320, -330, -900};
const int black_piece_values[12] = {-10000, -100, -500, -320, -330, -900, 10000, 100, 500, 320, 330, 900};
const int mobility_values[12] = {0, 1, 3, 5, 5, 0, 0, 1, 3, 5, 5, 0};
const int pawn_table[64] = {0, 0, 0, 0, 0, 0, 0, 0, 5, 10, 10, -20, -20, 10, 10, 5, 5, -5, -10, 0, 0, -10, -5, 5, 0, 0, 0, 20, 20, 0, 0, 0, 5, 5, 10, 25, 25, 10, 5, 5, 10, 10, 20, 30, 30, 20, 10, 10, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0};
const int queen_table[64] = {-20, -10, -10, -5, -5, -10, -10, -20, -10, 0, 5, 0, 0, 0, 0, -10, -10, 5, 5, 5, 5, 5, 0, -10, 0, 0, 5, 5, 5, 5, 0, -5, -5, 0, 5, 5, 5, 5, 0, -5, -10, 0, 5, 5, 5, 5, 0, -10, -10, 0, 0, 0, 0, 0, 0, -10, -20, -10, -10, -5, -5, -10, -10, -20};
const int rook_table[64] = {0, 0, 0, 5, 5, 0, 0, 0, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, 5, 10, 10, 10, 10, 10, 10, 5, 0, 0, 0, 0, 0, 0, 0, 0};
const int bishop_table[64] = {-20, -10, -10, -10, -10, -10, -10, -20, -10, 5, 0, 0, 0, 0, 5, -10, -10, 10, 10, 10, 10, 10, 10, -10, -10, 0, 10, 10, 10, 10, 0, -10, -10, 5, 5, 10, 10, 5, 5, -10, -10, 0, 5, 10, 10, 5, 0, -10, -10, 0, 0, 0, 0, 0, 0, -10, -20, -10, -10, -10, -10, -10, -10, -20};
const int knight_table[64] = {-50, -40, -30, -30,- 30, -30, -40, -50, -40, -20, 0, 5, 5, 0, -20, -40, -30, 5, 10, 15, 15, 10, 5, -30, -30, 0, 15, 20, 20, 15, 0, -30, -30, 5, 15, 20, 20, 15, 5, -30, -30, 0, 10, 15, 15, 10, 0, -30, -40, -20, 0, 0, 0, 0, -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};
const int king_mid_table[64] = {20, 30, 10, 0, 0, 10, 30, 20, 20, 20, 0, 0, 0, 0, 20, 20, -10, -20, -20, -20, -20, -20, -20, -10, -20, -30, -30, -40, -40, -30, -30, -20, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30};
const int king_end_table[64] = {-50, -30, -30, -30, -30, -30, -30, -50, -30, -30, 0, 0, 0, 0, -30, -30, -30, -10, 20, 30, 30, 20, -10, -30, -30, -10, 30, 40, 40, 30, -10, -30, -30, -10, 30, 40, 40, 30, -10, -30, -30, -10, 20, 30, 30, 20, -10, -30, -30, -20, -10, 0, 0, -10, -20, -30, -50, -40, -30, -20, -20, -30, -40, -50};

int ev_evaluate(_board *board){
    if (board->game_state == DRAW){
        return 0;
    }
    if (board->game_state != ONGOING){
        return INT_MIN + 1;
    }

    int white_score = 0;
    int black_score = 0;
    uint64_t white_pieces = board->white_pieces;
    uint64_t black_pieces = board->black_pieces;
    int phase_value = 0;
    int black_king_square = 0;
    int white_king_square = 0;
    uint64_t white_control = 0;
    uint64_t black_control = 0;

    while (white_pieces != 0){
        int square = bb_pop_lsb(&white_pieces) - 1;
        _piece piece = board->piece_array[square];
        white_score += white_piece_values[piece];
        phase_value += piece_phase_value[piece];
        
        switch(piece){
            case W_PAWN:
                white_score += 3 * (square / 8) + pawn_table[square];
                white_control |= pawn_attacks[WHITE][square];
                break;
            case W_QUEEN:
                white_score += queen_table[square];
                break;
            case W_ROOK:
                white_score += rook_table[square];
                white_control |= mv_get_semi_legal_rook_moves(board, square);
                break;
            case W_BISHOP:
                white_score += bishop_table[square];
                white_control |= mv_get_semi_legal_bishop_moves(board, square);
                break;
            case W_KNIGHT:
                white_score += knight_table[square];
                white_control |= knight_attacks[square];
                break;
            case W_KING:
                white_king_square = square;
                break;
            default:
                break;
        }
    }

    while (black_pieces != 0){
        int square = bb_pop_lsb(&black_pieces) - 1;
        _piece piece = board->piece_array[square];
        black_score += black_piece_values[piece];
        phase_value += piece_phase_value[piece];
        
        switch(piece){
            case B_PAWN:
                black_score += 3 * (7 - (square / 8)) + pawn_table[63 - square];
                black_control |= pawn_attacks[BLACK][square];
                break;
            case B_QUEEN:
                black_score += queen_table[63 - square];
                break;
            case B_ROOK:
                black_score += rook_table[63 - square];
                black_control |= mv_get_semi_legal_rook_moves(board, square);
                break;
            case B_BISHOP:
                black_score += bishop_table[63 - square];
                black_control |= mv_get_semi_legal_bishop_moves(board, square);
                break;
            case B_KNIGHT:
                black_score += knight_table[63 - square];
                black_control |= knight_attacks[square];
                break;
            case B_KING:
                black_king_square = square;
                break;
            default:
                break;
        }
    }

    // white_score += bb_get_bits_set(white_control);
    // black_score += bb_get_bits_set(black_control);

    int endgame_value = king_end_table[white_king_square];
    white_score += (king_mid_table[white_king_square] - endgame_value) * phase_value / TOTAL_PHASE_VALUE + endgame_value;
    endgame_value = king_end_table[63 - black_king_square];
    black_score += (king_mid_table[63 - black_king_square] - endgame_value) * phase_value / TOTAL_PHASE_VALUE + endgame_value;

    int score = board->turn == WHITE ? white_score - black_score : black_score - white_score;
    _move moves[MAX_MOVES];
    int move_count;
    mv_generate_semi_legal_moves(board, moves, &move_count);
    for (int i = 0; i < move_count; i++){
        _move move = moves[i];
        score += mobility_values[move.piece];
        if (move.capture != NONE){
            score++;
        }
    }
    
    mv_generate_enemy_moves(board, moves, &move_count);
    for (int i = 0; i < move_count; i++){
        _move move = moves[i];
        score -= mobility_values[move.piece];
        if (move.capture != NONE){
            score--;
        }
    }

    return score;
}
