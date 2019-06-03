/* meters.lv2 openGL frontend
 *
 * Copyright (C) 2013-2016 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

//#undef HAVE_IDLE_IFACE // simulate old LV2
//#define USE_GUI_THREAD // use own thread (not idle callback), should be preferred even w/idle availale on most platforms
//#define THREADSYNC     // wake up GUI thread on port-event

#ifdef XTERNAL_UI
#if defined USE_GUI_THREAD && defined _WIN32
#  define INIT_PUGL_IN_THREAD
#endif
#endif

#define TIMED_RESHAPE // resize view when idle

//#define DEBUG_RESIZE
//#define DEBUG_EXPOSURE
//#define VISIBLE_EXPOSE
//#define DEBUG_UI

/* either USE_GUI_THREAD or HAVE_IDLE_IFACE needs to be defined.
 *
 * if both are defined:
 * the goniometer window can change size by itself..
 *
 * IFF HAVE_IDLE_IFACE is available we can use it to
 * resize the suil-swallowed window (gtk host only), by
 * making a call to gtk_window_resize of the gtk_widget_get_toplevel
 * in the thread-context of the main application.
 *
 * without USE_GUI_THREAD but with HAVE_IDLE_IFACE:
 * All plugins run single threaded in the host's process context.
 *
 * That will avoid various threading issues - particularly with
 * libcairo < 1.12.10, libpixman < 0.30.2 and libpango < 1.32.6
 * which are not threadsafe)
 *
 * The plugin UI launching its own thread yields generally better
 * performance (the host's idle call's timing is inaccurate and
 * using the idle interface will also slow down the host's UI...
 */
#if (!defined HAVE_IDLE_IFACE && !defined USE_GUI_THREAD)
#error At least one of HAVE_IDLE_IFACE or USE_GUI_THREAD must be defined.
#endif

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>

#include "pugl/pugl.h"

#ifdef __APPLE__
#include "OpenGL/glu.h"
#else
#include <GL/glu.h>
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1 // GL_BGRA_EXT
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 GL_RGBA
#endif
#ifndef GL_TEXTURE_RECTANGLE_ARB
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#endif

#ifdef USE_GTK_RESIZE_HACK
#include <gtk/gtk.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ROBTK_MOD_SHIFT PUGL_MOD_SHIFT
#define ROBTK_MOD_CTRL PUGL_MOD_CTRL
#include "gl/posringbuf.h"
#include "robtk.h"

#ifdef WITH_SIGNATURE
# include "gpg_init.c"
#endif

static void opengl_init () {
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	glDisable (GL_DEPTH_TEST);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_TEXTURE_RECTANGLE_ARB);
}

static void opengl_draw (int width, int height, unsigned char* surf_data, unsigned int texture_id) {
	if (!surf_data) { return; }

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT);

	glPushMatrix ();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture_id);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8,
			width, height, /*border*/ 0,
			GL_BGRA, GL_UNSIGNED_BYTE, surf_data);

	glBegin(GL_QUADS);
	glTexCoord2f(           0.0f, (GLfloat) height);
	glVertex2f(-1.0f, -1.0f);

	glTexCoord2f((GLfloat) width, (GLfloat) height);
	glVertex2f( 1.0f, -1.0f);

	glTexCoord2f((GLfloat) width, 0.0f);
	glVertex2f( 1.0f,  1.0f);

	glTexCoord2f(            0.0f, 0.0f);
	glVertex2f(-1.0f,  1.0f);
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}

