#include <stdlib.h>

#include "chess_engine/moves.h"
#include "chess_engine/board.h"
#include "moves_internal.h"
#include "utils/bitboard_util.h"

#include <stdio.h>

static void add_moves_to_buffer(_board *board, int square, uint64_t moves){
    while (moves != 0){
        int target = bb_pop_lsb(&moves) - 1;
        board->move_pool[board->move_count] = mv_init_move(board, square, target, NONE);
        board->move_count++;
    }
}

static void add_pawn_moves_to_buffer(_board *board, int square, uint64_t moves){
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
        int target = bb_pop_lsb(&moves) - 1;
        if (((1UL << target) & promotion_rank) == 0){
            board->move_pool[board->move_count] = mv_init_move(board, square, target, NONE);
            board->move_count++;
        }
        else{
            board->move_pool[board->move_count] = mv_init_move(board, square, target, promotions[0]);
            board->move_count++;
            board->move_pool[board->move_count] = mv_init_move(board, square, target, promotions[1]);
            board->move_count++;
            board->move_pool[board->move_count] = mv_init_move(board, square, target, promotions[2]);
            board->move_count++;
            board->move_pool[board->move_count] = mv_init_move(board, square, target, promotions[3]);
            board->move_count++;
        }
    }
}

// returns an int, 0 no castle, 1 left castle, 2 right castle, 3 both
static int check_castle(_board *board, _color side){
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

static uint64_t get_legal_moves_in_check(_board *board, _color side, int king_square, int *num_attackers){
    _piece attackers[6];
    uint64_t moves_in_check = 0;
    *num_attackers = 0;
    if (side == WHITE){
        attackers[0] = B_KING;
        attackers[1] = B_PAWN;
        attackers[2] = B_ROOK;
        attackers[3] = B_KNIGHT;
        attackers[4] = B_BISHOP;
        attackers[5] = B_QUEEN;
    }
    else{
        attackers[0] = W_KING;
        attackers[1] = W_PAWN;
        attackers[2] = W_ROOK;
        attackers[3] = W_KNIGHT;
        attackers[4] = W_BISHOP;
        attackers[5] = W_QUEEN;
    }

    // pawn, knight, king attacks
    moves_in_check |= (pawn_attacks[side][king_square] & board->bitboards[attackers[1]]) | 
            (knight_attacks[king_square] & board->bitboards[attackers[3]]) | 
            (king_attacks[king_square] & board->bitboards[attackers[0]]);
    *num_attackers += bb_get_bits_set(moves_in_check);

    // rooks, queens
    // north, east, south, west
    uint64_t sliders = board->bitboards[attackers[2]] | board->bitboards[attackers[5]] | board->bitboards[attackers[4]];
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

        if ((sliders & (1UL << potential_attacker_square)) != 0){
            moves_in_check |= potential_attack_ray;
            (*num_attackers)++;
        }
    }
    
    return moves_in_check;
}

