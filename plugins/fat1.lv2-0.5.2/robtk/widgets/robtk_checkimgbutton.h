/* radio button widget
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

#ifndef _ROB_TK_CIMGBTN_H_
#define _ROB_TK_CIMGBTN_H_

typedef struct {
	RobWidget* rw;

	bool sensitive;
	bool prelight;
	bool enabled;
	int  temporary_mode;

	bool (*cb) (RobWidget* w, void* handle);
	void* handle;

	void (*touch_cb) (void*, uint32_t, bool);
	void*    touch_hd;
	uint32_t touch_id;

	cairo_pattern_t* btn_enabled;
	cairo_pattern_t* btn_inactive;
	cairo_surface_t* sf_img_normal;
	cairo_surface_t* sf_img_enabled;

	float w_width, w_height, i_width, i_height;
} RobTkIBtn;

/******************************************************************************
 * some helpers
 */

static void robtk_ibtn_update_enabled(RobTkIBtn * d, bool enabled) {
	if (enabled != d->enabled) {
		d->enabled = enabled;
		if (d->cb) d->cb(d->rw, d->handle);
		queue_draw(d->rw);
	}
}

static void create_ibtn_pattern(RobTkIBtn * d) {
	float c_bg[4]; get_color_from_theme(1, c_bg);

	if (d->btn_inactive) cairo_pattern_destroy(d->btn_inactive);
	if (d->btn_enabled) cairo_pattern_destroy(d->btn_enabled);

	d->btn_inactive = cairo_pattern_create_linear (0.0, 0.0, 0.0, d->w_height);
	cairo_pattern_add_color_stop_rgb (d->btn_inactive, ISBRIGHT(c_bg) ? 0.5 : 0.0, SHADE_RGB(c_bg, 1.95));
	cairo_pattern_add_color_stop_rgb (d->btn_inactive, ISBRIGHT(c_bg) ? 0.0 : 0.5, SHADE_RGB(c_bg, 0.75));

	d->btn_enabled = cairo_pattern_create_linear (0.0, 0.0, 0.0, d->w_height);
	cairo_pattern_add_color_stop_rgb (d->btn_enabled, ISBRIGHT(c_bg) ? 0.5 : 0.0, SHADE_RGB(c_bg, .95));
	cairo_pattern_add_color_stop_rgb (d->btn_enabled, ISBRIGHT(c_bg) ? 0.0 : 0.5, SHADE_RGB(c_bg, 2.4));
}

/******************************************************************************
 * main drawing callback
 */

static bool robtk_ibtn_expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	RobTkIBtn * d = (RobTkIBtn *)GET_HANDLE(handle);
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	float c[4];
	get_color_from_theme(1, c);

	cairo_scale (cr, d->rw->widget_scale, d->rw->widget_scale);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	if (d->enabled) {
		cairo_set_source(cr, d->btn_enabled);
	} else if (!d->sensitive) {
		cairo_set_source_rgb (cr, c[0], c[1], c[2]);
	} else {
		cairo_set_source(cr, d->btn_inactive);
	}

	rounded_rectangle(cr, 2.5, 2.5, d->w_width - 4, d->w_height -4, C_RAD);
	cairo_fill_preserve (cr);
	if (!d->sensitive && d->enabled) {
		cairo_set_source_rgba (cr, c[0], c[1], c[2], .6);
		cairo_fill_preserve (cr);
	}
	cairo_set_line_width (cr, .75);
	cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
	cairo_stroke(cr);

	const float xalign = 5 + rint((d->w_width - 9 - d->i_width) * d->rw->xalign);
	const float yalign = 5 + rint((d->w_height - 9 - d->i_height) * d->rw->yalign);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	if (d->enabled) {
		cairo_set_source_surface(cr, d->sf_img_enabled, xalign, yalign);
	} else {
		cairo_set_source_surface(cr, d->sf_img_normal, xalign, yalign);
	}
	cairo_paint (cr);

	if (d->sensitive && d->prelight) {
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		if (ISBRIGHT(c)) {
			cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, .1);
		} else {
			cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, .1);
		}
		rounded_rectangle(cr, 2.5, 2.5, d->w_width - 4, d->w_height -4, C_RAD);
		cairo_fill_preserve(cr);
		cairo_set_line_width (cr, .75);
		cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
		cairo_stroke(cr);
	}
	return TRUE;
}


/******************************************************************************
 * UI callbacks
 */

static RobWidget* robtk_ibtn_mousedown(RobWidget *handle, RobTkBtnEvent *event) {
	RobTkIBtn * d = (RobTkIBtn *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }
	if (!d->prelight) { return NULL; }
	if (d->touch_cb && event->button == 1) {
		d->touch_cb (d->touch_hd, d->touch_id, true);
	}
	if (   ((d->temporary_mode & 1) && event->button == 3)
	    || ((d->temporary_mode & 2) && event->state & ROBTK_MOD_SHIFT)
	    || ((d->temporary_mode & 4) && event->state & ROBTK_MOD_CTRL)
		 )
	{
		robtk_ibtn_update_enabled(d, ! d->enabled);
	}
	return NULL;
}


