#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "chess_engine/board.h"
#include "board_internal.h"
#include "chess_engine/init_chess_engine.h"
#include "chess_engine/moves.h"
#include "chess_engine/transposition_table.h"
#include "utils/bitboard_util.h"
#include "utils/error_handling.h"

int en_passant_squares[2][8] = {{40, 41, 42, 43, 44, 45, 46, 47}, {16, 17, 18, 19, 20, 21, 22, 23}};

void inline cb_calculate_game_state(_board *board){
    // checkmate, stalemate, 50 move rule
    if (board->in_check == 1 && board->move_count == 0){
        board->game_state = board->turn == WHITE ? B_WIN : W_WIN;
        return;
    }
    else if ((board->in_check == 0 && board->move_count == 0) || board->halfmove_clock >= 100){
        board->game_state = DRAW;
        return;
    }

    // threefold repetition
    if (tt_is_key_in_table(board->history, board->zobrist_hash) == 1){
        if (*((int *)tt_get_item(board->history, board->zobrist_hash)) == 3){
            board->game_state = DRAW;
            return;
        }
    }

    // insufficient material
    // KN vs K, KB vs K, K vs K
    int num_pieces = bb_get_bits_set(board->board);
    int num_knights = bb_get_bits_set(board->bitboards[W_KNIGHT] | board->bitboards[B_KNIGHT]);
    int num_bishops = bb_get_bits_set(board->bitboards[W_BISHOP] | board->bitboards[B_BISHOP]);
    if (num_pieces == 2 || (num_pieces == 3 && (num_knights == 1 || num_bishops == 1))){
        board->game_state = DRAW;
    }
    else if (num_pieces == 4 && bb_get_bits_set(board->bitboards[W_BISHOP]) == 1 && bb_get_bits_set(board->bitboards[B_BISHOP] == 1)){
        // KB vs KB (same color bishops)
        int w_bishop_square = bb_get_lsb(board->bitboards[W_BISHOP]) - 1;
        int b_bishop_square = bb_get_lsb(board->bitboards[B_BISHOP]) - 1;
        int wb_rank = w_bishop_square / 8;
        int bb_rank = b_bishop_square / 8;
        int wb_file = w_bishop_square % 8;
        int bb_file = w_bishop_square % 8;
        if ((abs(wb_rank - bb_rank) + abs(wb_file - bb_file)) % 2 == 0){
            board->game_state = DRAW;
        }
    }
}

_board *cb_create_board(){
    if (is_chess_engine_ready() != 1){
        eh_die("cb_init_chess_board() not called");
    }
    _board *board = (_board *)malloc(sizeof(_board));
    if (board == NULL){
        eh_die("malloc() failed");
    }

    board->bitboards[W_KING] = 16UL;
    board->bitboards[W_PAWN] = 65280UL;
    board->bitboards[W_ROOK] = 129UL;
    board->bitboards[W_KNIGHT] = 66UL;
    board->bitboards[W_BISHOP] = 36UL;
    board->bitboards[W_QUEEN] = 8UL;
    board->bitboards[B_KING] = 1152921504606846976UL;
    board->bitboards[B_PAWN] = 71776119061217280UL;
    board->bitboards[B_ROOK] = 9295429630892703744UL;
    board->bitboards[B_KNIGHT] = 4755801206503243776UL;
    board->bitboards[B_BISHOP] = 2594073385365405696UL;
    board->bitboards[B_QUEEN] = 576460752303423488UL;

    board->white_pieces = 65535UL;
    board->black_pieces = 18446462598732840960UL;
    board->board = 18446462598732906495UL;

    uint8_t first_rank[8] = {W_ROOK, W_KNIGHT, W_BISHOP, W_QUEEN, W_KING, W_BISHOP, W_KNIGHT, W_ROOK};
    for (int i = 0; i < 8; i++){
        board->piece_array[i] = first_rank[i];
        board->piece_array[i + 8] = W_PAWN;
        board->piece_array[i + 16] = NONE;
        board->piece_array[i + 24] = NONE;
        board->piece_array[i + 32] = NONE;
        board->piece_array[i + 40] = NONE;
        board->piece_array[i + 48] = B_PAWN;
        board->piece_array[i + 56] = first_rank[i] + 6;
    }

    board->plies = 0;
    board->turn = WHITE;
    board->en_passant_square = 0;
    board->castling_rights = 15;
    board->halfmove_clock = 0;
    board->fullmove_clock = 1;
    board->in_check = 0;
    board->game_state = ONGOING;

    board->zobrist_hash = 0;
    uint64_t pieces_bitboard = board->board;
    while (pieces_bitboard != 0){
        int square = bb_pop_lsb(&pieces_bitboard) - 1;
        _piece piece = board->piece_array[square];
        board->zobrist_hash ^= piece_keys[square][piece];
    }

    board->zobrist_hash ^= castling_keys[board->castling_rights];
    board->history = tt_create_transposition_table(sizeof(int));
    int one = 1;
    tt_insert_item(board->history, board->zobrist_hash, &one);
    
    board->previous_moves = (_list *)gl_create_list(sizeof(_move_state));

    mv_generate_moves(board);

    return board;
}

