#include <stdint.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <wayland-server.h>

#ifndef WOLFEN_DISPLAY
#define WOLFEN_DISPLAY

#define WOLFEN_SCREEN_VENDOR "WLonX/Wolfen"
#define WOLFEN_SCREEN_MODEL_CORE "Core/Zaphod Screen %d"
#define WOLFEN_SCREEN_MODEL_CORE_DEFAULT "Core/Zaphod Screen %d (Default)"
#define WOLFEN_SCREEN_MODEL_XINERAMA "Xinerama Screen %d"
#define WOLFEN_SCREEN_MODEL_UNKNOWN "Screen"
	
typedef enum {
	WOLFEN_SCREEN_TYPE_CORE,
	WOLFEN_SCREEN_TYPE_XINERAMA,
	WOLFEN_SCREEN_TYPE_XRANDR
} WolfenScreenType;

typedef struct {
	struct _WolfenDisplay *display;
	
	WolfenScreenType type;
	
	char *vendor;
	char *model;
	/* DO NOT EXECUTE IF NULL */
	void (*vendor_free_func)(void *p);
	void (*model_free_func)(void *p);
	
	int screen_number; /* multi monitor mode dependent */
	int x_org;
	int y_org;
	int width;
	int height;
	
	struct wl_list link;
} WolfenScreen;

typedef struct _WolfenDisplay {
	/* X11 DISPLAY */
	Display *x_display;
    bool x_has_xinerama;
	bool x_has_vidmode;
	
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
} WolfenDisplay;

#endif
