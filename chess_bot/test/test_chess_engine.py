import pytest
import ctypes
import os

class Board(ctypes.Structure):
    pass

class Move(ctypes.Structure):
    _fields_ = [("to", ctypes.c_int),
                ("from", ctypes.c_int),
                ("piece", ctypes.c_int),
                ("capture", ctypes.c_int),
                ("promotion", ctypes.c_int),
                ("special_move", ctypes.c_int)]

def find_bin():
    current_dir = os.path.dirname(os.path.abspath(__file__))
    while True:
        if os.path.isdir(os.path.join(current_dir, "bin")):
            return os.path.join(current_dir, "bin")
        parent_dir = os.path.dirname(current_dir)
        if parent_dir == current_dir:
            raise RuntimeError("Project root not found")
        current_dir = parent_dir

funcs = ctypes.CDLL(os.path.join(find_bin(), "test.so"))

funcs.helper_find_move.restype = ctypes.c_int
funcs.helper_find_move.argtypes = [ctypes.POINTER(Board), ctypes.c_char_p]
funcs.helper_get_move.restype = Move
funcs.helper_get_move.argtypes = [ctypes.POINTER(Board), ctypes.c_int]
funcs.helper_are_fens_equal.restype = ctypes.c_int
funcs.helper_are_fens_equal.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
funcs.helper_is_in_check.restype = ctypes.c_int
funcs.helper_is_in_check.argtypes = [ctypes.POINTER(Board)]
funcs.helper_get_game_state.restype = ctypes.c_int
funcs.helper_get_game_state.argtypes = [ctypes.POINTER(Board)]
funcs.helper_get_move_count.restype = ctypes.c_int
funcs.helper_get_move_count.argtypes = [ctypes.POINTER(Board)]
funcs.helper_get_previous_moves_count.restype = ctypes.c_int
funcs.helper_get_previous_moves_count.argtypes = [ctypes.POINTER(Board)]
funcs.helper_get_zobrist_hash.restype = ctypes.c_uint64
funcs.helper_get_zobrist_hash.argtypes = [ctypes.POINTER(Board)]
funcs.cb_fen_to_board.restype = None
funcs.cb_fen_to_board.argtypes = [ctypes.POINTER(Board), ctypes.c_char_p]
funcs.cb_board_to_fen.restype = None
funcs.cb_board_to_fen.argtypes = [ctypes.POINTER(Board), ctypes.c_char_p]
funcs.cb_create_board.restype = ctypes.POINTER(Board)
funcs.cb_create_board.argtypes = []
funcs.cb_make_move.restype = None
funcs.cb_make_move.argtypes = [ctypes.POINTER(Board), Move]
funcs.cb_undo_move.restype = None
funcs.cb_undo_move.argtypes = [ctypes.POINTER(Board)]
funcs.cb_hash_board.restype = ctypes.c_uint64
funcs.cb_hash_board.argtypes = [ctypes.POINTER(Board)]
funcs.mv_uci_to_move.restype = Move
funcs.mv_uci_to_move.argtypes = [ctypes.c_char_p, ctypes.POINTER(Board)]
funcs.mv_move_to_uci.restype = None
funcs.mv_move_to_uci.argtypes = [Move, ctypes.c_char_p]
funcs.init_chess_engine.restype = None
funcs.init_chess_engine.argtypes = []
funcs.init_chess_engine()

max_fen_length = 90
max_move_uci_length = 6

def convert_to_c_string(str, max):
    return ctypes.create_string_buffer(str.encode('utf-8'), max)

def play_move(board, move_uci):
    move = funcs.mv_uci_to_move(convert_to_c_string(move_uci, max_move_uci_length), board)
    funcs.cb_make_move(board, move)

def set_board(board, position):
    fen = convert_to_c_string(position, max_fen_length)
    funcs.cb_fen_to_board(board, fen)

def get_move_uci(move):
    move_buffer = convert_to_c_string("filler", max_move_uci_length)
    funcs.mv_move_to_uci(move, move_buffer)
    return move_buffer.value.decode('utf-8')