// assumes move is valid
void cb_make_move(_board *board, _move move){
    if (board->game_state != ONGOING){
        eh_die("game has ended");
    }

    _move_state move_state = {move, board->castling_rights, board->halfmove_clock, board->en_passant_square};
    if (board->en_passant_square != 0){
        board->zobrist_hash ^= en_passant_keys[board->en_passant_square % 8];
    }
    board->en_passant_square = 0;
    board->halfmove_clock++;

    // make the move
    // if it is a promotion, i only need to check if it is a capture as well
    // no need to check jumps and castling rights
    if (move.special_move == PROMOTION){
        board->bitboards[move.promotion] ^= 1UL << move.to;
        board->bitboards[move.piece] ^= 1UL << move.from;
        board->piece_array[move.to] = move.promotion;
        board->piece_array[move.from] = NONE;
        board->zobrist_hash ^= piece_keys[move.to][move.promotion] ^ piece_keys[move.from][move.piece];
        board->halfmove_clock = 0;
    }
    else{
        board->bitboards[move.piece] ^= (1UL << move.from) | (1UL << move.to);
        board->piece_array[move.to] = move.piece;
        board->piece_array[move.from] = NONE;
        board->zobrist_hash ^= piece_keys[move.to][move.piece] ^ piece_keys[move.from][move.piece];

        // check en passant and castling rights
        _piece piece = move.piece;
        if (piece == W_PAWN || piece == B_PAWN){
            board->halfmove_clock = 0;
            if (abs(move.to - move.from) == 16){
                board->en_passant_square = en_passant_squares[board->turn][move.from % 8];
                board->zobrist_hash ^= en_passant_keys[board->en_passant_square % 8];
            }
        }
        else if (piece == W_ROOK || piece == B_ROOK){
            board->zobrist_hash ^= castling_keys[board->castling_rights];
            switch (move.from){
                case 0:
                    board->castling_rights &= ~1UL;
                    break;
                case 7:
                    board->castling_rights &= ~2UL;
                    break;
                case 56:
                    board->castling_rights &= ~4UL;
                    break;
                case 63:
                    board->castling_rights &= ~8UL;
                    break;
            }
            board->zobrist_hash ^= castling_keys[board->castling_rights];
        }
        else if (piece == W_KING || piece == B_KING){
	        if (move.special_move == CASTLE){
                switch (move.to){
                    case 2:
                        board->bitboards[W_ROOK] ^= 9UL;
                        board->piece_array[0] = NONE;
                        board->piece_array[3] = W_ROOK;
                        board->zobrist_hash ^= piece_keys[3][W_ROOK] ^ piece_keys[0][W_ROOK];
                        break;
                    case 6:
                        board->bitboards[W_ROOK] ^= 160UL;
                        board->piece_array[7] = NONE;
                        board->piece_array[5] = W_ROOK;
                        board->zobrist_hash ^= piece_keys[5][W_ROOK] ^ piece_keys[7][W_ROOK];
                        break;
                    case 58:
                        board->bitboards[B_ROOK] ^= 648518346341351424UL;
                        board->piece_array[56] = NONE;
                        board->piece_array[59] = B_ROOK;
                        board->zobrist_hash ^= piece_keys[59][B_ROOK] ^ piece_keys[56][B_ROOK];
                        break;
                    case 62:
                        board->bitboards[B_ROOK] ^= 11529215046068469760UL;
                        board->piece_array[63] = NONE;
                        board->piece_array[61] = B_ROOK;
                        board->zobrist_hash ^= piece_keys[61][B_ROOK] ^ piece_keys[63][B_ROOK];
                        break;
                }
            }

            board->zobrist_hash ^= castling_keys[board->castling_rights];
            if (move.from == 4){
                board->castling_rights &= ~3UL;
            }
            else if (move.from == 60){
                board->castling_rights &= ~12UL;
            }
            board->zobrist_hash ^= castling_keys[board->castling_rights];
        }
    }

    // check captures
    if (move.capture != NONE){
        board->halfmove_clock = 0;
        // check if en passant or not
        if (move.special_move == EN_PASSANT){
            int captured_pawn_square = move.to + (board->turn == WHITE ? -8 : 8);
            board->zobrist_hash ^= piece_keys[captured_pawn_square][move.capture];
            board->bitboards[move.capture] ^= 1UL << captured_pawn_square;
            board->piece_array[captured_pawn_square] = NONE;
        }
        else{
            board->zobrist_hash ^= piece_keys[move.to][move.capture];
            board->bitboards[move.capture] ^= 1UL << move.to;
        }
    }

    board->white_pieces = board->bitboards[W_KING] | board->bitboards[W_PAWN] | board->bitboards[W_ROOK] | board->bitboards[W_BISHOP] | board->bitboards[W_KNIGHT] | board->bitboards[W_QUEEN];
    board->black_pieces = board->bitboards[B_KING] | board->bitboards[B_PAWN] | board->bitboards[B_ROOK] | board->bitboards[B_BISHOP] | board->bitboards[B_KNIGHT] | board->bitboards[B_QUEEN];
    board->board = board->white_pieces | board->black_pieces;
    board->zobrist_hash ^= black_turn_key;
    if (board->turn == BLACK){
        board->fullmove_clock++;
    }

    board->plies++;
    uint64_t potential_king_in_check;
    if (board->turn == WHITE){
        board->turn = BLACK;
        potential_king_in_check = board->bitboards[B_KING];
    }
    else{
        board->turn = WHITE;
        potential_king_in_check = board->bitboards[W_KING];
    }

    // handle checks
    board->in_check = mv_is_square_attacked(bb_get_lsb(potential_king_in_check) - 1, board, board->board, board->turn);

    mv_generate_moves(board);

    // update history
    if (tt_is_key_in_table(board->history, board->zobrist_hash) == 1){
        int *num = (int *)tt_get_item(board->history, board->zobrist_hash);
        (*num)++;
    }
    else{
        int one = 1;
        tt_insert_item(board->history, board->zobrist_hash, &one);
    }

    // add move to previous moves
    gl_append_item(board->previous_moves, &move_state);

    cb_calculate_game_state(board);
}