static void opengl_reallocate_texture (int width, int height, unsigned int* texture_id) {
	glViewport (0, 0, width, height);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glOrtho (-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

	glClear (GL_COLOR_BUFFER_BIT);

	glDeleteTextures (1, texture_id);
	glGenTextures (1, texture_id);
	glBindTexture (GL_TEXTURE_RECTANGLE_ARB, *texture_id);
	glTexImage2D (GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8,
			width, height, 0,
			GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

static cairo_t* opengl_create_cairo_t (int width, int height, cairo_surface_t** surface, unsigned char** buffer)
{
	cairo_t* cr;
	const int bpp = 4;

	*buffer = (unsigned char*) calloc (bpp * width * height, sizeof (unsigned char));
	if (!*buffer) {
		fprintf (stderr, "robtk: opengl surface out of memory.\n");
		return NULL;
	}

	*surface = cairo_image_surface_create_for_data (*buffer,
			CAIRO_FORMAT_ARGB32, width, height, bpp * width);
	if (cairo_surface_status (*surface) != CAIRO_STATUS_SUCCESS) {
		free (*buffer);
		fprintf (stderr, "robtk: failed to create cairo surface\n");
		return NULL;
	}

	cr = cairo_create (*surface);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
		free (*buffer);
		fprintf (stderr, "robtk: cannot create cairo context\n");
		return NULL;
	}

	return cr;
}

/*****************************************************************************/

#include "gl/xternalui.h"

typedef struct {
	PuglView*            view;
	LV2UI_Resize*        resize;
	LV2UI_Write_Function write; // unused for now
	LV2UI_Controller     controller; // unused for now
	PuglNativeWindow     parent; // only valud during init
	bool                 ontop;
	unsigned long        transient_id; // X11 window ID

	struct lv2_external_ui_host *extui;
	struct lv2_external_ui xternal_ui;

	int                  width;
	int                  height;
	int                  xoff;
	int                  yoff;
	float                xyscale;
	bool                 gl_initialized;
#ifdef WITH_SIGNATURE
	bool                 gpg_verified;
	char                 gpg_data[128];
	float                gpg_shade;
#endif
#ifdef INIT_PUGL_IN_THREAD
	int                  ui_initialized;
#endif
	bool                 resize_in_progress;
	bool                 resize_toplevel;

#ifdef USE_GUI_THREAD
	int                  ui_queue_puglXWindow;
	pthread_t            thread;
	int                  exit;
#endif
#ifdef TIMED_RESHAPE
	uint64_t         queue_reshape;
	int              queue_w;
	int              queue_h;
#else
	bool             relayout;
#endif

	cairo_t*         cr;
	cairo_surface_t* surface;
	unsigned char*   surf_data;
#if __BIG_ENDIAN__
	unsigned char*   surf_data_be;
#endif
	unsigned int     texture_id;

	/* top-level */
	RobWidget    *tl;
	LV2UI_Handle  ui;

	/* toolkit state stuff */
	volatile cairo_rectangle_t expose_area;
	RobWidget *mousefocus;
	RobWidget *mousehover;

	posringbuf *rb;

#if (defined USE_GUI_THREAD && defined HAVE_IDLE_IFACE)
	bool do_the_funky_resize;
#endif
	bool queue_canvas_realloc;

	void (* ui_closed)(void* controller);
	bool close_ui; // used by xternalui

#if (defined USE_GUI_THREAD && defined THREADSYNC)
	pthread_mutex_t msg_thread_lock;
	pthread_cond_t data_ready;
#endif

	void (* expose_overlay)(RobWidget* toplevel, cairo_t* cr, cairo_rectangle_t *ev);
	float queue_widget_scale;

} GLrobtkLV2UI;

static const char * robtk_info(void *h) {
#ifdef WITH_SIGNATURE
	GLrobtkLV2UI * self = (GLrobtkLV2UI*) h;
	return self->gpg_data;
#else
	return "v" VERSION;
#endif
}

static void robtk_close_self(void *h) {
	GLrobtkLV2UI * self = (GLrobtkLV2UI*) h;
	if (self->ui_closed) {
		self->ui_closed(self);
	}
}

static int robtk_open_file_dialog(void *h, const char *title) {
	GLrobtkLV2UI * self = (GLrobtkLV2UI*) h;
	return puglOpenFileDialog(self->view, title);
}

/*****************************************************************************/

#include PLUGIN_SOURCE

/*****************************************************************************/

#ifdef WITH_SIGNATURE
#include WITH_SIGNATURE
#endif

#ifdef ROBTKAPP
static const void*
extension_data(const char* uri)
{
	return NULL;
}

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
}
#endif

/*****************************************************************************/

#include "gl/xternalui.c"

static void reallocate_canvas(GLrobtkLV2UI* self);

/*****************************************************************************/
/* RobWidget implementation & glue */

typedef struct {
	RobWidget *rw;
	cairo_rectangle_t a;
} RWArea;

#ifdef WITH_SIGNATURE
static void lc_expose (GLrobtkLV2UI * self) {
	assert (self->tl);
	if (!self->tl) { return; }
	cairo_rectangle_t expose_area;
	posrb_read_clear(self->rb); // no fast-track

	expose_area.x = expose_area.y = 0;
	expose_area.width = self->width;
	expose_area.height = self->height;
	cairo_save(self->cr);
	self->tl->resized = TRUE; // full re-expose
	self->tl->expose_event(self->tl, self->cr, &expose_area);
	cairo_restore(self->cr);

	expose_area.x = expose_area.y = 0;
	expose_area.width = self->width;
	expose_area.height = self->height;

	PangoFontDescription *xfont = pango_font_description_from_string("Sans 16px");
	cairo_rectangle (self->cr, 0, 0, self->width, self->height);
	cairo_set_operator (self->cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba(self->cr, 0, 0, 0, .15 + self->gpg_shade);
	if (self->gpg_shade < .6) self->gpg_shade += .001;
	cairo_fill(self->cr);
	write_text_full(self->cr, "Unregistered Version\nhttp://x42-plugins.com",
			xfont, self->width * .5, self->height * .5,
			self->width < 200 ? M_PI * -.5 : 0, -2, c_wht);
	pango_font_description_free(xfont);

	cairo_surface_mark_dirty(self->surface);
}
#endif

static void cairo_expose(GLrobtkLV2UI * self) {

	if (self->expose_overlay) {
		cairo_rectangle_t expose_area;
		posrb_read_clear(self->rb); // no fast-track
		self->tl->resized = TRUE; // full re-expose
		expose_area.x = expose_area.y = 0;
		expose_area.width = self->width;
		expose_area.height = self->height;

		cairo_save(self->cr);
		self->tl->expose_event(self->tl, self->cr, &expose_area);
		cairo_restore(self->cr);

		cairo_save(self->cr);
		self->expose_overlay (self->tl, self->cr, &expose_area);
		cairo_restore(self->cr);
		return;
	}

	/* FAST TRACK EXPOSE */
	int qq = posrb_read_space(self->rb) / sizeof(RWArea);
	bool dirty = qq > 0;
#ifdef DEBUG_FASTTRACK
	/*if (qq > 0)*/ fprintf(stderr, " fast track %d events\n", qq);
#endif
	cairo_rectangle_t fast_track_area = {0,0,0,0};
	uint32_t fast_track_cnt = 0;
	while (--qq >= 0) {
		RWArea a;
		posrb_read(self->rb, (uint8_t*) &a, sizeof(RWArea));
		assert(a.rw);

		/* skip duplicates */
		if (fast_track_cnt > 0) {
			if (   a.a.x+a.rw->trel.x >= fast_track_area.x
					&& a.a.y+a.rw->trel.y >= fast_track_area.y
					&& a.a.x+a.rw->trel.x+a.a.width  <= fast_track_area.x + fast_track_area.width
					&& a.a.y+a.rw->trel.y+a.a.height <= fast_track_area.y + fast_track_area.height) {
#ifdef DEBUG_FASTTRACK
				fprintf(stderr, " skip fast-track event #%d (%.1f x %.1f + %.1f + %.1f)\n", fast_track_cnt,
						a.a.width, a.a.height, a.a.x+a.rw->trel.x, a.a.y+a.rw->trel.y);
#endif
				continue;
			}
		}
		cairo_save(self->cr);
		cairo_translate(self->cr, a.rw->trel.x, a.rw->trel.y);
		a.rw->expose_event(a.rw, self->cr, &a.a);

		/* keep track of exposed parts */
		a.a.x += a.rw->trel.x;
		a.a.y += a.rw->trel.y;
#ifdef DEBUG_FASTTRACK
		fprintf(stderr, "                       #%d (%.1f x %.1f @ %.1f + %.1f\n", fast_track_cnt,
						a.a.width, a.a.height, a.a.x, a.a.y);
#endif
#ifndef FASTTRACK_INTERSECT
		memcpy(&fast_track_area, &a.a, sizeof(cairo_rectangle_t));
		fast_track_cnt++;
#else // intersects
		if (fast_track_cnt++ == 0) {
			fast_track_area.x = a.a.x;
			fast_track_area.y = a.a.y;
			fast_track_area.width = a.a.width;
			fast_track_area.height = a.a.height;
		} else {
			rect_intersection(&fast_track_area, &a.a, &fast_track_area);
			//rect_combine(&fast_track_area, &a.a, &fast_track_area);
		}
#endif

#ifdef VISIBLE_EXPOSE
		static int ftrk = 0;
		static int fcol = 0;
		ftrk = (ftrk + 1) %11;
		fcol = (fcol + 1) %17;
		cairo_rectangle (self->cr, 0, 0, a.rw->trel.width, a.rw->trel.height);
		cairo_set_operator (self->cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_rgba(self->cr, .8, .5 + ftrk/25.0, .5 - fcol/40.0, .25 + ftrk/30.0);
		cairo_fill(self->cr);
#endif

		cairo_restore(self->cr);
	}

	if (self->expose_area.width == 0 || self->expose_area.height == 0) {
#ifdef DEBUG_EXPOSURE
		fprintf(stderr, " --- NO DRAW\n");
#endif
		if (dirty) {
			cairo_surface_mark_dirty(self->surface);
		}
		return;
	}

#ifdef DEBUG_EXPOSURE
	fprintf(stderr, "XPS %.1f+%.1f  %.1fx%.1f\n", self->expose_area.x, self->expose_area.y, self->expose_area.width, self->expose_area.height);
#endif

	/* make copy and clear -- should be an atomic op when using own thread */
	cairo_rectangle_t area;
	area.x      = self->expose_area.x;
	area.y      = self->expose_area.y;
	area.width  = self->expose_area.width;
	area.height = self->expose_area.height;

	self->expose_area.x = 0;
	self->expose_area.y = 0;
	self->expose_area.width = 0;
	self->expose_area.height = 0;

#ifdef DEBUG_EXPOSURE
	fprintf(stderr, "---- XPS ---\n");
#endif

	// intersect exposure with toplevel
	cairo_rectangle_t expose_area;
	expose_area.x      = MAX(0, area.x - self->tl->area.x);
	expose_area.y      = MAX(0, area.y - self->tl->area.y);
	expose_area.width  = MIN(area.x + area.width, self->tl->area.x + self->tl->area.width) - MAX(area.x, self->tl->area.x);
	expose_area.height = MIN(area.y + area.height, self->tl->area.y + self->tl->area.height) - MAX(area.y, self->tl->area.y);

	if (expose_area.width < 0 || expose_area.height < 0) {
		fprintf(stderr, " !!! EMPTY AREA\n"); return;
	}

#define XPS_NO_DRAW { fprintf(stderr, " !!! OUTSIDE DRAW %.1fx%.1f %.1f+%.1f %.1fx%.1f\n", area.x, area.y,  self->tl->area.x, self->tl->area.y, self->tl->area.width, self->tl->area.height); return; }

#if 1
	if (area.x > self->tl->area.x + self->tl->area.width) XPS_NO_DRAW
	if (area.y > self->tl->area.y + self->tl->area.height) XPS_NO_DRAW
	if (area.x < self->tl->area.x) XPS_NO_DRAW
	if (area.y < self->tl->area.y) XPS_NO_DRAW
#endif

	cairo_save(self->cr);
	self->tl->expose_event(self->tl, self->cr, &expose_area);
	cairo_restore(self->cr);

#ifdef VISIBLE_EXPOSE
	static int move = 0;
	static int hueh = 0;
	move = (move + 1) %10;
	hueh = (hueh + 1) %13;
	cairo_rectangle (self->cr, expose_area.x, expose_area.y, expose_area.width, expose_area.height);
	cairo_set_operator (self->cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba(self->cr, .7 + hueh / 50.0, .3, .5 - hueh/30, .25 + move/20.0);
	cairo_fill(self->cr);
#endif

	cairo_surface_mark_dirty(self->surface);
}

static void queue_draw_full(RobWidget *rw) {
	GLrobtkLV2UI * const self =
		(GLrobtkLV2UI*) robwidget_get_toplevel_handle(rw);
	if (!self || !self->view) {
		rw->redraw_pending = true;
		return;
	}
#ifdef DEBUG_EXPOSURE
	fprintf(stderr, "~~ queue_draw_full '%s'\n", ROBWIDGET_NAME(rw));
#endif

	self->expose_area.x = 0;
	self->expose_area.y = 0;
	self->expose_area.width = self->width;
	self->expose_area.height = self->height;
	puglPostRedisplay(self->view);
}

static void queue_draw_area(RobWidget *rw, int x, int y, int width, int height) {
	GLrobtkLV2UI * self =
		(GLrobtkLV2UI*) robwidget_get_toplevel_handle(rw);
	if (!self || !self->view) {
		rw->redraw_pending = true;
		return;
	}

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x + width > rw->area.width) width = rw->area.width - x;
	if (y + height > rw->area.height) height = rw->area.height - y;

	if (self->expose_area.width == 0 || self->expose_area.height == 0) {
		RobTkBtnEvent ev; ev.x = x; ev.y = y;
#ifdef DEBUG_EXPOSURE
		fprintf(stderr, "Q PARTIAL NEW   '%s': ", ROBWIDGET_NAME(rw));
#endif
		offset_traverse_from_child(rw, &ev);
#ifdef DEBUG_EXPOSURE
		fprintf(stderr, "Q PARTIAL -> %d+%d  -> %d+%d (%dx%d)\n", x, y, ev.x, ev.y, width, height);
#endif
		self->expose_area.x = ev.x;
		self->expose_area.y = ev.y;
		self->expose_area.width = width;
		self->expose_area.height = height;
	} else {
		RobTkBtnEvent ev; ev.x = x; ev.y = y;
#ifdef DEBUG_EXPOSURE
		fprintf(stderr, "Q PARTIAL AMEND '%s': ", ROBWIDGET_NAME(rw));
#endif
		offset_traverse_from_child(rw, &ev);

#ifdef DEBUG_EXPOSURE
		fprintf(stderr, "Q PARTIAL -> %d+%d  -> %d+%d (%dx%d)\n", x, y, ev.x, ev.y, width, height);
#endif

		cairo_rectangle_t r;
		r.x = ev.x; r.y = ev.y;
		r.width = width; r.height = height;
		rect_combine((cairo_rectangle_t*) &self->expose_area, &r, (cairo_rectangle_t*) &self->expose_area);
	}
	puglPostRedisplay(self->view);
}

static void queue_draw(RobWidget *rw) {
#ifdef DEBUG_EXPOSURE
	fprintf(stderr, "FULL widget -> ");
#endif
	queue_draw_area(rw, 0, 0, rw->area.width, rw->area.height);
}

static void queue_tiny_rect(RobWidget *rw, cairo_rectangle_t *a) {
	if (!rw->cached_position) {
		rw->redraw_pending = true;
		queue_draw(rw);
		return;
	}
	GLrobtkLV2UI * self =
		(GLrobtkLV2UI*) robwidget_get_toplevel_handle(rw);// XXX cache ?!
	if (!self || !self->view) {
		rw->redraw_pending = true;
		return;
	}

	RWArea b;
	b.rw = rw;
	memcpy(&b.a, a, sizeof(cairo_rectangle_t));
	if (posrb_write(self->rb,  (uint8_t*) &b, sizeof(RWArea))<0) {
		queue_draw_area(rw, a->x, a->y, a->width, a->height);
	}
	puglPostRedisplay(self->view);
}

static void queue_tiny_area(RobWidget *rw, float x, float y, float w, float h) {
	cairo_rectangle_t a;
	a.x = x; a.width = w;
	a.y = y - 1;
	a.height = h + 1;
	queue_tiny_rect(rw, &a);
}

static void robwidget_layout(GLrobtkLV2UI * const self, bool setsize, bool init) {
	RobWidget * rw = self->tl;
#ifdef DEBUG_RESIZE
	printf("robwidget_layout(%s) setsize:%s init:%s\n", ROBWIDGET_NAME(self->tl),
			setsize ? "true" : "false",
			init ? "true" : "false"
			);
#endif

	int oldw = self->width;
	int oldh = self->height;
	bool size_changed = FALSE;

	int nox, noy;

	rtoplevel_scale (self->tl, self->tl->widget_scale);
	self->tl->size_request(self->tl, &nox, &noy);

	if (!init && rw->size_limit) {
		self->tl->size_limit(self->tl, &self->width, &self->height);
		if (oldw != self->width || oldh != self->height) {
			size_changed = TRUE;
		}
	} else if (setsize) {
		if (oldw != nox || oldh != noy) {
			size_changed = TRUE;
		}
		self->width = nox;
		self->height = noy;
	} else if (nox > self->width || noy > self->height) {
		enum LVGLResize rsz = plugin_scale_mode(self->ui);
		if (rsz == LVGL_ZOOM_TO_ASPECT || rsz == LVGL_LAYOUT_TO_FIT) {
			puglUpdateGeometryConstraints(self->view, nox, noy, rsz == LVGL_ZOOM_TO_ASPECT);
			return;
		}
		fprintf(stderr, "WINDOW IS SMALLER THAN MINIMUM SIZE! %d > %d h: %d > %d\n",
				nox, self->width, noy, self->height);
		if (nox > self->width) self->width = nox;
		if (noy > self->height) self->height = noy;
	} else if (nox < self->width || noy < self->height) {
		enum LVGLResize rsz = plugin_scale_mode(self->ui);
		if (rsz == LVGL_ZOOM_TO_ASPECT || rsz == LVGL_LAYOUT_TO_FIT) {
			puglUpdateGeometryConstraints(self->view, nox, noy, rsz == LVGL_ZOOM_TO_ASPECT);
		}
	}

	if (rw->size_allocate) {
		self->tl->size_allocate(rw, self->width, self->height);
	}

	rtoplevel_cache(rw, TRUE);

	if (init) {
		return;
	}

	if (setsize && size_changed) {
		self->resize_in_progress = TRUE;
		puglPostResize(self->view);
	} else {
		queue_draw_full(rw);
	}
}

// called by a widget if size changes
static void resize_self(RobWidget *rw) {

	GLrobtkLV2UI * const self =
		(GLrobtkLV2UI*) robwidget_get_toplevel_handle(rw);
	if (!self || !self->view) { return; }

#ifdef DEBUG_RESIZE
	printf("resize_self(%s)\n", ROBWIDGET_NAME(rw));
#endif
	robwidget_layout(self, TRUE, FALSE);
}

static void resize_toplevel(RobWidget *rw, int w, int h) {
	GLrobtkLV2UI * const self =
		(GLrobtkLV2UI*) robwidget_get_toplevel_handle(rw);
	if (!self || !self->view) { return; }
	self->width = w;
	self->height = h;
	resize_self(rw);
	self->resize_in_progress = TRUE;
	self->resize_toplevel = TRUE;
	puglPostResize(self->view);
}

static void relayout_toplevel(RobWidget *rw) {
	GLrobtkLV2UI * const self =
		(GLrobtkLV2UI*) robwidget_get_toplevel_handle(rw);
	if (!self || !self->view) { return; }
#ifdef TIMED_RESHAPE
	if (!self->resize_in_progress) {
		self->queue_w = self->width;
		self->queue_h = self->height;
		self->resize_in_progress = TRUE;
		self->queue_reshape = 1;
	}
#else
	self->relayout = TRUE;
#endif
	puglPostRedisplay(self->view);
}

/*****************************************************************************/
/* UI scaling  */

static void robtk_queue_scale_change (RobWidget *rw, const float ws) {
	GLrobtkLV2UI * const self =
		(GLrobtkLV2UI*) robwidget_get_toplevel_handle(rw);
	self->queue_widget_scale = ws;
	queue_draw (rw);
}

static void set_toplevel_expose_overlay(RobWidget *rw, void (*expose_event)(struct _robwidget*, cairo_t*, cairo_rectangle_t *)){
	GLrobtkLV2UI * const self = (GLrobtkLV2UI*) robwidget_get_toplevel_handle(rw);
	self->expose_overlay = expose_event;
	rw->resized = TRUE; //full re-expose
	queue_draw (rw);
}

static void robtk_expose_ui_scale (RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev) {
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_set_source_rgba (cr, 0, 0, 0, .6);
	cairo_fill (cr);

	// ui-scale buttons
	const int nbtn_col = 4;
	const int nbtn_row = 2;
	float bt_w = ev->width / (float)(nbtn_col * 2 + 1);
	float bt_h = ev->height / (float)(nbtn_row * 2 + 1);

	static const char scales[8][8] = { "100%", "110%", "115%", "120%", "125%", "150%", "175%", "200%" };
	PangoFontDescription *font = pango_font_description_from_string("Sans 24px");

	write_text_full (cr, "GUI Scaling", font, floor(ev->width * .5), floor(bt_h * .5), 0, 2, c_wht);

	pango_font_description_free (font);
	font = pango_font_description_from_string("Sans 14px");

	for (int y = 0; y < nbtn_row; ++y) {
		for (int x = 0; x < nbtn_col; ++x) {
			float x0 = floor ((1 + 2 * x) * bt_w);
			float y0 = floor ((1 + 2 * y) * bt_h);

			rounded_rectangle (cr, x0, y0, floor (bt_w), floor (bt_h), 8);
			CairoSetSouerceRGBA(c_wht);
			cairo_set_line_width(cr, 1.5);
			cairo_stroke_preserve (cr);
			cairo_set_source_rgba (cr, .2, .2, .2, 1.0);
			cairo_fill (cr);

			int pos = x + y * nbtn_col;
			write_text_full (cr, scales[pos], font, floor(x0 + bt_w * .5), floor(y0 + bt_h * .5), 0, 2, c_wht);
		}
	}
	pango_font_description_free (font);
}

static bool robtk_event_ui_scale (RobWidget* rw, RobTkBtnEvent *ev) {
	const int nbtn_col = 4;
	const int nbtn_row = 2;
	float bt_w = rw->area.width / (float)(nbtn_col * 2 + 1);
	float bt_h = rw->area.height / (float)(nbtn_row * 2 + 1);
	int xp = floor (ev->x / bt_w);
	int yp = floor (ev->y / bt_h);
	if ((xp & 1) == 0 || (yp & 1) == 0) {
		return FALSE;
	}
	const int pos = (xp - 1) / 2 + nbtn_col * (yp - 1) / 2;
	if (pos < 0 || pos >= nbtn_col * nbtn_row) {
		// possible rounding-error, bottom right corner
		return FALSE;
	}

	static const float scales[8] = { 1.0, 1.1, 1.15, 1.20, 1.25, 1.50, 1.75, 2.0 };
	robtk_queue_scale_change (rw, scales [pos]);
	return TRUE;
}

static RobWidget* robtk_tl_mousedown (RobWidget* rw, RobTkBtnEvent *ev) {
	if (rw->block_events) {
		if (robtk_event_ui_scale (rw, ev)) {
			rw->block_events = FALSE;
			set_toplevel_expose_overlay (rw, NULL);
		}
		return NULL;
	}

	RobWidget *rv = rcontainer_mousedown (rw, ev);
	if (rv) return rv;
	if (ev->button != 3) {
		return NULL;
	}

	RobWidget * c = decend_into_widget_tree(rw, ev->x, ev->y);
	if (c && c->mousedown) return NULL;

	rw->block_events = TRUE;
	set_toplevel_expose_overlay (rw, &robtk_expose_ui_scale);
	return NULL;
}

static void robwidget_toplevel_enable_scaling (RobWidget* rw) {
	assert (rw->parent == rw);
	assert (rw->mousedown == &rcontainer_mousedown);
	robwidget_set_mousedown (rw, robtk_tl_mousedown);
}


/*****************************************************************************/
/* helper functions */

static uint64_t microtime(float offset) {
	struct timespec now;
	rtk_clock_gettime(&now);

	now.tv_nsec += 1000000000 * offset;
	while (now.tv_nsec >= 1000000000) {
		now.tv_nsec -= 1000000000;
		now.tv_sec += 1;
	}
	return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static void myusleep(uint32_t usec) {
#ifdef _WIN32
	Sleep(usec / 1000);
#else
	struct timespec slp;
	slp.tv_sec = usec / 1000000;
	slp.tv_nsec = (usec % 1000000) * 1000;
	nanosleep(&slp, NULL);
#endif
}

/*****************************************************************************/

static void reallocate_canvas(GLrobtkLV2UI* self) {
#ifdef DEBUG_RESIZE
	printf("reallocate_canvas to %d x %d\n", self->width, self->height);
#endif
	self->queue_canvas_realloc = false;
	if (self->cr) {
		free (self->surf_data);
#if __BIG_ENDIAN__
		free (self->surf_data_be);
#endif
		cairo_destroy (self->cr);
	}
	opengl_reallocate_texture(self->width, self->height, &self->texture_id);
	self->cr = opengl_create_cairo_t(self->width, self->height, &self->surface, &self->surf_data);

#if __BIG_ENDIAN__
	self->surf_data_be = (unsigned char*) calloc (4 * self->width * self->height, sizeof (unsigned char));
#endif

	/* clear top window */
	cairo_save(self->cr);
	cairo_set_source_rgba (self->cr, .0, .0, .0, 1.0);
	cairo_set_operator (self->cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (self->cr, 0, 0, self->width, self->height);
	cairo_fill (self->cr);
	cairo_restore(self->cr);
}

static void
onRealReshape(PuglView* view, int width, int height)
{
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)puglGetHandle(view);
	self->resize_in_progress = FALSE;
	self->resize_toplevel = FALSE;
#ifdef DEBUG_RESIZE
	printf("onRealReshape (%s) %dx%d\n",
			ROBWIDGET_NAME(self->tl), width, height);
#endif
	switch(plugin_scale_mode(self->ui)) {
		case LVGL_LAYOUT_TO_FIT:
			self->xoff = 0; self->yoff = 0; self->xyscale = 1.0;
#ifdef DEBUG_RESIZE
			printf("onRealReshape pre-layout %dx%d -> %dx%d\n",
					self->width, self->height, width, height);
#endif
			self->width = width;
			self->height = height;
			robwidget_layout(self, FALSE, FALSE);
			self->width = self->tl->area.width;
			self->height = self->tl->area.height;
#ifdef DEBUG_RESIZE
			printf("onRealReshape post-layout %dx%d\n",
					self->width, self->height);
#endif
			reallocate_canvas(self);
			// fall-thru to scale
		case LVGL_ZOOM_TO_ASPECT:
			if (self->queue_canvas_realloc) {
				reallocate_canvas(self);
			}
			rtoplevel_cache(self->tl, TRUE); // redraw background
			if (self->width == width && self->height == height) {
	self->xoff = 0; self->yoff = 0; self->xyscale = 1.0;
	glViewport (0, 0, self->width, self->height);
			} else
			{
	reallocate_canvas(self);
	const float gl_aspect = width / (float) height;
	const float cl_aspect = self->width / (float) self->height;
	if (gl_aspect > cl_aspect) {
		self->xyscale = (float) self->height / (float) height;
		self->xoff = (width - self->width / self->xyscale)/2;
		self->yoff = (height - self->height / self->xyscale)/2;
	} else {
		self->xoff = 0;
		self->xyscale = (float) self->width / (float) width;
		self->xoff = (width - self->width / self->xyscale)/2;
		self->yoff = (height - self->height / self->xyscale)/2;
	}
	glViewport (self->xoff, self->yoff, self->width / self->xyscale, self->height / self->xyscale);
			}
			break;
		case LVGL_CENTER:
			{
	self->xyscale = 1.0;
	self->xoff = (width - self->width)/2;
	self->yoff = (height - self->height)/2;
	glViewport (self->xoff, self->yoff, self->width, self->height);

			}
			break;
		case LVGL_TOP_LEFT:
			{
	self->xoff = 0; self->yoff = 0; self->xyscale = 1.0;
	glViewport (0, (height - self->height), self->width, self->height);
			}
			break;
	}

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glOrtho (-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
	queue_draw_full(self->tl);
}

/*****************************************************************************/
/* puGL callbacks */

static void onGlInit (PuglView *view) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)puglGetHandle(view);
#ifdef DEBUG_UI
	printf("OpenGL version: %s\n", glGetString (GL_VERSION));
	printf("OpenGL vendor: %s\n", glGetString (GL_VENDOR));
	printf("OpenGL renderer: %s\n", glGetString (GL_RENDERER));
#endif
	opengl_init();
	reallocate_canvas(self);
}

static void onClose(PuglView* view) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)puglGetHandle(view);
	self->close_ui = TRUE;
}

// callback from puGL -outsize GLX context(!) when we requested a resize
static void onResize(PuglView* view, int *width, int *height, int *set_hints) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)puglGetHandle(view);
	assert(width && height);
