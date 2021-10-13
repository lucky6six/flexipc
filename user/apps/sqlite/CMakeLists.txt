cmake_minimum_required(VERSION 3.11)

# ChCore does not consider SSE registers for now.
# We use this walkaround to suppress the compiler complaint.
# TODO: saving/restore SSE states
string(REPLACE "-mno-sse" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

file (GLOB libc_source_files
	"../../lib/*.c"
	"../../common-lib/*.c"
)
add_library(chcore_abi STATIC ${libc_source_files})

file(
	GLOB sqlite_source_files
	"main.c"
	"sqlite3.c"
)
add_executable(sqlite.bin ${sqlite_source_files})
target_link_libraries(sqlite.bin chcore_abi)

target_include_directories(chcore_abi
    PRIVATE "../../include"
)
