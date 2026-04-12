cmake_minimum_required(VERSION 3.16)
project(GBCee C)

set(CMAKE_C_STANDARD 11)

# Collect all source files
file(GLOB_RECURSE SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.c
)

# ✅ FIRST: create executable
add_executable(gbcee ${SOURCES})

# 🔥 Include ALL subdirectories inside includes/
file(GLOB_RECURSE HEADER_DIRS LIST_DIRECTORIES true
    ${PROJECT_SOURCE_DIR}/includes/*
)

set(INCLUDE_DIRS "")
foreach(dir ${HEADER_DIRS})
    if(IS_DIRECTORY ${dir})
        list(APPEND INCLUDE_DIRS ${dir})
    endif()
endforeach()

# ✅ NOW: apply includes
target_include_directories(gbcee PRIVATE ${INCLUDE_DIRS})

# SDL2 (MinGW)
target_include_directories(gbcee PRIVATE
    C:/msys64/mingw64/include/SDL2
)

target_link_directories(gbcee PRIVATE
    C:/msys64/mingw64/lib
)

target_link_libraries(gbcee SDL2)