#ifdef DEBUG_RESIZE
	printf("onResize( %d x %d  -> %d x %d)\n", *width, *height, self->width, self->height);
#endif

	if (*width != self->width || *height != self->height) {
		self->queue_canvas_realloc = true;
	}

	*width = self->width;
	*height = self->height;
	if (self->resize_toplevel) {
		*set_hints = 0;
	}

	if (self->extui) { return ; } // all taken care of already
	if (!self->resize) { return ; }

#if (defined USE_GUI_THREAD && defined HAVE_IDLE_IFACE)

	// schedule resize in parent GUI's thread (idle interface)
	self->do_the_funky_resize = TRUE;

#elif (!defined USE_GUI_THREAD && defined HAVE_IDLE_IFACE)

	// we can do that here if effectively called by the idle callback of the host
	self->resize->ui_resize(self->resize->handle, self->width, self->height);
#ifdef USE_GTK_RESIZE_HACK
	printf("GTK window hack: %d %d\n", self->width, self->height);
	guint w = self->width;
	guint h = self->height;
	if (gtk_widget_get_toplevel(GTK_WIDGET(self->resize->handle))) {
		gtk_window_resize(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(self->resize->handle))), w, h);
	}
#endif

#else
	// crash ahead:
	//self->resize->ui_resize(self->resize->handle, self->width, self->height);
