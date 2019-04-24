/* label widget
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

#ifndef _ROB_TK_LBL_H_
#define _ROB_TK_LBL_H_

typedef struct _RobTkLbl {
	RobWidget *rw;

	bool sensitive;
	cairo_surface_t* sf_txt;
	float w_width, w_height;
	float min_width, min_width_scaled;
	float min_height, min_height_scaled;
	char *txt;
	char *fontdesc;
	float fg[4];
	float bg[4];
	bool  rounded;
	pthread_mutex_t _mutex;
	float scale;

	void (*ttip) (struct _RobTkLbl* d, bool on, void* handle);
	void* ttip_handle;

} RobTkLbl;

static void priv_lbl_prepare_text(RobTkLbl *d, const char *txt) {
	// _mutex must be held to call this function
	int ww, wh;
	PangoFontDescription *fd;

	if (d->fontdesc) {
		fd = pango_font_description_from_string(d->fontdesc);
	} else {
		fd = get_font_from_theme();
	}

	get_text_geometry(txt, fd, &ww, &wh);

	d->w_width = ww + 4;
	d->w_height = wh + 4;

	if (d->scale != d->rw->widget_scale) {
		d->min_width_scaled = d->min_width * d->rw->widget_scale;
		d->min_height_scaled = d->min_height * d->rw->widget_scale;
	}

	d->w_width = ceil (d->w_width * d->rw->widget_scale);
	d->w_height = ceil (d->w_height * d->rw->widget_scale);
	d->scale = d->rw->widget_scale;

	if (d->w_width < d->min_width_scaled) d->w_width = d->min_width_scaled;
	if (d->w_height < d->min_height_scaled) d->w_height = d->min_height_scaled;

#ifndef GTK_BACKEND // never shrink for given scale - no jitter
	if (d->w_width > d->min_width_scaled) d->min_width_scaled = d->w_width;
	if (d->w_height > d->min_height_scaled) d->min_height_scaled = d->w_height;
#elif 0 // resize window|widget
	robwidget_hide(d->rw, false);
	robwidget_show(d->rw, true);
#endif

	create_text_surface3(&d->sf_txt,
			d->w_width, d->w_height,
			ceil (d->w_width / 2.0) + 1,
			ceil (d->w_height / 2.0) + 1,
			txt, fd, d->fg, d->rw->widget_scale);

	pango_font_description_free(fd);

	robwidget_set_size(d->rw, d->w_width, d->w_height);
	// TODO trigger re-layout  resize_self()

	queue_tiny_area(d->rw, 0, 0, d->w_width, d->w_height);
}

static bool robtk_lbl_expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	RobTkLbl* d = (RobTkLbl *)GET_HANDLE(handle);

	if (pthread_mutex_trylock (&d->_mutex)) {
		queue_draw(d->rw);
		return TRUE;
	}
	if (d->scale != d->rw->widget_scale) {
		priv_lbl_prepare_text(d, d->txt);
	}

	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	cairo_set_source_rgba (cr, d->bg[0], d->bg[1], d->bg[2], d->bg[3]);
	if (!d->rounded) {
		cairo_rectangle (cr, 0, 0, d->w_width, d->w_height);
		cairo_fill(cr);
	} else {
		rounded_rectangle(cr, 0.5, 0.5, d->w_width - 1, d->w_height - 1, C_RAD);
		cairo_fill_preserve(cr);
		cairo_set_line_width (cr, .75);
		cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
		cairo_stroke(cr);
	}

	if (d->sensitive) {
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	} else {
		cairo_set_operator (cr, CAIRO_OPERATOR_EXCLUSION);
	}
	cairo_set_source_surface(cr, d->sf_txt, 0, 0);
	cairo_paint (cr);

	pthread_mutex_unlock (&d->_mutex);
	return TRUE;
}

/******************************************************************************
 * RobWidget stuff
 */

static void
priv_lbl_size_request(RobWidget* handle, int *w, int *h) {
	RobTkLbl* d = (RobTkLbl*)GET_HANDLE(handle);
	if (d->rw->widget_scale != d->scale) {
		pthread_mutex_lock (&d->_mutex);
		priv_lbl_prepare_text(d, d->txt);
		pthread_mutex_unlock (&d->_mutex);
	}
	*w = d->w_width;
	*h = d->w_height;
}

