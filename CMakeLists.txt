project(taskmaster)

set(CMAKE_CXX_STANDARD 14)

# Add source files from the src directory
file(GLOB_RECURSE SOURCES "src/*.c")

# Add cjson.c to your sources
list(APPEND SOURCES "cjson/cJSON.c")

add_executable(taskmaster ${SOURCES})

include_directories(.)
include_directories(src)
include_directories(cjson)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

