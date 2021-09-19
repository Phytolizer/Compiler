cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)

project(
  Compiler
  LANGUAGES C CXX
  VERSION 0.1.0
  DESCRIPTION "A compiler in C++")

add_subdirectory(Source)
