# Bitboard Test Demo - Compare bitboard vs legacy move generation

add_executable(bitboard_test
    src/main.cpp
)

target_link_libraries(bitboard_test PRIVATE
    chess::board
    chess::utils
)

chess_set_target_properties(bitboard_test)
chess_set_compile_features(bitboard_test)

# Set output directory
set_target_properties(bitboard_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/$<CONFIG>"
)

# Register target for installation
set_property(GLOBAL APPEND PROPERTY CHESS_EXECUTABLE_TARGETS bitboard_test)

# Stockfish comparison test
add_executable(stockfish_compare
    src/stockfish_compare.cpp
)

target_link_libraries(stockfish_compare PRIVATE
    chess::board
    chess::utils
)

chess_set_target_properties(stockfish_compare)
chess_set_compile_features(stockfish_compare)

# Set output directory
set_target_properties(stockfish_compare PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/$<CONFIG>"
)

# Register target for installation
set_property(GLOBAL APPEND PROPERTY CHESS_EXECUTABLE_TARGETS stockfish_compare)
