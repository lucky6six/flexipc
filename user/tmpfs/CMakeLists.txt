cmake_minimum_required(VERSION 3.11)

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3")

add_executable(tmpfs.srv
    "tmpfs.c"
    "util.c"
)
