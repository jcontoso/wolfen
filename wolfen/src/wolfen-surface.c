#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef WOLFEN_HAS_XEXT
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef WOLFEN_HAS_XEXT
#include <X11/extensions/XShm.h>
#endif
#include <wayland-server.h>
#include <pixman.h>
#include "wolfen-misc.h"
#include "wolfen-display.h"
#include "wolfen-pixfmt.h"
#include "wolfen-surface.h"

bool wolfen_surface_needs_transform(WolfenSurface *surface) {
	if (surface->state.buffer_scale > 1 || surface->state.buffer_tf != WL_OUTPUT_TRANSFORM_NORMAL) {
		return true;
	} else {
		return false;
	}	
}

void wolfen_surface_delete(struct wl_resource *res) {
	WolfenSurface *surface;
	
	surface = (WolfenSurface *)wl_resource_get_user_data(res);	

	pixman_region32_fini(&surface->viewport);
	pixman_region32_fini(&surface->actual_damage);

	if (surface->contents.img.conversion_buffer) {
		free(surface->contents.img.conversion_buffer);
	}

	if (surface->contents.img.x_img) {
		#ifdef WOLFEN_HAS_XEXT
		if (surface->display->x_has_shm) {
			XShmDetach(surface->display->x_display, &surface->contents.img.shm_info);
			XDestroyImage(surface->contents.img.x_img);
			shmdt(surface->contents.img.shm_info.shmaddr);
			shmctl(surface->contents.img.shm_info.shmid, IPC_RMID, 0);
		} else 
		#endif
		{
			surface->contents.img.x_img->data = NULL;
			XDestroyImage(surface->contents.img.x_img);
		}
	}

	if (surface->contents.img.p_img) {
		pixman_image_unref(surface->contents.img.p_img);
	}
	
	if (surface->contents.img.fmt) {
		wolfen_fmt_free(surface->contents.img.fmt);
	}			
			
	wl_list_remove(&surface->link);
	
	free(surface);
}

