# Chess Menus Library - Menu system

add_library(chess_menus STATIC
    src/manager.cpp
)

target_include_directories(chess_menus PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/include  # for legacy menus/*.h headers
)

target_link_libraries(chess_menus PUBLIC
    chess::ui
    chess::board  # needed for <chess/board/board.h> include
)

target_compile_features(chess_menus PUBLIC cxx_std_17)

# Set target properties
chess_set_target_properties(chess_menus)
chess_set_compile_features(chess_menus)

# Create namespace alias
add_library(chess::menus ALIAS chess_menus)

# Register target for installation
set_property(GLOBAL APPEND PROPERTY CHESS_LIBRARY_TARGETS chess_menus)