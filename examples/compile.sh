cc list_globals.c -o list_globals -g -std=c99 -Wno-deprecated-declarations -g `pkg-config --cflags --libs wayland-client` 
cc shm.c -o shm -g -std=c99 -Wno-deprecated-declarations -g `pkg-config --cflags --libs wayland-client` 
cc shm2.c -o shm2 -g -std=c99 -Wno-deprecated-declarations -g `pkg-config --cflags --libs wayland-client` 

wayland-scanner private-code \
  < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  > xdg-shell-protocol.c
wayland-scanner client-header \
  < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  > xdg-shell-client-protocol.h

cc shm-xdg.c xdg-shell-protocol.c -o shm-xdg -std=c99 -Wno-deprecated-declarations -g `pkg-config --cflags --libs wayland-client` 
