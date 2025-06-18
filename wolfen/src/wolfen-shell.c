#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef WOLFEN_HAS_XEXT
#include <X11/extensions/XShm.h>
#include <X11/extensions/shape.h>
#endif
#include <wayland-server.h>
#include "wolfen-misc.h"
#include "wolfen-display.h"
#include "wolfen-surface.h"
#include "wolfen-shell.h"
#include "motif-util.h"

void wolfen_wlshell_surface_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial) {

	
}
void wolfen_wlshell_surface_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial) {
	
}

void wolfen_wlshell_surface_resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, uint32_t edges) {
	
}	 
      
void wolfen_wlshell_surface_set_toplevel(struct wl_client *client, struct wl_resource *resource) {
	WolfenShellSurface *shsurface;
	
	shsurface = (WolfenShellSurface *)wl_resource_get_user_data(resource);	
	if (shsurface->type) {
		shsurface->type = WOLFEN_SHELL_SURFACE_TYPE_TOPLEVEL;
	}
}

void wolfen_wlshell_surface_set_transient(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags) {
	
}

void wolfen_wlshell_surface_set_fullscreen(struct wl_client *client, struct wl_resource *resource, uint32_t method, uint32_t framerate, struct wl_resource *output) {
	
}

void wolfen_wlshell_surface_set_popup(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags) {
	
}

void wolfen_wlshell_surface_set_maximized(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output) {
	
}

void wolfen_wlshell_surface_set_title(struct wl_client *client, struct wl_resource *resource, const char *title) {
	WolfenShellSurface *shsurface;
	
	shsurface = (WolfenShellSurface *)wl_resource_get_user_data(resource);	
	if(!shsurface->type) {
		return;
	}	
	
	XStoreName(shsurface->display->x_display, shsurface->x_window, title);
    XSetIconName(shsurface->display->x_display, shsurface->x_window, title);		
    XChangeProperty(shsurface->display->x_display, shsurface->x_window, XInternAtom(shsurface->display->x_display, "_NET_WM_NAME", False), XInternAtom(shsurface->display->x_display, "UTF8_STRING", False), 8, PropModeReplace, (unsigned char *)title, strlen(title)); 
}

void wolfen_wlshell_surface_set_class(struct wl_client *client, struct wl_resource *resource, const char *klass) {

}

void *thread_func(void *ptr) {
	WolfenShellSurface *shsurface;
	
	shsurface = (WolfenShellSurface *)ptr;
	for (;;) {
		wl_shm_buffer_begin_access(shsurface->surface->contents.img.shm_buffer);
		#ifdef WOLFEN_HAS_XEXT
		if (shsurface->display->x_has_shm) {
			XShmPutImage(shsurface->display->x_display, shsurface->x_window, shsurface->x_gc, shsurface->surface->contents.img.x_img, 0, 0, shsurface->extents->x1, shsurface->extents->y1, shsurface->extents->x2 - shsurface->extents->x1, shsurface->extents->y2 - shsurface->extents->y1, True);
		} else 
		#endif
		{
			XPutImage(shsurface->display->x_display, shsurface->x_window, shsurface->x_gc, shsurface->surface->contents.img.x_img, 0, 0, shsurface->extents->x1, shsurface->extents->y1, shsurface->extents->x2 - shsurface->extents->x1, shsurface->extents->y2 - shsurface->extents->y1);
		}
		XFlush(shsurface->display->x_display);
		wl_shm_buffer_end_access(shsurface->surface->contents.img.shm_buffer);
	}
	return ptr;
}

