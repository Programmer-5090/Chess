# Chess Utils Library - Logging and profiling utilities

add_library(chess_utils STATIC
    src/logger.cpp
    src/profiler.cpp
)

target_include_directories(chess_utils PUBLIC
    include
)

target_link_libraries(chess_utils PUBLIC
    chess::core
)

target_compile_features(chess_utils PUBLIC cxx_std_17)

# Set target properties
chess_set_target_properties(chess_utils)
chess_set_compile_features(chess_utils)

# Create namespace alias
add_library(chess::utils ALIAS chess_utils)

# Register target for installation
set_property(GLOBAL APPEND PROPERTY CHESS_LIBRARY_TARGETS chess_utils)