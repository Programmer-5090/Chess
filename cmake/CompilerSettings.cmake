
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "Build type not specified, defaulting to Release")
endif()

if(MSVC)
    add_compile_options(/W4 /FS)
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od /Zi)
        add_compile_definitions(DEBUG)
        message(STATUS "Debug build - optimizations disabled, debug symbols enabled")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2)
        add_compile_definitions(NDEBUG)
        message(STATUS "Release build - optimizations enabled")
    endif()
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # GCC/Clang settings
    add_compile_options(-Wall -Wextra -Wno-unused-function -Wno-unused-label 
                       -Wno-unused-parameter -Wno-unused-variable)
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
        add_compile_definitions(DEBUG)
        message(STATUS "Debug build - optimizations disabled, debug symbols enabled")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
        add_compile_definitions(NDEBUG)
        message(STATUS "Release build - optimizations enabled")
    endif()
    
else()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()


function(chess_set_target_properties target_name)
    # Set output directories
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/$<CONFIG>"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/lib/$<CONFIG>"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/lib/$<CONFIG>"
    )
    
    # Enable position independent code
    set_target_properties(${target_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

# Function to set common compile features
function(chess_set_compile_features target_name)
    target_compile_features(${target_name} PRIVATE cxx_std_17)
endfunction()