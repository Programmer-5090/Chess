# Utils Perft Demo Application
add_executable(utils_perft_demo 
    src/main.cpp
)

target_include_directories(utils_perft_demo PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Chess AI/include
)
target_link_libraries(utils_perft_demo PRIVATE chess::utils chess::ai)

chess_set_target_properties(utils_perft_demo)