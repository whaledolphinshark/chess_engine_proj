CC = gcc
CFLAGS = -Wall -g -O2 -I./src -I./include -fsanitize=address,undefined
BIN_DIR = bin

INIT = src/init_chess_engine/init_chess_engine.c
BOARD = src/board/init_board.c src/board/board.c  src/board/fen.c
MOVES = src/moves/init_moves.c src/moves/move_gen.c src/moves/move.c
TRANSPOSITION_TABLE = src/transposition_table/transposition_table.c
SEARCH = src/search/search.c
EVALUATE = src/evaluate/evaluate.c
UTILS = src/utils/list/list.c src/utils/bitboard_util.c src/utils/error_handling.c

ALL = $(BOARD) $(MOVES) $(TRANSPOSITION_TABLE) $(SEARCH) $(EVALUATE) $(UTILS) $(INIT)
PROG_1 = test/perft.c $(BOARD) $(MOVES) $(TRANSPOSITION_TABLE) $(UTILS) $(INIT)
PROG_2 = test/manual_test.c $(BOARD) $(MOVES) $(TRANSPOSITION_TABLE) $(UTILS) $(INIT)
PROG_3 = test/test_transposition_table.c $(TRANSPOSITION_TABLE) $(UTILS)
PROG_4 = test/test_match.c $(ALL) test/alt_bot/alt_search.c
PROG_5 = test/test_bots.c $(ALL) test/alt_bot/alt_search.c

TARGET_1 = $(BIN_DIR)/perft
TARGET_2 = $(BIN_DIR)/manual_test
TARGET_3 = $(BIN_DIR)/tt_test
TARGET_4 = $(BIN_DIR)/test_match
TARGET_5 = $(BIN_DIR)/test_bots

all: $(TARGET_1) $(TARGET_2) $(TARGET_3)

bots: all $(TARGET_4) $(TARGET_5)

$(BIN_DIR):
	 mkdir -p $(BIN_DIR)

$(TARGET_1): $(PROG_1) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET_1) $(PROG_1)

$(TARGET_2): $(PROG_2) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET_2) $(PROG_2)

$(TARGET_3): $(PROG_3) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET_3) $(PROG_3)

$(TARGET_4): $(PROG_4) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET_4) $(PROG_4)
    
$(TARGET_5): $(PROG_5) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET_5) $(PROG_5)

.PHONY: all bots clean
clean:
	rm -f $(TARGET_1) $(TARGET_2) $(TARGET_3) $(TARGET_4) $(TARGET_5)

