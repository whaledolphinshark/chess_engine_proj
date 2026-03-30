#include <stdlib.h>

#include "chess_engine/moves.h"
#include "chess_engine/board.h"
#include "moves_internal.h"
#include "utils/bitboard_util.h"
#include "utils/error_handling.h"

#include <stdio.h>

static void add_moves_to_buffer(_board *restrict board, _move move_buffer[restrict MAX_MOVES], int *restrict move_count, const int square, uint64_t moves){
    while (moves != 0){
        move_buffer[*move_count] = mv_init_move(board, square, bb_pop_lsb(&moves) - 1, NONE);
        (*move_count)++;
    }
}

static void add_pawn_moves_to_buffer(_board *restrict board, _move move_buffer[restrict MAX_MOVES], int *restrict move_count, const int square, uint64_t moves){
    uint64_t promotion_rank;
    _piece promotions[4];
    if (board->turn == WHITE){
        promotion_rank = 18374686479671623680UL;
        promotions[0] = W_QUEEN;
        promotions[1] = W_ROOK;
        promotions[2] = W_BISHOP;
        promotions[3] = W_KNIGHT;
    }
    else{
        promotion_rank = 255UL;
        promotions[0] = B_QUEEN;
        promotions[1] = B_ROOK;
        promotions[2] = B_BISHOP;
        promotions[3] = B_KNIGHT;
    }

    while (moves != 0){
        const int target = bb_pop_lsb(&moves) - 1;
        if (((1UL << target) & promotion_rank) == 0){
            move_buffer[*move_count] = mv_init_move(board, square, target, NONE);
            (*move_count)++;
        }
        else{
            move_buffer[*move_count] = mv_init_move(board, square, target, promotions[0]);
            (*move_count)++;
            move_buffer[*move_count] = mv_init_move(board, square, target, promotions[1]);
            (*move_count)++;
            move_buffer[*move_count] = mv_init_move(board, square, target, promotions[2]);
            (*move_count)++;
            move_buffer[*move_count] = mv_init_move(board, square, target, promotions[3]);
            (*move_count)++;
        }
    }
}

// returns an int, 0 no castle, 1 left castle, 2 right castle, 3 both
static int check_castle(_board *restrict board, const _color side){
    if (board->in_check == 1){
        return 0;
    }

    int code = 0;
    int safe_squares[4];
    if (side == WHITE){
        safe_squares[0] = cb_is_square_attacked(2, board, board->board, side);
        safe_squares[1] = cb_is_square_attacked(3, board, board->board, side);
        safe_squares[2] = cb_is_square_attacked(5, board, board->board, side);
        safe_squares[3] = cb_is_square_attacked(6, board, board->board, side);
        // left castle
        if ((board->castling_rights & 1UL) != 0 && safe_squares[0] == 0 && safe_squares[1] == 0 && (board->board & 14UL) == 0){
            code++;
        }
        // right castle
        if ((board->castling_rights & 2UL) != 0 && safe_squares[2] == 0 && safe_squares[3] == 0 && (board->board & 96UL) == 0){
            code += 2;
        }
    }
    else{
        safe_squares[0] = cb_is_square_attacked(58, board, board->board, side);
        safe_squares[1] = cb_is_square_attacked(59, board, board->board, side);
        safe_squares[2] = cb_is_square_attacked(61, board, board->board, side);
        safe_squares[3] = cb_is_square_attacked(62, board, board->board, side);
        if ((board->castling_rights & 4UL) != 0 && safe_squares[0] == 0 && safe_squares[1] == 0 && (board->board & 1008806316530991104UL) == 0){
            code++;
        }
        if ((board->castling_rights & 8UL) != 0 && safe_squares[2] == 0 && safe_squares[3] == 0 && (board->board & 6917529027641081856UL) == 0){
            code += 2;
        }
    }

    return code;
}

