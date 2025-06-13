#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/shape.h>
#include <wayland-server.h>
#include <babl/babl.h>

#include "wolfen-misc.h"
#include "wolfen-display.h"
#include "wolfen-pixfmt.h"
#include "wolfen-surface.h"
#include "wolfen-compositor.h"
#include "wolfen-shell.h"
#include "wolfen-pnp-names.h"

bool wolfen_screen_has_compositor(Display *display, int core_screen) {
	char *atom_name;
	Atom atom_atom;
	
	atom_name = malloc(strlen("_NET_WM_CM_S")+wolfen_digit_count(core_screen)*sizeof(char)+1);
    sprintf(atom_name, "_NET_WM_CM_S%d", core_screen);
    atom_atom = XInternAtom(display, atom_name, False);
    free(atom_name);
    if (XGetSelectionOwner(display, atom_atom) == None) {
		return false;
	} else {
		return true;
	}
}

int wolfen_screen_get_core_screen(WolfenScreen *screen) {
	if (screen->type == WOLFEN_SCREEN_TYPE_CORE) {
		return screen->screen_number;
	} else {
		return DefaultScreen(screen->display->x_display);
	}		
}

void wolfen_display_create_screens_xinerama(WolfenDisplay *wlonx, XineramaScreenInfo *xin_info, int xin_count) {
	WolfenScreen *first;
	int i;
	bool is_compositing;
	
	first = NULL;
	is_compositing = wolfen_screen_has_compositor(wlonx->x_display, DefaultScreen(wlonx->x_display));
	
	wlonx->x_screen_count = xin_count;
	for (i = 0; i < wlonx->x_screen_count; i++) {
		WolfenScreen *screen;
		
		screen = malloc(sizeof(WolfenScreen));
		if (!first) {
			first = screen;
		}
		screen->type = WOLFEN_SCREEN_TYPE_XINERAMA;
		
		/* use vidmode to get information for the main screen? */
		screen->name = malloc(strlen(WOLFEN_SCREEN_NAME)+wolfen_digit_count(xin_info[i].screen_number)+1);
		sprintf(screen->name, WOLFEN_SCREEN_NAME, xin_info[i].screen_number);
		screen->name_free_func = free;
		
		screen->make = WOLFEN_SCREEN_MAKE_NAME;
		screen->make_free_func = NULL;
		
		screen->model = malloc(strlen(WOLFEN_SCREEN_MODEL_NAME)+wolfen_digit_count(xin_info[i].screen_number)+1);
		sprintf(screen->model, WOLFEN_SCREEN_MODEL_NAME, xin_info[i].screen_number);
		screen->model_free_func = free;
		
		screen->screen_number = xin_info[i].screen_number;
		screen->x_org = xin_info[i].x_org;
		screen->y_org = xin_info[i].y_org;
		screen->width = xin_info[i].width;
		screen->height = xin_info[i].height;
		screen->is_compositing = is_compositing;
		screen->sp = WL_OUTPUT_SUBPIXEL_UNKNOWN;
		screen->tf = WL_OUTPUT_TRANSFORM_NORMAL;
		/* oh well, use Display*MM instead maybe? */
		screen->widthmm = xin_info[i].width;
		screen->heightmm = xin_info[i].height;
						
		wl_list_insert(&wlonx->x_screen_list, &screen->link);
	}
	wlonx->x_screen_default = first;
}

