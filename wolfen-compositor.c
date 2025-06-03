#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server.h>
#include <pixman.h>
#include "wolfen-misc.h"
#include "wolfen-display.h"
#include "wolfen-pixfmt.h"
#include "wolfen-surface.h"
#include "wolfen-compositor.h"

void wlonx_compositor_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	WolfenSurface *surface;
	
	surface = malloc(sizeof(WolfenSurface));
	surface->display = wl_resource_get_user_data(resource);
	surface->x_img = NULL;
	surface->commit_cb = NULL;
	surface->conversion_buffer = NULL;
	surface->use_last_x_img_xvi = false;
	surface->last_wl_fmt_set = false;
	surface->fmt = NULL;
	memset(&surface->state, 0, sizeof(WolfenSurfaceState));
	memset(&surface->state.damage, 0, sizeof(pixman_rectangle32_t));
	memset(&surface->state.damage_buffer, 0, sizeof(pixman_rectangle32_t));
	surface->state.buffer_scale = 1;
	surface->state.buffer_tf = WL_OUTPUT_TRANSFORM_NORMAL;
	pixman_region32_init(&surface->viewport);
	surface->state_buffer = surface->state;
	surface->wl_rc_surface = wl_resource_create(client, &wl_surface_interface, wl_surface_interface.version, id);
	wl_resource_set_implementation(surface->wl_rc_surface, &surface->display->surface_imp, surface, &wolfen_surface_delete);
	wl_list_insert(&surface->display->surfaces_list, &surface->link);
}

void wlonx_compositor_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id) {

}

void wlonx_compositor_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
	WolfenDisplay *display;
	struct wl_resource *rc;
	
	display = (WolfenDisplay*)data;
	rc = wl_resource_create(client, &wl_compositor_interface, wl_compositor_interface.version, id);
	wl_resource_set_implementation(rc, &display->comp_imp, data, NULL);
}

