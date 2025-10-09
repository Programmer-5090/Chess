# Enhanced UI Demo Application
add_executable(enhanced_ui_demo
    src/main.cpp
)

target_link_libraries(enhanced_ui_demo PRIVATE 
    chess::ui
    OpenGL::GL
)

chess_set_target_properties(enhanced_ui_demo)

# Copy assets for demo
add_custom_command(TARGET enhanced_ui_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:enhanced_ui_demo>/assets
    COMMENT "Copying demo assets"
)