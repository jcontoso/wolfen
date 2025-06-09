#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <wayland-client.h>
#include "wolfen-test.h"
#include "wolfen-test-lg.h"
#include "wolfen-test-shm.h"

WolfenTestState state;

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

int wolfen_test_ask_for_int(char *message) {
	int i;
	int a;
	
	a = 0;
	i = -1;
	printf("%s?: ", message);
	a = scanf("%d", &i);

	if (a != 1 || i < 0)  {
		while ((a = getchar()) != '\n' && a != EOF) { }
		printf("\nInvalid!\n");
		wolfen_test_ask_for_int(message);
	} else {
		return i;
	}
}

bool wolfen_test_ask_for_bool(char *message) {
	char i;
	int a;
	
	a = 0;
	i = 'N';
	printf("%s? [Y/N]: ", message);
	a = scanf(" %c", &i);

	if (a != 1)  {
		while ((a = getchar()) != '\n' && a != EOF) { }
		printf("\nInvalid!\n");
		wolfen_test_ask_for_bool(message);
	} else {
		if (i == 'Y' || i == 'y') {
			return true;
		} else {
			return false;
		}
	}
}

void wolfen_test_menu_input(struct wl_display *display, WolfenTest *tests, int tests_sz) {
	int i;
	int a;
	a = 0;
	i = -1;
	a = scanf("%d", &i);

	if (a != 1 || i < 0 || i > tests_sz)  {
		while ((a = getchar()) != '\n' && a != EOF) { }
		printf("\nInvalid test number! Pick another one:");
		wolfen_test_menu_input(display, tests, tests_sz);
	} else {
		printf("Running test %d...\n", tests[i].id);
		state.test_cleanup = tests[i].cleanup;
		tests[i].run(display);
		puts("Done!");
		if (tests[i].cleanup) {
			tests[i].cleanup();
		}
	}
}

void wolfen_test_cleanup(int sig) {
	puts("\nGoodbye!");
	if (state.test_cleanup) {
		state.test_cleanup();
	}
	wl_display_disconnect(state.display);
	exit(0);
}

int main(int argc, char *argv[]) {
	WolfenTest tests[2];
	struct wl_display *display;
	int i;	
	int tests_sz;
	
	/* load tests */
	i = 0;
	tests_sz = ARRAY_LENGTH(tests);
	
	tests[i].name = "List globals";
	tests[i].run = wolfen_test_lg_run;
	tests[i].cleanup = NULL;
	i++;
	
	tests[i].name = "SHM + wl_shell";
	tests[i].run = wolfen_test_shm_run;
	tests[i].cleanup = wolfen_test_shm_free;
	i++;

	/* sign on and wl display */
	printf("Welcome to the Wolfen/WLonX testing utility!\n");
	printf("Use CTRL+C to quit at any time.\n\n");
	srand(time(NULL));
	state.test_cleanup = NULL;
	state.display = display = wl_display_connect(NULL);
	if (!display) {
		printf("Unable to connect to display!\n");
		return 0;
	}
	signal(SIGINT, wolfen_test_cleanup);
	
	/* tests menu */
	printf("Available tests:\n");
	for (i = 0; i < tests_sz; i++) {
		tests[i].id = i;
		printf("%d: %s\n", tests[i].id, tests[i].name);		
	}
	printf("\nWhich test would you like to run?:");
	wolfen_test_menu_input(display, tests, tests_sz - 1);
	wl_display_disconnect(display);
	return 0;
}
