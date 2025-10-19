# Chess Board Library - Game logic and chess rules

add_library(chess_board STATIC
    src/board/board.cpp
    src/board/move_executor.cpp
    src/board/piece_manager.cpp
    src/board/game_logic.cpp
    src/board/fen_utils.cpp
    src/pieces/piece.cpp
    src/pieces/pawn.cpp
    src/pieces/rook.cpp
    src/pieces/knight.cpp
    src/pieces/bishop.cpp
    src/pieces/queen.cpp
    src/pieces/king.cpp
    
    # Bitboard move generation system
    src/bitboard/zobrist.cpp
    src/bitboard/precomputed_data.cpp
    src/bitboard/move_generator_bb.cpp
    src/bitboard/board_state.cpp
    src/bitboard/move.cpp
    src/bitboard/move_exec.cpp
)

# This command makes the headers in this library's 'include' directory
# available to any other target that links against 'chess_board'.
# It effectively adds 'libs/board/include' to the include path.
target_include_directories(chess_board PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"          # this lib's headers under chess/board
    "${CMAKE_SOURCE_DIR}/libs/core/include"        # chess/enums.h
    "${CMAKE_SOURCE_DIR}/libs/utils/include"       # chess/utils/logger.h etc.
    "${CMAKE_SOURCE_DIR}/libs/rendering/include"   # chess/rendering/texture_cache.h
    "${CMAKE_SOURCE_DIR}/libs/ui/include"          # chess/ui/input.h
    "${CMAKE_SOURCE_DIR}/Chess AI/include"         # chess/AI/*.h
)

# This command links the board library against its dependencies.
# Because these are PUBLIC dependencies, any include directories
# marked as PUBLIC or INTERFACE in those targets will be inherited
# by chess_board. This ensures we locate headers like "chess/enums.h", "SDL.h", etc.
target_link_libraries(chess_board PUBLIC
    chess::core
    chess::utils
    chess::rendering
    chess::ui
)

target_compile_features(chess_board PUBLIC cxx_std_20)

# Set target properties
chess_set_target_properties(chess_board)
chess_set_compile_features(chess_board)

# Create namespace alias
add_library(chess::board ALIAS chess_board)

# Register target for installation
set_property(GLOBAL APPEND PROPERTY CHESS_LIBRARY_TARGETS chess_board)