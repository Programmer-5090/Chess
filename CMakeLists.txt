cmake_minimum_required(VERSION 3.20)
project(ChessGame VERSION 1.0.0 LANGUAGES CXX)

# =============================================================================
# CMAKE MODULE INCLUDES
# =============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CompilerSettings)
include(Dependencies)

# Dependencies are handled in Dependencies.cmake
find_package(OpenGL REQUIRED)

# =============================================================================
# LIBRARY TARGETS
# =============================================================================
add_subdirectory(libs/core)
add_subdirectory(libs/utils)  
add_subdirectory(libs/rendering)
add_subdirectory(libs/ui)
add_subdirectory(libs/board)
add_subdirectory(libs/menus)

# =============================================================================
# MAIN APPLICATION
# =============================================================================
add_subdirectory(apps/chess-game)

# =============================================================================
# DEMO APPLICATIONS (Optional)
# =============================================================================
option(BUILD_DEMOS "Build demo applications" ON)
if(BUILD_DEMOS)
    message(STATUS "Building demo applications")
    add_subdirectory(apps/demos/enhanced-ui)
    add_subdirectory(apps/demos/menu-system)
    add_subdirectory(apps/demos/board-perft)
    add_subdirectory(apps/demos/profile-perft)
    add_subdirectory(apps/demos/utils-perft)
endif()

# =============================================================================
# INSTALLATION
# =============================================================================
include(InstallConfig)
