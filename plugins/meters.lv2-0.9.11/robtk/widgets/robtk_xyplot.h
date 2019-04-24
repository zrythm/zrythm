/* XY plot/drawing area
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
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

#ifndef _ROB_TK_XYP_H_
#define _ROB_TK_XYP_H_

typedef struct {
	RobWidget *rw;
	float w_width, w_height;
	cairo_surface_t* bg;

	void (*clip_cb) (cairo_t *cr, void* handle);
	void* handle;

	float line_width;
	float col[4];

	pthread_mutex_t _mutex;
	uint32_t n_points;
	uint32_t n_alloc;
	float *points_x;
	float *points_y;

	float map_x_scale;
	float map_x_offset;
	float map_y_scale;
	float map_y_offset;

	float map_x0;
	float map_xw;
	float map_y0;
	float map_yh;
} RobTkXYp;

enum RobTkXYmode {
	RobTkXY_yraw_line,
	RobTkXY_yraw_zline,
	RobTkXY_yraw_point,
	RobTkXY_yavg_line,
	RobTkXY_yavg_zline,
	RobTkXY_yavg_point,
	RobTkXY_ymax_line,
	RobTkXY_ymax_zline,
	RobTkXY_ymax_point,
};

static inline void robtk_xydraw_expose_common(RobTkXYp* d, cairo_t* cr, cairo_rectangle_t* ev) {
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	if (d->bg) {
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(cr, d->bg, 0, 0);
		cairo_paint (cr);
	} else {
		cairo_rectangle (cr, 0, 0, d->w_width, d->w_height);
		cairo_set_source_rgba(cr, 0, 0, 0, 1);
		cairo_fill(cr);
	}
	if (d->clip_cb) d->clip_cb(cr, d->handle);
}

#define GEN_DRAW_FN(ID, PROC, DRAW) \
static bool robtk_xydraw_expose_ ## ID (RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) { \
	RobTkXYp* d = (RobTkXYp *)GET_HANDLE(handle); \
	robtk_xydraw_expose_common(d, cr, ev); \
	if (pthread_mutex_trylock(&d->_mutex)) return FALSE; \
	/* localize variables */ \
	const float x0 = d->map_x0; \
	const float y0 = d->map_y0; \
	const float fx = d->map_xw * d->map_x_scale; \
	const float fy = -(d->map_yh * d->map_y_scale); \
	const float x1 = d->map_x0 + d->map_xw; \
	const float y1 = d->map_y0 + d->map_yh; \
	const float ox = d->map_x0 + d->map_x_offset * d->map_xw; \
	const float oy = y1 - d->map_y_offset * d->map_yh; \
	PROC ## _SETUP \
	DRAW ## _SETUP \
	for (uint32_t i = 0; i < d->n_points; ++i) { \
		float x = d->points_x[i] * fx + ox; \
		float y = d->points_y[i] * fy + oy; \
		float cx, cy; \
		if (x < x0) continue; \
		if (y < y0) y = y0; \
		if (x > x1) continue; \
		if (y > y1) y = y1; \
		PROC ## _PROC \
		DRAW ## _DRAW \
	} \
	if (PROC ## _POST) { \
		for (uint32_t i = d->n_points; i < d->n_points + 1; ++i) { \
			float x = -1; \
			float y = -1; \
			float cx, cy; \
			PROC ## _PROC \
			DRAW ## _DRAW \
		} \
	} \
	pthread_mutex_unlock (&d->_mutex); \
	DRAW ## _FINISH \
	return TRUE; \
}

/**** drawing routines ****/

/* line */
#define DR_LINE_SETUP {}

#define DR_LINE_DRAW \
	if (i==0) cairo_move_to(cr, cx, cy+.5);\
	else cairo_line_to(cr, cx, cy+.5);

#define DR_LINE_FINISH \
	if (d->n_points > 0) { \
		cairo_set_line_width (cr, d->line_width); \
		cairo_set_source_rgba(cr, d->col[0], d->col[1], d->col[2], d->col[3]); \
		cairo_stroke(cr); \
	}

