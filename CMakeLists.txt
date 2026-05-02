cmake_minimum_required(VERSION 3.24)

project(ChessEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHESSENGINE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(CHESSENGINE_FETCH_CATCH2 "Download Catch2 for tests when it is not installed" OFF)

add_library(chess_core
    src/core/board.cpp
    src/core/fen.cpp
    src/core/game_status.cpp
    src/core/move.cpp
    src/core/movegen.cpp
    src/core/perft.cpp
    src/core/zobrist.cpp
)
target_include_directories(chess_core PUBLIC include)
target_compile_options(chess_core PRIVATE -Wall -Wextra -Wpedantic)
if(CHESSENGINE_WARNINGS_AS_ERRORS)
    target_compile_options(chess_core PRIVATE -Werror)
endif()

add_library(chess_engine
    src/engine/evaluation.cpp
    src/engine/position_suite.cpp
    src/engine/search.cpp
    src/engine/static_exchange.cpp
    src/engine/transposition_table.cpp
)
target_link_libraries(chess_engine PUBLIC chess_core)
target_compile_options(chess_engine PRIVATE -Wall -Wextra -Wpedantic)
if(CHESSENGINE_WARNINGS_AS_ERRORS)
    target_compile_options(chess_engine PRIVATE -Werror)
endif()

add_executable(chess_perft apps/perft_main.cpp)
target_link_libraries(chess_perft PRIVATE chess_core)

add_executable(chess_uci apps/uci_main.cpp)
target_link_libraries(chess_uci PRIVATE chess_engine)

add_executable(chess_bench apps/bench_main.cpp)
target_link_libraries(chess_bench PRIVATE chess_engine)

add_executable(chess_referee apps/referee_main.cpp)
target_link_libraries(chess_referee PRIVATE chess_core)

include(CTest)
if(BUILD_TESTING)
    find_package(Catch2 3 QUIET)
    if(NOT Catch2_FOUND AND CHESSENGINE_FETCH_CATCH2)
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.6.0.tar.gz
        )
        FetchContent_MakeAvailable(Catch2)
    endif()

    if(NOT Catch2_FOUND AND NOT TARGET Catch2::Catch2WithMain)
        message(FATAL_ERROR "Catch2 is required for tests. Install catch2 or configure with -DCHESSENGINE_FETCH_CATCH2=ON.")
    endif()

    add_executable(chess_core_tests tests/core_tests.cpp)
    target_link_libraries(chess_core_tests PRIVATE chess_core Catch2::Catch2WithMain)
    add_executable(chess_engine_tests tests/engine_tests.cpp)
    target_link_libraries(chess_engine_tests PRIVATE chess_engine Catch2::Catch2WithMain)
    include(Catch)
    catch_discover_tests(chess_core_tests)
    catch_discover_tests(chess_engine_tests)

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME uci_smoke
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/uci_smoke_test.py $<TARGET_FILE:chess_uci>
        )
        add_test(
            NAME bench_smoke
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/bench_smoke_test.py $<TARGET_FILE:chess_bench>
        )
        add_test(
            NAME gauntlet_smoke
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/gauntlet_smoke_test.py
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/gauntlet.py
                $<TARGET_FILE:chess_uci>
                $<TARGET_FILE:chess_referee>
        )
    endif()
endif()
