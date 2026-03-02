import pytest
import ctypes
import os

class Board(ctypes.Structure):
    pass

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

board_funcs.helper_has_move.restype = ctypes.c_int
board_funcs.helper_has_move.argtypes = [ctypes.POINTER(Board), ctypes.c_char_p]
board_funcs.cb_fen_to_board.restype = None
board_funcs.cb_fen_to_board.argtypes = [ctypes.POINTER(Board), ctypes.c_char_p]
board_funcs.cb_create_board.restype = ctypes.POINTER(Board)
board_funcs.cb_create_board.argtypes = []
board_funcs.init_chess_engine.restype = None
board_funcs.init_chess_engine.argtypes = []
board_funcs.init_chess_engine()

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
    max_fen_length = 90
    max_move_length = 6
    castles = ["e1c1", "e1g1", "e8c8", "e8g8"]
    castle_test_fen_moves = [("rnb1kbnr/pp3ppp/1q6/2Pp4/4p3/B1N1PQ2/P1PP1PPP/R3KBNR b KQkq - 0 1", [0]), 
            ("rnb1kbnr/pp3ppp/2q5/2Pp4/4p3/B1N1PQ2/P1PP1PPP/R3KBNR w KQkq - 0 1", [0]), 
            ("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", [0, 1, 2, 3]), 
            ("r3k2r/8/8/8/8/8/8/R3K1NR w KQkq - 0 1", [0, 2, 3]),
            ("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1", []), 
            ("r3k2r/8/8/8/8/8/8/R2QK2R w KQkq - 0 1", [1, 2, 3]),
            ("r3k2r/8/8/8/8/8/4r3/R3K2R w KQkq - 0 1", [2, 3]), 
            ("r3k2r/8/8/8/8/6r1/8/R3K2R w KQkq - 0 1", [0, 2, 3]), 
            ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", []),
            ("r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1", [0, 3])]
    board = make_board()
    for fen_mov in castle_test_fen_moves:
        fen = ctypes.create_string_buffer(fen_mov[0].encode('utf-8'), max_fen_length)
        board_funcs.cb_fen_to_board(board, fen)
        for castle_move_index in fen_mov[1]:
            castle_move = ctypes.create_string_buffer(castles[castle_move_index].encode('utf-8'), max_move_length)
            assert board_funcs.helper_has_move(board, castle_move) == 1


