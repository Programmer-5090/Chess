# Chess Rendering Library - Graphics and rendering components

add_library(chess_rendering STATIC
    src/renderer.cpp
    src/texture_cache.cpp
    src/board_renderer.cpp
)

target_include_directories(chess_rendering PUBLIC
    include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(chess_rendering PUBLIC
    chess::utils
    ${SDL2_LIBRARIES}
)

target_link_libraries(chess_rendering PRIVATE
    chess::ui
    chess::board
)

target_compile_features(chess_rendering PUBLIC cxx_std_17)

# Set target properties
chess_set_target_properties(chess_rendering)
chess_set_compile_features(chess_rendering)

# Create namespace alias
add_library(chess::rendering ALIAS chess_rendering)

# Register target for installation
set_property(GLOBAL APPEND PROPERTY CHESS_LIBRARY_TARGETS chess_rendering)