// gets all legal non king moves in check, 
// with an exception where a checking pawn can be captured by en passant
// such a case is detected with en_passant_checker and needs to be handled separately outside the function
// function assumes there is at least one checking piece
static uint64_t get_legal_non_king_moves_in_check(_board *restrict board, const _color side, const int king_square, int *restrict en_passant_checker){
    // pawns, knights
    uint64_t pawns;
    uint64_t knights;
    // rooks, bishops, queens, 0 = diagonal, 1 = orthogonal
    uint64_t sliders[2];
    uint64_t non_king_moves_in_check = 0;
    *en_passant_checker = 0;
    if (side == WHITE){
        pawns = board->bitboards[B_PAWN];
        knights = board->bitboards[B_KNIGHT];
        sliders[0] = board->bitboards[B_BISHOP] | board->bitboards[B_QUEEN];
        sliders[1] = board->bitboards[B_ROOK] | board->bitboards[B_QUEEN];
    }
    else{
        pawns = board->bitboards[W_PAWN];
        knights = board->bitboards[W_KNIGHT];
        sliders[0] = board->bitboards[W_BISHOP] | board->bitboards[W_QUEEN];
        sliders[1] = board->bitboards[W_ROOK] | board->bitboards[W_QUEEN];
    }

    // pawn, knight, king attacks
    non_king_moves_in_check |= (pawn_attacks[side][king_square] & pawns) | (knight_attacks[king_square] & knights);
    int num_attackers = bb_get_bits_set(non_king_moves_in_check);

    // if double or more check, only king moves are available
    if (num_attackers > 1){
        return 0;
    }

    // check for special case where a checking pawn can be taken en passant
    if (board->en_passant_square != 0 && (non_king_moves_in_check & pawns) != 0){
        *en_passant_checker = 1;
    }

    for (int i = 0; i < 8; i++){
        uint64_t ray = rays[king_square][i];
        uint64_t occupied = board->board & ray;
        if (occupied == 0){
            continue;
        }

        int potential_attacker_square;
        uint64_t potential_attack_ray;
        if (i < 4){
            potential_attacker_square = bb_get_lsb(board->board & ray) - 1;
            potential_attack_ray = ray & ~(UINT64_MAX << potential_attacker_square << 1);
        }
        else{
            potential_attacker_square = bb_get_msb(board->board & ray) - 1;
            potential_attack_ray = ray & (UINT64_MAX << potential_attacker_square);
        }

        if (((1UL << potential_attacker_square) & sliders[i % 2]) != 0){
            non_king_moves_in_check |= potential_attack_ray;
            num_attackers += 1;
            if (num_attackers > 1){
                return 0;
            }
        }
    }

    return non_king_moves_in_check;
}

