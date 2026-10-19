cmake_minimum_required(VERSION 3.20)
project(MUWindowManager LANGUAGES CXX)

add_executable(MUWindowManager WIN32 MUWindowManager.cpp)
target_compile_features(MUWindowManager PRIVATE cxx_std_17)
target_compile_definitions(MUWindowManager PRIVATE UNICODE _UNICODE)
target_link_libraries(MUWindowManager PRIVATE shell32 user32)
