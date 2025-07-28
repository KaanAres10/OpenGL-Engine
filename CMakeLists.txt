cmake_minimum_required(VERSION 3.12)
project("OpenGL-Engine" LANGUAGES C CXX)

find_package(OpenGL REQUIRED)

foreach(cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${cfg} CFG)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFG} "${PROJECT_SOURCE_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG} "${PROJECT_SOURCE_DIR}/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} "${PROJECT_SOURCE_DIR}/bin")
endforeach()

add_subdirectory(third_party)
add_subdirectory(src)