static void get_pin_rays(const int king_square, _board *restrict board, const _color side, uint64_t pin_ray_buffer[restrict 64]){
    uint64_t friendly_pieces;
    // 0 = diagonal, 1 = orthogonal
    uint64_t attackers[2];
    // for use only if needed to check if en passant is not pinned by horizontal slider
    uint64_t target_pawn_neighbors, friendly_pawns;
    if (side == WHITE){
        friendly_pieces = board->white_pieces;
        attackers[0] = board->bitboards[B_BISHOP] | board->bitboards[B_QUEEN];
        attackers[1] = board->bitboards[B_ROOK] | board->bitboards[B_QUEEN];
        target_pawn_neighbors = board->board & ~(attackers[1] | board->bitboards[W_KING]) & en_passant_pinned_mask[BLACK][board->en_passant_square % 8];
        friendly_pawns = board->bitboards[W_PAWN];
    }
    else{
        friendly_pieces = board->black_pieces;
        attackers[0] = board->bitboards[W_BISHOP] | board->bitboards[W_QUEEN];
        attackers[1] = board->bitboards[W_ROOK] | board->bitboards[W_QUEEN];
        target_pawn_neighbors = board->board & ~(attackers[1] | board->bitboards[B_KING]) & en_passant_pinned_mask[WHITE][board->en_passant_square % 8];
        friendly_pawns = board->bitboards[B_PAWN];
    }

    // if piece not pinned, no pin ray
    for (int i = 0; i < 64; i++){
        pin_ray_buffer[i] = UINT64_MAX;
    }

    // from northwest clockwise
    for (int i = 0; i < 8; i++){
        const uint64_t ray = rays[king_square][i];
        uint64_t occupied = ray & board->board;
        if (bb_get_bits_set(occupied) < 2){
            continue;
        }

        int pinned_square, pinner_square;
        if (i < 4){
            pinned_square = bb_pop_lsb(&occupied) - 1;
            pinner_square = bb_pop_lsb(&occupied) - 1;
        }
        else{
            pinned_square = bb_pop_msb(&occupied) - 1;
            pinner_square = bb_pop_msb(&occupied) - 1;
        }

        if (((1UL << pinner_square) & attackers[i % 2]) != 0 && ((1UL << pinned_square) & friendly_pieces) != 0){
            pin_ray_buffer[pinned_square] = ray;
        }
    }

    // check for special case of en passant being pinned by horizontal slider, example: 8/2p5/3p4/KP5r/1R3pPk/8/4P3/8 b - g3 0 1
    if (board->en_passant_square != 0 && bb_get_bits_set(target_pawn_neighbors) == 1){
        const int pinned_square = bb_pop_lsb(&target_pawn_neighbors) - 1;
        // check if that piece is a pawn or not
        if (((1UL << pinned_square) & friendly_pawns) == 0){
            return;
        }

        // detect if king is on the same rank
        if (pinned_square / 8 == king_square / 8){
            // from that king fire ray in direction of pinned_square and see if it hits a rook or queen
            const int target_pawn_square = board->en_passant_square + (side == WHITE ? -8 : 8);
            uint64_t pieces = board->board & ~((1UL << pinned_square) | (1UL << target_pawn_square));
            int pinner_square;
            if (king_square % 8 > pinned_square % 8){
                // west
                uint64_t occupied = rays[king_square][7] & pieces;
                if (bb_get_bits_set(occupied) == 0){
                    return;
                }
                pinner_square = bb_pop_msb(&occupied) - 1;
            }
            else{
                // east
                uint64_t occupied = rays[king_square][3] & pieces;
                if (bb_get_bits_set(occupied) == 0){
                    return;
                }
                pinner_square = bb_pop_lsb(&occupied) - 1;
            }

            if (((1UL << pinner_square) & attackers[1]) != 0){
                // prevent from taking en passant, but can still move normally otherwise
                pin_ray_buffer[pinned_square] &= ~(1UL << board->en_passant_square);
            }
        }
    }
}

static uint64_t get_semi_legal_rook_moves(_board *restrict board, const int square){
    const uint64_t blockers = rook_masks[square] & board->board;
    const uint64_t index = (blockers * rook_magics[square]) >> rook_shifts[square];
    return rook_attacks[square][index];
}

static uint64_t get_semi_legal_bishop_moves(_board *restrict board, const int square){
    const uint64_t blockers = bishop_masks[square] & board->board;
    const uint64_t index = (blockers * bishop_magics[square]) >> bishop_shifts[square];
    return bishop_attacks[square][index];
}

static uint64_t get_semi_legal_pawn_moves(_board *restrict board, const int square){
    const uint64_t friendly_pieces = board->turn == WHITE ? board->white_pieces : board->black_pieces;
    const uint64_t enemy_pieces = board->board ^ friendly_pieces;
    const uint64_t en_passant_mask = board->en_passant_square == 0 ? 0 : 1UL << board->en_passant_square;
    const uint64_t forward = board->turn == WHITE ? 1UL << (square + 8) : 1UL << (square - 8);
    const uint64_t attack_mask = pawn_attacks[board->turn][square] & ~friendly_pieces & (enemy_pieces | en_passant_mask);
    const uint64_t movement_mask = (board->board & forward) != 0 ? 0 : pawn_moves[board->turn][square] & ~board->board;
    return attack_mask | movement_mask;
}

