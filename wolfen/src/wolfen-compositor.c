#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wayland-server.h>
#include <pixman.h>
#include "wolfen-misc.h"
#include "wolfen-display.h"
#include "wolfen-pixfmt.h"
#include "wolfen-surface.h"
#include "wolfen-compositor.h"

void wlonx_compositor_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	WolfenSurface *surface;
	
	/* alloc and set display */
	surface = malloc(sizeof(WolfenSurface));
	surface->display = wl_resource_get_user_data(resource);
	
	/* state setup */
	memset(&surface->state, 0, sizeof(WolfenSurfaceState));
	memset(&surface->state.damage, 0, sizeof(pixman_rectangle32_t));
	memset(&surface->state.damage_buffer, 0, sizeof(pixman_rectangle32_t));
	surface->state.buffer_scale = 1;
	surface->state.buffer_tf = WL_OUTPUT_TRANSFORM_NORMAL;	
	surface->state_buffer = surface->state;
	
	/* init viewport, etc */
	pixman_region32_init(&surface->viewport);
	pixman_region32_init(&surface->actual_damage);
	
	/* setup callbacks */
	surface->commit_cb = NULL;

	/* setup img contents */
	surface->contents.img.x_img = NULL;
	surface->contents.img.conversion_buffer = NULL;
	surface->contents.img.use_last_x_img_xvi = false;
	surface->contents.img.last_wl_fmt_set = false;
	surface->contents.img.fmt = NULL;
	surface->contents.img.p_img = NULL;
		
	/* create wl resource and insert */
	surface->wl_rc_surface = wl_resource_create(client, &wl_surface_interface, wl_surface_interface.version, id);
	wl_resource_set_implementation(surface->wl_rc_surface, &surface->display->surface_imp, surface, &wolfen_surface_delete);
	wl_list_insert(&surface->display->surfaces_list, &surface->link);
}

void wlonx_region_delete(struct wl_resource *res) {
	pixman_region32_t *region;
	
	region = (pixman_region32_t *)wl_resource_get_user_data(res);	

	pixman_region32_fini(region);
	
	free(region);
}

void wolfen_region_destroy(struct wl_client *client, struct wl_resource *res) {
	wlonx_region_delete(res);
	wl_resource_destroy(res);
}

void wolfen_region_add(struct wl_client *client, struct wl_resource *res, int32_t x, int32_t y, int32_t width, int32_t height) {
	pixman_region32_t *region;
	
	region = (pixman_region32_t *)wl_resource_get_user_data(res);
	pixman_region32_union_rect(region, region, x, y, width, height);	
}

void wolfen_region_subtract(struct wl_client *client, struct wl_resource *res, int32_t x, int32_t y, int32_t width, int32_t height) {
	pixman_region32_t *region;
	pixman_region32_t rect;
	
	region = (pixman_region32_t *)wl_resource_get_user_data(res);
	pixman_region32_union_rect(region, region, x, y, width, height);
	pixman_region32_init_rect(&rect, x, y, width, height);
	pixman_region32_subtract(region, region, &rect);
	pixman_region32_fini(&rect);
}

void wlonx_compositor_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
	WolfenDisplay *display;
	pixman_region32_t *region;
	struct wl_resource *region_rc;
	
	display = wl_resource_get_user_data(resource);
	region = malloc(sizeof(pixman_region32_t));
	pixman_region32_init(region);
	region_rc = wl_resource_create(client, &wl_region_interface, wl_region_interface.version, id);
	wl_resource_set_implementation(region_rc, &display->region_imp, region, &wlonx_region_delete);
}

void wlonx_compositor_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
	WolfenDisplay *display;
	struct wl_resource *rc;
	
	display = (WolfenDisplay*)data;
	rc = wl_resource_create(client, &wl_compositor_interface, wl_compositor_interface.version, id);
	wl_resource_set_implementation(rc, &display->comp_imp, data, NULL);
}

