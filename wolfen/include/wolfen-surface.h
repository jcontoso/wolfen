#include <stdint.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <wayland-server.h>
#include <pixman.h>
#include "wolfen-misc.h"
#include "wolfen-display.h"
#include "wolfen-pixfmt.h"

#ifndef WOLFEN_SURFACE
#define WOLFEN_SURFACE

typedef enum {
	WOLFEN_SURFACE_PROP_CHANGED_NONE = 0,
    WOLFEN_SURFACE_PROP_CHANGED_BUFFER_SCALE=  1 << 0,
    WOLFEN_SURFACE_PROP_CHANGED_BUFFER_TF = 1 << 1,
    WOLFEN_SURFACE_PROP_CHANGED_FRAME_CB = 1 << 2,
	WOLFEN_SURFACE_PROP_CHANGED_BUFFER = 1 << 3,   
	WOLFEN_SURFACE_PROP_CHANGED_BUFFER_X_Y = 1 << 4,   
	WOLFEN_SURFACE_PROP_CHANGED_DAMAGE = 1 << 5,   
	WOLFEN_SURFACE_PROP_CHANGED_DAMAGE_BUFFER = 1 << 6,   
} WolfenSurfacePropChanged;

typedef struct {
	WolfenSurfacePropChanged prop_changed;
	long buffer_scale;
	long buffer_tf;
	struct wl_resource *frame_cb;
	struct wl_resource *buffer;
	long buffer_x;
	long buffer_y;
	pixman_rectangle32_t damage;
	pixman_rectangle32_t damage_buffer;
	struct wl_resource *opaque_region;
	struct wl_resource *input_region;
} WolfenSurfaceState;

typedef struct _WolfenSurface {
	WolfenDisplay *display;
	struct wl_resource *wl_rc_surface;
	
	/* state information */
	WolfenSurfaceState state_buffer;
	WolfenSurfaceState state;
	
	/* viewport and damage areas */
	pixman_region32_t actual_damage;
	pixman_region32_t viewport;

	/* surface contents */
	union _WolfenSurfaceContents {
		struct _WolfenSurfaceImage {
			/* format information */
			WolfenPixelFmt *fmt;
			XVisualInfo last_x_img_xvi;
			long last_x_img_xvi_mask;
			uint32_t last_wl_fmt;
			bool last_wl_fmt_set;
			bool use_last_x_img_xvi;	
	
			/* buffer */	
			struct wl_shm_buffer *shm_buffer;
			XImage *x_img;
			pixman_image_t *p_img; /* USED ONLY FOR BUFFER TFS ON NON XRENDER SETUPS */
	
			/* for fmt conversions */
			void *conversion_buffer;
		} img;
	} contents;
	
	/* callbacks for other wolfen code */
	void *commit_cb_data;
	void (*commit_cb)(struct _WolfenSurface *s, void *d);

	/* wl_list link */
	struct wl_list link;
} WolfenSurface;

void wolfen_surface_delete(struct wl_resource *resource);
void wolfen_surface_destroy(struct wl_client *client, struct wl_resource *res);
void wolfen_surface_attach(struct wl_client *client, struct wl_resource *res, struct wl_resource *buffer, int32_t x, int32_t y);
void wolfen_surface_damage(struct wl_client *client, struct wl_resource *res, int32_t x, int32_t y, int32_t width, int32_t height);
void wolfen_surface_frame(struct wl_client *client, struct wl_resource *res, uint32_t callback);
void wolfen_surface_set_opaque_region(struct wl_client *client, struct wl_resource *res, struct wl_resource *region);
void wolfen_surface_set_input_region(struct wl_client *client, struct wl_resource *res, struct wl_resource *region);
void wolfen_surface_set_buffer_transform(struct wl_client *client, struct wl_resource *res, int32_t transform);
void wolfen_surface_set_buffer_scale(struct wl_client *client, struct wl_resource *res, int32_t scale);
void wolfen_surface_commit(struct wl_client *client, struct wl_resource *res);
void wolfen_surface_damage_buffer(struct wl_client *client, struct wl_resource *res, int32_t x, int32_t y, int32_t width,int32_t height);
void wolfen_surface_offset(struct wl_client *client, struct wl_resource *res, int32_t x, int32_t y);

#endif
