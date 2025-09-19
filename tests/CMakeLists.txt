cmake_minimum_required(VERSION 3.16)

# Add test executable
add_executable(turbobook_tests test_main.cpp)

# Link with main library if you create one later
# target_link_libraries(turbobook_tests PRIVATE turbobook_lib)

# Include directories
target_include_directories(turbobook_tests PRIVATE ../include)
