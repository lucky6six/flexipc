cmake_minimum_required(VERSION 3.14)

project(test_buddy_and_slab C)
set(SOURCE_PATH ../../kernel/mm)
set(OBJECT_DIR ${CMAKE_BINARY_DIR}/CMakeFiles/test_buddy_and_slab.dir)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -g")

set(ARCH "aarch64")
set(PLAT "raspi3")

set(SOURCES
	test_buddy_and_slab.c
	stub.c
	"${SOURCE_PATH}/buddy.c"
	"${SOURCE_PATH}/slab.c"
)

add_executable(test_buddy_and_slab ${SOURCES})
include_directories(
    ../../kernel/mm/
    ../../kernel/include/
    ../include
)

target_compile_options(
	test_buddy_and_slab PRIVATE 
	-fno-builtin-memset
	-fno-builtin-memcpy                       
)                                             

target_link_libraries(test_buddy_and_slab -lm)

add_custom_target(
    lcov
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -z
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -b . --initial -c -o lcov.info
    COMMAND CTEST_OUTPUT_ON_FAILURE=1 ${CMAKE_MAKE_PROGRAM} test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_custom_command(
    TARGET lcov
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -c -o lcov.info
    COMMAND genhtml -o report --prefix=`pwd` lcov.info
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS test_buddy_and_slab
)

enable_testing()
add_test(test_buddy_and_slab ${CMAKE_CURRENT_BINARY_DIR}/test_buddy_and_slab)