void cb_undo_move(_move move, _board *board){
    unsigned long prev_moves_count = gl_size_of_list(board->previous_moves);
    if (prev_moves_count == 0){
        eh_die("no moves to undo");
    }
    _move_state prev_move_state = *((_move_state *)gl_access_item(board->previous_moves, prev_moves_count - 1));
    if (mv_moves_equal(move, prev_move_state.move) != 1){
        eh_die("cannot undo move that was not played");
    }

    board->game_state = ONGOING;
    board->plies--;
    board->halfmove_clock = prev_move_state.halfmove_clock;
    if (board->turn == WHITE){
        board->fullmove_clock--;
        board->turn = BLACK;
    }
    else{
        board->turn = WHITE;
    }

    board->zobrist_hash ^= black_turn_key ^ castling_keys[board->castling_rights] ^ castling_keys[prev_move_state.castling_rights];
    board->castling_rights = prev_move_state.castling_rights;
    if (board->en_passant_square != 0){
        board->zobrist_hash ^= en_passant_keys[board->en_passant_square % 8];
    }
    if (prev_move_state.en_passant_square != 0){
        board->zobrist_hash ^= en_passant_keys[prev_move_state.en_passant_square % 8];
    }
    board->en_passant_square = prev_move_state.en_passant_square;

    if (move.special_move == PROMOTION){
        board->bitboards[move.promotion] ^= 1UL << move.to;
        board->bitboards[move.piece] ^= 1UL << move.from;
        board->piece_array[move.to] = NONE;
        board->piece_array[move.from] = move.piece;
        board->zobrist_hash ^= piece_keys[move.to][move.promotion] ^ piece_keys[move.from][move.piece];
    }
    else{
        board->bitboards[move.piece] ^= (1UL << move.from) | (1UL << move.to);
        board->piece_array[move.to] = NONE;
        board->piece_array[move.from] = move.piece;
        board->zobrist_hash ^= piece_keys[move.to][move.piece] ^ piece_keys[move.from][move.piece];
        
        if (move.special_move == CASTLE){
            switch (move.to){
                case 2:
                    board->bitboards[W_ROOK] ^= 9UL;
                    board->piece_array[0] = W_ROOK;
                    board->piece_array[3] = NONE;
                    board->zobrist_hash ^= piece_keys[3][W_ROOK] ^ piece_keys[0][W_ROOK];
                    break;
                case 6:
                    board->bitboards[W_ROOK] ^= 160UL;
                    board->piece_array[7] = W_ROOK;
                    board->piece_array[5] = NONE;
                    board->zobrist_hash ^= piece_keys[5][W_ROOK] ^ piece_keys[7][W_ROOK];
                    break;
                case 58:
                    board->bitboards[B_ROOK] ^= 648518346341351424UL;
                    board->piece_array[56] = B_ROOK;
                    board->piece_array[59] = NONE;
                    board->zobrist_hash ^= piece_keys[59][B_ROOK] ^ piece_keys[56][B_ROOK];
                    break;
                case 62:
                    board->bitboards[B_ROOK] ^= 11529215046068469760UL;
                    board->piece_array[63] = B_ROOK;
                    board->piece_array[61] = NONE;
                    board->zobrist_hash ^= piece_keys[61][B_ROOK] ^ piece_keys[63][B_ROOK];
                    break;
            }
        }
    }

    if (move.capture != NONE){
        if (move.special_move == EN_PASSANT){
            int captured_pawn_square = move.to + (board->turn == WHITE ? -8 : 8);
            board->zobrist_hash ^= piece_keys[captured_pawn_square][move.capture];
            board->bitboards[move.capture] ^= 1UL << captured_pawn_square;
            board->piece_array[captured_pawn_square] = move.capture;
        }
        else{
            board->zobrist_hash ^= piece_keys[move.to][move.capture];
            board->bitboards[move.capture] ^= 1UL << move.to;
            board->piece_array[move.to] = move.capture;
        }
    }

    board->white_pieces = board->bitboards[W_KING] | board->bitboards[W_PAWN] | board->bitboards[W_ROOK] | board->bitboards[W_BISHOP] | board->bitboards[W_KNIGHT] | board->bitboards[W_QUEEN];
    board->black_pieces = board->bitboards[B_KING] | board->bitboards[B_PAWN] | board->bitboards[B_ROOK] | board->bitboards[B_BISHOP] | board->bitboards[B_KNIGHT] | board->bitboards[B_QUEEN];
    board->board = board->white_pieces | board->black_pieces;

    uint64_t potential_king_in_check = board->bitboards[board->turn == WHITE ? W_KING : B_KING];
    board->in_check = mv_is_square_attacked(bb_get_lsb(potential_king_in_check) - 1, board, board->board, board->turn);

    mv_generate_moves(board);

    tt_delete_item(board->history, board->zobrist_hash);

    gl_remove_item(board->previous_moves, prev_moves_count - 1);
}

