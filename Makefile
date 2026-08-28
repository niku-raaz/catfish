CXX ?= c++
BUILD_DIR ?= build/make
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Iinclude -O0 -g

CORE_SRCS := \
	src/bitboard.cpp \
	src/board.cpp \
	src/eval.cpp \
	src/fen.cpp \
	src/move.cpp \
	src/movegen.cpp \
	src/opening_book.cpp \
	src/perft.cpp \
	src/search.cpp \
	src/tablebase.cpp \
	src/uci.cpp \
	src/zobrist.cpp

TEST_SRCS := \
	tests/test_bitboard.cpp \
	tests/test_eval.cpp \
	tests/test_fen.cpp \
	tests/test_movegen.cpp \
	tests/test_perft.cpp \
	tests/test_search.cpp \
	tests/test_uci.cpp \
	tests/test_zobrist.cpp

.PHONY: all test clean

all: $(BUILD_DIR)/catfish $(BUILD_DIR)/catfish_uci $(BUILD_DIR)/catfish_perft $(BUILD_DIR)/catfish_tests

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/catfish: $(CORE_SRCS) src/main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CORE_SRCS) src/main.cpp -o $@

$(BUILD_DIR)/catfish_uci: $(CORE_SRCS) src/uci_main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CORE_SRCS) src/uci_main.cpp -o $@

$(BUILD_DIR)/catfish_perft: $(CORE_SRCS) tools/perft_runner.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CORE_SRCS) tools/perft_runner.cpp -o $@

$(BUILD_DIR)/catfish_tests: $(CORE_SRCS) $(TEST_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CORE_SRCS) $(TEST_SRCS) -o $@

test: $(BUILD_DIR)/catfish_tests
	$(BUILD_DIR)/catfish_tests

clean:
	rm -rf build
