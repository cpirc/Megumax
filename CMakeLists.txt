cmake_minimum_required(VERSION 3.12)

project(megumax)

###
# Find packages
###
find_package(Threads REQUIRED)
###

###
# Base directory relative includes for everyone
###
include_directories(include src)
###

###
# Global Flags
###
add_compile_options(-Wall -Wextra -Wpedantic)
###

###
# Flags
###
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()
###

###
# Targets
###
# megumax
add_executable(
    megumax
    src/main.cpp
    src/search.cpp
    src/search_globals.cpp
    src/rng_service.cpp
)
target_link_libraries(megumax Threads::Threads)
