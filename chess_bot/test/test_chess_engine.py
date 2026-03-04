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
funcs.helper_has_moves_from_square.restype = ctypes.c_int
funcs.helper_has_moves_from_square.argtypes = [ctypes.POINTER(Board), ctypes.c_int]
funcs.cb_fen_to_board.restype = None
funcs.cb_fen_to_board.argtypes = [ctypes.POINTER(Board), ctypes.c_char_p]
funcs.cb_board_to_fen.restype = None
funcs.cb_board_to_fen.argtypes = [ctypes.POINTER(Board), ctypes.c_char_p]
funcs.cb_create_board.restype = ctypes.POINTER(Board)
funcs.cb_create_board.argtypes = []
funcs.cb_make_move.restype = None
funcs.cb_make_move.argtypes = [ctypes.POINTER(Board), Move]
funcs.mv_uci_to_move.restype = Move
funcs.mv_uci_to_move.argtypes = [ctypes.c_char_p, ctypes.POINTER(Board)]
funcs.init_chess_engine.restype = None
funcs.init_chess_engine.argtypes = []
funcs.init_chess_engine()

max_fen_length = 90
max_move_uci_length = 6

def convert_to_c_string(str, max):
    return ctypes.create_string_buffer(str.encode('utf-8'), max)

def check_move_is_correct(board, move_uci, correct_position):
    fen_buffer = convert_to_c_string("filler", max_fen_length)
    move = funcs.mv_uci_to_move(convert_to_c_string(move_uci, max_move_uci_length), board)
    funcs.cb_make_move(board, move)
    funcs.cb_board_to_fen(board, fen_buffer)
    fen_to_check = convert_to_c_string(correct_position, max_fen_length)
    assert funcs.helper_are_fens_equal(fen_buffer, fen_to_check) == 1, f"{fen_buffer.value} != {fen_to_check.value}"

def check_move_in_position(board, move_uci):
    fen_buffer = convert_to_c_string("filler", max_fen_length)
    move = convert_to_c_string(move_uci, max_move_uci_length)
    funcs.cb_board_to_fen(board, fen_buffer)
    assert funcs.helper_find_move(board, move) != -1, f"{move_uci} not in moves in position: {fen_buffer.value}"

def check_move_not_in_position(board, move_uci):
    fen_buffer = convert_to_c_string("filler", max_fen_length)
    move = convert_to_c_string(move_uci, max_move_uci_length)
    funcs.cb_board_to_fen(board, fen_buffer)
    assert funcs.helper_find_move(board, move) == -1, f"{fen_buffer.value} contains illegal move: {move_uci}"

def test_make_board():
    board = funcs.cb_create_board()
    assert board is not None
    assert isinstance(board, ctypes.POINTER(Board))

def test_make_and_undo_move():
    pass

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
        fen = convert_to_c_string(test_case[0], max_fen_length)
        funcs.cb_fen_to_board(board, fen)
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
        funcs.cb_fen_to_board(board, convert_to_c_string(test_case[0], max_fen_length))
        check_move_is_correct(board, test_case[1], test_case[2])