/* line from bottom to ypos */
#define DR_ZLINE_SETUP \
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);\
	cairo_set_line_width (cr, d->line_width);\
	cairo_set_source_rgba(cr, d->col[0], d->col[1], d->col[2], d->col[3]);

#define DR_ZLINE_DRAW \
	cairo_move_to(cr, cx, cy+.5); \
	cairo_line_to(cr, cx, y1); \
	cairo_stroke(cr);

#define DR_ZLINE_FINISH {}

/* points */
#define DR_POINT_SETUP \
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);\
	cairo_set_line_width (cr, d->line_width);\
	cairo_set_source_rgba(cr, d->col[0], d->col[1], d->col[2], d->col[3]);

#define DR_POINT_DRAW \
	cairo_move_to(cr, cx, cy+.5);\
	cairo_close_path(cr);\
	cairo_stroke(cr);

#define DR_POINT_FINISH {}

/**** data pre-processing routines ****/

/* noop */
#define PR_RAW_SETUP {}

#define PR_RAW_PROC \
	cx = MAX(0, x - .5); \
	cy = y;

#define PR_RAW_POST 0

/* avg y-value for each x-pos */
#define PR_YAVG_SETUP \
	int px = -1; \
	float yavg = 0; \
	int ycnt = 0;

#define PR_YAVG_PROC \
	if (px == rint(x)) { \
		yavg += y; \
		ycnt ++; \
		continue; \
	} \
	if (ycnt > 0) { \
		cy = (yavg) / (float)(ycnt); \
	} else { \
		continue; \
	} \
	yavg = y; ycnt = 1; \
	cx = MAX(0, px - .5); \
	px = rintf(x);

#define PR_YAVG_POST 1

/* max y-value for each x-pos */
#define PR_YMAX_SETUP \
	int px = d->n_points > 0 ? d->points_x[0] * fx + ox : -1; \
	float ypeak = y1; \

#define PR_YMAX_PROC \
	if (px == rint(x)) { \
		if (ypeak > y) ypeak = y; \
		continue; \
	} \
	cy = ypeak; \
	ypeak = y; \
	cx = MAX(0, px - .5); \
	px = rintf(x);

#define PR_YMAX_POST 1


GEN_DRAW_FN(yraw_line,  PR_RAW, DR_LINE)
GEN_DRAW_FN(yraw_zline, PR_RAW, DR_ZLINE)
GEN_DRAW_FN(yraw_point, PR_RAW, DR_POINT)

GEN_DRAW_FN(yavg_line,  PR_YAVG, DR_LINE)
GEN_DRAW_FN(yavg_zline, PR_YAVG, DR_ZLINE)
GEN_DRAW_FN(yavg_point, PR_YAVG, DR_POINT)

GEN_DRAW_FN(ymax_line,  PR_YMAX, DR_LINE)
GEN_DRAW_FN(ymax_zline, PR_YMAX, DR_ZLINE)
GEN_DRAW_FN(ymax_point, PR_YMAX, DR_POINT)


/******************************************************************************
 * RobWidget stuff
 */

static void
priv_xydraw_size_request(RobWidget* handle, int *w, int *h) {
	RobTkXYp* d = (RobTkXYp*)GET_HANDLE(handle);
	*w = d->w_width;
	*h = d->w_height;
}


/******************************************************************************
 * public functions
 */

static RobTkXYp * robtk_xydraw_new(int w, int h) {
	RobTkXYp *d = (RobTkXYp *) malloc(sizeof(RobTkXYp));
	d->w_width = w;
	d->w_height = h;
	d->line_width = 1.5;

	d->clip_cb = NULL;
	d->handle = NULL;

	d->bg = NULL;
	d->n_points = 0;
	d->n_alloc = 0;
	d->points_x = NULL;
	d->points_y = NULL;

	d->map_x_scale = 1.0;
	d->map_x_offset = 0.0;
	d->map_y_scale = 1.0;
	d->map_y_offset = 0.0;
	d->map_x0 = 0;
	d->map_y0 = 0;
	d->map_xw = w;
	d->map_yh = h;

	d->col[0] =  .9;
	d->col[1] =  .3;
	d->col[2] =  .2;
	d->col[3] = 1.0;

	pthread_mutex_init (&d->_mutex, 0);
	d->rw = robwidget_new(d);
	ROBWIDGET_SETNAME(d->rw, "xydraw");
	robwidget_set_expose_event(d->rw, robtk_xydraw_expose_yraw_line);
	robwidget_set_size_request(d->rw, priv_xydraw_size_request);

	return d;
}

