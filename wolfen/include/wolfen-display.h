#include <stdint.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <wayland-server.h>

#ifndef WOLFEN_DISPLAY
#define WOLFEN_DISPLAY

typedef enum {
	WOLFEN_SCREEN_TYPE_CORE,
	WOLFEN_SCREEN_TYPE_XINERAMA,
	WOLFEN_SCREEN_TYPE_XRANDR
} WolfenScreenType;

typedef struct {
	struct _WolfenDisplay *display;
	
	WolfenScreenType type;
	
	char *name;
	void (*name_free_func)(void *p); /* DO NOT EXECUTE IF NULL */
	char *make;
	void (*make_free_func)(void *p); /* DO NOT EXECUTE IF NULL */
	char *model;
	void (*model_free_func)(void *p); /* DO NOT EXECUTE IF NULL */
	
	int screen_number; /* multi monitor mode dependent */
	int x_org;
	int y_org;
	int width;
	int height;
	int widthmm;
	int heightmm;
	enum wl_output_transform tf;
	enum wl_output_subpixel sp;
		
	bool is_compositing;
	
	struct wl_list link;
} WolfenScreen;

int wolfen_screen_get_core_screen(WolfenScreen *screen);

typedef struct _WolfenDisplay {
	/* X11 DISPLAY */
	Display *x_display;
    bool x_has_xinerama;
	bool x_has_vidmode;
	bool x_has_render;
	bool x_has_shape;
	bool x_has_randr;
	bool x_has_shm;
	
	/* X11 OUTPUTS */
	struct wl_list x_screen_list;
	WolfenScreen *x_screen_default;
	int x_screen_count;
	
    /* WL */
	struct wl_display *wl_display;

	/* INTERNALS */
    struct wl_list surfaces_list;
	struct wl_list shell_surfaces_list;
  
    /* INTERFACE IMPLEMENTATIONS */
	struct wl_compositor_interface comp_imp;
	struct wl_surface_interface surface_imp;
	struct wl_shell_interface wlshell_imp;
	struct wl_shell_surface_interface wlshell_surface_imp;
	struct wl_region_interface region_imp;
} WolfenDisplay;

#endif