/******************************************************************************
 * public functions
 */

static void robtk_lbl_set_text(RobTkLbl *d, const char *txt) {
	assert(txt);
	pthread_mutex_lock (&d->_mutex);
	free(d->txt);
	d->txt = strdup(txt);
	priv_lbl_prepare_text(d, d->txt);
	pthread_mutex_unlock (&d->_mutex);
}

static RobTkLbl * robtk_lbl_new(const char * txt) {
	assert(txt);
	RobTkLbl *d = (RobTkLbl *) malloc(sizeof(RobTkLbl));

	d->sf_txt = NULL;
	d->min_width_scaled = d->min_width = d->w_width = 0;
	d->min_height_scaled = d->min_height = d->w_height = 0;
	d->txt = NULL;
	d->fontdesc = NULL;
	d->sensitive = TRUE;
	d->rounded = FALSE;
	d->scale = 1.0;
	d->ttip = NULL;
	d->ttip_handle = NULL;
	pthread_mutex_init (&d->_mutex, 0);
	d->rw = robwidget_new(d);
	ROBWIDGET_SETNAME(d->rw, "label");
	robwidget_set_expose_event(d->rw, robtk_lbl_expose_event);
	robwidget_set_size_request(d->rw, priv_lbl_size_request);

	get_color_from_theme(1, d->bg);
	get_color_from_theme(0, d->fg);
	robtk_lbl_set_text(d, txt);
	return d;
}

static void robtk_lbl_destroy(RobTkLbl *d) {
	robwidget_destroy(d->rw);
	pthread_mutex_destroy(&d->_mutex);
	cairo_surface_destroy(d->sf_txt);
	free(d->txt);
	free(d->fontdesc);
	free(d);
}

static void robtk_lbl_set_alignment(RobTkLbl *d, float x, float y) {
	robwidget_set_alignment(d->rw, x, y);
}

static void robtk_lbl_set_min_geometry(RobTkLbl *d, float w, float h) {
	d->min_width = w;
	d->min_height = h;
	assert(d->txt);
	pthread_mutex_lock (&d->_mutex);
	priv_lbl_prepare_text(d, d->txt);
	pthread_mutex_unlock (&d->_mutex);
}

static RobWidget * robtk_lbl_widget(RobTkLbl *d) {
	return d->rw;
}

static void robtk_lbl_set_sensitive(RobTkLbl *d, bool s) {
	if (d->sensitive != s) {
		d->sensitive = s;
		queue_draw(d->rw);
	}
}

static void robtk_lbl_ttip_show (RobWidget *handle) {
	RobTkLbl* d = (RobTkLbl *)GET_HANDLE(handle);
	if (d->ttip) {
		d->ttip (d, true, d->ttip_handle);
	}
}

static void robtk_lbl_ttip_hide (RobWidget *handle) {
	RobTkLbl* d = (RobTkLbl *)GET_HANDLE(handle);
	if (d->ttip) {
		d->ttip (d, false, d->ttip_handle);
	}
}

static void robtk_lbl_annotation_callback(RobTkLbl *d, void (*cb) (RobTkLbl* d, bool, void* handle), void* handle) {
	d->ttip = cb;
	d->ttip_handle = handle;
	if (d->ttip) {
		robwidget_set_enter_notify(d->rw, robtk_lbl_ttip_show);
		robwidget_set_leave_notify(d->rw, robtk_lbl_ttip_hide);
	} else {
		robwidget_set_enter_notify(d->rw, NULL);
		robwidget_set_leave_notify(d->rw, NULL);
	}
}

static void robtk_lbl_set_color(RobTkLbl *d, float r, float g, float b, float a) {
	d->fg[0] = r;
	d->fg[1] = g;
	d->fg[2] = b;
	d->fg[3] = a;
	assert(d->txt);
	pthread_mutex_lock (&d->_mutex);
	priv_lbl_prepare_text(d, d->txt);
	pthread_mutex_unlock (&d->_mutex);
}

static void robtk_lbl_set_fontdesc(RobTkLbl *d, const char *fontdesc) {
	free(d->fontdesc);
	d->fontdesc = strdup(fontdesc);
	assert(d->txt);
	pthread_mutex_lock (&d->_mutex);
	priv_lbl_prepare_text(d, d->txt);
	pthread_mutex_unlock (&d->_mutex);
}

#endif
