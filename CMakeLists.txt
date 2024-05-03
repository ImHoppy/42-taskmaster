project(taskmaster)

set(CMAKE_CXX_STANDARD 14)

# Set the output directory for all executables except 'taskmaster'
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# Add source files from the src directory
file(GLOB_RECURSE SOURCES "src/*.c")

add_executable(taskmaster ${SOURCES})

include_directories(src)
include_directories(.)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

# Add cjson library
add_subdirectory(cjson)

# Link cjson library to the taskmaster executable
target_link_libraries(taskmaster cjson)
