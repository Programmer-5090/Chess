# Chess AI Library

add_library(chess_ai STATIC
    src/ai.cpp
    src/ai_bb.cpp
)

target_include_directories(chess_ai PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/libs/core/include"
    "${CMAKE_SOURCE_DIR}/libs/utils/include"
    "${CMAKE_SOURCE_DIR}/libs/board/include"
)

target_link_libraries(chess_ai PUBLIC
    chess_core
    chess_utils
    chess_board
)

# Set C++ standard
target_compile_features(chess_ai PUBLIC cxx_std_17)

# Set target properties (consistent with other libs)
chess_set_target_properties(chess_ai)
chess_set_compile_features(chess_ai)

# Create namespace alias
add_library(chess::ai ALIAS chess_ai)

# Register target for installation
set_property(GLOBAL APPEND PROPERTY CHESS_LIBRARY_TARGETS chess_ai)