static void get_pin_rays(int king_square, _board *board, _color side, uint64_t pin_ray_buffer[64]){
    uint64_t friendly_pieces;
    // 0 = diagonal, 1 = orthogonal
    uint64_t attackers[2];
    if (side == WHITE){
        friendly_pieces = board->white_pieces;
        attackers[0] = board->bitboards[B_BISHOP] | board->bitboards[B_QUEEN];
        attackers[1] = board->bitboards[B_ROOK] | board->bitboards[B_QUEEN];
    }
    else{
        friendly_pieces = board->black_pieces;
        attackers[0] = board->bitboards[W_BISHOP] | board->bitboards[W_QUEEN];
        attackers[1] = board->bitboards[W_ROOK] | board->bitboards[W_QUEEN];
    }

    // if piece not pinned, no pin ray
    for (int i = 0; i < 64; i++){
        pin_ray_buffer[i] = UINT64_MAX;
    }

    // from northwest clockwise
    for (int i = 0; i < 8; i++){
        uint64_t ray = rays[king_square][i];
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
}

// int mv_is_square_attacked(int square, _board *board, uint64_t occupied, _color side){
//     _piece attackers[6];
//     if (side == WHITE){
//         attackers[0] = B_KING;
//         attackers[1] = B_PAWN;
//         attackers[2] = B_ROOK;
//         attackers[3] = B_KNIGHT;
//         attackers[4] = B_BISHOP;
//         attackers[5] = B_QUEEN;
//     }
//     else{
//         attackers[0] = W_KING;
//         attackers[1] = W_PAWN;
//         attackers[2] = W_ROOK;
//         attackers[3] = W_KNIGHT;
//         attackers[4] = W_BISHOP;
//         attackers[5] = W_QUEEN;
//     }

//     // pawn, knight, king attacks
//     if ((pawn_attacks[side][square] & board->bitboards[attackers[1]]) != 0){
//         return 1;
//     }
//     if ((knight_attacks[square] & board->bitboards[attackers[3]]) != 0){
//         return 1;
//     }
//     if ((king_attacks[square] & board->bitboards[attackers[0]]) != 0){
//         return 1;
//     }

//     // rooks, queens
//     // north, east, south, west
//     uint64_t orthogonal_attackers = board->bitboards[attackers[2]] | board->bitboards[attackers[5]];
//     // perhaps this may be better, idk
//     // uint64_t potential_attackers = ((1UL << get_lsb(occupied & rays[square][1])) >> 1) | 
//     //                                 ((1UL << get_lsb(occupied & rays[square][3])) >> 1) |
//     //                                 ((1UL << get_msb(occupied & rays[square][5])) >> 1) |
//     //                                 ((1UL << get_msb(occupied & rays[square][7])) >> 1);
//     uint64_t potential_attacker = (1UL << bb_get_lsb(occupied & rays[square][1])) >> 1;
//     if ((orthogonal_attackers & potential_attacker) != 0){
//         return 1;
//     }
//     potential_attacker = (1UL << bb_get_lsb(occupied & rays[square][3])) >> 1;
//     if ((orthogonal_attackers & potential_attacker) != 0){
//         return 1;
//     }
//     potential_attacker = (1UL << bb_get_msb(occupied & rays[square][5])) >> 1;
//     if ((orthogonal_attackers & potential_attacker) != 0){
//         return 1;
//     }
//     potential_attacker = (1UL << bb_get_msb(occupied & rays[square][7])) >> 1;
//     if ((orthogonal_attackers & potential_attacker) != 0){
//         return 1;
//     }

//     // bishops, queens
//     uint64_t diagonal_attackers = board->bitboards[attackers[4]] | board->bitboards[attackers[5]];
//     potential_attacker = (1UL << bb_get_lsb(occupied & rays[square][0])) >> 1;
//     if ((diagonal_attackers & potential_attacker) != 0){
//         return 1;
//     }
//     potential_attacker = (1UL << bb_get_lsb(occupied & rays[square][2])) >> 1;
//     if ((diagonal_attackers & potential_attacker) != 0){
//         return 1;
//     }
//     potential_attacker = (1UL << bb_get_msb(occupied & rays[square][4])) >> 1;
//     if ((diagonal_attackers & potential_attacker) != 0){
//         return 1;
//     }
//     potential_attacker = (1UL << bb_get_msb(occupied & rays[square][6])) >> 1;
//     if ((diagonal_attackers & potential_attacker) != 0){
//         return 1;
//     }

//     return 0;
// }

void mv_generate_moves(_board *board){
    board->move_count = 0;

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
    int king_square = bb_get_lsb(king) - 1;
    get_pin_rays(king_square, board, board->turn, pin_ray_buffer);

    // if king in check
    uint64_t moves_in_check;
    if (board->in_check == 1){
        int num_checkers;
        moves_in_check = get_legal_moves_in_check(board, board->turn, king_square, &num_checkers);
        // if there are multiple checkers the only legal moves should be king moves
        if (num_checkers > 1){
            moves_in_check = 0;
        }
    }
    else{
        moves_in_check = UINT64_MAX;
    }

    // rook
    while (rooks != 0){
        int square = bb_pop_lsb(&rooks) - 1;
        uint64_t blockers = rook_masks[square] & board->board;
        uint64_t index = (blockers * rook_magics[square]) >> rook_shifts[square];
        uint64_t moves = rook_attacks[square][index] & ~friendly_pieces & pin_ray_buffer[square] & moves_in_check;
        add_moves_to_buffer(board, square, moves);
    }

    // bishop
    while (bishops != 0){
        int square = bb_pop_lsb(&bishops) - 1;
        uint64_t blockers = bishop_masks[square] & board->board;
        uint64_t index = (blockers * bishop_magics[square]) >> bishop_shifts[square];
        uint64_t moves = bishop_attacks[square][index] & ~friendly_pieces & pin_ray_buffer[square] & moves_in_check;
        add_moves_to_buffer(board, square, moves);
    }

    // queen
    while (queens != 0){
        int square = bb_pop_lsb(&queens) - 1;
        uint64_t diagonal_blockers = bishop_masks[square] & board->board;
        uint64_t diagonal_index = (diagonal_blockers * bishop_magics[square]) >> bishop_shifts[square];
        uint64_t diagonal_moves = bishop_attacks[square][diagonal_index] & ~friendly_pieces;
        uint64_t orthogonal_blockers = rook_masks[square] & board->board;
        uint64_t orthogonal_index = (orthogonal_blockers * rook_magics[square]) >> rook_shifts[square];
        uint64_t orthogonal_moves = rook_attacks[square][orthogonal_index] & ~friendly_pieces;
        uint64_t moves = (diagonal_moves | orthogonal_moves) & pin_ray_buffer[square] & moves_in_check;
        add_moves_to_buffer(board, square, moves);
    }

    // knight
    while (knights != 0){
        int square = bb_pop_lsb(&knights) - 1;
        uint64_t moves = knight_attacks[square] & ~friendly_pieces & pin_ray_buffer[square] & moves_in_check;
        add_moves_to_buffer(board, square, moves);
    }

    // pawn
    uint64_t enemy_pieces = board->board ^ friendly_pieces;
    uint64_t en_passant_mask = board->en_passant_square == 0 ? 0 : 1UL << board->en_passant_square;
    while (pawns != 0){
        int square = bb_pop_lsb(&pawns) - 1;
        uint64_t forward;
        uint64_t jump;
        if (board->turn == WHITE){
            forward = 1UL << square << 8;
            jump = forward << 8;
        }
        else{
            forward = 1UL << square >> 8;
            jump = forward >> 8;
        }

        uint64_t attack_mask = pawn_attacks[board->turn][square] & ~friendly_pieces & (enemy_pieces | en_passant_mask);
        uint64_t movement_mask;
        if ((forward & board->board) != 0){
            movement_mask = 0;
        }
        else{
            movement_mask = pawn_moves[board->turn][square];
            // check pawn jump if applicable
            if ((jump & movement_mask) != 0 && (jump & board->board) != 0){
                movement_mask &= ~jump;
            }
        }

        uint64_t moves = (attack_mask | movement_mask) & pin_ray_buffer[square] & moves_in_check;
        add_pawn_moves_to_buffer(board, square, moves);
    }

    // king
    // get danger squares
    uint64_t potential_danger_squares = king_attacks[king_square] & ~friendly_pieces;
    uint64_t occupied = board->board ^ (1UL << king_square);
    uint64_t danger_squares = 0UL;
    while (potential_danger_squares != 0){
        int square = bb_pop_lsb(&potential_danger_squares) - 1;
        if (cb_is_square_attacked(square, board, occupied, board->turn) == 1){
            danger_squares |= (1UL << square);
            // i can make it quicker i think
        }
    }
    // check castles
    uint64_t castle_mask = castle_moves[board->turn][check_castle(board, board->turn)];
    uint64_t moves = (king_attacks[king_square] & ~friendly_pieces & ~danger_squares) | castle_mask;
    add_moves_to_buffer(board, king_square, moves);
}