def get_board_fen(board):
    fen_buffer = convert_to_c_string("filler", max_fen_length)
    funcs.cb_board_to_fen(board, fen_buffer)
    return fen_buffer.value.decode('utf-8')

def check_move_is_correct(board, move_uci, correct_position):
    fen_buffer = convert_to_c_string("filler", max_fen_length)
    move = funcs.mv_uci_to_move(convert_to_c_string(move_uci, max_move_uci_length), board)
    funcs.cb_make_move(board, move)
    funcs.cb_board_to_fen(board, fen_buffer)
    fen_to_check = convert_to_c_string(correct_position, max_fen_length)
    assert funcs.helper_are_fens_equal(fen_buffer, fen_to_check) == 1, f"{fen_buffer.value} != {fen_to_check.value}"

def check_undo_move_is_correct(board, correct_position, previous_moves = -1):
    fen_buffer = convert_to_c_string("filler", max_fen_length)
    funcs.cb_undo_move(board)
    funcs.cb_board_to_fen(board, fen_buffer)
    fen_to_check = convert_to_c_string(correct_position, max_fen_length)
    assert funcs.helper_are_fens_equal(fen_buffer, fen_to_check) == 1, f"{fen_buffer.value} != {fen_to_check.value}"
    if previous_moves > -1:
        assert funcs.helper_get_previous_moves_count(board) == previous_moves, "boards previous moves count != predicted previous moves count"

def check_move_in_position(board, move_uci):
    fen_buffer = convert_to_c_string("filler", max_fen_length)
    move = convert_to_c_string(move_uci, max_move_uci_length)
    funcs.cb_board_to_fen(board, fen_buffer)
    assert funcs.helper_find_move(board, move) != -1, f"{move_uci} not in moves in position: {fen_buffer.value}"

def check_move_not_in_position(board, move_uci):
    fen_buffer = convert_to_c_string("filler", max_fen_length)
    move = convert_to_c_string(move_uci, max_move_uci_length)
    funcs.cb_board_to_fen(board, fen_buffer)
    assert funcs.helper_find_move(board, move) == -1, f"position: {fen_buffer.value} contains illegal move: {move_uci}"

def check_game_state(board, expected_game_state):
    fen_buffer = convert_to_c_string("filler", max_fen_length)
    funcs.cb_board_to_fen(board, fen_buffer)
    assert funcs.helper_get_game_state(board) == expected_game_state, f"game state of position {fen_buffer.value} != {expected_game_state}"

def check_hash_is_correct(board):
    correct_hash = funcs.cb_hash_board(board)
    hash_to_check  = funcs.helper_get_zobrist_hash(board)
    assert correct_hash == hash_to_check, f"{correct_hash} != {hash_to_check}"

def update_stats(board, captures=0, en_passants=0, castles=0, promotions=0, checks=0, checkmates=0):
    for i in range(funcs.helper_get_move_count(board)):
        move = funcs.helper_get_move(board, i)
        if move.capture != 12:
            captures += 1
        match (move.special_move):
            case 1:
                en_passants += 1
            case 2:
                castles += 1
            case 3:
                promotions += 1
        funcs.cb_make_move(board, move)
        if funcs.helper_is_in_check(board) == 1:
            checks += 1
        if funcs.helper_get_game_state(board) == 1 or funcs.helper_get_game_state(board) == -1:
            checkmates += 1
        funcs.cb_undo_move(board)
    return captures, en_passants, castles, promotions, checks, checkmates

