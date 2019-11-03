/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file pugl_test.c A simple Pugl test that creates a top-level window.
*/

#define GL_SILENCE_DEPRECATION 1

#include "test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"
#include "pugl/pugl_gl_backend.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const int borderWidth = 64;

typedef struct
{
	PuglWorld* world;
	PuglView*  parent;
	PuglView*  child;
	bool       continuous;
	int        quit;
	float      xAngle;
	float      yAngle;
	float      dist;
	double     lastMouseX;
	double     lastMouseY;
	double     lastDrawTime;
	unsigned   framesDrawn;
	bool       mouseEntered;
} PuglTestApp;

static PuglRect
getChildFrame(const PuglRect parentFrame)
{
	const PuglRect childFrame = {
		borderWidth,
		borderWidth,
		parentFrame.width - 2 * borderWidth,
		parentFrame.height - 2 * borderWidth
	};

	return childFrame;
}

static void
onReshape(PuglView* view, int width, int height)
{
	(void)view;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, width, height);

	float projection[16];
	perspective(projection, 1.8f, width / (float)height, 1.0, 100.0f);
	glLoadMatrixf(projection);
}

static void
onDisplay(PuglView* view)
{
	PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

	const double thisTime = puglGetTime(app->world);
	if (app->continuous) {
		const double dTime = thisTime - app->lastDrawTime;
		app->xAngle = fmodf((float)(app->xAngle + dTime * 100.0f), 360.0f);
		app->yAngle = fmodf((float)(app->yAngle + dTime * 100.0f), 360.0f);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, app->dist * -1);
	glRotatef(app->xAngle, 0.0f, 1.0f, 0.0f);
	glRotatef(app->yAngle, 1.0f, 0.0f, 0.0f);

	const float bg = app->mouseEntered ? 0.2f : 0.1f;
	glClearColor(bg, bg, bg, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (puglHasFocus(app->child)) {
		// Draw cube surfaces
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, cubeStripVertices);
		glColorPointer(3, GL_FLOAT, 0, cubeStripVertices);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);

		glColor3f(0.0f, 0.0f, 0.0f);
	} else {
		glColor3f(1.0f, 1.0f, 1.0f);
	}

	// Draw cube wireframe
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, cubeFrontLineLoop);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	glVertexPointer(3, GL_FLOAT, 0, cubeBackLineLoop);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	glVertexPointer(3, GL_FLOAT, 0, cubeSideLines);
	glDrawArrays(GL_LINES, 0, 8);
	glDisableClientState(GL_VERTEX_ARRAY);

	app->lastDrawTime = thisTime;
	++app->framesDrawn;
}

static void
swapFocus(PuglTestApp* app)
{
	if (puglHasFocus(app->parent)) {
		puglGrabFocus(app->child);
	} else {
		puglGrabFocus(app->parent);
	}

	puglPostRedisplay(app->parent);
	puglPostRedisplay(app->child);
}

static void
onKeyPress(PuglView* view, const PuglEventKey* event, const char* prefix)
{
	PuglTestApp* app   = (PuglTestApp*)puglGetHandle(view);
	PuglRect     frame = puglGetFrame(view);

	if (event->key == '\t') {
		swapFocus(app);
	} else if (event->key == 'q' || event->key == PUGL_KEY_ESCAPE) {
		app->quit = 1;
	} else if (event->state & PUGL_MOD_CTRL && event->key == 'c') {
		puglSetClipboard(view, NULL, "Pugl test", strlen("Pugl test") + 1);
		fprintf(stderr, "%sCopy \"Pugl test\"\n", prefix);
	} else if (event->state & PUGL_MOD_CTRL && event->key == 'v') {
		const char* type = NULL;
		size_t      len  = 0;
		const char* text = (const char*)puglGetClipboard(view, &type, &len);
		fprintf(stderr, "%sPaste \"%s\"\n", prefix, text);
	} else if (event->state & PUGL_MOD_SHIFT) {
		if (event->key == PUGL_KEY_UP) {
			frame.height += 10;
		} else if (event->key == PUGL_KEY_DOWN) {
			frame.height -= 10;
		} else if (event->key == PUGL_KEY_LEFT) {
			frame.width -= 10;
		} else if (event->key == PUGL_KEY_RIGHT) {
			frame.width += 10;
		} else {
			return;
		}
		puglSetFrame(view, frame);
	} else {
		if (event->key == PUGL_KEY_UP) {
			frame.y -= 10;
		} else if (event->key == PUGL_KEY_DOWN) {
			frame.y += 10;
		} else if (event->key == PUGL_KEY_LEFT) {
			frame.x -= 10;
		} else if (event->key == PUGL_KEY_RIGHT) {
			frame.x += 10;
		} else {
			return;
		}
		puglSetFrame(view, frame);
	}
}

