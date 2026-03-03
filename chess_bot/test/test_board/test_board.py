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

funcs = ctypes.CDLL(os.path.join(find_bin(), "board.so"))

funcs.helper_find_move.restype = ctypes.c_int
funcs.helper_find_move.argtypes = [ctypes.POINTER(Board), ctypes.c_char_p]
funcs.helper_get_move.restype = Move
funcs.helper_get_move.argtypes = [ctypes.POINTER(Board), ctypes.c_int]
funcs.helper_are_fens_equal.restype = ctypes.c_int
funcs.helper_are_fens_equal.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
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

def make_board():
    return funcs.cb_create_board()

print(type(make_board()))
print(type(ctypes.POINTER(Board)))

def test_make_board():
    board = make_board()
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
    board = make_board()
    for test_case in test_cases_1:
        fen = ctypes.create_string_buffer(test_case[0].encode('utf-8'), max_fen_length)
        funcs.cb_fen_to_board(board, fen)
        # check if castle is there
        for castle_move_index in test_case[1]:
            castle_move = ctypes.create_string_buffer(castles[castle_move_index].encode('utf-8'), max_move_uci_length)
            assert funcs.helper_find_move(board, castle_move) != -1, f"{castles[castle_move_index]} not in moves in position: {test_case[0]}"
         # check if castle is not there
        for castle_move_index in test_case[2]:
            castle_move = ctypes.create_string_buffer(castles[castle_move_index].encode('utf-8'), max_move_uci_length)
            assert funcs.helper_find_move(board, castle_move) == -1, f"position: {test_case[0]} contains illegal castling move: {castles[castle_move_index]}"

    # play move and see if correct
    fen_buffer = ctypes.create_string_buffer(b"filler", max_fen_length)
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
        funcs.cb_fen_to_board(board, ctypes.create_string_buffer(test_case[0].encode('utf-8'), max_fen_length))
        move = funcs.mv_uci_to_move(ctypes.create_string_buffer(test_case[1].encode('utf-8'), max_move_uci_length), board)
        funcs.cb_make_move(board, move)
        funcs.cb_board_to_fen(board, fen_buffer)
        fen_to_check = ctypes.create_string_buffer(test_case[2].encode('utf-8'), max_fen_length)
        assert funcs.helper_are_fens_equal(fen_buffer, fen_to_check) == 1, f"{fen_buffer.value} != {fen_to_check.value} "

def test_en_passant():
    test_cases = [("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "e5d6", "4k3/8/3P4/8/8/8/8/4K3 b - - 0 1"), 
                            ("k3r3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "", ""), 
                            ("k7/8/8/2pPp3/8/8/8/4K3 w - c6 0 1", "d5c6", "k7/8/2P5/4p3/8/8/8/4K3 b - - 0 1"), 
                            ("8/8/8/3pP3/8/8/8/k4K2 w - - 0 1", "", ""), 
                            ("8/8/8/8/3pP3/8/8/k4K2 b - e3 0 1", "d4e3", "8/8/8/8/8/4p3/8/k4K2 w - - 0 2"), 
                            ("3k4/8/8/8/3pP3/8/8/3R1K2 b - e3 0 1", "", "")]

    board = make_board()
    fen_buffer = ctypes.create_string_buffer(b"filler", max_fen_length)
    for test_case in test_cases:
        fen = ctypes.create_string_buffer(test_case[0].encode('utf-8'), max_fen_length)
        funcs.cb_fen_to_board(board, fen)
        # see if en passant is there
        en_passant_move = ctypes.create_string_buffer(test_case[1].encode('utf-8'), max_move_uci_length)
        move_index = funcs.helper_find_move(board, en_passant_move)
        if test_case[1] == "":
            assert move_index == -1, f"{test_case[0]} contains illegal en passant move"
        else:
            assert move_index != -1, f"{test_case[1]} not in moves in position: {test_case[0]}"
            # play move and see if correct
            funcs.cb_make_move(board, funcs.helper_get_move(board, move_index))
            funcs.cb_board_to_fen(board, fen_buffer)
            fen_to_check = ctypes.create_string_buffer(test_case[2].encode('utf-8'), max_fen_length)
            assert funcs.helper_are_fens_equal(fen_buffer, fen_to_check) == 1, f"{fen_buffer.value} != {fen_to_check.value}"
    
    # play other move and see if en passant is gone
    funcs.cb_fen_to_board(board, ctypes.create_string_buffer(b"4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", max_fen_length))
    move = funcs.mv_uci_to_move(ctypes.create_string_buffer(b"e1d1", max_move_uci_length), board)
    funcs.cb_make_move(board, move)
    funcs.cb_board_to_fen(board, fen_buffer)
    fen_to_check = ctypes.create_string_buffer(b"4k3/8/8/3pP3/8/8/8/3K4 b - - 1 1", max_fen_length)
    assert funcs.helper_are_fens_equal(fen_buffer, fen_to_check) == 1, f"{fen_buffer.value} != {fen_to_check.value}"
    move =  funcs.mv_uci_to_move(ctypes.create_string_buffer(b"e8f8", max_move_uci_length), board)
    funcs.cb_make_move(board, move)
    funcs.cb_board_to_fen(board, fen_buffer)
    fen_to_check = ctypes.create_string_buffer(b"5k2/8/8/3pP3/8/8/8/3K4 w - - 2 2", max_fen_length)
    assert funcs.helper_are_fens_equal(fen_buffer, fen_to_check) == 1, f"{fen_buffer.value} != {fen_to_check.value}"