void wolfen_surface_destroy(struct wl_client *client, struct wl_resource *res) {
	wolfen_surface_delete(res);
	wl_resource_destroy(res);
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

int fake_destroy() {
	return 0;
}

#ifdef WOLFEN_HAS_XEXT
WolfenSurfaceMask *wolfen_surface_mask_generate(WolfenSurface *surface, Drawable drawable) {
	WolfenSurfaceMask *ret;
	int x;
	int y;	
	int c;	
	int s;
	
	c = 0;
	s = 0;
	ret = malloc(sizeof(WolfenSurfaceMask));
	ret->for_surface = surface;
	ret->data = calloc(surface->contents.img.x_img->width * surface->contents.img.x_img->height / 8, sizeof(char));

	wl_shm_buffer_begin_access(surface->contents.img.shm_buffer);
	for (y = 0; y < surface->contents.img.x_img->height; y++) {		
		for (x = 0; x < surface->contents.img.x_img->width; x++) {
			int pixel;	

			if (c >= 8) {
				c = 0;
				s++;
			}	
		
			pixel = ((((uint32_t *)surface->contents.img.x_img->data)[x * surface->contents.img.x_img->width + y] >> 24) & 0xFF);
			if (pixel > 0) {
				ret->data[s] |= 1 << c;
			}
			
			c++;
		}	
	}
	  
	ret->mask = XCreateBitmapFromData(surface->display->x_display, drawable, ret->data, surface->contents.img.x_img->width, surface->contents.img.x_img->height);
	wl_shm_buffer_end_access(surface->contents.img.shm_buffer);
	return ret;
}

void wolfen_surface_mask_free(WolfenSurfaceMask *mask) {
	XFreePixmap(mask->for_surface->display->x_display, mask->mask);
	free(mask->data);
	free(mask);
}
#endif

void wolfen_surface_commit(struct wl_client *client, struct wl_resource *res) {
	WolfenSurface *surface;
	int xs /*scaled buffer x offset */;
	int ys /*scaled buffer y offset */;
	
	surface = (WolfenSurface *)wl_resource_get_user_data(res);

	/* copy over state from buffer */
	surface->state = surface->state_buffer;
	surface->state_buffer.prop_changed = WOLFEN_SURFACE_PROP_CHANGED_NONE;	
	
	/* damage */
	pixman_region32_clear(&surface->actual_damage);   
	xs = surface->state.buffer_x * surface->state.buffer_scale;
	ys = surface->state.buffer_y * surface->state.buffer_scale;
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
		pixman_region32_translate(&surface->actual_damage, xs, ys);
	} else {
		pixman_region32_translate(&surface->actual_damage, ys, xs);
	}
	
	/* buffer */
	if (surface->state.prop_changed & WOLFEN_SURFACE_PROP_CHANGED_BUFFER) {
		void *data_src;
		void *data_for_x;
		int w;
		int h;
		int strid;
		
		surface->contents.img.shm_buffer = wl_shm_buffer_get(surface->state.buffer);
		wl_shm_buffer_begin_access(surface->contents.img.shm_buffer);
		w = wl_shm_buffer_get_width(surface->contents.img.shm_buffer);
		h = wl_shm_buffer_get_height(surface->contents.img.shm_buffer);
		strid = wl_shm_buffer_get_stride(surface->contents.img.shm_buffer);
		
		/* UPDATE DATA INSTEAD OF DESTROYING IMAGE? */
		if (surface->contents.img.x_img) {
			XDestroyImage(surface->contents.img.x_img);
		}
		if (surface->contents.img.conversion_buffer) {
			free(surface->contents.img.conversion_buffer);
		}
	
		/* FORMAT */
		if (surface->contents.img.use_last_x_img_xvi) {
			if (!surface->contents.img.fmt || (surface->contents.img.fmt->wl_fmt != surface->contents.img.last_wl_fmt)) {
				wolfen_fmt_free(surface->contents.img.fmt);
				surface->contents.img.fmt = wolfen_fmt_from_xvi_for_wl_fmt(surface->display, &surface->contents.img.last_x_img_xvi, surface->contents.img.last_x_img_xvi_mask, wl_shm_buffer_get_format(surface->contents.img.shm_buffer));
			}
		} else {
			if (surface->contents.img.fmt) {
				if ((surface->contents.img.fmt->wl_fmt != surface->contents.img.last_wl_fmt) || !surface->contents.img.last_wl_fmt_set) {
					wolfen_fmt_free(surface->contents.img.fmt);
					goto WOLFEN_SURFACE_CREATE_FMT;
				}
			} else {
				WOLFEN_SURFACE_CREATE_FMT:				
				surface->contents.img.fmt = wolfen_fmt_from_wl_fmt(surface->display, wolfen_screen_get_core_screen(surface->display->x_screen_default), wl_shm_buffer_get_format(surface->contents.img.shm_buffer));
			}
		}
					
		/* VIEWPORT */
		pixman_region32_clear(&surface->viewport);   
		pixman_region32_init_rect(&surface->viewport, xs, ys, w * surface->state.buffer_scale, h * surface->state.buffer_scale);
		
		/* BUFFER */
		if (!surface->display->x_has_render) {
			if (wolfen_surface_needs_transform(surface)) {
				pixman_transform_t pixman_tf;
				
				pixman_transform_init_scale(&pixman_tf, surface->state.buffer_scale, surface->state.buffer_scale);       
				surface->contents.img.p_img = pixman_image_create_bits_no_clear(surface->contents.img.fmt->pixman_fmt, w, h, wl_shm_buffer_get_data(surface->contents.img.shm_buffer), strid);
				pixman_image_set_transform(surface->contents.img.p_img, &pixman_tf);
				data_src = pixman_image_get_data(surface->contents.img.p_img); 
				w = pixman_image_get_width(surface->contents.img.p_img);                     
				h = pixman_image_get_height(surface->contents.img.p_img);                     
			} else {
				data_src = wl_shm_buffer_get_data(surface->contents.img.shm_buffer);
			}
			
			if(surface->contents.img.fmt->fish) {
				int pxcount;
			
				pxcount = w * h;
				surface->contents.img.conversion_buffer = malloc(pxcount * babl_format_get_bytes_per_pixel(surface->contents.img.fmt->xvi_babl));
				babl_process(surface->contents.img.fmt->fish, data_src, surface->contents.img.conversion_buffer, pxcount);
				data_for_x = surface->contents.img.conversion_buffer;
			} else {
				data_for_x = data_src;
			}
		
			#ifdef WOLFEN_HAS_XEXT
			if (surface->display->x_has_shm) {
				surface->contents.img.x_img = XShmCreateImage(surface->display->x_display, surface->contents.img.fmt->xvi.visual, surface->contents.img.fmt->xvi.depth, ZPixmap, NULL, &surface->contents.img.shm_info, w, h);
				surface->contents.img.shm_info.shmid = shmget(IPC_PRIVATE, strid * h, 0777);
				surface->contents.img.shm_info.shmaddr = surface->contents.img.x_img->data = (char*)shmat(surface->contents.img.shm_info.shmid, NULL, 0);
				memcpy(surface->contents.img.shm_info.shmaddr, data_for_x, strid * h); /* hack, this defeats the whole fucking point */
				surface->contents.img.shm_info.readOnly = True;
				XShmAttach(surface->display->x_display, &surface->contents.img.shm_info);
				shmctl(surface->contents.img.shm_info.shmid, IPC_RMID, 0);
			} else 
			#endif
			{
				surface->contents.img.x_img = XCreateImage(surface->display->x_display, surface->contents.img.fmt->xvi.visual, surface->contents.img.fmt->xvi.depth, ZPixmap, 0, data_for_x, w, h, 32, strid);
			}
			surface->contents.img.last_x_img_xvi = surface->contents.img.fmt->xvi;
			surface->contents.img.last_x_img_xvi_mask = surface->contents.img.fmt->xvi_mask;
		}

		wl_shm_buffer_end_access(surface->contents.img.shm_buffer);
	}
	
	/* cb */
	if (surface->commit_cb) {
		surface->commit_cb(surface, surface->commit_cb_data);
	}
	
	/* buffer 2 */
	if (surface->state.prop_changed & WOLFEN_SURFACE_PROP_CHANGED_BUFFER) {
		if (!surface->contents.img.last_wl_fmt_set) {
			surface->contents.img.last_wl_fmt_set = true;
		}
		surface->contents.img.last_wl_fmt = surface->contents.img.fmt->wl_fmt;
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

