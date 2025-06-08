#define _DEFAULT_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <wayland-client.h>
#include "wolfen-test.h"
#include "wolfen-test-shm.h"

typedef struct {
	int w;
	int h;
	int s;
	int x;
	int y;	
	bool alt;

	struct wl_compositor *compositor;
	struct wl_shell *shell;
	struct wl_shm *shm;
	
	int fd;
 	void *pixels;
	size_t pixels_sz;
} WolfenSHMState;

WolfenSHMState state_shm;

int wolfen_test_shm_create_fd(off_t size) {
	char *path;
	char *name;
	int rnd;
    int fd;
    size_t rnd_sz;
    
    path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
		puts("XDG_RUNTIME_DIR not defined! Using /tmp!");
        path = "/tmp";
    }

	rnd = rand() % (9999 + 1 - 1000) + 1000;
	rnd_sz = floor(log10(abs(rnd)))+1;
	name = malloc(strlen(path)+strlen("/shm-test-")+rnd_sz+1);
    sprintf(name, "%s/shm-test-%d", path, rnd);

    fd = open(name, O_RDWR | O_EXCL | O_CREAT);
	unlink(name);
	free(name);
	ftruncate(fd, size);
	
    return fd;
}

void wolfen_test_shm_paint_pattern_a(uint32_t *pixels, int w, int h) {
	int x;
	int y;
	
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
			uint32_t color;
			int mx;
			int my;
			
            color = 0;
            mx = x / 20;
            my = y / 20;
            
            if (mx % 2 == 0 && my % 2 == 0) {
				uint32_t code; 
                uint32_t r;
                uint32_t g;
                uint32_t b;
                uint32_t a; 
                
				code = (mx / 2) % 8; 
                r = code & 1 ? 0xFF0000 : 0;
				g = code & 2 ? 0x00FF00 : 0;
				b = code & 4 ? 0x0000FF : 0;
				a = (my / 2) % 8 * 32 << 24; 
	            color = a + r + g + b;
            }
            
            pixels[x + (y * w)] = color;
        }
    }
}
	
void wolfen_test_shm_paint_pattern_b(uint32_t *pixels, int w, int h) {
	int x;
	int y;

	for (y = 0; y < h; ++y) {
		for (x = 0; x < w; ++x) {
            if ((x + y / 8 * 8) % 16 < 8) {
                pixels[y * w + x] = 0xFF666666;
            } else {
                pixels[y * w + x] = 0xFFEEEEEE;
			}
		}
    }
}
	
	
void wolfen_test_shm_handle(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
	WolfenSHMState *state_shm;
	
	state_shm = (WolfenSHMState *)data;
    if (!strcmp(interface, "wl_compositor")) {
        state_shm->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    } else if (!strcmp(interface, "wl_shell")) {
        state_shm->shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
    } else if (!strcmp(interface, "wl_shm")) {
       	state_shm->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    }
}

void wolfen_test_shm_handle_remove(void *data, struct wl_registry *registry, uint32_t id) {
	printf("%d has been removed.", id);
}

void wolfen_test_shm_handle_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial){
    wl_shell_surface_pong(shell_surface, serial);
}

void wolfen_test_shm_handle_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {
	puts("SHELL CONFIGURE!");
}

void wolfen_test_shm_handle_popup_done(void *data, struct wl_shell_surface *shell_surface) {
	puts("POPUP DONE!");
}

void wolfen_test_shm_free() {
	munmap(state_shm.pixels, state_shm.pixels_sz);
	close(state_shm.fd);
}
	
void wolfen_test_shm_run(struct wl_display *display) {
	struct wl_registry *registry;
	struct wl_surface *surface;
	struct wl_shell_surface *shell_surface;
	struct wl_shm_pool *pool;
    struct wl_buffer *buffer;
 	void *pixels;
	struct wl_registry_listener listener;
	struct wl_shell_surface_listener listener2;
	int stride;
	int fd;
	int size;	
	uint32_t fmt;
	
	state_shm.compositor = NULL;
	state_shm.w = wolfen_test_ask_for_int("Width");
	state_shm.h = wolfen_test_ask_for_int("Height");
	state_shm.s = wolfen_test_ask_for_int("Buffer scale");
	state_shm.x = wolfen_test_ask_for_int("Buffer x offset");
	state_shm.y = wolfen_test_ask_for_int("Buffer y offset");
	state_shm.alt = wolfen_test_ask_for_bool("Buffer alternate contents");
	if (state_shm.alt) {
		fmt = WL_SHM_FORMAT_XRGB8888;
	} else {
		fmt = WL_SHM_FORMAT_ARGB8888;
	}
	
	listener.global = wolfen_test_shm_handle;
	listener.global_remove = wolfen_test_shm_handle_remove;
	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &listener, &state_shm);
    wl_display_dispatch(display);
	wl_display_roundtrip(display);
	
    listener2.ping = wolfen_test_shm_handle_ping;
    listener2.configure = wolfen_test_shm_handle_configure;
    listener2.popup_done = wolfen_test_shm_handle_popup_done;
   	surface = wl_compositor_create_surface(state_shm.compositor);
	shell_surface = wl_shell_get_shell_surface(state_shm.shell, surface);
    wl_shell_surface_set_toplevel(shell_surface);
    wl_shell_surface_add_listener(shell_surface, &listener2, NULL);

    stride = state_shm.w * 4;
	state_shm.pixels_sz = stride * state_shm.h;
    state_shm.fd = wolfen_test_shm_create_fd(size);
    state_shm.pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, state_shm.fd, 0);
    pool = wl_shm_create_pool(state_shm.shm, state_shm.fd, state_shm.pixels_sz);
    buffer = wl_shm_pool_create_buffer(pool, 0, state_shm.w, state_shm.h, stride, fmt);
    wl_shm_pool_destroy(pool);
    wl_surface_attach(surface, buffer, state_shm.x, state_shm.y);
	wl_surface_set_buffer_scale(surface, state_shm.s);
    wl_surface_commit(surface);
  	if (state_shm.alt) {
		wolfen_test_shm_paint_pattern_b(state_shm.pixels, state_shm.w, state_shm.h);
	} else {
		wolfen_test_shm_paint_pattern_a(state_shm.pixels, state_shm.w, state_shm.h);
	} 
	
    while (wl_display_dispatch(display) != -1) {
	
    }
    
    wolfen_test_shm_free();
}
