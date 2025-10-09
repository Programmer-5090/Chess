# Board Perft Demo Application
add_executable(board_perft_demo
    src/main.cpp
)

target_link_libraries(board_perft_demo PRIVATE 
    chess::board      # brings in core/utils/rendering/ui transitively, but link explicitly to silence missing symbol
    chess::core
    ${SDL2_LIBRARIES}
)

chess_set_target_properties(board_perft_demo)

# Copy resources for demo
add_custom_command(TARGET board_perft_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:board_perft_demo>/resources
    COMMENT "Copying demo resources"
)