static uint64_t get_legal_king_moves(_board *restrict board, const int king_square){
    const uint64_t friendly_pieces = board->turn == WHITE ? board->white_pieces : board->black_pieces;
    const uint64_t castle_mask = board->in_check == 1 ? 0 : castle_moves[board->turn][check_castle(board, board->turn)];
    const uint64_t occupied = board->board ^ (1UL << king_square);
    uint64_t potential_danger_squares = king_attacks[king_square] & ~friendly_pieces;
    uint64_t danger_squares = 0UL;
    while (potential_danger_squares != 0){
        const int square = bb_pop_lsb(&potential_danger_squares) - 1;
        if (cb_is_square_attacked(square, board, occupied, board->turn) == 1){
            danger_squares |= (1UL << square);
            // i can make it quicker i think
        }
    }

    return (king_attacks[king_square] & ~friendly_pieces & ~danger_squares) | castle_mask;
}

void mv_generate_moves(_board *restrict board, _move move_buffer[restrict MAX_MOVES], int *restrict move_count){
    if (board == NULL){
        eh_die("passed in null pointer");
    }
    *move_count = 0;

    uint64_t pawns, rooks, knights, bishops, queens, king;
    uint64_t friendly_pieces;
    if (board->turn == WHITE){
        pawns = board->bitboards[W_PAWN];
        rooks = board->bitboards[W_ROOK];
        knights = board->bitboards[W_KNIGHT];
        bishops = board->bitboards[W_BISHOP];
        queens = board->bitboards[W_QUEEN];
        king = board->bitboards[W_KING];
        friendly_pieces = board->white_pieces;
    }
    else{
        pawns = board->bitboards[B_PAWN];
        rooks = board->bitboards[B_ROOK];
        knights = board->bitboards[B_KNIGHT];
        bishops = board->bitboards[B_BISHOP];
        queens = board->bitboards[B_QUEEN];
        king = board->bitboards[B_KING];
        friendly_pieces = board->black_pieces;
    }

    uint64_t pin_ray_buffer[64];
    const int king_square = bb_get_lsb(king) - 1;
    get_pin_rays(king_square, board, board->turn, pin_ray_buffer);

    const uint64_t king_moves = get_legal_king_moves(board, king_square);
    add_moves_to_buffer(board, move_buffer, move_count, king_square, king_moves);

    int en_passant_checker = 0;
    uint64_t non_king_moves_in_check = board->in_check == 1 ? get_legal_non_king_moves_in_check(board, board->turn, king_square, &en_passant_checker) : UINT64_MAX;
    // if there are multiple checkers, non_king_moves_in_check will be 0
    if (non_king_moves_in_check == 0){
        return;
    }

    // rook
    while (rooks != 0){
        const int square = bb_pop_lsb(&rooks) - 1;
        const uint64_t moves = get_semi_legal_rook_moves(board, square) & ~friendly_pieces & pin_ray_buffer[square] & non_king_moves_in_check;
        add_moves_to_buffer(board, move_buffer, move_count, square, moves);
    }

    // bishop
    while (bishops != 0){
        const int square = bb_pop_lsb(&bishops) - 1;
        const uint64_t moves = get_semi_legal_bishop_moves(board, square) & ~friendly_pieces & pin_ray_buffer[square] & non_king_moves_in_check;
        add_moves_to_buffer(board, move_buffer, move_count, square, moves);
    }

    // queen
    while (queens != 0){
        const int square = bb_pop_lsb(&queens) - 1;
        const uint64_t orthogonal_moves = get_semi_legal_rook_moves(board, square);
        const uint64_t diagonal_moves = get_semi_legal_bishop_moves(board, square);
        const uint64_t moves = (diagonal_moves | orthogonal_moves) & ~friendly_pieces & pin_ray_buffer[square] & non_king_moves_in_check;
        add_moves_to_buffer(board, move_buffer, move_count, square, moves);
    }

    // knight
    while (knights != 0){
        const int square = bb_pop_lsb(&knights) - 1;
        const uint64_t moves = knight_attacks[square] & ~friendly_pieces & pin_ray_buffer[square] & non_king_moves_in_check;
        add_moves_to_buffer(board, move_buffer, move_count, square, moves);
    }

    // pawn
    if (en_passant_checker == 1){
        non_king_moves_in_check |= (1UL << board->en_passant_square);
    }
    while (pawns != 0){
        const int square = bb_pop_lsb(&pawns) - 1;
        const uint64_t moves = get_semi_legal_pawn_moves(board, square) & pin_ray_buffer[square] & non_king_moves_in_check;
        add_pawn_moves_to_buffer(board, move_buffer, move_count, square, moves);
    }
}

