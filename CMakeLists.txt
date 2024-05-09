cmake_minimum_required(VERSION 3.22)

project(taskmaster)

set(CMAKE_CXX_STANDARD 14)

# Shared source files
file(GLOB_RECURSE SOURCES_SHARED "src/shared/*.c")

#
# Client Executable
#
# Add source files from the src directory
file(GLOB_RECURSE SOURCES_CLIENT "src/client/*.c")

add_executable(taskmasterctl ${SOURCES_CLIENT} ${SOURCES_SHARED})
target_include_directories(taskmasterctl PRIVATE src/client)
target_include_directories(taskmasterctl PRIVATE src/shared)

#
# Server Executable
#
# Add source files from the src directory
file(GLOB_RECURSE SOURCES_SERVER "src/server/*.c")
# Add cjson.c to your sources
list(APPEND SOURCES_SERVER "cjson/cJSON.c")

add_executable(taskmasterd ${SOURCES_SERVER} ${SOURCES_SHARED})
target_include_directories(taskmasterd PRIVATE src/server)
target_include_directories(taskmasterd PRIVATE src/shared)
target_include_directories(taskmasterd PRIVATE cjson)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

# Custom target to recompile
add_custom_target(re
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target taskmasterd
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target taskmasterctl
    COMMENT "Rebuilding"
)
