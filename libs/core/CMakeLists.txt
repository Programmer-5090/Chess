# Chess Core Library - Essential types and utilities
# This library provides common types, enums, and utility functions that are header-only.

add_library(chess_core STATIC
    src/utilities.cpp
)

target_include_directories(chess_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(chess_core PUBLIC cxx_std_17)

# Create namespace alias
add_library(chess::core ALIAS chess_core)