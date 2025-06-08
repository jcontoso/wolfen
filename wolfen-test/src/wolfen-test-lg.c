#include <stdint.h>
#include <stdio.h>
#include <wayland-client.h>

void wolfen_test_lg_handle(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
	printf("%s (version: %d, id: %d)\n" , interface, version, id);
}

void wolfen_test_lg_handle_remove(void *data, struct wl_registry *registry, uint32_t id) {
	printf("The global with the id %d has been removed.", id);
}

void wolfen_test_lg_run(struct wl_display *display) {
	struct wl_registry *registry;
	struct wl_registry_listener listener;

	listener.global = wolfen_test_lg_handle;
	listener.global_remove = wolfen_test_lg_handle_remove;
	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &listener, NULL);
	wl_display_roundtrip(display);
}
