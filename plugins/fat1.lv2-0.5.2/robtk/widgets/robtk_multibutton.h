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

#ifndef _ROB_TK_MBTN_H_
#define _ROB_TK_MBTN_H_

#define MBT_LED_RADIUS (11.0)

typedef struct {
	RobWidget* rw;

	bool sensitive;
	bool prelight;

	bool (*cb) (RobWidget* w, void* handle);
	void* handle;

	int num_mode;
	int cur_mode;
	int tog_mode;
	int dfl_mode;

	cairo_pattern_t* btn_enabled;
	cairo_pattern_t* btn_inactive;
	cairo_pattern_t* btn_led;

	float w_width, w_height;
	float *c_led;
} RobTkMBtn;

/******************************************************************************
 * some helpers
 */

static void robtk_mbtn_update_mode(RobTkMBtn * d, int mode) {
	if (mode != d->cur_mode && mode >=0 && mode <= d->num_mode) {
		d->cur_mode = mode;
		if (d->cb) d->cb(d->rw, d->handle);
		queue_draw(d->rw);
	}
}

static void create_mbtn_pattern(RobTkMBtn * d) {
	float c_bg[4]; get_color_from_theme(1, c_bg);
	d->btn_inactive = cairo_pattern_create_linear (0.0, 0.0, 0.0, d->w_height);
	cairo_pattern_add_color_stop_rgb (d->btn_inactive, ISBRIGHT(c_bg) ? 0.5 : 0.0, SHADE_RGB(c_bg, 1.95));
	cairo_pattern_add_color_stop_rgb (d->btn_inactive, ISBRIGHT(c_bg) ? 0.0 : 0.5, SHADE_RGB(c_bg, 0.75));

	d->btn_enabled = cairo_pattern_create_linear (0.0, 0.0, 0.0, d->w_height);
	cairo_pattern_add_color_stop_rgb (d->btn_enabled, ISBRIGHT(c_bg) ? 0.5 : 0.0, SHADE_RGB(c_bg, .95));
	cairo_pattern_add_color_stop_rgb (d->btn_enabled, ISBRIGHT(c_bg) ? 0.0 : 0.5, SHADE_RGB(c_bg, 2.4));

	d->btn_led = cairo_pattern_create_linear (0.0, 0.0, 0.0, MBT_LED_RADIUS);
	cairo_pattern_add_color_stop_rgba (d->btn_led, 0.0, 0.0, 0.0, 0.0, 0.4);
	cairo_pattern_add_color_stop_rgba (d->btn_led, 1.0, 1.0, 1.0, 1.0 , 0.7);
}


/******************************************************************************
 * main drawing callback
 */

