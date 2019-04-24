/* robtk GUI application
 *
 * Copyright (C) 2012-2015 Robin Gareus
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef UI_UPDATE_FPS
#define UI_UPDATE_FPS 25
#endif

///////////////////////////////////////////////////////////////////////////////

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef WIN32
#include <windows.h>
#include <pthread.h>
#define pthread_t //< override jack.h def
#endif

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
extern void rtk_osx_api_init(void);
extern void rtk_osx_api_terminate(void);
extern void rtk_osx_api_run(void);
extern void rtk_osx_api_err(const char *msg);
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <getopt.h>
#include <assert.h>

#if (defined _WIN32 && defined RTK_STATIC_INIT)
#include <glib-object.h>
#endif

#ifndef _WIN32
#include <signal.h>
#include <pthread.h>
#else
#include <sys/timeb.h>
#endif

#include "./gl/xternalui.h"

#define LV2_EXTERNAL_UI_RUN(ptr) (ptr)->run(ptr)
#define LV2_EXTERNAL_UI_SHOW(ptr) (ptr)->show(ptr)
#define LV2_EXTERNAL_UI_HIDE(ptr) (ptr)->hide(ptr)

#define nan NAN

typedef struct _RtkLv2Description {
	const LV2UI_Descriptor* (*lv2ui_descriptor)(uint32_t index);
	const uint32_t gui_descriptor_id;
	const char *plugin_human_id;
} RtkLv2Description;


static const LV2UI_Descriptor *plugin_gui;

//static LV2_Handle plugin_instance = NULL;
static LV2UI_Handle gui_instance = NULL;

static pthread_mutex_t gui_thread_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  data_ready = PTHREAD_COND_INITIALIZER;


static RtkLv2Description const *inst;

/* a simple state machine for this client */
static volatile enum {
	Run,
	Exit
} client_state = Run;

struct lv2_external_ui_host extui_host;
struct lv2_external_ui *extui = NULL;


static const RtkLv2Description _plugin = {
	& RTK_DESCRIPTOR, 0, APPTITLE
};

/******************************************************************************
 * MAIN
 */

static void cleanup(int sig) {
	if (plugin_gui && gui_instance && plugin_gui->cleanup) {
		plugin_gui->cleanup(gui_instance);
	}
	fprintf(stderr, "bye.\n");
}

static void run_one() {
	plugin_gui->port_event(gui_instance, 0, 0, 0, NULL);
	LV2_EXTERNAL_UI_RUN(extui);
}

#ifdef __APPLE__

static void osx_loop (CFRunLoopTimerRef timer, void *info) {
	if (client_state == Run) {
		run_one();
	}
	if (client_state == Exit) {
		rtk_osx_api_terminate();
	}
}

#else

static void main_loop(void) {
	struct timespec timeout;
	pthread_mutex_lock (&gui_thread_lock);
	while (client_state != Exit) {
		run_one();

		if (client_state == Exit) break;

#ifdef _WIN32
		//Sleep(1000/UI_UPDATE_FPS);
#if (defined(__MINGW64__) || defined(__MINGW32__)) && __MSVCRT_VERSION__ >= 0x0601
		struct __timeb64 timebuffer;
		_ftime64(&timebuffer);
#else
		struct __timeb32 timebuffer;
		_ftime(&timebuffer);
#endif
		timeout.tv_nsec = timebuffer.millitm * 1000000;
		timeout.tv_sec = timebuffer.time;
#else // POSIX
		clock_gettime(CLOCK_REALTIME, &timeout);
#endif
		timeout.tv_nsec += 1000000000 / (UI_UPDATE_FPS);
		if (timeout.tv_nsec >= 1000000000) {timeout.tv_nsec -= 1000000000; timeout.tv_sec+=1;}
		pthread_cond_timedwait (&data_ready, &gui_thread_lock, &timeout);

	} /* while running */
	pthread_mutex_unlock (&gui_thread_lock);
}
#endif // APPLE RUNLOOP

static void catchsig (int sig) {
	fprintf(stderr,"caught signal - shutting down.\n");
	client_state=Exit;
	pthread_cond_signal (&data_ready);
}

static void on_external_ui_closed(void* controller) {
	catchsig(0);
}

int main (int argc, char **argv) {
	int rv = 0;

	inst = & _plugin;

#ifdef __APPLE__
	rtk_osx_api_init();
#endif

#ifdef _WIN32
	pthread_win32_process_attach_np();
#endif
#if (defined _WIN32 && defined RTK_STATIC_INIT)
	glib_init_static();
	gobject_init_ctor();
#endif

	struct { int argc; char **argv; } rtkargv;
	rtkargv.argc = argc;
	rtkargv.argv = argv;

	const LV2_Feature external_lv_feature = { LV2_EXTERNAL_UI_URI, &extui_host};
	const LV2_Feature external_kx_feature = { LV2_EXTERNAL_UI_URI__KX__Host, &extui_host};
	const LV2_Feature robtk_argv = { "http://gareus.org/oss/lv2/robtk#argv", &rtkargv};

	// TODO add argv[] as feature
	const LV2_Feature* ui_features[] = {
		&external_lv_feature,
		&external_kx_feature,
		&robtk_argv,
		NULL
	};

	extui_host.plugin_human_id = inst->plugin_human_id;

	plugin_gui = inst->lv2ui_descriptor(inst->gui_descriptor_id);

	if (plugin_gui) {
	/* init plugin GUI */
	extui_host.ui_closed = on_external_ui_closed;
	gui_instance = plugin_gui->instantiate(plugin_gui,
			"URI TODO", NULL, NULL, NULL,
			(void **)&extui, ui_features);

	}

	if (!gui_instance || !extui) {
		fprintf(stderr, "Error: GUI was not initialized.\n");
		rv |= 2;
		goto out;
	}

#ifndef _WIN32
	signal (SIGHUP, catchsig);
	signal (SIGINT, catchsig);
#endif

	{

		LV2_EXTERNAL_UI_SHOW(extui);

#ifdef __APPLE__
		CFRunLoopRef runLoop = CFRunLoopGetCurrent();
		CFRunLoopTimerContext context = {0, NULL, NULL, NULL, NULL};
		CFRunLoopTimerRef timer = CFRunLoopTimerCreate(kCFAllocatorDefault, 0, 1.0/UI_UPDATE_FPS, 0, 0, &osx_loop, &context);
		CFRunLoopAddTimer(runLoop, timer, kCFRunLoopCommonModes);
		rtk_osx_api_run();
#else

		main_loop();
#endif

		LV2_EXTERNAL_UI_HIDE(extui);
	}

out:
	cleanup(0);
#ifdef _WIN32
	pthread_win32_process_detach_np();
#endif
#if (defined _WIN32 && defined RTK_STATIC_INIT)
	glib_cleanup_static();
#endif
	return(rv);
}
/* vi:set ts=2 sts=2 sw=2: */
