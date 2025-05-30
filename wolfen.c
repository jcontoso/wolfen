#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/xf86vmode.h>
#include <wayland-server.h>
#include <babl/babl.h>

#include "wolfen-misc.h"
#include "wolfen-display.h"
#include "wolfen-pixfmt.h"
#include "wolfen-surface.h"
#include "wolfen-compositor.h"
#include "wolfen-shell.h"

void wolfen_display_create_screens_xinerama(WolfenDisplay *wlonx, XineramaScreenInfo *xin_info, int xin_count) {
	int i;
	WolfenScreen *first;
	
	first = NULL;
	wlonx->x_screen_count = xin_count;
	for (i = 0; i < wlonx->x_screen_count; i++) {
		WolfenScreen *screen;
		
		screen = malloc(sizeof(WolfenScreen));
		if (!first) {
			first = screen;
		}
		screen->type = WOLFEN_SCREEN_TYPE_XINERAMA;
		
		/* use vidmode to get information for the main screen? */
		screen->vendor = WOLFEN_SCREEN_VENDOR;
		screen->vendor_free_func = NULL;
		screen->model = calloc(strlen(WOLFEN_SCREEN_MODEL_XINERAMA)+wolfen_digit_count(xin_info[i].screen_number)+1, sizeof(char));
		if (screen->model) {
			sprintf(screen->model, WOLFEN_SCREEN_MODEL_XINERAMA, xin_info[i].screen_number);
			screen->model_free_func = free;
		} else {
			screen->model = WOLFEN_SCREEN_MODEL_UNKNOWN;
			screen->model_free_func = NULL;		
		}
		
		screen->screen_number = xin_info[i].screen_number;
		screen->x_org = xin_info[i].x_org;
		screen->y_org = xin_info[i].y_org;
		screen->width = xin_info[i].width;
		screen->height = xin_info[i].height;
		
		wl_list_insert(&wlonx->x_screen_list, &screen->link);
	}
	wlonx->x_screen_default = first;
}

void wolfen_display_create_screens_core(WolfenDisplay *wlonx) {
	int i;
	WolfenScreen *default_screen;

	wlonx->x_screen_count = ScreenCount(wlonx->x_display);	
	for (i = 0; i < wlonx->x_screen_count; i++) {
		WolfenScreen *screen;
		
		screen = malloc(sizeof(WolfenScreen));		
		screen->type = WOLFEN_SCREEN_TYPE_CORE;
		if (i == DefaultScreen(wlonx->x_display)) {
			default_screen = screen;
		}
		
		if (wlonx->x_has_vidmode) {
			XF86VidModeMonitor vm_mon;
			
			XF86VidModeGetMonitor(wlonx->x_display, i, &vm_mon);
			
			screen->vendor = vm_mon.vendor;
			screen->vendor_free_func = wolfen_xfree;
			screen->model = vm_mon.model;
			screen->model_free_func = wolfen_xfree;	
			
			XFree(vm_mon.hsync);
			XFree(vm_mon.vsync);		
		} else {
			screen->vendor = WOLFEN_SCREEN_VENDOR;
			screen->vendor_free_func = NULL;
		
			if (i == DefaultScreen(wlonx->x_display)) {
				screen->model = calloc(strlen(WOLFEN_SCREEN_MODEL_CORE_DEFAULT)+wolfen_digit_count(i)+1, sizeof(char));
				if (screen->model) {
					sprintf(screen->model, WOLFEN_SCREEN_MODEL_CORE_DEFAULT, i);
					screen->model_free_func = free;
				} else {
					goto CALLOC_FAIL;
				}		
			} else {
				screen->model = calloc(strlen(WOLFEN_SCREEN_MODEL_CORE)+wolfen_digit_count(i)+1, sizeof(char));
				if (screen->model) {
					sprintf(screen->model, WOLFEN_SCREEN_MODEL_CORE, i);
					screen->model_free_func = free;
				} else {
					CALLOC_FAIL:
					screen->model = WOLFEN_SCREEN_MODEL_UNKNOWN;
					screen->model_free_func = NULL;		
				}
			}	
		}

		screen->screen_number = i;
		screen->x_org = 0;
		screen->y_org = 0;
		screen->width = DisplayWidth(wlonx->x_display, i);
		screen->height = DisplayHeight(wlonx->x_display, i);
		
		wl_list_insert(&wlonx->x_screen_list, &screen->link);
	}
	
	wlonx->x_screen_default = default_screen;
}

