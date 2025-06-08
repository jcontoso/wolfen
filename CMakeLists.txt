cmake_minimum_required(VERSION 3.0)
project(wolfen C)

include(CheckCSourceCompiles)
include(CMakePushCheckState)
include(GNUInstallDirs)
option(TESTS "Build tests" TRUE)

set(CMAKE_C_STANDARD 90)
find_package(PkgConfig REQUIRED)

add_subdirectory(wolfen)
if(TESTS)
	add_subdirectory(wolfen-test)
endif()
