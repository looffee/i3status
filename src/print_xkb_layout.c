// vim:ts=4:sw=4:expandtab
#include <config.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>

#include "i3status.h"

#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

Display *display = NULL;
int isListening = 0;

Display *openDisplay() {
	if (display != NULL) {
		return display;
	}

	int event;
	int error;
	int major_version = XkbMajorVersion;
	int minor_version = XkbMinorVersion;
	int reason;

	Display *display = XkbOpenDisplay(
		NULL,
		&event,
		&error,
		&major_version,
		&minor_version,
		&reason
	);

	if (reason != XkbOD_Success) {
		const char *errorStr;

		switch (reason) {
		case XkbOD_BadLibraryVersion:
			errorStr = "Bad library version!\n";
			break;
		case XkbOD_ConnectionRefused:
			errorStr = "Connection refused!\n";
			break;
		case XkbOD_BadServerVersion:
			errorStr = "Bad server version!\n";
			break;
		case XkbOD_NonXkbServer:
			errorStr = "Non-Xkb server!\n";
			break;
		default: errorStr = "Unknown error reason!\n";
		}

		fprintf(stderr, errorStr);

		return NULL;
	}

	return display;
}

int init() {
	display = openDisplay();

	if (display == NULL) {
		fprintf(stderr, "Cannot open display!\n");
		return 1;
	}

	return 0;
}

int startListening() {
	int result;

	result = XkbSelectEventDetails(
		display,
		XkbUseCoreKbd,
		XkbStateNotify,
		XkbGroupStateMask,
		XkbGroupStateMask
	);

	if (!result) {
		fprintf(stderr, "Cannot start listening events!\n");
		return result;
	}

	isListening = 1;

	return 0;
}

void waitNextEvent() {
	if (display == NULL) {
		fprintf(stderr, "Lib is not inited, call 'init' function first!\n");
		return;
	}

	if (!isListening) {
		fprintf(stderr, "Start listening events first by calling 'startListening' function!\n");
		return;
	}

	XkbEvent *event = (XkbEvent*)malloc(sizeof(XkbEvent));
	XNextEvent(display, &(*event).core);
}

// I don't like this approach to get current keyboard layout, but I didn't find better one :(
char *getLayout() {
	if (display == NULL) {
		fprintf(stderr, "Lib is not inited, call 'init' function first!\n");
		return (char *)"";
	}

	XkbStateRec state;
	XkbGetState(display, XkbUseCoreKbd, &state);
		
	XkbRF_VarDefsRec vd;
	XkbRF_GetNamesProp(display, NULL, &vd);

	char *tok = strtok(vd.layout, ",");

	for (int i = 0; i < state.group; i++) {
		tok = strtok(NULL, ",");
		if (tok == NULL) {
			return NULL;
		}
	}

	return tok;
}

void cleanup() {
	if (display == NULL) {
		return;
	}

	if (isListening) {
		XkbSelectEventDetails(
			display,
			XkbUseCoreKbd,
			XkbStateNotify,
			1,
			0
		);
		isListening = 0;
	}

	XCloseDisplay(display);
	display = NULL;
}

void print_xkb_layout(yajl_gen json_gen, char *buffer) {
	char *outwalk = buffer;
	init();
	char *current_layout = getLayout();
	buffer = malloc(2);
	strcpy(buffer, current_layout);
	OUTPUT_FULL_TEXT(buffer);
	cleanup();
	free(buffer);
}