static RobWidget* robtk_ibtn_mouseup(RobWidget *handle, RobTkBtnEvent *event) {
	RobTkIBtn * d = (RobTkIBtn *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }
	if (event->button !=1 && !((d->temporary_mode & 1) && event->button == 3)) { return NULL; }
	if (d->prelight) {
		robtk_ibtn_update_enabled(d, ! d->enabled);
	}
	if (d->touch_cb && event->button == 1) {
		d->touch_cb (d->touch_hd, d->touch_id, false);
	}
	return NULL;
}

static void robtk_ibtn_enter_notify(RobWidget *handle) {
	RobTkIBtn * d = (RobTkIBtn *)GET_HANDLE(handle);
	if (!d->prelight) {
		d->prelight = TRUE;
		queue_draw(d->rw);
	}
}

static void robtk_ibtn_leave_notify(RobWidget *handle) {
	RobTkIBtn * d = (RobTkIBtn *)GET_HANDLE(handle);
	if (d->prelight) {
		d->prelight = FALSE;
		queue_draw(d->rw);
	}
}

/******************************************************************************
 * RobWidget stuff
 */

static void
priv_ibtn_size_request(RobWidget* handle, int *w, int *h) {
	RobTkIBtn* d = (RobTkIBtn*)GET_HANDLE(handle);
	*w = (d->i_width + 9) * d->rw->widget_scale;
	*h = (d->i_height + 9) * d->rw->widget_scale;
}

static void
priv_ibtn_size_allocate(RobWidget* handle, int w, int h) {
	RobTkIBtn* d = (RobTkIBtn*)GET_HANDLE(handle);
	bool recreate_patterns = FALSE;
	if (h != d->w_height * d->rw->widget_scale) recreate_patterns = TRUE;
	d->w_width = w / d->rw->widget_scale;
	d->w_height = h / d->rw->widget_scale;
	if (recreate_patterns) create_ibtn_pattern(d);
	robwidget_set_size(handle, w, h);
}


/******************************************************************************
 * public functions
 */

static RobTkIBtn * robtk_ibtn_new(cairo_surface_t *n, cairo_surface_t *e) {
	RobTkIBtn *d = (RobTkIBtn *) malloc(sizeof(RobTkIBtn));

	d->cb = NULL;
	d->handle = NULL;
	d->touch_cb = NULL;
	d->touch_hd = NULL;
	d->touch_id = 0;
	d->sf_img_normal = n;
	d->sf_img_enabled = e;
	d->btn_enabled = NULL;
	d->btn_inactive = NULL;
	d->temporary_mode = 0;
	d->sensitive = TRUE;
	d->prelight = FALSE;
	d->enabled = FALSE;

	d->i_width = cairo_image_surface_get_width(n);
	d->i_height = cairo_image_surface_get_height(n);
	d->w_width = d->i_width + 9;
	d->w_height = d->i_height + 9;

	d->rw = robwidget_new(d);
	robwidget_set_alignment(d->rw, .5, .5);
	ROBWIDGET_SETNAME(d->rw, "ibtn");

	robwidget_set_size_request(d->rw, priv_ibtn_size_request);
	robwidget_set_size_allocate(d->rw, priv_ibtn_size_allocate);
	robwidget_set_expose_event(d->rw, robtk_ibtn_expose_event);
	robwidget_set_mousedown(d->rw, robtk_ibtn_mousedown);
	robwidget_set_mouseup(d->rw, robtk_ibtn_mouseup);
	robwidget_set_enter_notify(d->rw, robtk_ibtn_enter_notify);
	robwidget_set_leave_notify(d->rw, robtk_ibtn_leave_notify);

	create_ibtn_pattern(d);

	return d;
}

static void robtk_ibtn_destroy(RobTkIBtn *d) {
	robwidget_destroy(d->rw);
	cairo_pattern_destroy(d->btn_enabled);
	cairo_pattern_destroy(d->btn_inactive);
	free(d);
}

static void robtk_ibtn_set_alignment(RobTkIBtn *d, float x, float y) {
	robwidget_set_alignment(d->rw, x, y);
}

static RobWidget * robtk_ibtn_widget(RobTkIBtn *d) {
	return d->rw;
}

static void robtk_ibtn_set_callback(RobTkIBtn *d, bool (*cb) (RobWidget* w, void* handle), void* handle) {
	d->cb = cb;
	d->handle = handle;
}

static void robtk_ibtn_set_touch(RobTkIBtn *d, void (*cb) (void*, uint32_t, bool), void* handle, uint32_t id) {
	d->touch_cb = cb;
	d->touch_hd = handle;
	d->touch_id = id;
}

static void robtk_ibtn_set_active(RobTkIBtn *d, bool v) {
	robtk_ibtn_update_enabled(d, v);
}

static void robtk_ibtn_set_sensitive(RobTkIBtn *d, bool s) {
	if (d->sensitive != s) {
		d->sensitive = s;
		queue_draw(d->rw);
	}
}

static void robtk_ibtn_set_temporary_mode(RobTkIBtn *d, int i) {
	d->temporary_mode = i;
}

static bool robtk_ibtn_get_active(RobTkIBtn *d) {
	return (d->enabled);
}
#endif
