cmake_minimum_required(VERSION 3.0)

pkg_check_modules(WOLFEN_TEST_PC_DEPS REQUIRED IMPORTED_TARGET "wayland-client")

file(GLOB wolfen_test_src "src/*.c")
add_executable(wolfen_test ${wolfen_test_src})
add_executable(wolfen_test::wolfen_test ALIAS wolfen_test)
target_include_directories(wolfen_test PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_link_libraries(wolfen_test PRIVATE PkgConfig::WOLFEN_TEST_PC_DEPS)
target_link_libraries(wolfen_test PRIVATE m)
