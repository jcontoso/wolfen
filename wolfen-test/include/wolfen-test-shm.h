#include <wayland-client.h>

#ifndef WOLFEN_TEST_SHM
#define WOLFEN_TEST_SHM

void wolfen_test_shm_run(struct wl_display *display);
void wolfen_test_shm_free();

#endif
