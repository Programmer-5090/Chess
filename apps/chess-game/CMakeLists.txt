# Main Chess Game Application

add_executable(chess_game
    src/main.cpp
    ${CMAKE_SOURCE_DIR}/libs/rendering/src/screen.cpp
)

target_include_directories(chess_game PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(chess_game PRIVATE
    chess::utils
    chess::board
    chess::ui
    chess::rendering
    chess::menus
    chess::ai
    OpenGL::GL
)

# Set target properties
chess_set_target_properties(chess_game)
chess_set_compile_features(chess_game)

# Copy resources after build
add_custom_command(TARGET chess_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:chess_game>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:chess_game>/assets
    COMMENT "Copying game assets..."
)