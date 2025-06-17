cmake_minimum_required(VERSION 3.0)

set(WOLFEN_TESTS AUTO CACHE STRING "Build test suite")
set_property(CACHE WOLFEN_TESTS PROPERTY STRINGS AUTO ON OFF) 

if(WOLFEN_TESTS STREQUAL "AUTO")
	set(WOLFEN_TEST_PC_DEPS_FOUND FALSE)
	pkg_check_modules(WOLFEN_TEST_PC_DEPS REQUIRED IMPORTED_TARGET "wayland-client")
elseif(WOLFEN_TESTS)
	pkg_check_modules(WOLFEN_TEST_PC_DEPS IMPORTED_TARGET "wayland-client")
else()
	set(WOLFEN_TEST_PC_DEPS_FOUND "???")
endif()

if(WOLFEN_TESTS STREQUAL "AUTO" OR WOLFEN_TESTS AND WOLFEN_TEST_PC_DEPS_FOUND)
	file(GLOB wolfen_test_src "src/*.c")
	add_executable(wolfen_test ${wolfen_test_src})
	add_executable(wolfen_test::wolfen_test ALIAS wolfen_test)
	target_include_directories(wolfen_test PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
	target_link_libraries(wolfen_test PRIVATE PkgConfig::WOLFEN_TEST_PC_DEPS)
	target_link_libraries(wolfen_test PRIVATE m)
endif()

message(STATUS "")

message(STATUS "Wolfen test suite configuation:")
message(STATUS "Enabled: ${WOLFEN_TESTS} (Wayland client libraries found: ${WOLFEN_TEST_PC_DEPS_FOUND})")
message(STATUS "")
