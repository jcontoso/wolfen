/*
 * Wayland shm example
 * Based on https://jan.newmarch.name/Wayland/SharedMemory/
 * Build: cc -o shm shm.c -lwayland-client
 */

#define _DEFAULT_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

struct wl_compositor *compositor = NULL;
struct wl_shell *shell;
struct wl_shm *shm;

int WIDTH = 320;
int HEIGHT = 320;

void paint_pixels(uint32_t *pixel) {

  for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            if ((x + y / 8 * 8) % 16 < 8)
                pixel[y * WIDTH + x] = 0xFF666666;
            else
                pixel[y * WIDTH + x] = 0xFFEEEEEE;
        }
    }

}

/*
* Return a newly created and resized file descriptor.
* For real use case, mkstemp instead of open is better.
* See create_tmpfile_cloexec in https://cgit.freedesktop.org/wayland/weston/tree/shared/os-compatibility.c
*/
int create_shared_fd(off_t size)
{
    char name[1024] = "";

    const char *path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        fprintf(stderr, "Please define environment variable XDG_RUNTIME_DIR\n");
        exit(1);
    }

    strcpy(name, path);
    strcat(name, "/shm-test");

    int fd = open(name, O_RDWR | O_EXCL | O_CREAT);
    if (fd < 0) {
        fprintf(stderr, "File cannot open: %s\n", name);
        exit(1);
    } else {
        unlink(name);
    }

    if (ftruncate(fd, size) < 0) {
         fprintf(stderr, "ftruncate failed: fd=%i, size=%li\n", fd, size);
         close(fd);
         exit(1);
    }

    return fd;
}

/*
 * Create a window and return the attached buffer
 */
void * create_window(struct wl_surface *surface) {
    int stride = WIDTH * 4; // 4 bytes per pixel
    int size = stride * HEIGHT;

    int fd = create_shared_fd(size);
    void *shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_data == MAP_FAILED) {
	    fprintf(stderr, "mmap failed: %m\n");
	    close(fd);
	    exit(1);
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, WIDTH, HEIGHT, stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);

    wl_surface_attach(surface, buffer, 16, 20);
    wl_surface_commit(surface);
    return shm_data;
}

void shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
}

struct wl_shm_listener shm_listener = {
	shm_format
};

/*
 * Event listeners for wl_registry_add_listener
 */
void global_registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
    } else if (strcmp(interface, "wl_shm") == 0) {
        shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
        wl_shm_add_listener(shm, &shm_listener, NULL);
    }
}

void global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
}

const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};

/*
 * Event listeners for wl_shell_surface_add_listener
 */
void handle_ping(void *_data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}

void handle_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height)
{
}

void handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

const struct wl_shell_surface_listener shell_surface_listener = {
    handle_ping,
    handle_configure,
    handle_popup_done
};

int main(int argc, char **argv) {

    struct wl_display *display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Can't connect to display\n");
        exit(1);
    }
    printf("connected to display\n");

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (compositor == NULL) {
        fprintf(stderr, "Can't find compositor\n");
        exit(1);
    }

    struct wl_surface *surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
        fprintf(stderr, "Can't create surface\n");
        exit(1);
    }

    struct wl_shell_surface *shell_surface = wl_shell_get_shell_surface(shell, surface);
    if (shell_surface == NULL) {
        fprintf(stderr, "Can't create shell surface\n");
        exit(1);
    }

    wl_shell_surface_set_toplevel(shell_surface);
    wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, NULL);

    void *shm_data = create_window(surface);
    paint_pixels(shm_data);

    while (wl_display_dispatch(display) != -1) {
	;
    }

    wl_display_disconnect(display);
    printf("disconnected from display\n");
    
    exit(0);
}