static bool robtk_mbtn_expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	RobTkMBtn * d = (RobTkMBtn *)GET_HANDLE(handle);
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);
	cairo_scale (cr, d->rw->widget_scale, d->rw->widget_scale);

	float led_r, led_g, led_b; // TODO consolidate with c[]

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	float c[4];
	get_color_from_theme(1, c);
	cairo_set_source_rgb (cr, c[0], c[1], c[2]);
	cairo_rectangle (cr, 0, 0, d->w_width, d->w_height);
	cairo_fill(cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	if (!d->sensitive) {
		led_r = c[0]; led_g = c[1]; led_b = c[2];
	} else {
		const int m = d->cur_mode;
		led_r = d->c_led[m*3+0]; led_g = d->c_led[m*3+1]; led_b = d->c_led[m*3+2];
	}

	if (d->cur_mode > 0) {
		cairo_set_source(cr, d->btn_enabled);
	} else if (!d->sensitive) {
		cairo_set_source_rgb (cr, c[0], c[1], c[2]);
	} else {
		cairo_set_source(cr, d->btn_inactive);
	}

	rounded_rectangle(cr, 2.5, 2.5, d->w_width - 4, d->w_height -4, C_RAD);
	cairo_fill_preserve (cr);
	if (!d->sensitive && d->cur_mode > 0) {
		cairo_set_source_rgba (cr, c[0], c[1], c[2], .6);
		cairo_fill_preserve (cr);
	}
	cairo_set_line_width (cr, .75);
	cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
	cairo_stroke(cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_save(cr);
	cairo_translate(cr, MBT_LED_RADIUS/2 + 7, d->w_height/2.0 + 1);

	cairo_set_source (cr, d->btn_led);
	cairo_arc (cr, 0, 0, MBT_LED_RADIUS/2, 0, 2 * M_PI);
	cairo_fill(cr);

	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_arc (cr, 0, 0, MBT_LED_RADIUS/2 - 2, 0, 2 * M_PI);
	cairo_fill(cr);
	cairo_set_source_rgba (cr, led_r, led_g, led_b, 1.0);
	cairo_arc (cr, 0, 0, MBT_LED_RADIUS/2 - 3, 0, 2 * M_PI);
	cairo_fill(cr);
	cairo_restore(cr);


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

static RobWidget* robtk_mbtn_mouseup(RobWidget *handle, RobTkBtnEvent *ev) {
	RobTkMBtn * d = (RobTkMBtn *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }
	if (!d->prelight) { return NULL; }
	if (ev->state & ROBTK_MOD_SHIFT) {
		robtk_mbtn_update_mode(d, d->dfl_mode);
	} else if (ev->state & ROBTK_MOD_CTRL) {
		int cur = d->cur_mode;
		robtk_mbtn_update_mode(d, d->tog_mode);
		d->tog_mode = cur;
	} else {
		robtk_mbtn_update_mode(d, (d->cur_mode + 1) % d->num_mode);
	}
	return NULL;
}

static void robtk_mbtn_enter_notify(RobWidget *handle) {
	RobTkMBtn * d = (RobTkMBtn *)GET_HANDLE(handle);
	if (!d->prelight) {
		d->prelight = TRUE;
		queue_draw(d->rw);
	}
}

static void robtk_mbtn_leave_notify(RobWidget *handle) {
	RobTkMBtn * d = (RobTkMBtn *)GET_HANDLE(handle);
	if (d->prelight) {
		d->prelight = FALSE;
		queue_draw(d->rw);
	}
}

/******************************************************************************
 * RobWidget stuff
 */

static void
priv_mbtn_size_request(RobWidget* handle, int *w, int *h) {
	RobTkMBtn* d = (RobTkMBtn*)GET_HANDLE(handle);
	*w = d->w_width * d->rw->widget_scale;
	*h = d->w_height * d->rw->widget_scale;
}


/******************************************************************************
 * public functions
 */

static RobTkMBtn * robtk_mbtn_new(int modes) {
	RobTkMBtn *d = (RobTkMBtn *) malloc(sizeof(RobTkMBtn));
	assert(modes > 1);

	d->cb = NULL;
	d->handle = NULL;
	d->sensitive = TRUE;
	d->prelight = FALSE;
	d->num_mode = modes;
	d->cur_mode = 0;
	d->tog_mode = 0;
	d->dfl_mode = 0;

	d->c_led = (float*) malloc(3 * modes * sizeof(float));
	d->c_led[0] = d->c_led[1] =  d->c_led[2] = .4;
	for (int c = 1; c < modes; ++c) {
		const float hue = /* (.0 / (modes-1.f)) + */ (c-1.f) / (modes-1.f) ;
		const float sat = .95;
		const float lum = .55;

		const float cq = lum < 0.5 ? lum * (1 + sat) : lum + sat - lum * sat;
		const float cp = 2.f * lum - cq;

		d->c_led[c*3+0] = rtk_hue2rgb(cp, cq, hue + 1.f/3.f);
		d->c_led[c*3+1] = rtk_hue2rgb(cp, cq, hue);
		d->c_led[c*3+2] = rtk_hue2rgb(cp, cq, hue - 1.f/3.f);
		//printf("h %d %.3f  -> %.3f %.3f %.3f\n", c, hue, d->c_led[c*3+0], d->c_led[c*3+1], d->c_led[c*3+2]);
	}

	int ww, wh;
	PangoFontDescription *fd = get_font_from_theme();
	get_text_geometry("", fd, &ww, &wh);
	pango_font_description_free(fd);

	d->w_width =  13 + MBT_LED_RADIUS;
	d->w_height = wh + 8;

	d->rw = robwidget_new(d);
	robwidget_set_alignment(d->rw, 0, .5);
	ROBWIDGET_SETNAME(d->rw, "mbtn");

	robwidget_set_size_request(d->rw, priv_mbtn_size_request);
	robwidget_set_expose_event(d->rw, robtk_mbtn_expose_event);
	robwidget_set_mouseup(d->rw, robtk_mbtn_mouseup);
	robwidget_set_enter_notify(d->rw, robtk_mbtn_enter_notify);
	robwidget_set_leave_notify(d->rw, robtk_mbtn_leave_notify);

	create_mbtn_pattern(d);

	return d;
}

static void robtk_mbtn_destroy(RobTkMBtn *d) {
	robwidget_destroy(d->rw);
	cairo_pattern_destroy(d->btn_enabled);
	cairo_pattern_destroy(d->btn_inactive);
	cairo_pattern_destroy(d->btn_led);
	free(d->c_led);
	free(d);
}

static void robtk_mbtn_set_alignment(RobTkMBtn *d, float x, float y) {
	robwidget_set_alignment(d->rw, x, y);
}

static RobWidget * robtk_mbtn_widget(RobTkMBtn *d) {
	return d->rw;
}

static void robtk_mbtn_set_callback(RobTkMBtn *d, bool (*cb) (RobWidget* w, void* handle), void* handle) {
	d->cb = cb;
	d->handle = handle;
}

static void robtk_mbtn_set_leds_rgb(RobTkMBtn *d, const float *c) {
	memcpy(d->c_led, c, d->num_mode * 3 * sizeof(float));
}

static void robtk_mbtn_set_default(RobTkMBtn *d, int v) {
	d->dfl_mode = v;
}

static void robtk_mbtn_set_active(RobTkMBtn *d, int v) {
	robtk_mbtn_update_mode(d, v);
}

static void robtk_mbtn_set_sensitive(RobTkMBtn *d, bool s) {
	if (d->sensitive != s) {
		d->sensitive = s;
		queue_draw(d->rw);
	}
}

static int robtk_mbtn_get_active(RobTkMBtn *d) {
	return (d->cur_mode);
}
#endif
