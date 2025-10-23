include(FetchContent)

# Set policies
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

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

# Check if SDL2 libraries already exist to avoid re-downloading
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/sdl2_download-src/lib/${SDL2_LIB_SUBDIR}/SDL2.lib")
    message(STATUS "🔍 Checking for existing SDL2 libraries in project...")
    message(STATUS "✓ Found existing SDL2 libraries in project directory")
    
    # Set up paths directly if libraries exist
    set(SDL2_INCLUDE_DIRS 
        "${CMAKE_BINARY_DIR}/_deps/sdl2_download-src/include"
        "${CMAKE_BINARY_DIR}/_deps/sdl2_image_download-src/include"
        "${CMAKE_BINARY_DIR}/_deps/sdl2_ttf_download-src/include"
    )
    
    set(SDL2_LIBRARIES 
        "${CMAKE_BINARY_DIR}/_deps/sdl2_download-src/${SDL2_LIB_SUBDIR}/SDL2.lib"
        "${CMAKE_BINARY_DIR}/_deps/sdl2_download-src/${SDL2_LIB_SUBDIR}/SDL2main.lib"
        "${CMAKE_BINARY_DIR}/_deps/sdl2_image_download-src/${SDL2_LIB_SUBDIR}/SDL2_image.lib"
        "${CMAKE_BINARY_DIR}/_deps/sdl2_ttf_download-src/${SDL2_LIB_SUBDIR}/SDL2_ttf.lib"
    )
else()
    # Download and extract SDL2 libraries
    FetchContent_Declare(SDL2_Download URL ${SDL2_URL} DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_Declare(SDL2_image_Download URL ${SDL2_IMAGE_URL} DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_Declare(SDL2_ttf_Download URL ${SDL2_TTF_URL} DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    
    FetchContent_MakeAvailable(SDL2_Download SDL2_image_Download SDL2_ttf_Download)
    
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
endif()

message(STATUS "✓ SDL2 ${SDL2_VERSION} configured")
message(STATUS "  - Include dirs: ${SDL2_INCLUDE_DIRS}")
message(STATUS "  - Libraries: ${SDL2_LIBRARIES}")

# Find OpenGL
find_package(OpenGL REQUIRED)

# Fetch xtl into the build tree (no writes to workspace external/)
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps")
FetchContent_Declare(
  xtl
  GIT_REPOSITORY https://github.com/xtensor-stack/xtl.git
  GIT_TAG        0.7.7
)
FetchContent_MakeAvailable(xtl)
if(TARGET xtl::xtl)
  set(XTL_TARGET xtl::xtl)
elseif(TARGET xtl)
  set(XTL_TARGET xtl)
endif()
if(XTL_TARGET)
  message(STATUS "✓ XTL via FetchContent (build/_deps) -> target: ${XTL_TARGET}")
endif()