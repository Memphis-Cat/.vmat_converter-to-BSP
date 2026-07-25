cmake_minimum_required(VERSION 3.20)
project(VMSourceCONV VERSION 0.7.0 LANGUAGES C CXX)

option(VMSOURCECONV_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(LZ4_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LZ4_BUNDLED_MODE ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    vmsourceconv_lz4
    GIT_REPOSITORY https://github.com/lz4/lz4.git
    GIT_TAG v1.10.0
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR build/cmake
)
FetchContent_MakeAvailable(vmsourceconv_lz4)

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZSTD_BUILD_COMPRESSION OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_DECOMPRESSION ON CACHE BOOL "" FORCE)
set(ZSTD_BUILD_DICTBUILDER OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    vmsourceconv_zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG v1.5.7
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR build/cmake
)
FetchContent_MakeAvailable(vmsourceconv_zstd)

add_library(vmsourceconv_compression INTERFACE)
target_link_libraries(vmsourceconv_compression INTERFACE lz4_static libzstd_static)

add_subdirectory(src/VMSourceCONV)
