# Bitboard Perft Demo Application
add_executable(bitboard_perft_demo
    src/main.cpp
)

# Link against the same core libs as board_perft_demo so we can reuse utilities
# and data structures (logger, enums, etc.)
target_link_libraries(bitboard_perft_demo PRIVATE 
    chess::board
    chess::core
)

chess_set_target_properties(bitboard_perft_demo)
