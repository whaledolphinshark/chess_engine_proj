CC = gcc
CFLAGS = -Wall -g -O2 -I./src -I./include
BIN_DIR = bin

INIT = src/init_chess_engine/init_chess_engine.c
BOARD = src/board/init_board.c src/board/board.c  src/board/fen.c
MOVES = src/moves/init_moves.c src/moves/move_gen.c src/moves/move.c
TRANSPOSITION_TABLE = src/transposition_table/transposition_table.c
SEARCH = src/search/search.c
EVALUATE = src/evaluate/evaluate.c
UTILS = src/utils/list/list.c src/utils/bitboard_util.c src/utils/error_handling.c

OBJS = src/bot.c $(BOARD) $(MOVES) $(TRANSPOSITION_TABLE) $(SEARCH) $(EVALUATE) $(UTILS) $(INIT)

TARGET = $(BIN_DIR)/bot

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -f $(TARGET)
