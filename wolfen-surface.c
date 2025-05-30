#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <wayland-server.h>
#include "wolfen-misc.h"
#include "wolfen-display.h"
#include "wolfen-pixfmt.h"
#include "wolfen-surface.h"

void wolfen_surface_delete(struct wl_resource *resource) {

}

void wolfen_surface_destroy(struct wl_client *client, struct wl_resource *res) {
	
}

void wolfen_surface_attach(struct wl_client *client, struct wl_resource *res, struct wl_resource *buffer, int32_t x, int32_t y) {
	WolfenSurface *surface;
	
	surface = (WolfenSurface *)wl_resource_get_user_data(res);
	surface->state_buffer.prop_changed |= WOLFEN_SURFACE_PROP_CHANGED_BUFFER | WOLFEN_SURFACE_PROP_CHANGED_BUFFER_X_Y;
	surface->state_buffer.buffer = buffer;	
	surface->state_buffer.buffer_x = x;
	surface->state_buffer.buffer_y = y;
}

void wolfen_surface_damage(struct wl_client *client, struct wl_resource *res, int32_t x, int32_t y, int32_t width, int32_t height) {
	WolfenSurface *surface;
	
	surface = (WolfenSurface *)wl_resource_get_user_data(res);
	surface->state_buffer.prop_changed |= WOLFEN_SURFACE_PROP_CHANGED_DAMAGE;
	surface->state_buffer.damage.x = x;		
	surface->state_buffer.damage.y = y;		
	surface->state_buffer.damage.width = width;		
	surface->state_buffer.damage.height = height;		
}


void wolfen_surface_frame(struct wl_client *client, struct wl_resource *res, uint32_t callback) {
	WolfenSurface *surface;
	
	surface = (WolfenSurface *)wl_resource_get_user_data(res);
	surface->state_buffer.prop_changed |= WOLFEN_SURFACE_PROP_CHANGED_FRAME_CB;
	surface->state_buffer.frame_cb = wl_resource_create(client, &wl_callback_interface, 1, callback);
}

void wolfen_surface_set_opaque_region(struct wl_client *client, struct wl_resource *res, struct wl_resource *region) {
	WolfenSurface *surface;
	
	surface = (WolfenSurface *)wl_resource_get_user_data(res);
	surface->state_buffer.opaque_region = region;			
}

void wolfen_surface_set_input_region(struct wl_client *client, struct wl_resource *res, struct wl_resource *region) {
	WolfenSurface *surface;
	
	surface = (WolfenSurface *)wl_resource_get_user_data(res);
	surface->state_buffer.input_region = region;		
}

void wolfen_surface_set_buffer_transform(struct wl_client *client, struct wl_resource *res, int32_t transform) {
	WolfenSurface *surface;
	
	surface = (WolfenSurface *)wl_resource_get_user_data(res);
	surface->state_buffer.prop_changed |= WOLFEN_SURFACE_PROP_CHANGED_BUFFER_TF;
	surface->state_buffer.buffer_tf = transform;	
}

void wolfen_surface_set_buffer_scale(struct wl_client *client, struct wl_resource *res, int32_t scale) {
	WolfenSurface *surface;
	
	surface = (WolfenSurface *)wl_resource_get_user_data(res);
	surface->state_buffer.prop_changed |= WOLFEN_SURFACE_PROP_CHANGED_BUFFER_SCALE;
	surface->state_buffer.buffer_scale = scale;
}