#endif
}

#ifdef TIMED_RESHAPE
static void doReshape(GLrobtkLV2UI *self) {
	self->queue_reshape = 0;
	onRealReshape(self->view, self->queue_w, self->queue_h);
}
#endif

static void onFileSelected(PuglView* view, const char *filename) {
#ifdef RTK_FILE_DIRECT_CALLBACK
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)puglGetHandle(view);
	RTK_FILE_DIRECT_CALLBACK(self->ui, filename);
#else
	// TODO create port event (using urid:map)
#endif
}

static void onReshape(PuglView* view, int width, int height) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)puglGetHandle(view);
	if (!self->gl_initialized) {
		onGlInit(view);
		self->gl_initialized = true;
		onRealReshape(view, width, height);
		return;
	}

#ifdef DEBUG_RESIZE
	printf("onReshape - %d %d\n", width, height);
#endif
#ifdef TIMED_RESHAPE
	if (self->resize_in_progress) {
		self->queue_reshape = 0;
		onRealReshape(view, width, height);
	} else if (!self->queue_reshape) {
		self->queue_reshape = microtime(.08);
	}
	self->queue_w = width;
	self->queue_h = height;
#else
	onRealReshape(view, width, height);
#endif
}

static void onDisplay(PuglView* view) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)puglGetHandle(view);
	if (!self->gl_initialized) {
		onGlInit(view);
		self->gl_initialized = true;
		onRealReshape(view, self->width, self->height);
	}