void mv_generate_enemy_moves(_board *restrict board, _move move_buffer[restrict MAX_MOVES], int *restrict move_count){
    int temp_en_passant_square = board->en_passant_square;
    int temp_turn = board->turn;
    int temp_check = board->in_check;
    board->en_passant_square = 0;
    board->turn = board->turn == WHITE ? BLACK : WHITE;
    board->in_check = 0;
    mv_generate_moves(board, move_buffer, move_count);
    board->en_passant_square = temp_en_passant_square;
    board->turn = temp_turn;
    board->in_check = temp_check;
}

int mv_has_moves(_board *restrict board){
    uint64_t pawns, rooks, knights, bishops, queens, king;
    uint64_t friendly_pieces;
    if (board->turn == WHITE){
        pawns = board->bitboards[W_PAWN];
        rooks = board->bitboards[W_ROOK];
        knights = board->bitboards[W_KNIGHT];
        bishops = board->bitboards[W_BISHOP];
        queens = board->bitboards[W_QUEEN];
        king = board->bitboards[W_KING];
        friendly_pieces = board->white_pieces;
    }
    else{
        pawns = board->bitboards[B_PAWN];
        rooks = board->bitboards[B_ROOK];
        knights = board->bitboards[B_KNIGHT];
        bishops = board->bitboards[B_BISHOP];
        queens = board->bitboards[B_QUEEN];
        king = board->bitboards[B_KING];
        friendly_pieces = board->black_pieces;
    }

    const int king_square = bb_get_lsb(king) - 1;
    const uint64_t king_moves = get_legal_king_moves(board, king_square);
    if (king_moves != 0){
        return 1;
    }

    int en_passant_checker = 0;
    uint64_t non_king_moves_in_check = board->in_check == 1 ? get_legal_non_king_moves_in_check(board, board->turn, king_square, &en_passant_checker) : UINT64_MAX;
    if (non_king_moves_in_check == 0){
        return 0;
    }

    uint64_t pin_ray_buffer[64];
    get_pin_rays(king_square, board, board->turn, pin_ray_buffer);

    // knight
    while (knights != 0){
        const int square = bb_pop_lsb(&knights) - 1;
        const uint64_t moves = knight_attacks[square] & ~friendly_pieces & pin_ray_buffer[square] & non_king_moves_in_check;
        if (moves != 0){
            return 1;
        }
    }

    // rook
    while (rooks != 0){
        const int square = bb_pop_lsb(&rooks) - 1;
        const uint64_t moves = get_semi_legal_rook_moves(board, square) & ~friendly_pieces & pin_ray_buffer[square] & non_king_moves_in_check;
        if (moves != 0){
            return 1;
        }
    }

    // bishop
    while (bishops != 0){
        const int square = bb_pop_lsb(&bishops) - 1;
        const uint64_t moves = get_semi_legal_bishop_moves(board, square) & ~friendly_pieces & pin_ray_buffer[square] & non_king_moves_in_check;
        if (moves != 0){
            return 1;
        }
    }

    // queen
    while (queens != 0){
        const int square = bb_pop_lsb(&queens) - 1;
        const uint64_t orthogonal_moves = get_semi_legal_rook_moves(board, square);
        const uint64_t diagonal_moves = get_semi_legal_bishop_moves(board, square);
        const uint64_t moves = (diagonal_moves | orthogonal_moves) & ~friendly_pieces & pin_ray_buffer[square] & non_king_moves_in_check;
        if (moves != 0){
            return 1;
        }
    }

    // pawn
    if (en_passant_checker == 1){
        non_king_moves_in_check |= (1UL << board->en_passant_square);
    }
    while (pawns != 0){
        const int square = bb_pop_lsb(&pawns) - 1;
        const uint64_t moves = get_semi_legal_pawn_moves(board, square) & pin_ray_buffer[square] & non_king_moves_in_check;
        if (moves != 0){
            return 1;
        }
    }

    return 0;
}