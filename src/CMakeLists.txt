cmake_minimum_required(VERSION 3.16)
project(jbf)

enable_testing()

# Set up language settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# Find Halide
find_package(Halide REQUIRED)

add_executable(jbf_generator jbf_gen.cpp)
target_link_libraries(jbf_generator PRIVATE Halide::Generator Halide::Tools)
add_halide_library(jbf FROM jbf_generator GENERATOR joint_bilateral_grid TARGETS cmake)

add_executable(jbf_run jbf_run.cpp)
target_link_libraries(jbf_run jbf Halide::ImageIO)
