#include <stdint.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <wayland-server.h>
#include "wolfen-misc.h"
#include "wolfen-display.h"
#include "wolfen-pixfmt.h"
#include "wolfen-surface.h"

#ifndef WOLFEN_SHELL
#define WOLFEN_SHELL

typedef struct {
	WolfenDisplay *display;
	
	struct wl_resource *rc;
	
	struct wl_resource *surface_rc;
	WolfenSurface *surface;
	pixman_box32_t *extents;
	WolfenPixelFmt *fmt_used;
	Window x_window;
	GC x_gc;
	bool x_created;
	
	struct wl_list link;
} WolfenShellSurface;

void wolfen_wlshell_get_shell_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface);
void wolfen_wlshell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id);

void wolfen_wlshell_surface_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial);
void wolfen_wlshell_surface_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial);
void wolfen_wlshell_surface_resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, uint32_t edges);
void wolfen_wlshell_surface_set_toplevel(struct wl_client *client, struct wl_resource *resource);
void wolfen_wlshell_surface_set_transient(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags);
void wolfen_wlshell_surface_set_fullscreen(struct wl_client *client, struct wl_resource *resource, uint32_t method, uint32_t framerate, struct wl_resource *output);
void wolfen_wlshell_surface_set_popup(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags);
void wolfen_wlshell_surface_set_maximized(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output);
void wolfen_wlshell_surface_set_title(struct wl_client *client, struct wl_resource *resource, const char *title);
void wolfen_wlshell_surface_set_class(struct wl_client *client, struct wl_resource *resource, const char *klass);

#endif