void wolfen_surface_commit(struct wl_client *client, struct wl_resource *res) {
	WolfenSurface *surface;

	surface = (WolfenSurface *)wl_resource_get_user_data(res);
	surface->state = surface->state_buffer;
	surface->state_buffer.prop_changed = 0;

	if (surface->state.prop_changed & WOLFEN_SURFACE_PROP_CHANGED_DAMAGE_BUFFER) {
		if (surface->state.buffer_tf == WL_OUTPUT_TRANSFORM_270 || surface->state.buffer_tf == WL_OUTPUT_TRANSFORM_90 || surface->state.buffer_tf == WL_OUTPUT_TRANSFORM_FLIPPED_90 || surface->state.buffer_tf == WL_OUTPUT_TRANSFORM_FLIPPED_270) {
			surface->state.damage_buffer.y = surface->state.damage_buffer.x * surface->state.buffer_scale;		
			surface->state.damage_buffer.x = surface->state.damage_buffer.y * surface->state.buffer_scale;		
			surface->state.damage_buffer.height = surface->state.damage_buffer.width * surface->state.buffer_scale;		
			surface->state.damage_buffer.width = surface->state.damage_buffer.height * surface->state.buffer_scale;			
		} else {
			surface->state.damage_buffer.x = surface->state.damage_buffer.x * surface->state.buffer_scale;		
			surface->state.damage_buffer.y = surface->state.damage_buffer.y * surface->state.buffer_scale;		
			surface->state.damage_buffer.width = surface->state.damage_buffer.width * surface->state.buffer_scale;		
			surface->state.damage_buffer.height = surface->state.damage_buffer.height * surface->state.buffer_scale;					
		}
	}
	pixman_region32_init_rect(&surface->actual_damage, surface->state_buffer.damage.x, surface->state_buffer.damage.y, surface->state_buffer.damage.width, surface->state_buffer.damage.height);
	pixman_region32_union_rect(&surface->actual_damage, &surface->actual_damage, surface->state_buffer.damage_buffer.x, surface->state_buffer.damage_buffer.y, surface->state_buffer.damage_buffer.width, surface->state_buffer.damage_buffer.height);
	if (surface->state.buffer_tf == WL_OUTPUT_TRANSFORM_270 || surface->state.buffer_tf == WL_OUTPUT_TRANSFORM_90 || surface->state.buffer_tf == WL_OUTPUT_TRANSFORM_FLIPPED_90 || surface->state.buffer_tf == WL_OUTPUT_TRANSFORM_FLIPPED_270) {
		pixman_region32_translate(&surface->actual_damage, surface->state.buffer_x * surface->state.buffer_scale, surface->state.buffer_y * surface->state.buffer_scale);
	} else {
		pixman_region32_translate(&surface->actual_damage, surface->state.buffer_y * surface->state.buffer_scale, surface->state.buffer_x * surface->state.buffer_scale);
	}

	if (surface->state.prop_changed & WOLFEN_SURFACE_PROP_CHANGED_BUFFER) {
		struct wl_shm_buffer *shm_buffer;
		void *pxdata;

		shm_buffer = wl_shm_buffer_get(surface->state.buffer);
		wl_shm_buffer_begin_access(shm_buffer);
		surface->width = wl_shm_buffer_get_width(shm_buffer);
		surface->height = wl_shm_buffer_get_height(shm_buffer);
	
		/* UPDATE DATA INSTEAD OF DESTROYING IMAGE? */
		if (surface->x_img) {
			XDestroyImage(surface->x_img);
		}
		if (surface->conversion_buffer) {
			free(surface->conversion_buffer);
		}
	
		if (surface->use_last_x_img_xvi) {
			if (!surface->fmt || (surface->fmt->wl_fmt != surface->last_wl_fmt)) {
				wolfen_fmt_free(surface->fmt);
				surface->fmt = wolfen_fmt_from_xvi_for_wl_fmt(surface->display, &surface->last_x_img_xvi, surface->last_x_img_xvi_mask, wl_shm_buffer_get_format(shm_buffer));
			}
		} else {
			if (surface->fmt) {
				if ((surface->fmt->wl_fmt != surface->last_wl_fmt) || !surface->last_wl_fmt_set) {
					wolfen_fmt_free(surface->fmt);
					goto WOLFEN_SURFACE_CREATE_FMT;
				}
			} else {
				WOLFEN_SURFACE_CREATE_FMT:
				int screen;
	
				if (surface->display->x_screen_default->type == WOLFEN_SCREEN_TYPE_CORE) {
					screen = surface->display->x_screen_default->screen_number;
				} else {
					screen = DefaultScreen(surface->display->x_display);
				}			
				
				surface->fmt = wolfen_fmt_from_wl_fmt(surface->display, screen, wl_shm_buffer_get_format(shm_buffer));
			}
		}
		
		if(surface->fmt->fish) {
			int pxcount;
			
			pxcount = surface->width * surface->height;
			surface->conversion_buffer = malloc(pxcount * babl_format_get_bytes_per_pixel(surface->fmt->xvi_babl));
			babl_process(surface->fmt->fish, wl_shm_buffer_get_data(shm_buffer), surface->conversion_buffer, pxcount);
			pxdata = surface->conversion_buffer;
		} else {
			pxdata = wl_shm_buffer_get_data(shm_buffer);
		}
		surface->x_img = XCreateImage(surface->display->x_display, surface->fmt->xvi.visual, surface->fmt->xvi.depth, ZPixmap, 0, pxdata,surface->width, surface->height, 32, wl_shm_buffer_get_stride(shm_buffer));
		surface->last_x_img_xvi = surface->fmt->xvi;
		surface->last_x_img_xvi_mask = surface->fmt->xvi_mask;
		surface->width += surface->state.buffer_x;
		surface->height += surface->state.buffer_y;
		wl_shm_buffer_end_access(shm_buffer);
	}

	if (surface->commit_cb) {
		surface->commit_cb(surface, surface->commit_cb_data);
	}
	
	if (surface->state.prop_changed & WOLFEN_SURFACE_PROP_CHANGED_BUFFER) {
		if (!surface->last_wl_fmt_set) {
			surface->last_wl_fmt_set = true;
		}
		surface->last_wl_fmt = surface->fmt->wl_fmt;
	}
}

void wolfen_surface_damage_buffer(struct wl_client *client, struct wl_resource *res, int32_t x, int32_t y, int32_t width,int32_t height)  {
	WolfenSurface *surface;
	
	surface = (WolfenSurface *)wl_resource_get_user_data(res);
	surface->state_buffer.prop_changed |= WOLFEN_SURFACE_PROP_CHANGED_DAMAGE;
	surface->state_buffer.damage_buffer.x = x;		
	surface->state_buffer.damage_buffer.y = y;		
	surface->state_buffer.damage_buffer.width = width;		
	surface->state_buffer.damage_buffer.height = height;			
}

void wolfen_surface_offset(struct wl_client *client, struct wl_resource *res, int32_t x, int32_t y) {
	WolfenSurface *surface;
	
	surface = (WolfenSurface *)wl_resource_get_user_data(res);
	surface->state_buffer.prop_changed |= WOLFEN_SURFACE_PROP_CHANGED_BUFFER_X_Y;
	surface->state_buffer.buffer_x = x;
	surface->state_buffer.buffer_y = y;
}