void wolfen_display_create_screens_xrandr(WolfenDisplay *wlonx) {
	XRRScreenResources *screen_res;
	WolfenScreen *first;
	int i;
	bool is_compositing;
	
	/* is there a way of getting the default monitor on xrandr? the header has nothing? */
	first = NULL;
	is_compositing = wolfen_screen_has_compositor(wlonx->x_display, DefaultScreen(wlonx->x_display));
	screen_res = XRRGetScreenResources(wlonx->x_display, RootWindow(wlonx->x_display, DefaultScreen(wlonx->x_display)));	

	for (i = 0; i < screen_res->noutput; i++) {
		XRROutputInfo *out_info;
	
		out_info = XRRGetOutputInfo(wlonx->x_display, screen_res, screen_res->outputs[i]);	
	
		if (out_info->connection == RR_Connected) {
			XRRCrtcInfo *crtc_info;
			Atom edid_atom;
			WolfenScreen *screen;

			crtc_info = XRRGetCrtcInfo(wlonx->x_display, screen_res, out_info->crtc);

			screen = malloc(sizeof(WolfenScreen));
			if (!first) {
				first = screen;
			}
			screen->type = WOLFEN_SCREEN_TYPE_XRANDR;
			
			screen->name = strndup(out_info->name, out_info->nameLen);
			screen->name_free_func = free;
			
			edid_atom = XInternAtom(wlonx->x_display, RR_PROPERTY_RANDR_EDID, True);
			if (edid_atom != None) {
				unsigned char *prop;
				unsigned long nitems;
				unsigned long bytes_after;
				Atom act_atom;
				int act_type;
				char edid_pnp_name[4];
				int c;
				int j;
				char *token;

				XRRGetOutputProperty(wlonx->x_display, screen_res->outputs[i], edid_atom, 0, 128, False, False, AnyPropertyType, &act_atom, &act_type, &nitems, &bytes_after, &prop); /* THIS HANGS ON NETBSD */
				if (nitems <= 0) {
					goto WOLFEN_XRANDR_INVALID_EDID;
				}
		
				for (c = 0; c < 8; i++) {
					if (!(((c == 0 || c == 7) && prop[c] == 0x00) || (prop[c] == 0xff))) {
						goto WOLFEN_XRANDR_INVALID_EDID;
					}
				}	
					
				edid_pnp_name[0] = (prop[8] >> 2 & 0x1f) + 'A' - 1;
				edid_pnp_name[1] = (((prop[8] & 0x3) << 3) | ((prop[9] & 0xe0) >> 5)) + 'A' - 1;
				edid_pnp_name[2] = (prop[9] & 0x1f) + 'A' - 1;
				edid_pnp_name[3] = '|';
				token = strtok(wolfen_pnp_names, "\n");
				while (token) {
					if (token[0] != '\n') {
						if (!strncmp(token, edid_pnp_name, 4)) {
							screen->make = malloc(strlen(token) - 3);
							strncpy(screen->make, token + 4, strlen(token) - 4);
							screen->make[strlen(token)-1] = '\0';
							screen->make_free_func = free;
							printf("EDID Make: %s\n", screen->make);
						}
					} else {
						break;
					}
					token = strtok(NULL, "\n");			
				}
				
				screen->model = malloc(14);
				for (c = 0x36; c < 0x7E; c += 0x12) {
					if (prop[c] == 0x00) { 
						if (prop[c+3] == 0xfc) {
							for (j = 0; j < 13; j++) {
								if (prop[c+5+j] == 0x0a) {
									screen->model[j] = 0x00;
								} else {
									screen->model[j] = prop[i+5+j];
								}
							}
						}
					}
				}
				screen->model[13] = '\0';
				screen->model_free_func = free;
				printf("EDID Model: %s\n", screen->model);
			} else {
				WOLFEN_XRANDR_INVALID_EDID:
				screen->make = "";
				screen->make_free_func = NULL;
				screen->model = "";
				screen->model_free_func = NULL;
			}

			screen->screen_number = i;
			screen->x_org = crtc_info->x;
			screen->y_org = crtc_info->y;
			screen->width = crtc_info->width;
			screen->height = crtc_info->height;
			screen->is_compositing = is_compositing;
			screen->widthmm = out_info->mm_width;
			screen->heightmm = out_info->mm_width;
	
			switch (out_info->subpixel_order) {
				case SubPixelHorizontalRGB:
					screen->sp = WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB;
					break;
				case SubPixelHorizontalBGR:
					screen->sp = WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR;
					break;
				case SubPixelVerticalRGB:
					screen->sp = WL_OUTPUT_SUBPIXEL_VERTICAL_RGB;
					break;
				case SubPixelVerticalBGR:
					screen->sp = WL_OUTPUT_SUBPIXEL_VERTICAL_BGR;
					break;
				case SubPixelNone:
					screen->sp = WL_OUTPUT_SUBPIXEL_NONE;
					break;
				default:
					screen->sp = WL_OUTPUT_SUBPIXEL_UNKNOWN;
			}

			switch (crtc_info->rotation) {
				case RR_Rotate_90:
					if (crtc_info->rotation & RR_Reflect_X || crtc_info->rotation & RR_Reflect_Y) {
						screen->tf = WL_OUTPUT_TRANSFORM_FLIPPED_90;
					} else {
						screen->tf = WL_OUTPUT_TRANSFORM_90;					
					}
					break;
				case RR_Rotate_180:
					if (crtc_info->rotation & RR_Reflect_X || crtc_info->rotation & RR_Reflect_Y) {
						screen->tf = WL_OUTPUT_TRANSFORM_FLIPPED_180;
					} else {
						screen->tf = WL_OUTPUT_TRANSFORM_180;					
					}
					break;
				case RR_Rotate_270:
					if (crtc_info->rotation & RR_Reflect_X || crtc_info->rotation & RR_Reflect_Y) {
						screen->tf = WL_OUTPUT_TRANSFORM_FLIPPED_270;
					} else {
						screen->tf = WL_OUTPUT_TRANSFORM_270;					
					}
					break;
				default:
					if (crtc_info->rotation & RR_Reflect_X || crtc_info->rotation & RR_Reflect_Y) {
						screen->tf = WL_OUTPUT_TRANSFORM_FLIPPED;					
					} else {
						screen->tf = WL_OUTPUT_TRANSFORM_NORMAL;
					}
			}
															
			XRRFreeCrtcInfo(crtc_info);
			
			wl_list_insert(&wlonx->x_screen_list, &screen->link);	
		}
		
		XRRFreeOutputInfo(out_info);
	}
	
	wlonx->x_screen_default = first;
}

