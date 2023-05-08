cmake_minimum_required(VERSION 3.20)
project(post_dns C)

set(CMAKE_C_STANDARD 11)

include_directories(${PROJECT_SOURCE_DIR}/include)
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRCS)

add_executable(PostDns ${SRCS} main.c)
target_link_libraries(PostDns)