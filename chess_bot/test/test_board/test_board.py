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