#ifdef TIMED_RESHAPE
	if (self->queue_reshape) {
		uint64_t now = microtime(0);
		if (self->queue_reshape < now) {
			doReshape(self);
		}
	}
#endif

#if 1
	if (self->tl && self->queue_widget_scale != self->tl->widget_scale) {
		self->tl->widget_scale = self->queue_widget_scale;
		resize_self (self->tl);
		robwidget_resize_toplevel (self->tl, self->tl->area.width, self->tl->area.height);
	}
#endif

	if (self->resize_in_progress) { return; }
	if (!self->cr) return; // XXX exit failure

#ifndef TIMED_RESHAPE
	if (self->relayout) {
		self->relayout = FALSE;
		onRealReshape(view, self->width, self->height);
	}
#endif

#ifdef WITH_SIGNATURE
	if (!self->gpg_verified) {
		lc_expose(self);
	} else {
		cairo_expose(self);
	}
#else
	cairo_expose(self);
#endif
	cairo_surface_flush(self->surface);
#if __BIG_ENDIAN__
	int x, y;
	for (y = 0; y < self->height; ++y) {
		for (x = 0; x < self->width; ++x) {
			const int off = 4 * (y * self->width + x);
			self->surf_data_be[off + 0] = self->surf_data[off + 3];
			self->surf_data_be[off + 1] = self->surf_data[off + 2];
			self->surf_data_be[off + 2] = self->surf_data[off + 1];
			self->surf_data_be[off + 3] = self->surf_data[off + 0];
		}
	}
	opengl_draw(self->width, self->height, self->surf_data_be, self->texture_id);
