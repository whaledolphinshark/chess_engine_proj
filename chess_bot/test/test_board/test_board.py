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

board_funcs = ctypes.CDLL(os.path.join(find_bin(), "board.so"))

board_funcs.helper_find_move.restype = ctypes.c_int
board_funcs.helper_find_move.argtypes = [ctypes.POINTER(Board), ctypes.c_char_p]
board_funcs.helper_get_move.restype = Move
board_funcs.helper_get_move.argtypes = [ctypes.POINTER(Board), ctypes.c_int]
board_funcs.helper_are_fens_equal.restype = ctypes.c_int
board_funcs.helper_are_fens_equal.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
board_funcs.cb_fen_to_board.restype = None
board_funcs.cb_fen_to_board.argtypes = [ctypes.POINTER(Board), ctypes.c_char_p]
board_funcs.cb_board_to_fen.restype = None
board_funcs.cb_board_to_fen.argtypes = [ctypes.POINTER(Board), ctypes.c_char_p]
board_funcs.cb_create_board.restype = ctypes.POINTER(Board)
board_funcs.cb_create_board.argtypes = []
board_funcs.cb_make_move.restype = None
board_funcs.cb_make_move.argtypes = [ctypes.POINTER(Board), Move]
board_funcs.mv_uci_to_move.restype = Move
board_funcs.mv_uci_to_move.argtypes = [ctypes.c_char_p, ctypes.POINTER(Board)]
board_funcs.init_chess_engine.restype = None
board_funcs.init_chess_engine.argtypes = []
board_funcs.init_chess_engine()

max_fen_length = 90
max_move_uci_length = 6

def make_board():
    return board_funcs.cb_create_board()

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
    castle_test_fen_moves = [("rnb1kbnr/pp3ppp/1q6/2Pp4/4p3/B1N1PQ2/P1PP1PPP/R3KBNR w KQkq - 0 1", [0]), 
            ("rnb1kbnr/pp3ppp/2q5/2Pp4/4p3/B1N1PQ2/P1PP1PPP/R3KBNR w KQkq - 0 1", [0]), 
            ("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", [0, 1]), 
            ("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", [2, 3]), 
            ("r3k2r/8/8/8/8/8/8/R3K1NR w KQkq - 0 1", [0]),
            ("r3k2r/8/8/8/8/8/8/R2QK2R w KQkq - 0 1", [1]),
            ("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1", []), 
            ("r3k2r/8/8/8/8/8/4r3/R3K2R b KQkq - 0 1", [2, 3]), 
            ("r3k2r/8/8/8/8/8/4r3/R3K2R w KQkq - 0 1", []), 
            ("r3k2r/8/8/8/8/5r2/8/R3K2R w KQkq - 0 1", [0]),
            ("r3k2r/8/8/8/8/6r1/8/R3K2R w KQkq - 0 1", [0]), 
            ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", []),
            ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1", []),
            ("r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1", [1]), 
            ("r3k2r/8/8/8/8/8/8/R3K2R b Kq - 0 1", [2])]
    board = make_board()
    for fen_mov in castle_test_fen_moves:
        fen = ctypes.create_string_buffer(fen_mov[0].encode('utf-8'), max_fen_length)
        board_funcs.cb_fen_to_board(board, fen)
        for castle_move_index in fen_mov[1]:
            # check if castle is there
            castle_move = ctypes.create_string_buffer(castles[castle_move_index].encode('utf-8'), max_move_uci_length)
            assert board_funcs.helper_find_move(board, castle_move) != -1, f"{castles[castle_move_index]} not in moves in position: {fen_mov[0]}"
            # play move and see if correct

def test_en_passant():
    en_passant_test_fens = [("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "e5d6", "4k3/8/3P4/8/8/8/8/4K3 b - - 0 1"), 
                            ("k3r3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "", ""), 
                            ("k7/8/8/2pPp3/8/8/8/4K3 w - c6 0 1", "d5c6", "k7/8/2P5/4p3/8/8/8/4K3 b - - 0 1"), 
                            ("8/8/8/3pP3/8/8/8/k4K2 w - - 0 1", "", ""), 
                            ("8/8/8/8/3pP3/8/8/k4K2 b - e3 0 1", "d4e3", "8/8/8/8/8/4p3/8/k4K2 w - - 0 2"), 
                            ("3k4/8/8/8/3pP3/8/8/3R1K2 b - e3 0 1", "", "")]

    board = make_board()
    fen_buffer = ctypes.create_string_buffer(b"filler", max_fen_length)
    for tests in en_passant_test_fens:
        fen = ctypes.create_string_buffer(tests[0].encode('utf-8'), max_fen_length)
        board_funcs.cb_fen_to_board(board, fen)
        # see if en passant is there
        en_passant_move = ctypes.create_string_buffer(tests[1].encode('utf-8'), max_move_uci_length)
        move_index = board_funcs.helper_find_move(board, en_passant_move)
        if tests[1] == "":
            assert move_index == -1, f"{tests[0]} contains illegal en passant move"
        else:
            assert move_index != -1, f"{tests[1]} not in moves in position: {tests[0]}"
            # play move and see if correct
            board_funcs.cb_make_move(board, board_funcs.helper_get_move(board, move_index))
            board_funcs.cb_board_to_fen(board, fen_buffer)
            fen_after_move = ctypes.create_string_buffer(tests[2].encode('utf-8'), max_fen_length)
            assert board_funcs.helper_are_fens_equal(fen_buffer, fen_after_move) == 1, "fens not equal"
    
    # play other move and see if en passant is gone
    board_funcs.cb_fen_to_board(board, ctypes.create_string_buffer(b"4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", max_fen_length))
    move = board_funcs.mv_uci_to_move(ctypes.create_string_buffer(b"e1d1", max_move_uci_length), board)
    board_funcs.cb_make_move(board, move)
    board_funcs.cb_board_to_fen(board, fen_buffer)
    fen_after_move = ctypes.create_string_buffer(b"4k3/8/8/3pP3/8/8/8/3K4 b - - 1 1", max_fen_length)
    assert board_funcs.helper_are_fens_equal(fen_buffer, fen_after_move), "fens not equal"
    move =  board_funcs.mv_uci_to_move(ctypes.create_string_buffer(b"e8f8", max_move_uci_length), board)
    board_funcs.cb_make_move(board, move)
    board_funcs.cb_board_to_fen(board, fen_buffer)
    fen_after_move = ctypes.create_string_buffer(b"5k2/8/8/3pP3/8/8/8/3K4 w - - 2 2", max_fen_length)
    assert board_funcs.helper_are_fens_equal(fen_buffer, fen_after_move), "fens not equal"