def traverse_positions(board, max_plies, track_stats = False):
    if max_plies == 1:
        if track_stats:
            captures, en_passants, castles, promotions, checks, checkmates = update_stats(board)
            print(funcs.helper_get_move_count(board), captures, en_passants, castles, promotions, checks, checkmates)
        return funcs.helper_get_move_count(board)
    
    check_hash_is_correct(board)
    current_ply = 0
    position_count = 0
    captures = en_passants = castles = promotions = checks = checkmates = 0
    stack = []
    for i in range(funcs.helper_get_move_count(board)):
        stack.append((funcs.helper_get_move(board, i), current_ply))

    while len(stack) != 0:
        move, ply = stack.pop()
        while ply < current_ply:
            funcs.cb_undo_move(board)
            current_ply -= 1

        funcs.cb_make_move(board, move)
        check_hash_is_correct(board)
        current_ply += 1
        if current_ply == max_plies - 1 and funcs.helper_get_game_state(board) == 2:
            position_count += funcs.helper_get_move_count(board)
            if track_stats:
                captures, en_passants, castles, promotions, checks, checkmates = update_stats(board, captures, en_passants, castles, promotions, checks, checkmates)
        elif funcs.helper_get_game_state(board) != 2:
            current_ply -= 1
            funcs.cb_undo_move(board)
        else:
            for i in range(funcs.helper_get_move_count(board)):
                stack.append((funcs.helper_get_move(board, i), current_ply))
        
    while 0 < current_ply:
        funcs.cb_undo_move(board)
        current_ply -= 1

    if track_stats:
        print(position_count, captures, en_passants, castles, promotions, checks, checkmates)
    
    return position_count

def test_make_board():
    board = funcs.cb_create_board()
    assert board is not None
    assert isinstance(board, ctypes.POINTER(Board))

