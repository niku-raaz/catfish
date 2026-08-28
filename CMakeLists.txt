cmake_minimum_required(VERSION 3.20)

project(Catfish
    VERSION 0.1.0
    DESCRIPTION "Catfish bitboard chess engine"
    LANGUAGES CXX
)

option(CATFISH_ENABLE_SYZYGY "Enable local Syzygy tablebases through Fathom" OFF)
set(CATFISH_FATHOM_ROOT "" CACHE PATH "Path to a Fathom source checkout")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(catfish_core
    src/bitboard.cpp
    src/board.cpp
    src/eval.cpp
    src/fen.cpp
    src/move.cpp
    src/movegen.cpp
    src/opening_book.cpp
    src/perft.cpp
    src/search.cpp
    src/tablebase.cpp
    src/uci.cpp
    src/zobrist.cpp
)

target_include_directories(catfish_core PUBLIC include)

if(CATFISH_ENABLE_SYZYGY)
    if(NOT EXISTS "${CATFISH_FATHOM_ROOT}/src/tbprobe.c")
        message(FATAL_ERROR
            "CATFISH_ENABLE_SYZYGY requires CATFISH_FATHOM_ROOT to point to "
            "a Fathom checkout containing src/tbprobe.c")
    endif()
    enable_language(C)
    find_package(Threads REQUIRED)
    add_library(catfish_fathom STATIC "${CATFISH_FATHOM_ROOT}/src/tbprobe.c")
    target_include_directories(catfish_fathom PUBLIC "${CATFISH_FATHOM_ROOT}/src")
    target_link_libraries(catfish_fathom PUBLIC Threads::Threads)
    target_compile_definitions(catfish_core PRIVATE CATFISH_HAS_FATHOM=1)
    target_link_libraries(catfish_core PRIVATE catfish_fathom)
endif()

if(MSVC)
    target_compile_options(catfish_core PRIVATE /W4)
else()
    target_compile_options(catfish_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(catfish src/main.cpp)
target_link_libraries(catfish PRIVATE catfish_core)

add_executable(catfish_uci src/uci_main.cpp)
target_link_libraries(catfish_uci PRIVATE catfish_core)

add_executable(catfish_perft tools/perft_runner.cpp)
target_link_libraries(catfish_perft PRIVATE catfish_core)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