void wlonx_display_create_x11(WolfenDisplay *wlonx) {
	XineramaScreenInfo *xin_info;
	int xin_count;
	int event_base;
	int error_base;
	
	/* init various stuff before connecting */
	wlonx->x_screen_count = 0;
	wl_list_init(&wlonx->x_screen_list);
	
	/* connect to x server */
	wlonx->x_display = XOpenDisplay(NULL);
	XSynchronize(wlonx->x_display, True);

	/* used for multimonitor */
	wlonx->x_has_xinerama = XineramaQueryExtension(wlonx->x_display, &event_base, &error_base);
	/* used for getting monitor information in non-xrandr and zaphod configurations */
	wlonx->x_has_vidmode = XF86VidModeQueryExtension(wlonx->x_display, &event_base, &error_base);
	
	/* setup screens */
	if (wlonx->x_has_xinerama) {
		if (XineramaIsActive(wlonx->x_display)) {
			xin_info = XineramaQueryScreens(wlonx->x_display, &xin_count);
			if (xin_count > 1) {
				wolfen_display_create_screens_xinerama(wlonx, xin_info, xin_count);
				XFree(xin_info);
			} else {
				XFree(xin_info);
				goto WOLFEN_DISPLAY_CREATE_SCREENS_CORE;
			}
		}
	} else {
		WOLFEN_DISPLAY_CREATE_SCREENS_CORE:
		wolfen_display_create_screens_core(wlonx);
	}	
}

void wlonx_display_create_wl(WolfenDisplay *wlonx) {
	wlonx->wl_display = wl_display_create();	
    printf("display: %s\n", wl_display_add_socket_auto(wlonx->wl_display));
		
	wl_global_create(wlonx->wl_display, &wl_compositor_interface, wl_compositor_interface.version, wlonx, &wlonx_compositor_bind);
	wl_global_create (wlonx->wl_display, &wl_shell_interface, wl_shell_interface.version, wlonx, &wolfen_wlshell_bind);
	wl_display_init_shm(wlonx->wl_display);
	
	wlonx->comp_imp.create_surface = wlonx_compositor_create_surface;
	wlonx->comp_imp.create_region = wlonx_compositor_create_region;
	
	wlonx->surface_imp.destroy = wolfen_surface_destroy;
	wlonx->surface_imp.attach = wolfen_surface_attach; 
	wlonx->surface_imp.damage = wolfen_surface_damage; 
	wlonx->surface_imp.frame = wolfen_surface_frame;
	wlonx->surface_imp.set_opaque_region = wolfen_surface_set_opaque_region;
	wlonx->surface_imp.set_input_region = wolfen_surface_set_input_region; 
	wlonx->surface_imp.commit = wolfen_surface_commit; 
	wlonx->surface_imp.set_buffer_transform = wolfen_surface_set_buffer_transform;
	wlonx->surface_imp.set_buffer_scale = wolfen_surface_set_buffer_scale;	
	wlonx->surface_imp.damage_buffer = wolfen_surface_damage_buffer;
	wlonx->surface_imp.offset = wolfen_surface_offset;
	
	wlonx->wlshell_imp.get_shell_surface = wolfen_wlshell_get_shell_surface;
	
	wlonx->wlshell_surface_imp.pong = wolfen_wlshell_surface_pong;
	wlonx->wlshell_surface_imp.move = wolfen_wlshell_surface_move;
	wlonx->wlshell_surface_imp.resize = wolfen_wlshell_surface_resize;
	wlonx->wlshell_surface_imp.set_toplevel = wolfen_wlshell_surface_set_toplevel;
	wlonx->wlshell_surface_imp.set_fullscreen = wolfen_wlshell_surface_set_fullscreen;
	wlonx->wlshell_surface_imp.set_transient = wolfen_wlshell_surface_set_transient;
	wlonx->wlshell_surface_imp.set_popup = wolfen_wlshell_surface_set_popup;
	wlonx->wlshell_surface_imp.set_maximized = wolfen_wlshell_surface_set_maximized;
	wlonx->wlshell_surface_imp.set_title = wolfen_wlshell_surface_set_title;
	wlonx->wlshell_surface_imp.set_class = wolfen_wlshell_surface_set_class;
}

void wlonx_display_run(WolfenDisplay *wlonx) {
    wl_display_run(wlonx->wl_display);
}

void wlonx_display_destroy(WolfenDisplay *wlonx) {
    wl_display_destroy(wlonx->wl_display);
    XCloseDisplay(wlonx->x_display);
}

int main(int argc, char *argv[]) {
	WolfenDisplay wlonx;
	
	babl_init();
	wl_list_init(&wlonx.surfaces_list);
	wl_list_init(&wlonx.shell_surfaces_list);
	wlonx_display_create_x11(&wlonx);
	wlonx_display_create_wl(&wlonx);
 	wlonx_display_run(&wlonx);
 	wlonx_display_destroy(&wlonx);
    return 0;
}
