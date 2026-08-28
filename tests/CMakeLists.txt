add_executable(catfish_tests
    test_bitboard.cpp
    test_eval.cpp
    test_fen.cpp
    test_movegen.cpp
    test_perft.cpp
    test_search.cpp
    test_uci.cpp
    test_zobrist.cpp
)

target_link_libraries(catfish_tests PRIVATE catfish_core)

add_test(NAME catfish_tests COMMAND catfish_tests)
