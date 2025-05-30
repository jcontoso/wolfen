clear && cc *.c -std=c99 -Wno-deprecated-declarations -Iinclude -g -pthread -fms-extensions `pkg-config --cflags --libs pixman-1 wayland-server x11 xinerama xxf86vm babl` -lm && ./a.out