void cb_print_board(_board *board){
    char pieces[13] = {'K', 'P', 'R', 'N', 'B', 'Q', 'k', 'p', 'r', 'n', 'b', 'q', '#'};
    for (int i = 56; i >= 0; i -= 8){
        for (int j = 0; j < 8; j++){
            printf("%c", pieces[board->piece_array[i + j]]);
        }
        printf("\n");
    }
    printf("\n\n");
}

int cb_is_in_check(_board *board){
    return board->in_check;
}

uint64_t cb_hash_board(_board *board){
    if (is_chess_engine_ready() != 1){
        eh_die("cb_init_chess_board() not called");
    }
    uint64_t hash = 0UL;
    uint64_t pieces_bitboard = board->board;
    while (pieces_bitboard != 0){
        int square = bb_pop_lsb(&pieces_bitboard) - 1;
        _piece piece = board->piece_array[square];
        hash ^= piece_keys[square][piece];
    }

    if (board->turn == BLACK){
        hash ^= black_turn_key;
    }

    hash ^= castling_keys[board->castling_rights];

    if (board->en_passant_square != 0){
        hash ^= en_passant_keys[board->en_passant_square % 8];
    }

    return hash;
}

void cb_destroy_board(_board *board){
    if (board == NULL){
        eh_die("passed in null pointer");
    }
    tt_destroy_transposition_table(board->history);
    free(board);
}
