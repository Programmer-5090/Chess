cmake_minimum_required(VERSION 3.20)
project(ChessGame VERSION 1.0.0 LANGUAGES CXX)

# =============================================================================
# PROJECT CONFIGURATION
# =============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Default to Release for better performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "Build type not specified, defaulting to Release")
endif()

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /FS)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-function -Wno-unused-label -Wno-unused-parameter -Wno-unused-variable)
endif()

# =============================================================================
# DEPENDENCY SETUP (SDL2 + OpenGL)
# =============================================================================
include(FetchContent)

message(STATUS "⬇ Downloading SDL2 libraries (self-contained build)...")

set(SDL2_VERSION "2.30.7")
set(SDL2_IMAGE_VERSION "2.8.2")
set(SDL2_TTF_VERSION "2.22.0")

# Determine the correct SDL2 package based on compiler
if(MSVC)
    set(SDL2_URL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-devel-${SDL2_VERSION}-VC.zip")
    set(SDL2_IMAGE_URL "https://github.com/libsdl-org/SDL_image/releases/download/release-${SDL2_IMAGE_VERSION}/SDL2_image-devel-${SDL2_IMAGE_VERSION}-VC.zip")
    set(SDL2_TTF_URL "https://github.com/libsdl-org/SDL_ttf/releases/download/release-${SDL2_TTF_VERSION}/SDL2_ttf-devel-${SDL2_TTF_VERSION}-VC.zip")
    set(SDL2_LIB_SUBDIR "lib/x64")
elseif(MINGW)
    set(SDL2_URL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-devel-${SDL2_VERSION}-mingw.tar.gz")
    set(SDL2_IMAGE_URL "https://github.com/libsdl-org/SDL_image/releases/download/release-${SDL2_IMAGE_VERSION}/SDL2_image-devel-${SDL2_IMAGE_VERSION}-mingw.tar.gz")
    set(SDL2_TTF_URL "https://github.com/libsdl-org/SDL_ttf/releases/download/release-${SDL2_TTF_VERSION}/SDL2_ttf-devel-${SDL2_TTF_VERSION}-mingw.tar.gz")
    set(SDL2_LIB_SUBDIR "lib")
else()
    message(FATAL_ERROR "Unsupported compiler. This project supports MSVC and MinGW on Windows.")
endif()

# Download and extract SDL2 libraries
FetchContent_Declare(SDL2_Download URL ${SDL2_URL} DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(SDL2_image_Download URL ${SDL2_IMAGE_URL} DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(SDL2_ttf_Download URL ${SDL2_TTF_URL} DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_MakeAvailable(SDL2_Download SDL2_image_Download SDL2_ttf_Download)

# Set up include and library paths
set(SDL2_INCLUDE_DIRS 
    "${sdl2_download_SOURCE_DIR}/include"
    "${sdl2_image_download_SOURCE_DIR}/include"
    "${sdl2_ttf_download_SOURCE_DIR}/include"
)

set(SDL2_LIBRARIES 
    "${sdl2_download_SOURCE_DIR}/${SDL2_LIB_SUBDIR}/SDL2.lib"
    "${sdl2_download_SOURCE_DIR}/${SDL2_LIB_SUBDIR}/SDL2main.lib"
    "${sdl2_image_download_SOURCE_DIR}/${SDL2_LIB_SUBDIR}/SDL2_image.lib"
    "${sdl2_ttf_download_SOURCE_DIR}/${SDL2_LIB_SUBDIR}/SDL2_ttf.lib"
)

message(STATUS "✓ SDL2 ${SDL2_VERSION} downloaded and configured")
message(STATUS "  - Include dirs: ${SDL2_INCLUDE_DIRS}")

find_package(OpenGL REQUIRED)

# =============================================================================
# SHARED BACKEND LIBRARY
# =============================================================================
# Common functionality used by main game and demos
add_library(board_backend STATIC
    src/board/pieceManager.cpp
    src/board/moveExecutor.cpp
    src/board/boardRenderer.cpp
    src/perfProfiler.cpp
    src/textureCache.cpp
    src/logger.cpp
)

target_include_directories(board_backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(board_backend PUBLIC
    ${SDL2_LIBRARIES}
)

# =============================================================================
# MAIN CHESS GAME EXECUTABLE
# =============================================================================
set(GAME_SOURCES
    src/main.cpp
    src/screen.cpp
    src/board.cpp
    src/gameLogic.cpp
    src/input.cpp
    src/pieces/piece.cpp
    src/pieces/pawn.cpp
    src/pieces/rook.cpp
    src/pieces/knight.cpp
    src/pieces/bishop.cpp
    src/pieces/queen.cpp
    src/pieces/king.cpp
    src/ui/uiButton.cpp
    src/ui/uiManager.cpp
    src/ui/uiLabel.cpp
    src/ui/uiCheckbox.cpp
    src/ui/uiDialog.cpp
    src/ui/uiDropdown.cpp
    src/ui/uiTextInput.cpp
    src/ui/uiSlider.cpp
    src/ui/uiEnhancedBuilder.cpp
    src/ui/uiPromotionDialog.cpp
    src/menus/menuManager.cpp
)

add_executable(${PROJECT_NAME} ${GAME_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "chess_game"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    board_backend
    OpenGL::GL
)

# Copy assets after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/images $<TARGET_FILE_DIR:${PROJECT_NAME}>/images
    COMMENT "Copying game assets..."
)

# =============================================================================
# OPTIONAL: XTL INTEGRATION
# =============================================================================
set(XTL_DIR "${CMAKE_SOURCE_DIR}/extern/xtl")
if(EXISTS "${XTL_DIR}")
    if(EXISTS "${XTL_DIR}/CMakeLists.txt")
        add_subdirectory(${XTL_DIR} EXCLUDE_FROM_ALL)
        if(TARGET xtl::xtl)
            target_link_libraries(${PROJECT_NAME} PRIVATE xtl::xtl)
        elseif(TARGET xtl)
            target_link_libraries(${PROJECT_NAME} PRIVATE xtl)
        endif()
    elseif(EXISTS "${XTL_DIR}/include")
        add_library(xtl_headers INTERFACE)
        target_include_directories(xtl_headers INTERFACE "${XTL_DIR}/include")
        target_link_libraries(${PROJECT_NAME} PRIVATE xtl_headers)
    endif()
    message(STATUS "✓ XTL library integrated")
endif()

# =============================================================================
# DEMO EXECUTABLES (Optional - for testing UI components)
# =============================================================================

# Enhanced UI Demo
add_executable(enhanced_ui_demo
    demos/enhanced_ui_demo.cpp
    src/input.cpp
    src/screen.cpp
    src/board.cpp
    src/gameLogic.cpp
    src/pieces/piece.cpp
    src/pieces/pawn.cpp
    src/pieces/rook.cpp
    src/pieces/knight.cpp
    src/pieces/bishop.cpp
    src/pieces/queen.cpp
    src/pieces/king.cpp
    src/ui/uiButton.cpp
    src/ui/uiManager.cpp
    src/ui/uiLabel.cpp
    src/ui/uiCheckbox.cpp
    src/ui/uiDialog.cpp
    src/ui/uiDropdown.cpp
    src/ui/uiTextInput.cpp
    src/ui/uiSlider.cpp
    src/ui/uiEnhancedBuilder.cpp
    src/ui/uiPromotionDialog.cpp
    src/menus/menuManager.cpp
    ${HEADERS}
)

set_target_properties(enhanced_ui_demo PROPERTIES
    OUTPUT_NAME "enhanced_ui_demo"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output"
)

target_include_directories(enhanced_ui_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(enhanced_ui_demo PRIVATE board_backend OpenGL::GL)

add_custom_command(TARGET enhanced_ui_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:enhanced_ui_demo>/assets
    COMMENT "Copying demo assets"
)

# Menu Demo
add_executable(menu_demo
    demos/menu_demo.cpp
    src/input.cpp
    src/screen.cpp
    src/board.cpp
    src/gameLogic.cpp
    src/pieces/piece.cpp
    src/pieces/pawn.cpp
    src/pieces/rook.cpp
    src/pieces/knight.cpp
    src/pieces/bishop.cpp
    src/pieces/queen.cpp
    src/pieces/king.cpp
    src/ui/uiButton.cpp
    src/ui/uiManager.cpp
    src/ui/uiLabel.cpp
    src/ui/uiCheckbox.cpp
    src/ui/uiDialog.cpp
    src/ui/uiDropdown.cpp
    src/ui/uiTextInput.cpp
    src/ui/uiSlider.cpp
    src/ui/uiEnhancedBuilder.cpp
    src/ui/uiPromotionDialog.cpp
    src/menus/menuManager.cpp
    ${HEADERS}
)

set_target_properties(menu_demo PROPERTIES
    OUTPUT_NAME "menu_demo"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output"
)

target_include_directories(menu_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(menu_demo PRIVATE board_backend OpenGL::GL)

add_custom_command(TARGET menu_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:menu_demo>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/images $<TARGET_FILE_DIR:menu_demo>/images
    COMMENT "Copying demo assets"
)

# Performance Testing Demos
add_executable(utils_perft_demo demos/utils_perft_demo.cpp)
set_target_properties(utils_perft_demo PROPERTIES OUTPUT_NAME "utils_perft_demo" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output")
target_include_directories(utils_perft_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(utils_perft_demo PRIVATE board_backend)

# Board Perft Demo
add_executable(board_perft_demo
    demos/board_perft_demo.cpp
    src/board.cpp
    src/pieces/piece.cpp
    src/pieces/pawn.cpp
    src/pieces/rook.cpp
    src/pieces/knight.cpp
    src/pieces/bishop.cpp
    src/pieces/queen.cpp
    src/pieces/king.cpp
    src/ui/uiPromotionDialog.cpp
    src/ui/uiButton.cpp
    src/input.cpp
    ${HEADERS}
)

set_target_properties(board_perft_demo PROPERTIES OUTPUT_NAME "board_perft_demo" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output")
target_include_directories(board_perft_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(board_perft_demo PRIVATE board_backend ${SDL2_LIBRARIES})

add_custom_command(TARGET board_perft_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/images $<TARGET_FILE_DIR:board_perft_demo>/images
)

# Profile Perft Demo
add_executable(profile_perft
    demos/profile_perft.cpp
    src/board.cpp
    src/pieces/piece.cpp
    src/pieces/pawn.cpp
    src/pieces/rook.cpp
    src/pieces/knight.cpp
    src/pieces/bishop.cpp
    src/pieces/queen.cpp
    src/pieces/king.cpp
    src/ui/uiPromotionDialog.cpp
    src/ui/uiButton.cpp
    src/input.cpp
    ${HEADERS}
)

set_target_properties(profile_perft PROPERTIES OUTPUT_NAME "profile_perft" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output")
target_include_directories(profile_perft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(profile_perft PRIVATE board_backend ${SDL2_LIBRARIES})

add_custom_command(TARGET profile_perft POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/images $<TARGET_FILE_DIR:profile_perft>/images
)

# =============================================================================
# BUILD CONFIGURATION
# =============================================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /Od /Zi)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -g -O0)
    endif()
    message(STATUS "Debug build - optimizations disabled, debug symbols enabled")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    endif()
    message(STATUS "Release build - optimizations enabled")
endif()

# =============================================================================
# INSTALLATION (Optional)
# =============================================================================
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY images/ DESTINATION bin/images)
