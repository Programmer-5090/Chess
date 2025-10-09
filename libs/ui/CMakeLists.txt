# Chess UI Library - User interface components

add_library(chess_ui STATIC
    src/manager.cpp
    src/input.cpp
    src/controls/button.cpp
    src/controls/label.cpp
    src/controls/checkbox.cpp
    src/controls/dialog.cpp
    src/controls/dropdown.cpp
    src/controls/text_input.cpp
    src/controls/slider.cpp
    src/controls/promotion_dialog.cpp
    src/layouts/builder.cpp
    src/layouts/enhanced_builder.cpp
)

target_include_directories(chess_ui PUBLIC
    include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(chess_ui PUBLIC
    chess::rendering
    ${SDL2_LIBRARIES}
)

target_compile_features(chess_ui PUBLIC cxx_std_17)

# Set target properties
chess_set_target_properties(chess_ui)
chess_set_compile_features(chess_ui)

# Create namespace alias
add_library(chess::ui ALIAS chess_ui)

# Register target for installation
set_property(GLOBAL APPEND PROPERTY CHESS_LIBRARY_TARGETS chess_ui)