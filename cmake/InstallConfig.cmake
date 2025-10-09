# Install main executable
install(TARGETS chess_game 
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

# Install resources
install(DIRECTORY resources/ 
    DESTINATION share/chess/resources
    COMPONENT Resources
    PATTERN "*.png"
    PATTERN "*.jpg"
)

# Install assets (fonts, etc.)
install(DIRECTORY assets/ 
    DESTINATION share/chess/assets
    COMPONENT Assets
)

# Install libraries (if building shared)
get_property(CHESS_LIBRARIES GLOBAL PROPERTY CHESS_LIBRARY_TARGETS)
if(CHESS_LIBRARIES)
    install(TARGETS ${CHESS_LIBRARIES}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        COMPONENT Libraries
    )
endif()

# Create desktop entry on Linux
if(UNIX AND NOT APPLE)
    configure_file(
        "${CMAKE_SOURCE_DIR}/resources/chess.desktop.in"
        "${CMAKE_BINARY_DIR}/chess.desktop"
        @ONLY
    )
    
    install(FILES "${CMAKE_BINARY_DIR}/chess.desktop"
        DESTINATION share/applications
        COMPONENT Desktop
    )
endif()

# Set up CPack components
set(CPACK_COMPONENTS_ALL Runtime Resources Assets Libraries Desktop)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Chess Game")
set(CPACK_COMPONENT_RESOURCES_DISPLAY_NAME "Game Resources")
set(CPACK_COMPONENT_ASSETS_DISPLAY_NAME "Game Assets")
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Chess Libraries")
set(CPACK_COMPONENT_DESKTOP_DISPLAY_NAME "Desktop Integration")

set(CPACK_COMPONENT_RUNTIME_REQUIRED ON)
set(CPACK_COMPONENT_RESOURCES_REQUIRED ON)