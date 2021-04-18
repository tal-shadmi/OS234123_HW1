# cmake_minimum_required(VERSION <specify CMake version here>)
project(OS234123_HW1)

set(CMAKE_CXX_STANDARD 11)

add_executable(OS234123_HW1 skeleton_smash/Commands.cpp skeleton_smash/Commands.h
               skeleton_smash/signals.cpp skeleton_smash/signals.h skeleton_smash/smash.cpp)