cmake_minimum_required(VERSION 3.20)
project(VMSourceCONV VERSION 0.4.0 LANGUAGES CXX)

option(VMSOURCECONV_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_subdirectory(src/VMSourceCONV)
