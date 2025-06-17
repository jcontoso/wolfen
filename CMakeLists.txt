cmake_minimum_required(VERSION 3.0)
project(wolfen C)

include(CheckCSourceCompiles)
include(CMakePushCheckState)
include(GNUInstallDirs)

set(CMAKE_C_STANDARD 99)
find_package(PkgConfig REQUIRED)

add_subdirectory(wolfen)
add_subdirectory(wolfen-test)