static PuglStatus
onParentEvent(PuglView* view, const PuglEvent* event)
{
	PuglTestApp*   app         = (PuglTestApp*)puglGetHandle(view);
	const PuglRect parentFrame = puglGetFrame(view);

	printEvent(event, "Parent: ");

	switch (event->type) {
	case PUGL_CONFIGURE:
		onReshape(view,
		          (int)event->configure.width,
		          (int)event->configure.height);

		puglSetFrame(app->child, getChildFrame(parentFrame));
		break;
	case PUGL_EXPOSE:
		if (puglHasFocus(app->parent)) {
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, cubeStripVertices);
			glColorPointer(3, GL_FLOAT, 0, cubeStripVertices);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
		} else {
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		break;
	case PUGL_KEY_PRESS:
		onKeyPress(view, &event->key, "Parent: ");
		break;
	case PUGL_MOTION_NOTIFY:
		break;
	case PUGL_CLOSE:
		app->quit = 1;
		break;
	default:
		break;
	}

	return PUGL_SUCCESS;
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
	PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

	printEvent(event, "Child: ");

	switch (event->type) {
	case PUGL_CONFIGURE:
		onReshape(view, (int)event->configure.width, (int)event->configure.height);
		break;
	case PUGL_EXPOSE:
		onDisplay(view);
		break;
	case PUGL_CLOSE:
		app->quit = 1;
		break;
	case PUGL_KEY_PRESS:
		onKeyPress(view, &event->key, "Child: ");
		break;
	case PUGL_MOTION_NOTIFY:
		app->xAngle = fmodf(app->xAngle - (float)(event->motion.x - app->lastMouseX), 360.0f);
		app->yAngle = fmodf(app->yAngle + (float)(event->motion.y - app->lastMouseY), 360.0f);
		app->lastMouseX = event->motion.x;
		app->lastMouseY = event->motion.y;
		puglPostRedisplay(view);
		puglPostRedisplay(app->parent);
		break;
	case PUGL_SCROLL:
		app->dist = fmaxf(10.0f, app->dist + (float)event->scroll.dy);
		puglPostRedisplay(view);
		break;
	case PUGL_ENTER_NOTIFY:
		app->mouseEntered = true;
		break;
	case PUGL_LEAVE_NOTIFY:
		app->mouseEntered = false;
		break;
	default:
		break;
	}

	return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
	PuglTestApp app = {0};
	app.dist = 10;

	const PuglTestOptions opts = puglParseTestOptions(&argc, &argv);
	if (opts.help) {
		puglPrintTestUsage("pugl_test", "");
		return 1;
	}

	app.continuous = opts.continuous;

	app.world  = puglNewWorld();
	app.parent = puglNewView(app.world);
	app.child  = puglNewView(app.world);

	puglSetClassName(app.world, "Pugl Test");

	const PuglRect parentFrame = { 0, 0, 512, 512 };
	puglSetFrame(app.parent, parentFrame);
	puglSetMinSize(app.parent, borderWidth * 3, borderWidth * 3);
	puglSetAspectRatio(app.parent, 1, 1, 16, 9);
	puglSetBackend(app.parent, puglGlBackend());

	puglSetViewHint(app.parent, PUGL_RESIZABLE, opts.resizable);
	puglSetViewHint(app.parent, PUGL_SAMPLES, opts.samples);
	puglSetViewHint(app.parent, PUGL_DOUBLE_BUFFER, opts.doubleBuffer);
	puglSetViewHint(app.parent, PUGL_SWAP_INTERVAL, opts.doubleBuffer);
	puglSetViewHint(app.parent, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
	puglSetHandle(app.parent, &app);
	puglSetEventFunc(app.parent, onParentEvent);

	const uint8_t title[] = { 'P', 'u', 'g', 'l', ' ',
	                          'P', 'r', 0xC3, 0xBC, 'f', 'u', 'n', 'g', 0 };
	if (puglCreateWindow(app.parent, (const char*)title)) {
		fprintf(stderr, "error: Failed to create parent window\n");
		return 1;
	}

	puglSetFrame(app.child, getChildFrame(parentFrame));
	puglSetParentWindow(app.child, puglGetNativeWindow(app.parent));

	puglSetViewHint(app.child, PUGL_SAMPLES, opts.samples);
	puglSetViewHint(app.child, PUGL_DOUBLE_BUFFER, opts.doubleBuffer);
	puglSetViewHint(app.child, PUGL_SWAP_INTERVAL, 0);
	puglSetBackend(app.child, puglGlBackend());
	puglSetViewHint(app.child, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
	puglSetHandle(app.child, &app);
	puglSetEventFunc(app.child, onEvent);

	const int st = puglCreateWindow(app.child, NULL);
	if (st) {
		fprintf(stderr, "error: Failed to create child window (%d)\n", st);
		return 1;
	}

	puglShowWindow(app.parent);
	puglShowWindow(app.child);

	PuglFpsPrinter fpsPrinter         = { puglGetTime(app.world) };
	bool           requestedAttention = false;
	while (!app.quit) {
		const double thisTime = puglGetTime(app.world);

		if (app.continuous) {
			puglPostRedisplay(app.parent);
			puglPostRedisplay(app.child);
		} else {
			puglPollEvents(app.world, -1);
		}

		puglDispatchEvents(app.world);

		if (!requestedAttention && thisTime > 5.0) {
			puglRequestAttention(app.parent);
			requestedAttention = true;
		}

		if (app.continuous) {
			puglPrintFps(app.world, &fpsPrinter, &app.framesDrawn);
		}
	}

	puglFreeView(app.child);
	puglFreeView(app.parent);
	puglFreeWorld(app.world);

	return 0;
}