#else
	opengl_draw(self->width, self->height, self->surf_data, self->texture_id);
#endif

}

#define GL_MOUSEBOUNDS \
	x = (x - self->xoff) * self->xyscale ; \
	y = (y - self->yoff) * self->xyscale ;

#define GL_MOUSEEVENT \
	RobTkBtnEvent event; \
	event.x = x - self->tl->area.x; \
	event.y = y - self->tl->area.y; \
	event.state = puglGetModifiers(view); \
	event.direction = ROBTK_SCROLL_ZERO; \
	event.button = -1;

static void onMotion(PuglView* view, int x, int y) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)puglGetHandle(view);
	assert(self->tl->mousemove);

	GL_MOUSEBOUNDS;
	GL_MOUSEEVENT;
	//fprintf(stderr, "Motion: %d,%d\n", x, y);
	if (self->mousefocus && self->mousefocus->mousemove) {
		offset_traverse_parents(self->mousefocus, &event);
		self->mousefocus = self->mousefocus->mousemove(self->mousefocus, &event);
		// proagate ?
	} else {
		self->tl->mousemove(self->tl, &event);
	}

#if 1 // do not send enter/leave events when dragging
	if (self->mousefocus || self->tl->block_events) return;
#endif

	RobWidget *fc = decend_into_widget_tree(self->tl, x, y);
	if (self->mousehover && fc != self->mousehover && self->mousehover->leave_notify) {
		self->mousehover->leave_notify(self->mousehover);
	}
	if (fc && fc != self->mousehover && fc->enter_notify) {
		fc->enter_notify(fc);
	}
	if (fc && fc->leave_notify) {
		self->mousehover = fc;
	} else {
		self->mousehover = NULL;
	}
}

static void onMouse(PuglView* view, int button, bool press, int x, int y) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)puglGetHandle(view);
#ifdef WITH_SIGNATURE
	if (!self->gpg_verified) {
		if (press) {
			if (self->gpg_shade < .3) self->gpg_shade = .3;
			else if (self->gpg_shade < .5) self->gpg_shade += .05;
			puglPostRedisplay(self->view);
		} else {
			rtk_open_url (RTK_URI);
		}
		return;
	}
#endif
	//fprintf(stderr, "Mouse %d %s at %d,%d\n", button, press ? "down" : "up", x, y);

	GL_MOUSEBOUNDS;
	GL_MOUSEEVENT;
	event.button = button;

	if (!press) {
		if (self->tl->mouseup) {
			if (self->mousefocus && self->mousefocus->mouseup) {
				offset_traverse_parents(self->mousefocus, &event);
				self->mousefocus = self->mousefocus->mouseup(self->mousefocus, &event);
			} else {
				self->mousefocus = self->tl->mouseup(self->tl, &event);
			}
		}
	} else {
		if (x > self->tl->area.x + self->tl->area.width) return;
		if (y > self->tl->area.y + self->tl->area.height) return;
		if (x < self->tl->area.x) return;
		if (y < self->tl->area.y) return;
		if (self->tl->mousedown) {
			self->mousefocus = self->tl->mousedown(self->tl, &event);
		}
	}
}

static void onScroll(PuglView* view, int x, int y, float dx, float dy) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)puglGetHandle(view);
#ifdef WITH_SIGNATURE
	if (!self->gpg_verified) return;
