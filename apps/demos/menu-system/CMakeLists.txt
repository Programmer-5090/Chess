# Menu System Demo Application
add_executable(menu_demo
    src/main.cpp
)

target_link_libraries(menu_demo PRIVATE 
    chess::menus
    OpenGL::GL
)

chess_set_target_properties(menu_demo)

# Copy assets for demo
add_custom_command(TARGET menu_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:menu_demo>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:menu_demo>/resources
    COMMENT "Copying demo assets"
)