void wolfen_wlshell_surface_create_x11(WolfenShellSurface *shsurface) {
	if (shsurface->surface && !shsurface->type) {
		XSetWindowAttributes swa;
		XSizeHints sh;
		XColor color;
		Colormap map;
		Atom property;
		MotifWmHints hints;
		pthread_t thread;
		
		shsurface->fmt_used = shsurface->surface->contents.img.fmt;
		shsurface->extents = pixman_region32_extents(&shsurface->surface->viewport);
		
		map = XCreateColormap(shsurface->display->x_display, RootWindow(shsurface->display->x_display,  wolfen_screen_get_core_screen(shsurface->display->x_screen_default)), shsurface->fmt_used->xvi.visual, AllocNone);
		color.flags = DoRed|DoGreen|DoBlue;
		color.red = 0;
		color.green = 0;
		color.blue = 0;
		XAllocColor(shsurface->display->x_display, map, &color);
			
		sh.flags = PMinSize	| PMaxSize;
		sh.min_width = sh.max_width = shsurface->extents->x2 - shsurface->extents->x1;
		sh.min_height = sh.max_height = shsurface->extents->y2 - shsurface->extents->y1;
		swa.event_mask = ExposureMask;
		swa.colormap = map;
		swa.border_pixel = color.pixel;
		hints.flags = MWM_HINTS_DECORATIONS;
		hints.decorations = 0;
		property = XInternAtom(shsurface->display->x_display, "_MOTIF_WM_HINTS", True);
		shsurface->x_window = XCreateWindow(shsurface->display->x_display, RootWindow(shsurface->display->x_display,  wolfen_screen_get_core_screen(shsurface->display->x_screen_default)), 0, 0, sh.min_width, sh.min_height, 0, shsurface->fmt_used->xvi.depth, InputOutput, shsurface->fmt_used->xvi.visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
		shsurface->x_gc = XCreateGC(shsurface->display->x_display, shsurface->x_window, 0, NULL);
		XChangeProperty(shsurface->display->x_display, shsurface->x_window,property,property,32,PropModeReplace,(unsigned char *)&hints,5);
		XMapWindow(shsurface->display->x_display, shsurface->x_window);
		XSetWMNormalHints(shsurface->display->x_display, shsurface->x_window, &sh);
		shsurface->type = WOLFEN_SHELL_SURFACE_TYPE_JUST_CREATED;
		shsurface->surface->contents.img.use_last_x_img_xvi = true;
		#ifdef WOLFEN_HAS_XEXT
		if (shsurface->display->x_has_shm && !shsurface->display->x_screen_default->is_compositing) {
			WolfenSurfaceMask *mask;
			
			puts("hi");
			mask = wolfen_surface_mask_generate(shsurface->surface, shsurface->x_window);
			XShapeCombineMask(shsurface->display->x_display, shsurface->x_window, ShapeBounding, 0, 0, mask->mask, ShapeSet);
		} 
		#endif
		/* HORRIBLE HACK USED FOR DEMO PURPOSES, WILL NOT BE USED WHEN I GET IMPLEMENT EVENTS */
		pthread_create(&thread, NULL, thread_func, shsurface);
	}
}

void wolfen_wlshell_surface_on_commit(WolfenSurface *surface, void *udata) {
	WolfenShellSurface *shsurface;
	
	shsurface = (WolfenShellSurface *)udata;
	wolfen_wlshell_surface_create_x11(shsurface);
}

void wolfen_wlshell_get_shell_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface) {
	WolfenShellSurface *shsurface;

	shsurface = malloc(sizeof(WolfenShellSurface));
	shsurface->display = wl_resource_get_user_data(resource);
	shsurface->surface_rc = surface;
	shsurface->surface = wl_resource_get_user_data(shsurface->surface_rc);
	shsurface->surface->commit_cb = wolfen_wlshell_surface_on_commit;
	shsurface->surface->commit_cb_data = shsurface;
	shsurface->type = WOLFEN_SHELL_SURFACE_TYPE_NONE;

	shsurface->rc = wl_resource_create(client, &wl_shell_surface_interface, wl_shell_surface_interface.version, id);
	wl_resource_set_implementation(shsurface->rc, &shsurface->display->wlshell_surface_imp, shsurface, NULL);
	wl_list_insert(&shsurface->display->shell_surfaces_list, &shsurface->link);
}

void wolfen_wlshell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
	WolfenDisplay *display;
	struct wl_resource *rc;
	
	display = (WolfenDisplay*)data;
	rc = wl_resource_create(client, &wl_shell_interface, wl_shell_interface.version, id);
	wl_resource_set_implementation(rc, &display->wlshell_imp, data, NULL);
}