#endif
	self->mousefocus = NULL; // CHECK
	GL_MOUSEBOUNDS;
	GL_MOUSEEVENT;
	if (dx < 0)      event.direction = ROBTK_SCROLL_LEFT;
	else if (dx > 0) event.direction = ROBTK_SCROLL_RIGHT;
	else if (dy < 0) event.direction = ROBTK_SCROLL_DOWN;
	else if (dy > 0) event.direction = ROBTK_SCROLL_UP;
	//fprintf(stderr, "Scroll @%d+%d  %f %f\n", x, y, dx, dy);
	if (self->tl->mousescroll) self->tl->mousescroll(self->tl, & event);
}


/******************************************************************************
 * LV2 init/operation
 */

static int pugl_init(GLrobtkLV2UI* self) {
	int dflw = self->width;
	int dflh = self->height;

	if (self->tl->size_default) {
		self->tl->size_default(self->tl, &dflw, &dflh);
	}

	// Set up GL UI
	self->view = puglCreate(
			self->extui ? (PuglNativeWindow) NULL : self->parent,
			self->extui ? self->extui->plugin_human_id : RTK_URI,
			self->width, self->height,
			dflw, dflh,
#ifdef LVGL_RESIZEABLE
			true
#else
			false /* self->extui ? true : false */
#endif
			, self->ontop
			, self->transient_id
	);
	if (!self->view) {
		return -1;
	}

	puglSetHandle(self->view, self);
	puglSetDisplayFunc(self->view, onDisplay);
	puglSetReshapeFunc(self->view, onReshape);
	puglSetResizeFunc(self->view, onResize);
	puglSetFileSelectedFunc(self->view, onFileSelected);

	if (self->extui) {
		puglSetCloseFunc(self->view, onClose);
		self->ui_closed = self->extui->ui_closed;
		self->resize = NULL;
	}
	if (self->tl->mousemove) {
		puglSetMotionFunc(self->view, onMotion);
	}
	if (self->tl->mousedown || self->tl->mouseup) {
		puglSetMouseFunc(self->view, onMouse);
	}
	if (self->tl->mousescroll) {
		puglSetScrollFunc(self->view, onScroll);
	}

#ifndef XTERNAL_UI // TODO set minimum size during init in app's thread
	if (self->resize) { // XXX thread issue
		self->resize->ui_resize(self->resize->handle, self->width, self->height);
	}
#endif

	if (self->tl->size_default) {
		self->tl->size_default(self->tl, &self->width, &self->height);
		self->resize = NULL;
	}
#ifndef USE_GUI_THREAD
	ui_enable (self->ui);
#endif
	return 0;
}

static void pugl_cleanup(GLrobtkLV2UI* self) {
#ifndef USE_GUI_THREAD
	ui_disable (self->ui);
#endif
	glDeleteTextures (1, &self->texture_id); // XXX does his need glxContext ?!
	free (self->surf_data);
#if __BIG_ENDIAN__
	free (self->surf_data_be);
#endif
	cairo_destroy (self->cr);
	puglDestroy(self->view);
}

/////

/* main-thread to be called at regular intervals */
static int process_gui_events(LV2UI_Handle handle) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)handle;
	puglProcessEvents(self->view);
	if (!self->gl_initialized) {
		puglPostRedisplay(self->view);
	}
#ifdef TIMED_RESHAPE
	if (self->queue_reshape > 0) {
		puglPostRedisplay(self->view);
	}
#endif
	return 0;
}

#ifdef USE_GUI_THREAD
 // we can use idle to call 'ui_resize' in parent's thread context
static int idle(LV2UI_Handle handle) {
#ifdef HAVE_IDLE_IFACE
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)handle;

	if (self->do_the_funky_resize && self->resize) {
		self->resize->ui_resize(self->resize->handle, self->width, self->height);
#ifdef USE_GTK_RESIZE_HACK
		printf("GTK window hack (idle): %d %d\n", self->width, self->height);
		guint w = self->width;
		guint h = self->height;
		if (gtk_widget_get_toplevel(GTK_WIDGET(self->resize->handle))) {
			gtk_window_resize(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(self->resize->handle))), w, h);
		}
#endif
		self->do_the_funky_resize = FALSE;
	}

#endif
	return 0;
}

static void* ui_thread(void* handle) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)handle;
#ifdef THREADSYNC
	pthread_mutex_lock (&self->msg_thread_lock);
#endif
#ifdef INIT_PUGL_IN_THREAD
	if (pugl_init(self)) {
		self->ui_initialized = -1;
	}
	self->ui_initialized = 1;
#endif

	while (!self->exit) {
		if (self->ui_queue_puglXWindow > 0) {
			puglShowWindow(self->view);
			ui_enable(self->ui);
			self->ui_queue_puglXWindow = 0;
		}
		process_gui_events(self);
		if (self->ui_queue_puglXWindow < 0) {
			ui_disable(self->ui);
			puglHideWindow(self->view);
			self->ui_queue_puglXWindow = 0;
		}
#ifdef THREADSYNC
		//myusleep(1000000 / 60); // max FPS
		struct timespec now;
		rtk_clock_systime(&now);
		now.tv_nsec += 1000000000 / 25; // min FPS
		if (now.tv_nsec >= 1000000000) {
			now.tv_nsec -= 1000000000;
			now.tv_sec += 1;
		}
		assert(now.tv_nsec >= 0 && now.tv_nsec < 1000000000);
		pthread_cond_timedwait (&self->data_ready, &self->msg_thread_lock, &now);
#else
		myusleep(1000000 / 50); // FPS
#endif
	}
#ifdef THREADSYNC
	pthread_mutex_unlock (&self->msg_thread_lock);
#endif
#ifdef INIT_PUGL_IN_THREAD
	pugl_cleanup(self);
#endif
	return NULL;
}
#endif


/******************************************************************************
 * LV2 callbacks
 */

static LV2UI_Handle
gl_instantiate(const LV2UI_Descriptor*   descriptor,
               const char*               plugin_uri,
               const char*               bundle_path,
               LV2UI_Write_Function      write_function,
               LV2UI_Controller          controller,
               LV2UI_Widget*             widget,
               const LV2_Feature* const* features)
{
#ifdef DEBUG_UI
	printf("gl_instantiate: %s\n", plugin_uri);
#endif
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)calloc(1, sizeof(GLrobtkLV2UI));
	if (!self) {
		fprintf (stderr, "robtk: out of memory.\n");
		return NULL;
	}

	self->write      = write_function;
	self->controller = controller;
	self->view       = NULL;
	self->extui      = NULL;
	self->parent     = 0;
#ifdef DEFAULT_NOT_ONTOP
	self->ontop      = false;
#else
	self->ontop      = true;
#endif
	self->transient_id = 0;
	self->queue_widget_scale = 1.0;
	self->queue_canvas_realloc = false;

#if (defined USE_GUI_THREAD && defined THREADSYNC)
	pthread_mutex_init(&self->msg_thread_lock, NULL);
	pthread_cond_init(&self->data_ready, NULL);
