# Profile Perft Demo Application
add_executable(profile_perft
    src/main.cpp
)

target_link_libraries(profile_perft PRIVATE 
    chess::board
    chess::core
)

chess_set_target_properties(profile_perft)