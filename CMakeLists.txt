cmake_minimum_required(VERSION 3.20)
project(ChessGame VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set policy for download timestamps
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /FS)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-function -Wno-unused-label -Wno-unused-parameter -Wno-unused-variable)
endif()

# Include FetchContent for automatic dependency downloading
include(FetchContent)

# Find SDL2 (required)
find_package(SDL2 REQUIRED)
if (SDL2_FOUND)
    message(STATUS "Found SDL2: ${SDL2_VERSION}")
else()
    message(FATAL_ERROR "SDL2 not found. Install SDL2 or set SDL2_DIR/include paths.")
endif()

# Optionally find SDL_image if you use it
find_package(SDL2_image QUIET)
find_package(SDL2_ttf QUIET)
if(SDL2_image_FOUND AND SDL2_ttf_FOUND)
    message(STATUS "Using system SDL2 installation")
    set(SDL2_LIBRARIES SDL2::SDL2 SDL2::SDL2main SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)
else()
    set(SDL2_FOUND FALSE)
endif()

# 2. For MinGW, check MSYS2 installation
if(NOT SDL2_FOUND AND WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(EXISTS "C:/msys64/ucrt64/include/SDL2/SDL.h")
        message(STATUS "Using MSYS2 SDL2 installation")
        set(SDL2_INCLUDE_DIRS "C:/msys64/ucrt64/include/SDL2")
        set(SDL2_LIBRARIES 
            "C:/msys64/ucrt64/lib/libmingw32.a"
            "C:/msys64/ucrt64/lib/libSDL2main.a"
            "C:/msys64/ucrt64/lib/libSDL2.dll.a"
            "C:/msys64/ucrt64/lib/libSDL2_image.dll.a"
            "C:/msys64/ucrt64/lib/libSDL2_ttf.dll.a"
        )
        set(SDL2_FOUND TRUE)
    endif()
endif()

# 3. Automatic download as fallback
if(NOT SDL2_FOUND)
    message(STATUS "System SDL2 not found. Downloading automatically...")
    
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
    endif()
    
    # Download and extract SDL2
    if(DEFINED SDL2_URL)
        message(STATUS "Downloading SDL2 ${SDL2_VERSION}...")
        FetchContent_Declare(
            SDL2_Download
            URL ${SDL2_URL}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(SDL2_Download)
        
        message(STATUS "Downloading SDL2_image ${SDL2_IMAGE_VERSION}...")
        FetchContent_Declare(
            SDL2_image_Download
            URL ${SDL2_IMAGE_URL}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(SDL2_image_Download)
        
        message(STATUS "Downloading SDL2_ttf ${SDL2_TTF_VERSION}...")
        FetchContent_Declare(
            SDL2_ttf_Download
            URL ${SDL2_TTF_URL}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(SDL2_ttf_Download)
        
        # Set up the include and library paths
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
        
        set(SDL2_FOUND TRUE)
        message(STATUS "SDL2 automatically downloaded and configured")
    endif()
endif()

# Final check
if(NOT SDL2_FOUND)
    message(FATAL_ERROR "Failed to set up SDL2. Please install manually:
    - MSYS2 (MinGW): pacman -S mingw-w64-ucrt-x86_64-SDL2{,_image,_ttf}
    - vcpkg (MSVC): vcpkg install sdl2 sdl2-image sdl2-ttf
    - Or ensure internet connection for automatic download")
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Define source files
set(SOURCES
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
    src/logger.cpp
)

# Define header files
set(HEADERS
    include/screen.h
    include/board.h
    include/gameLogic.h
    include/input.h
    include/enums.h
    include/pieceClasses.h
    include/pieces/piece.h
    include/pieces/pawn.h
    include/pieces/rook.h
    include/pieces/knight.h
    include/pieces/bishop.h
    include/pieces/queen.h
    include/pieces/king.h
    include/ui/uiCommon.h
    include/ui/uiElement.h
    include/ui/uiButton.h
    include/ui/uiLabel.h
    include/ui/uiPanel.h
    include/ui/uiDropdown.h
    include/ui/uiCheckbox.h
    include/ui/uiTextInput.h
    include/ui/uiManager.h
    include/ui/uiDialog.h
    include/ui/uiSlider.h
    include/ui/uiEnhancedBuilder.h
    include/ui/uiPromotionDialog.h
    include/menus/menuManager.h
    include/menus/mainMenu.h
    include/menus/playMenu.h
    include/menus/vsCompMenu.h
    include/menus/vsPlayerMenu.h
    include/menus/startGameMenu.h
    include/menus/settingsMenu.h
    include/logger.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "chess_game"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/output"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/output"
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

# Link libraries for main target
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
)

# --- Add this block to build and link the missing board implementation files ---
add_library(board_backend STATIC
    src/board/pieceManager.cpp
    src/board/boardRenderer.cpp
    src/logger.cpp
)

target_include_directories(board_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(board_backend PRIVATE
    ${SDL2_LIBRARIES}
)

# Link the backend to the main target so symbols from PieceManager/BoardRenderer are resolved
target_link_libraries(${PROJECT_NAME} PRIVATE board_backend)

# NOTE: demo targets are declared later in this file; each demo target links board_backend
# where it is defined (avoid calling target_link_libraries before the target exists).

# ----------------------
# Optional: Vendored xtl (header-only) integration
# If you add xtl as a git submodule or clone it into `extern/xtl`, this block
# will try to use its CMake exported target; otherwise it creates an INTERFACE
# target that points to the xtl include directory so you can `target_link_libraries`.
# This is intentionally non-fatal: if xtl isn't present nothing changes.
set(XTL_DIR "${CMAKE_SOURCE_DIR}/extern/xtl")
if(EXISTS "${XTL_DIR}")
    message(STATUS "Found vendored xtl at ${XTL_DIR}")

    # If the vendored project provides CMake targets via add_subdirectory, try that first
    if(EXISTS "${XTL_DIR}/CMakeLists.txt")
        # Prefer not to pollute the parent namespace if xtl provides targets; wrap in ALIASing
        add_subdirectory(${XTL_DIR} EXCLUDE_FROM_ALL)
        if(TARGET xtl::xtl)
            message(STATUS "Using xtl::xtl from vendored xtl")
            target_link_libraries(${PROJECT_NAME} PRIVATE xtl::xtl)
        elseif(TARGET xtl)
            message(STATUS "Using xtl target from vendored xtl")
            target_link_libraries(${PROJECT_NAME} PRIVATE xtl)
        endif()
    else()
        # No CMakeLists provided — assume header-only layout under include/
        if(EXISTS "${XTL_DIR}/include")
            add_library(xtl_headers INTERFACE)
            target_include_directories(xtl_headers INTERFACE "${XTL_DIR}/include")
            message(STATUS "Created INTERFACE target xtl_headers -> ${XTL_DIR}/include")
            target_link_libraries(${PROJECT_NAME} PRIVATE xtl_headers)
        else()
            message(WARNING "Found ${XTL_DIR} but no include/ or CMakeLists.txt; xtl integration skipped")
        endif()
    endif()
else()
    # Not vendored: do nothing. Users may use conda/vcpkg/FetchContent instead.
    message(STATUS "No vendored xtl found at ${XTL_DIR}; skipping vendored xtl integration")
endif()

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Copy assets to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/images
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/images
    COMMENT "Copying assets to executable directory"
)

# ----------------------
# Refactored UI Demo App
# ----------------------
## Legacy ui_demo removed (replaced by enhanced_ui_demo)

# ----------------------
# Improved UI Demo App
# ----------------------
## Legacy improved_ui_demo removed (superseded by enhanced_ui_demo)

# ----------------------
# Enhanced UI Demo App
# ----------------------
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
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/output"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/output"
)

target_include_directories(enhanced_ui_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(enhanced_ui_demo PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
)

target_link_libraries(enhanced_ui_demo PRIVATE board_backend)

add_custom_command(TARGET enhanced_ui_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:enhanced_ui_demo>/assets
    COMMENT "Copying demo assets"
)

# ----------------------
# Menu System Demo App
# ----------------------
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
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/output"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/output"
)

target_include_directories(menu_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(menu_demo PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
)

target_link_libraries(menu_demo PRIVATE board_backend)

add_custom_command(TARGET menu_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:menu_demo>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/images
    $<TARGET_FILE_DIR:menu_demo>/images
    COMMENT "Copying demo assets and chess images"
)

# ----------------------
# Utils Perft Demo (headless)
# ----------------------
add_executable(utils_perft_demo
    demos/utils_perft_demo.cpp
)

set_target_properties(utils_perft_demo PROPERTIES
    OUTPUT_NAME "utils_perft_demo"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/output"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/output"
)

# Ensure the demo can include the utils header that lives in "Chess AI/"
target_include_directories(utils_perft_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}    # allow relative includes from demos/
)

# Link board_backend to utils_perft_demo so it can use Logger and board code
target_link_libraries(utils_perft_demo PRIVATE board_backend)

# ----------------------
# Board Perft Demo (uses SDL and Board)
# ----------------------
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

set_target_properties(board_perft_demo PROPERTIES
    OUTPUT_NAME "board_perft_demo"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/output"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/output"
)

target_include_directories(board_perft_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(board_perft_demo PRIVATE
    ${SDL2_LIBRARIES}
)

target_link_libraries(board_perft_demo PRIVATE board_backend)

add_custom_command(TARGET board_perft_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/images
    $<TARGET_FILE_DIR:board_perft_demo>/images
    COMMENT "Copying chess images for perft demo"
)

# ----------------------
# Profile Perft Demo (performance profiling)
# ----------------------
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

set_target_properties(profile_perft PROPERTIES
    OUTPUT_NAME "profile_perft"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/output"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/output"
)

target_include_directories(profile_perft PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(profile_perft PRIVATE
    ${SDL2_LIBRARIES}
)

target_link_libraries(profile_perft PRIVATE board_backend)

add_custom_command(TARGET profile_perft POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/images
    $<TARGET_FILE_DIR:profile_perft>/images
    COMMENT "Copying chess images for profile demo"
)

# Install targets
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY images/
    DESTINATION bin/images
)

# Debug build configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
    if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Od /Zi /showIncludes /FC)
    # Emit verbose link info and a map file to diagnose unresolved externals
    target_link_options(${PROJECT_NAME} PRIVATE /VERBOSE /MAP:${CMAKE_SOURCE_DIR}/build/chess_game.map)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -g -O0)
    endif()
endif()

# Release build configuration
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    endif()
endif()
