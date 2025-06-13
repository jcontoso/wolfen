cmake_minimum_required(VERSION 3.0)

# USE NATIVE CMAKE PACKAGES FOR THESE INSTEAD WHENEVER POSSIBLE
pkg_check_modules(WOLFEN_SERVER_PC_DEPS REQUIRED IMPORTED_TARGET "pixman-1 wayland-server x11 xext xrender xinerama xxf86vm xrandr babl")

file(GLOB wolfen_server_src "src/*.c")
add_executable(wolfen_server ${wolfen_server_src})
add_executable(wolfen_server::wolfen_server ALIAS wolfen_server)
set_target_properties(wolfen_server PROPERTIES OUTPUT_NAME "wolfen")
target_include_directories(wolfen_server PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_link_libraries(wolfen_server PRIVATE PkgConfig::WOLFEN_SERVER_PC_DEPS)
target_link_libraries(wolfen_server PRIVATE m)
