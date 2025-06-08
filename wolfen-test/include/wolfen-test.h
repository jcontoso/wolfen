#include <stdbool.h>
#include <wayland-client.h>

#ifndef WOLFEN_TEST
#define WOLFEN_TEST

typedef struct {
	int id;
	char *name;
	void (*run)(struct wl_display *display);
	void (*cleanup)(void);
} WolfenTest;

typedef struct {
	struct wl_display *display;
	void (*test_cleanup)(void);
} WolfenTestState;

int wolfen_test_ask_for_int(char *message);
bool wolfen_test_ask_for_bool(char *message);

#endif
