#include <stdint.h>
#include <stdbool.h>
#include <wayland-server.h>
#include "wolfen-misc.h"

#ifndef WOLFEN_COMPOSITOR
#define WOLFEN_COMPOSITOR

void wlonx_compositor_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id);
void wlonx_compositor_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id);
void wlonx_compositor_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id);
void wolfen_region_destroy(struct wl_client *client, struct wl_resource *res);
void wolfen_region_add(struct wl_client *client, struct wl_resource *res, int32_t x, int32_t y, int32_t width, int32_t height);
void wolfen_region_subtract(struct wl_client *client, struct wl_resource *res, int32_t x, int32_t y, int32_t width, int32_t height);

#endif
