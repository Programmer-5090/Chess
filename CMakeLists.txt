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

# Try to find SDL2 in various ways (priority order)
set(SDL2_FOUND FALSE)

# 1. Try CMake's built-in find modules first (for system installations)
find_package(SDL2 QUIET)
if(SDL2_FOUND)
    find_package(SDL2_image QUIET)
    find_package(SDL2_ttf QUIET)
    if(SDL2_image_FOUND AND SDL2_ttf_FOUND)
        message(STATUS "Using system SDL2 installation")
        set(SDL2_LIBRARIES SDL2::SDL2 SDL2::SDL2main SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)
    else()
        set(SDL2_FOUND FALSE)
    endif()
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

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
    OpenGL::GLU
)

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
add_executable(ui_demo
    demos/ui_demo.cpp
    src/input.cpp
    src/ui/uiButton.cpp
    src/ui/uiManager.cpp
    src/ui/uiLabel.cpp
    src/ui/uiCheckbox.cpp
    src/ui/uiDialog.cpp
    src/ui/uiDropdown.cpp
    src/ui/uiTextInput.cpp
    src/ui/uiSlider.cpp
    ${HEADERS}
)

set_target_properties(ui_demo PROPERTIES
    OUTPUT_NAME "ui_demo"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/output"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/output"
)

target_include_directories(ui_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(ui_demo PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
    OpenGL::GLU
)

add_custom_command(TARGET ui_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:ui_demo>/assets
    COMMENT "Copying demo assets"
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
        target_compile_options(${PROJECT_NAME} PRIVATE /Od /Zi)
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