def test_en_passant():
    test_cases = [("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "e5d6", "4k3/8/3P4/8/8/8/8/4K3 b - - 0 1"), 
                            ("k3r3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "", ""), 
                            ("k7/8/8/2pPp3/8/8/8/4K3 w - c6 0 1", "d5c6", "k7/8/2P5/4p3/8/8/8/4K3 b - - 0 1"), 
                            ("8/8/8/3pP3/8/8/8/k4K2 w - - 0 1", "", ""), 
                            ("8/8/8/8/3pP3/8/8/k4K2 b - e3 0 1", "d4e3", "8/8/8/8/8/4p3/8/k4K2 w - - 0 2"), 
                            ("3k4/8/8/8/3pP3/8/8/3R1K2 b - e3 0 1", "", "")]

    board = funcs.cb_create_board()
    for test_case in test_cases:
        fen = convert_to_c_string(test_case[0], max_fen_length)
        funcs.cb_fen_to_board(board, fen)
        if test_case[1] == "":
            check_move_not_in_position(board, test_case[1])
        else:
            check_move_in_position(board, test_case[1])
            check_move_is_correct(board, test_case[1], test_case[2])
    
    # play other move and see if en passant is gone
    funcs.cb_fen_to_board(board, convert_to_c_string("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", max_fen_length))
    check_move_is_correct(board, "e1d1", "4k3/8/8/3pP3/8/8/8/3K4 b - - 1 1")
    check_move_is_correct(board, "e8f8", "5k2/8/8/3pP3/8/8/8/3K4 w - - 2 2")

def test_promotions():
    promotions = ['q', 'r', 'b', 'n']
    test_cases = [("8/P7/8/8/8/8/8/k6K w - - 0 1", "a7a8*", "*7/8/8/8/8/8/8/k6K b - - 0 1"),
                ("7K/3k4/8/8/8/8/p7/8 b - - 0 1", "a2a1*", "7K/3k4/8/8/8/8/8/*7 w - - 0 2"),
                ("1r6/P7/8/8/8/8/8/5K1k w - - 0 1", "a7b8*", "1*6/8/8/8/8/8/8/5K1k b - - 0 1")]

    board = funcs.cb_create_board()
    for test_case in test_cases:
        fen = convert_to_c_string(test_case[0], max_fen_length)
        for promotion in promotions:
            funcs.cb_fen_to_board(board, fen)
            promotion_move = test_case[1].replace('*', promotion)
            fen_to_check = test_case[2].replace('*', promotion.upper() if 'w' in test_case[0] else promotion)
            check_move_in_position(board, promotion_move)
            check_move_is_correct(board, promotion_move, fen_to_check)
    
    test_cases_2 = [("8/P7/8/8/8/8/8/k6K w - - 0 1", "a7a8q", "Q7/8/8/8/8/8/8/k6K b - - 0 1", 1, 2),
                    ("8/P7/8/8/8/8/8/1k5K w - - 0 1", "a7a8q", "Q7/8/8/8/8/8/8/1k5K b - - 0 1", 0, 2),
                    ("8/P1k5/8/8/8/8/7P/6K1 w - - 0 1", "a7a8n", "N7/2k5/8/8/8/8/7P/6K1 b - - 0 1", 1, 2),
                    ("8/P1k1K3/8/8/8/8/8/1R6 w - - 0 1", "a7a8q", "Q7/2k1K3/8/8/8/8/8/1R6 b - - 0 1", 0, 0),
                    ("8/P1k1K3/8/8/8/8/8/1R6 w - - 0 1", "a7a8n", "N7/2k1K3/8/8/8/8/8/1R6 b - - 0 1", 1, 2),
                    ("6k1/P5pp/8/8/8/8/6PP/5RK1 w - - 0 1", "a7a8q", "Q5k1/6pp/8/8/8/8/6PP/5RK1 b - - 0 1", 1, 1),
                    ("5rk1/6pp/8/8/8/8/p5PP/6K1 b - - 0 1", "a2a1q", "5rk1/6pp/8/8/8/8/6PP/q5K1 w - - 0 2", 1, -1)]
    for test_case in test_cases_2:
        fen = convert_to_c_string(test_case[0], max_fen_length)
        funcs.cb_fen_to_board(board, fen)
        check_move_in_position(board, test_case[1])
        check_move_is_correct(board, test_case[1], test_case[2])
        assert funcs.helper_is_in_check(board) == test_case[3], f"position: {test_case[2]} check value incorrect"
        assert funcs.helper_get_game_state(board) == test_case[4], f"position: {test_case[2]} game state incorrect"

def test_pins():
    test_cases = [("4k3/8/8/8/1b6/8/3N4/4K3 w - - 0 1", 11, 0),
                  ("4k3/8/8/8/2n5/8/3N4/4K3 w - - 0 1", 11, 1),
                  ("4k3/8/8/8/2n5/8/3R4/4K3 w - - 0 1", 11, 1),
                  ("4k3/8/8/8/1b6/8/3R4/4K3 w - - 0 1", 11, 0),
                  ("4k3/3b4/8/1B6/8/8/8/4K3 b - - 0 1", 51, 1),
                  ("4k3/4b3/8/8/8/8/4R3/4K3 b - - 0 1", 52, 0),
                  ("4k3/3q4/8/1B6/8/8/8/4K3 b - - 0 1", 51, 1),
                  ("4k3/8/8/8/2b5/3N4/4R3/5K2 w - - 0 1", 19, 1),
                  ("4k3/8/8/8/2b5/3N4/4R3/5K2 w - - 0 1", 12, 1),
                  ("4k3/4r3/8/3pP3/8/8/8/4K3 w - d6 0 1", 36, 1),
                  ("4k3/8/4r3/3pP3/8/8/8/4K3 w - d6 0 1", 36, 0)]

    board = funcs.cb_create_board()
    for test_case in test_cases:
        fen = convert_to_c_string(test_case[0], max_fen_length)
        funcs.cb_fen_to_board(board, fen)
        assert funcs.helper_has_moves_from_square(board, test_case[1]) == test_case[2], f"position: {test_case[0]} has wrong move generation"

    test_cases_2 = [("4k3/4r3/8/3pP3/8/8/8/4K3 w - d6 0 1", [], ["e5d6"]),
                    ("4k3/8/8/8/1b6/8/3B4/4K3 w - - 0 1", ["d2c3", "d2b4"], ["d2c1", "d2e3"]),
                    ("4k3/8/8/8/4r3/8/4R3/4K3 w - - 0 1", ["e2e3", "e2e4"], ["e2d2", "e2f2"]),
                    ("4k3/5p2/6B1/8/8/8/8/4K3 b - - 0 1", ["f7g6"], ["f7f6", "f7f5"])]
    for test_case in test_cases_2:
        fen = convert_to_c_string(test_case[0], max_fen_length)
        funcs.cb_fen_to_board(board, fen)
        for move in test_case[1]:
            check_move_in_position(board, move)
        for move in test_case[2]:
            check_move_not_in_position(board, move)

def test_checks():
    pass

def test_game_end():
    pass