void wolfen_display_create_screens_core(WolfenDisplay *wlonx) {
	WolfenScreen *default_screen;
	int i;

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
			
			screen->make = vm_mon.vendor;
			screen->make_free_func = wolfen_xfree;
			
			screen->model = vm_mon.model;
			screen->model_free_func = wolfen_xfree;	
			
			screen->name = malloc(strlen(vm_mon.vendor) + 1 + strlen(vm_mon.model) + 2 + wolfen_digit_count(i) + 2);
			sprintf(screen->name, "%s %s (%d)", vm_mon.vendor, vm_mon.model, i);
			screen->name_free_func = free;
			
			XFree(vm_mon.hsync);
			XFree(vm_mon.vsync);		
		} else {
			screen->name = malloc(strlen(WOLFEN_SCREEN_NAME)+wolfen_digit_count(i)+1);
			sprintf(screen->name, WOLFEN_SCREEN_NAME, i);
			screen->name_free_func = free;
			
			screen->make = WOLFEN_SCREEN_MAKE_NAME;
			screen->make_free_func = NULL;
			
			screen->model = malloc(strlen(WOLFEN_SCREEN_MODEL_NAME)+wolfen_digit_count(i)+1);
			sprintf(screen->model, WOLFEN_SCREEN_MODEL_NAME, i);
			screen->model_free_func = free;			
		}

		screen->screen_number = i;
		screen->x_org = 0;
		screen->y_org = 0;
		screen->width = DisplayWidth(wlonx->x_display, i);
		screen->height = DisplayHeight(wlonx->x_display, i);
		screen->is_compositing = wolfen_screen_has_compositor(wlonx->x_display, i);
		screen->widthmm = DisplayWidthMM(wlonx->x_display, i);
		screen->heightmm = DisplayHeightMM(wlonx->x_display, i);
		screen->sp = WL_OUTPUT_SUBPIXEL_UNKNOWN;
		screen->tf = WL_OUTPUT_TRANSFORM_NORMAL;
		
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
	XSynchronize(wlonx->x_display, True); /* debug remove this later */

	/* used for monitor info in most setups */
	wlonx->x_has_randr = XRRQueryExtension(wlonx->x_display, &event_base, &error_base);
	/* used for multimonitor in non-xrandr setups */
	wlonx->x_has_xinerama = XineramaQueryExtension(wlonx->x_display, &event_base, &error_base);
	/* used for getting monitor information in non-xrandr and zaphod configurations */
	wlonx->x_has_vidmode = XF86VidModeQueryExtension(wlonx->x_display, &event_base, &error_base);

	/* used for hw accelerated buffer tfs */
	wlonx->x_has_render = XRenderQueryExtension(wlonx->x_display, &event_base, &error_base);
	wlonx->x_has_render = false; /* for development, no xrender functionality has been implemented yet */
	
	/* used for "transparency" if you have no compositor running */
	wlonx->x_has_shape = XShapeQueryExtension(wlonx->x_display, &event_base, &error_base); 
	
	/* setup screens */
	if (wlonx->x_has_randr) {
		wolfen_display_create_screens_xrandr(wlonx);
	} else if (wlonx->x_has_xinerama) {
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
	
	wlonx->region_imp.destroy = wolfen_region_destroy;
	wlonx->region_imp.add = wolfen_region_add;
	wlonx->region_imp.subtract = wolfen_region_subtract;
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
