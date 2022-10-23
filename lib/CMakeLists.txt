cmake_minimum_required(VERSION 3.0)

project(stream)

file(GLOB src_files src/*.c)

include_directories(include)
add_executable(${PROJECT_NAME} ${src_files})