#endif

	for (int i = 0; features && features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_UI__parent)) {
			self->parent = (PuglNativeWindow)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_UI__resize)) {
			self->resize = (LV2UI_Resize*)features[i]->data;
		}
#ifdef XTERNAL_UI
		else if (!strcmp(features[i]->URI, LV2_EXTERNAL_UI_URI) && !self->extui) {
			self->extui = (struct lv2_external_ui_host*) features[i]->data;
		}
		else if (!strcmp(features[i]->URI, LV2_EXTERNAL_UI_URI__KX__Host)) {
			self->extui = (struct lv2_external_ui_host*) features[i]->data;
#ifdef DEBUG_UI
		} else {
			printf("Feature: '%s'\n", features[i]->URI);
#endif
		}
#endif
	}

	if (!self->parent && !self->extui) {
		fprintf(stderr, "error: No parent window provided.\n");
		free(self);
		return NULL;
	}

	self->ui_closed = NULL;
	self->close_ui = FALSE;
	self->rb = posrb_alloc(sizeof(RWArea) * 48); // depends on plugin and threading stategy

#ifdef WITH_SIGNATURE
	self->gpg_shade = 0;
# include "gpg_check.c"
#endif

	self->tl = NULL;
	self->ui = instantiate(self,
			descriptor, plugin_uri, bundle_path,
			write_function, controller, &self->tl, features);

	if (!self->ui) {
		posrb_free(self->rb);
		free(self);
		return NULL;
	}
	if (!self->tl || !self->tl->expose_event || !self->tl->size_request) {
		posrb_free(self->rb);
		free(self);
		return NULL;
	}

	robwidget_layout(self, TRUE, TRUE);

	assert(self->width > 0 && self->height > 0);

	self->cr = NULL;
	self->surface= NULL; // not really needed, but hey
	self->surf_data = NULL; // ditto
#if __BIG_ENDIAN__
	self->surf_data_be = NULL;
#endif
	self->texture_id = 0; // already too much of this to keep valgrind happy
	self->xoff = self->yoff = 0; self->xyscale = 1.0;
	self->gl_initialized   = 0;
	self->expose_area.x = 0;
	self->expose_area.y = 0;
	self->expose_area.width = self->width;
	self->expose_area.height = self->height;
	self->mousefocus = NULL;
	self->mousehover = NULL;
	self->resize_in_progress = FALSE;
	self->resize_toplevel = FALSE;

#ifdef INIT_PUGL_IN_THREAD
	self->ui_initialized   = 0;
#endif
#ifdef TIMED_RESHAPE
	self->queue_reshape = 0;
	self->queue_w = 0;
	self->queue_h = 0;
#else
	self->relayout = FALSE;
#endif

#if (defined USE_GUI_THREAD && defined HAVE_IDLE_IFACE)
	self->do_the_funky_resize = FALSE;
#endif

#if (!defined USE_GUI_THREAD) || (!defined INIT_PUGL_IN_THREAD)
	if (pugl_init(self)) {
		return NULL;
	}
#endif

#ifdef USE_GUI_THREAD
	self->ui_queue_puglXWindow = 0;
	self->exit = false;
	pthread_create(&self->thread, NULL, ui_thread, self);
#ifdef INIT_PUGL_IN_THREAD
	while (!self->ui_initialized) {
		myusleep(1000);
		sched_yield();
	}
	if (self->ui_initialized < 0) {
		self->exit = true;
		pthread_join(self->thread, NULL);
		return NULL;
	}
#endif
#endif


#ifdef XTERNAL_UI
	if (self->extui) {
#ifdef DEBUG_UI
		printf("robtk: Xternal UI\n");
#endif
		self->xternal_ui.run  = &x_run;
		self->xternal_ui.show = &x_show;
		self->xternal_ui.hide = &x_hide;
		self->xternal_ui.self = (void*) self;
		*widget = (void*) &self->xternal_ui;
	} else
#endif
	{
#ifdef DEBUG_UI
		printf("robtk: Interal UI\n");
#endif
		*widget = (void*)puglGetNativeWindow(self->view);
	}

	return self;
}

static void gl_cleanup(LV2UI_Handle handle) {
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)handle;
#ifdef USE_GUI_THREAD
	self->exit = true;
	pthread_join(self->thread, NULL);
#endif
#if (!defined USE_GUI_THREAD) || (!defined INIT_PUGL_IN_THREAD)
	pugl_cleanup(self);
#endif
#if (defined USE_GUI_THREAD && defined THREADSYNC)
	pthread_mutex_destroy(&self->msg_thread_lock);
	pthread_cond_destroy(&self->data_ready);
#endif
	cleanup(self->ui);
	posrb_free(self->rb);
	free(self);
}

static void
gl_port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
	/* these arrive in GUI context of the parent thread
	 * -- could do some trickery here, too
	 */
	GLrobtkLV2UI* self = (GLrobtkLV2UI*)handle;
	port_event(self->ui, port_index, buffer_size, format, buffer);
#if (defined USE_GUI_THREAD && defined THREADSYNC)
	if (pthread_mutex_trylock (&self->msg_thread_lock) == 0) {
		pthread_cond_signal (&self->data_ready);
		pthread_mutex_unlock (&self->msg_thread_lock);
	}
#endif
}


/******************************************************************************
 * LV2 setup
 */

#ifdef HAVE_IDLE_IFACE
#ifdef USE_GUI_THREAD
static const LV2UI_Idle_Interface idle_iface = { idle };
#else
static const LV2UI_Idle_Interface idle_iface = { process_gui_events };
#endif
#endif

static const void*
gl_extension_data(const char* uri)
{
#ifdef HAVE_IDLE_IFACE
	if (!strcmp(uri, LV2_UI__idleInterface)) {
		return &idle_iface;
	} else
#endif
	return extension_data(uri);
}

static const LV2UI_Descriptor gl_descriptor = {
	RTK_URI RTK_GUI "_gl",
	gl_instantiate,
	gl_cleanup,
	gl_port_event,
	gl_extension_data
};

#ifdef RTK_DESCRIPTOR // standalone lv2

const LV2UI_Descriptor*
RTK_DESCRIPTOR(uint32_t index) {
	return &gl_descriptor;
}

#else

#undef LV2_SYMBOL_EXPORT
#ifdef _WIN32
#    define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#    define LV2_SYMBOL_EXPORT  __attribute__ ((visibility ("default")))
#endif
LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
#if (defined _WIN32 && defined RTK_STATIC_INIT)
	static int once = 0;
	if (!once) {once = 1; gobject_init_ctor();}
#endif
	switch (index) {
	case 0:
		return &gl_descriptor;
	default:
		return NULL;
	}
}

#if (defined _WIN32 && defined RTK_STATIC_INIT)
static void __attribute__((constructor)) x42_init() {
	pthread_win32_process_attach_np();
}

static void __attribute__((destructor)) x42_fini() {
	pthread_win32_process_detach_np();
}
#endif

#endif