static void robtk_xydraw_destroy(RobTkXYp *d) {
	pthread_mutex_destroy(&d->_mutex);
	robwidget_destroy(d->rw);
	d->n_points = 0;
	d->n_alloc = 0;
	free(d->points_x);
	free(d->points_y);
	free(d);
}

static void robtk_xydraw_set_alignment(RobTkXYp *d, float x, float y) {
	robwidget_set_alignment(d->rw, x, y);
}

static void robtk_xydraw_set_linewidth(RobTkXYp *d, float lw) {
	d->line_width = lw;
}

static void robtk_xydraw_set_drawing_mode(RobTkXYp *d, int mode) {
	switch(mode) {
		default:
		case RobTkXY_yraw_line:  robwidget_set_expose_event(d->rw, robtk_xydraw_expose_yraw_line);  break;
		case RobTkXY_yraw_zline: robwidget_set_expose_event(d->rw, robtk_xydraw_expose_yraw_zline); break;
		case RobTkXY_yraw_point: robwidget_set_expose_event(d->rw, robtk_xydraw_expose_yraw_point); break;
		case RobTkXY_yavg_line:  robwidget_set_expose_event(d->rw, robtk_xydraw_expose_yavg_line);  break;
		case RobTkXY_yavg_zline: robwidget_set_expose_event(d->rw, robtk_xydraw_expose_yavg_zline); break;
		case RobTkXY_yavg_point: robwidget_set_expose_event(d->rw, robtk_xydraw_expose_yavg_point); break;
		case RobTkXY_ymax_line:  robwidget_set_expose_event(d->rw, robtk_xydraw_expose_ymax_line);  break;
		case RobTkXY_ymax_zline: robwidget_set_expose_event(d->rw, robtk_xydraw_expose_ymax_zline); break;
		case RobTkXY_ymax_point: robwidget_set_expose_event(d->rw, robtk_xydraw_expose_ymax_point); break;
	}
}

static void robtk_xydraw_set_mapping(RobTkXYp *d, float xs, float xo, float ys, float yo) {
	d->map_x_scale = xs;
	d->map_x_offset = xo;
	d->map_y_scale = ys;
	d->map_y_offset = yo;
}

static void robtk_xydraw_set_area(RobTkXYp *d, float x0, float y0, float w, float h) {
	d->map_x0 = x0;
	d->map_y0 = y0;
	d->map_xw = w;
	d->map_yh = h;
}

static void robtk_xydraw_set_clip_callback(RobTkXYp *d, void (*cb) (cairo_t* cr, void* handle), void* handle) {
	d->clip_cb = cb;
	d->handle = handle;
}

static void robtk_xydraw_set_color(RobTkXYp *d, float r, float g, float b, float a) {
	d->col[0] = r;
	d->col[1] = g;
	d->col[2] = b;
	d->col[3] = a;
}

static void robtk_xydraw_set_points(RobTkXYp *d, const uint32_t np, const float *xp, const float *yp) {
	pthread_mutex_lock (&d->_mutex);
	if (np > d->n_alloc) {
		d->points_x = (float*) realloc(d->points_x, sizeof(float) * np);
		d->points_y = (float*) realloc(d->points_y, sizeof(float) * np);
		d->n_alloc = np;
	}
	memcpy(d->points_x, xp, sizeof(float) * np);
	memcpy(d->points_y, yp, sizeof(float) * np);
	d->n_points = np;
	pthread_mutex_unlock (&d->_mutex);
	queue_draw(d->rw);
}

static void robtk_xydraw_set_surface(RobTkXYp *d, cairo_surface_t *s) {
	d->bg = s;
}

static RobWidget * robtk_xydraw_widget(RobTkXYp *d) {
	return d->rw;
}
#endif