def test_board():
    board = funcs.cb_create_board()
    # test_cases_1 = [(1, 20), (2, 400), (3, 8902), (4, 197281), (5, 4865609), (6, 119060324)]
    # for plies, expected_positions in test_cases_1:
    #     position_count = traverse_positions(board, plies)
    #     assert position_count == expected_positions, f"{position_count} != {expected_positions}"

    set_board(board, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1")
    test_cases_2 = [(1, 48), (2, 2039), (3, 97862), (4, 4085603)]
    for plies, expected_positions in test_cases_2:
        position_count = traverse_positions(board, plies, True)
        assert position_count == expected_positions, f"{position_count} != {expected_positions}"

    # set_board(board, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1")
    # test_cases_3 = [(1, 14), (2, 191), (3, 2812), (4, 43238), (5, 674624), (6, 11030083)]
    # for plies, expected_positions in test_cases_3:
    #     position_count = traverse_positions(board, plies)
    #     assert position_count == expected_positions, f"{position_count} != {expected_positions}"
    
    # set_board(board, "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1")
    # test_cases_4 = [(1, 6), (2, 264), (3, 9467), (4, 422333)]
    # for plies, expected_positions in test_cases_4:
    #     position_count = traverse_positions(board, plies, True)
    #     assert position_count == expected_positions, f"{position_count} != {expected_positions}"

    # set_board(board, "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8")
    # test_cases_5 = [(1, 44), (2, 1486), (3, 62379), (4, 2103487)]
    # for plies, expected_positions in test_cases_5:
    #     position_count = traverse_positions(board, plies, True)
    #     assert position_count == expected_positions, f"{position_count} != {expected_positions}"

def test_castling():
    castles = ["e1c1", "e1g1", "e8c8", "e8g8"]
    test_cases_1 = [("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", [0, 1], [2, 3]), 
            ("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", [2, 3], [0, 1]), 
            ("r3k2r/8/8/8/8/8/8/R3K1NR w KQkq - 0 1", [0], [1, 2, 3]),
            ("r3k2r/8/8/8/8/8/8/R2QK2R w KQkq - 0 1", [1], [0, 2, 3]),
            ("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1", [], [0, 1, 2, 3]), 
            ("r3k2r/8/8/8/8/8/4r3/R3K2R b KQkq - 0 1", [2, 3], [0, 1]), 
            ("r3k2r/8/8/8/8/8/4r3/R3K2R w KQkq - 0 1", [], [0, 1, 2, 3]), 
            ("r3k2r/8/8/8/8/5r2/8/R3K2R w KQkq - 0 1", [0], [1, 2, 3]),
            ("r3k2r/8/8/8/8/6r1/8/R3K2R w KQkq - 0 1", [0], [1, 2, 3]), 
            ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", [], [0, 1, 2, 3]),
            ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1", [], [0, 1, 2, 3]),
            ("r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1", [1], [0, 2, 3]), 
            ("r3k2r/8/8/8/8/8/8/R3K2R b Kq - 0 1", [2], [0, 1, 3])]
    board = funcs.cb_create_board()
    for test_case in test_cases_1:
        set_board(board, test_case[0])
        for castle_move_index in test_case[1]:
            check_move_in_position(board, castles[castle_move_index])
        for castle_move_index in test_case[2]:
            check_move_not_in_position(board, castles[castle_move_index])

    test_cases_2 = [("r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R w KQkq - 0 1", "e1c1", "r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/2KR3R b kq - 1 1"), 
                ("r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R w KQkq - 0 1", "e1g1", "r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R4RK1 b kq - 1 1"),
                ("r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R b KQkq - 0 1", "e8g8", "r4rk1/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R w KQ - 1 2"),
                ("r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R b KQkq - 0 1", "e8c8", "2kr3r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R w KQ - 1 2"),
                ("r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R w KQkq - 0 1", "a1b1", "r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/1R2K2R b Kkq - 1 1"),
                ("r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R w KQkq - 0 1", "h1g1", "r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K1R1 b Qkq - 1 1"),
                ("r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R b KQkq - 0 1", "h8g8", "r3k1r1/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R w KQq - 1 2"), 
                ("r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R b KQkq - 0 1", "a8b8", "1r2k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R w KQk - 1 2"), 
                ("r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R b Kkq - 0 1", "a8b8", "1r2k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R w Kk - 1 2"), 
                ("r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/R3K2R w KQk - 0 1", "e1c1", "r3k2r/pp3ppp/1qnbb2n/2P1NQ2/2pPp3/B3P2N/P1P2PPP/2KR3R b k - 1 1")]
    for test_case in test_cases_2:
        set_board(board, test_case[0])
        check_move_is_correct(board, test_case[1], test_case[2])
        check_undo_move_is_correct(board, test_case[0], 0)


def test_en_passant():
    test_cases = [("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "e5d6", "4k3/8/3P4/8/8/8/8/4K3 b - - 0 1"), 
                  ("k3r3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "", ""), 
                  ("k7/8/8/2pPp3/8/8/8/4K3 w - c6 0 1", "d5c6", "k7/8/2P5/4p3/8/8/8/4K3 b - - 0 1"), 
                  ("8/8/8/3pP3/8/8/8/k4K2 w - - 0 1", "", ""), 
                  ("8/8/8/8/3pP3/8/8/k4K2 b - e3 0 1", "d4e3", "8/8/8/8/8/4p3/8/k4K2 w - - 0 2"), 
                  ("3k4/8/8/8/3pP3/8/8/3R1K2 b - e3 0 1", "", "")]

    board = funcs.cb_create_board()
    for test_case in test_cases:
        set_board(board, test_case[0])
        if test_case[1] == "":
            check_move_not_in_position(board, test_case[1])
        else:
            check_move_in_position(board, test_case[1])
            check_move_is_correct(board, test_case[1], test_case[2])
            check_undo_move_is_correct(board, test_case[0], 0)
    
    # play other move and see if en passant is gone
    set_board(board, "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1")
    check_move_is_correct(board, "e1d1", "4k3/8/8/3pP3/8/8/8/3K4 b - - 1 1")
    check_move_is_correct(board, "e8f8", "5k2/8/8/3pP3/8/8/8/3K4 w - - 2 2")
    check_undo_move_is_correct(board, "4k3/8/8/3pP3/8/8/8/3K4 b - - 1 1", 1)
    check_undo_move_is_correct(board, "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", 0)

def test_promotions():
    promotions = ['q', 'r', 'b', 'n']
    test_cases = [("8/P7/8/8/8/8/8/k6K w - - 0 1", "a7a8*", "*7/8/8/8/8/8/8/k6K b - - 0 1"),
                ("7K/3k4/8/8/8/8/p7/8 b - - 0 1", "a2a1*", "7K/3k4/8/8/8/8/8/*7 w - - 0 2"),
                ("1r6/P7/8/8/8/8/8/5K1k w - - 0 1", "a7b8*", "1*6/8/8/8/8/8/8/5K1k b - - 0 1")]

    board = funcs.cb_create_board()
    for test_case in test_cases:
        for promotion in promotions:
            set_board(board, test_case[0])
            promotion_move = test_case[1].replace('*', promotion)
            fen_to_check = test_case[2].replace('*', promotion.upper() if 'w' in test_case[0] else promotion)
            check_move_in_position(board, promotion_move)
            check_move_is_correct(board, promotion_move, fen_to_check)
            check_undo_move_is_correct(board, test_case[0], 0)

def test_pins():
    test_cases = [("4k3/8/8/8/1b6/8/3N4/4K3 w - - 0 1", ["e1d1", "e1e2", "e1f1", "e1f2"], 4),
                  ("4k3/8/8/8/2n5/8/3N4/4K3 w - - 0 1", ["d2b1", "d2b3", "d2c4", "d2e4", "d2f3", "d2f1", "e1d1", "e1e2", "e1f1", "e1f2"], 10),
                  ("4k3/8/8/8/2n5/3p4/1p1Rp3/4K3 w - - 0 1", ["d2c2", "d2b2", "d2d3", "d2e2", "d2d1", "e1f2"], 6),
                  ("4k3/8/8/8/1b6/8/3R4/4K3 w - - 0 1", ["e1d1", "e1e2", "e1f1", "e1f2"], 4),
                  ("4k3/3b4/8/1B6/8/8/8/4K3 b - - 0 1", ["d7c6", "d7b5", "e8d8", "e8e7", "e8f8", "e8f7"], 6),
                  ("4k3/4b3/8/8/8/8/4R3/4K3 b - - 0 1", ["e8f8", "e8d8", "e8f7", "e8d7"], 4),
                  ("4k3/3q4/8/1B6/8/8/8/4K3 b - - 0 1", ["d7c6", "d7b5", "e8d8", "e8e7", "e8f8", "e8f7"], 6),
                  ("4k3/8/8/8/2b5/3Np3/3pRp2/5K2 w - - 0 1", ["d3b2", "d3b4", "d3c5", "d3e5", "d3f4", "d3f2", "d3e1", "d3c1", "e2e1", "e2d2", "e2e3", "e2f2", "f1g2"], 13),
                  ("4k3/4r3/8/3pP3/8/8/8/4K3 w - d6 0 1", ["e5e6", "e1d1", "e1d2", "e1e2", "e1f1", "e1f2"], 6),
                  ("4k3/8/4r3/3pP3/8/8/8/4K3 w - d6 0 1", ["e1d1", "e1d2", "e1e2", "e1f1", "e1f2"], 5),
                  ("4k3/6b1/8/3pP3/8/8/8/K7 w - d6 0 1", ["a1a2", "a1b1", "a1b2"], 3),
                  ("K7/8/8/8/1R3pPk/8/8/8 b - g3 0 1", ["f4f3", "h4g4", "h4h3", "h4g3", "h4g5"], 5),
                  ("8/8/8/K7/4QPpk/8/8/8 b - f3 0 1", ["g4g3", "h4h5", "h4h3", "h4g3"], 4)]

    board = funcs.cb_create_board()
    for test_case in test_cases:
        set_board(board, test_case[0])
        for move_uci in test_case[1]:
            check_move_in_position(board, move_uci)
        assert funcs.helper_get_move_count(board) == test_case[2], f"position: {test_case[0]} contains wrong number of moves"

def test_checks():
    test_cases = [("4k3/8/3P4/8/8/8/8/4K3 w - - 0 1", "d6d7", ["e8e7", "e8d8", "e8f8", "e8d7", "e8f7"], 5),
                  ("4k3/3ppn2/r6q/8/4N3/b7/8/4K3 w - - 0 1", "e4d6", ["a3d6", "a6d6", "e7d6", "f7d6", "h6d6", "e8d8", "e8f8"], 7),
                  ("8/2pk4/8/8/2B5/6P1/8/4K3 w - - 0 1", "c4b5", ["d7d6", "d7d8", "d7e7", "d7c8", "d7e6", "c7c6"], 6),
                  ("8/2p5/8/4k3/8/8/1R6/4K3 w - - 0 1", "b2b5", ["e5d6", "e5d4", "e5e6", "e5e4", "e5f6", "e5f4", "c7c5"], 7),
                  ("8/4k3/8/8/8/8/2Q5/4K3 w - - 0 1", "c2c7", ["e7e6", "e7e8", "e7f6", "e7f8"], 4),
                  ("4k3/8/r7/8/4B3/5q2/4R3/4K3 w - - 0 1", "e4c6", ["e8d8", "e8f8", "e8f7"], 3),
                  ("4k3/8/8/4B3/8/2q1R3/8/7K w - - 0 1", "e5c7", ["c3e3", "e8d7", "e8f8", "e8f7"], 5),
                  ("8/4k3/8/3pP3/8/8/8/4K3 w - d6 0 1", "e5d6", ["e7e8", "e7e6", "e7d7", "e7d8", "e7d6", "e7f7", "e7f8", "e7f6"], 8),
                  ("8/P7/8/8/8/8/8/k6K w - - 0 1", "a7a8q", ["a1b2", "a1b1"], 2),
                  ("8/P1k5/8/8/8/8/7P/6K1 w - - 0 1","a7a8n", ["c7b7", "c7b8", "c7c8", "c7d8", "c7d7", "c7d6", "c7c6"], 7),
                  ("7k/1Q6/8/6K1/8/8/8/8 w - - 0 1", "b7g7", ["h8g7"], 1),
                  ("4k3/8/8/2R1Bb2/8/8/K7/8 w - - 0 1", "c5c8", ["e8e7", "e8d7", "e8f7", "f5c8"], 4),
                  ("8/2p5/7R/1P6/1K6/8/8/5k2 b - - 0 1", "c7c5", ["b5c6", "b4c5", "b4a5", "b4a4", "b4c4", "b4b3", "b4a3", "b4c3"], 8),
                  ("1r6/2p5/7R/1P6/1K6/8/8/5k2 b - - 0 1", "c7c5", ["b4c5", "b4a5", "b4a4", "b4c4", "b4b3", "b4a3", "b4c3"], 7)]
    board = funcs.cb_create_board()
    for test_case in test_cases:
        set_board(board, test_case[0])
        check_move_in_position(board, test_case[1])
        play_move(board, test_case[1])
        assert funcs.helper_is_in_check(board) == 1, f"position: {get_board_fen(board)} not in check"
        for move_uci in test_case[2]:
            check_move_in_position(board, move_uci)
        assert funcs.helper_get_move_count(board) == test_case[3], "number of moves on board not equal to what is predicted"
        check_undo_move_is_correct(board, test_case[0], 0)
        assert funcs.helper_is_in_check(board) == 0, f"position: {test_case[0]} in check"

    # make sure king cannot move into check
    set_board(board, "7k/8/2b5/5n1r/8/5rpn/6K1/8 w - - 0 1")
    check_move_in_position(board, "g2h1")
    assert funcs.helper_get_move_count(board) == 1, "number of moves on board not equal to what is predicted"

def test_stalemate():
    test_cases = [("7k/8/5QK1/8/8/8/8/8 w - - 0 1", "f6f7", 0),
                  ("7k/8/6Q1/8/8/8/8/K7 w - - 0 1", "a1b1", 0),
                  ("8/8/8/8/6q1/8/5k2/7K b - - 0 1", "g4g3", 0),
                  ("k7/8/8/8/8/6q1/8/7K b - - 0 1", "a8b8", 0),
                  ("7k/8/2p2QK1/1prp4/1PpPp3/2P1P3/8/8 w - - 0 1", "f6f7", 0),
                  ("7k/1p1p4/pP1PpQK1/P1n1P3/p3p3/Pp1pP3/1P1P4/8 w - - 0 1", "f6f7", 0),
                  ("8/8/8/2p1p3/2P1Pq2/2pBp1k1/2P1P3/7K b - - 0 1", "f4f2", 0),
                  ("7k/r7/6K1/5Q2/8/8/8/8 w - - 0 1", "f5f7", 2),
                  ("7k/5Q2/6K1/8/1p6/1P6/2P5/8 w - - 0 1", "c2c4", 2)]
    board = funcs.cb_create_board()
    for test_case in test_cases:
        set_board(board, test_case[0])
        check_move_in_position(board, test_case[1])
        play_move(board, test_case[1])
        check_game_state(board, test_case[2])
        check_undo_move_is_correct(board, test_case[0], 0)

def test_insufficient_material():
    test_cases = [("7k/8/3p4/1N6/8/8/8/4K3 w - - 0 1", "b5d6", 0),
                  ("7k/8/8/5n2/8/6P1/8/4K3 b - - 0 1", "f5g3", 0),
                  ("7k/8/8/4B3/8/6p1/8/4K3 w - - 0 1", "e5g3", 0),
                  ("7k/8/8/4b3/8/6P1/8/4K3 b - - 0 1", "e5g3", 0),
                  ("7k/8/8/8/6R1/6p1/8/4K3 w - - 0 1", "g4g3", 2),
                  ("7k/8/8/8/6r1/6P1/8/4K3 b - - 0 1", "g4g3", 2),
                  ("7k/8/8/8/8/6p1/7P/4K3 w - - 0 1", "h2g3", 2),
                  ("7k/8/8/8/8/6p1/7P/4K3 b - - 0 1", "g3h2", 2),
                  ("7k/8/8/8/8/8/5q2/4K3 w - - 0 1", "e1f2", 0),
                  ("7k/6Q1/8/8/8/8/8/4K3 b - - 0 1", "h8g7", 0),
                  ("7k/8/8/2B5/3p4/8/7b/4K3 w - - 0 1", "c5d4", 0),
                  ("7k/8/8/2B5/3p2b1/8/8/4K3 w - - 0 1", "c5d4", 2)]
    board = funcs.cb_create_board()
    for test_case in test_cases:
        set_board(board, test_case[0])
        check_move_in_position(board, test_case[1])
        play_move(board, test_case[1])
        check_game_state(board, test_case[2])
        check_undo_move_is_correct(board, test_case[0], 0)

def test_50_move_rule():
    test_cases = [("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 99 1", "e2e4", 2),
                  ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 99 1", "e7e6", 2),
                  ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 99 1", "b1a3", 0),
                  ("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 99 1", "e1c1", 0),
                  ("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 99 1", "e8c8", 0),
                  ("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 99 1", "a1a8", 2),
                  ("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 99 1", "a8a1", 2)]
    board = funcs.cb_create_board()
    for test_case in test_cases:
        set_board(board, test_case[0])
        check_move_in_position(board, test_case[1])
        play_move(board, test_case[1])
        check_game_state(board, test_case[2])
        check_undo_move_is_correct(board, test_case[0], 0)

def test_repetition():
    test_cases = [("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", ["b1a3", "b8a6", "a3b1", "a6b8"])]

    board = funcs.cb_create_board()
    for test_case in test_cases:
        set_board(board, test_case[0])
        previous_moves_count = 0
        history = []
        for move_uci in test_case[1] + test_case[1]:
            check_move_in_position(board, move_uci)
            history.append((get_board_fen(board), previous_moves_count))
            play_move(board, move_uci)
            previous_moves_count += 1
        check_game_state(board, 0)
        for fen, move_count in reversed(history):
            check_undo_move_is_correct(board, fen, move_count)

def test_checkmate():
    test_cases = [("6k1/5ppp/8/8/8/8/5PPP/4R1K1 w - - 0 1", "e1e8", 1),
                  ("6rk/6pp/8/8/6n1/8/6PP/6RK b - - 0 1", "g4f2", -1),
                  ("7k/1Q6/6K1/8/8/8/8/8 w - - 0 1", "b7g7", 1),
                  ("7k/8/8/3r4/1r6/8/8/K7 b - - 0 1", "d5a5", -1),
                  ("7k/5P1p/8/8/8/8/8/K7 w - - 0 1", "f7f8q", 1),
                  ("7k/8/8/8/8/p1n5/8/K1b5 b - - 99 1", "c1b2", -1)]
    board = funcs.cb_create_board()
    for test_case in test_cases:
        set_board(board, test_case[0])
        check_move_in_position(board, test_case[1])
        play_move(board, test_case[1])
        check_game_state(board, test_case[2])
        check_undo_move_is_correct(board, test_case[0], 0)

# test largest fen you can make
def test_fen():
    pass
