cmake_minimum_required(VERSION 3.20)
project(post_dns C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(libuv)

if (WIN32)
    file(COPY ${CMAKE_BINARY_DIR}/libuv/libuv.dll DESTINATION ${EXECUTABLE_OUTPUT_PATH}/)
endif ()

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/libuv/include)
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRCS)

add_executable(PostDns ${SRCS} main.c)
target_link_libraries(PostDns uv)