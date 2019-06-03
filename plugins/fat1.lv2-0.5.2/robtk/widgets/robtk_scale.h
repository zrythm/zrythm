/* scale/slider widget
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

#ifndef _ROB_TK_SCALE_H_
#define _ROB_TK_SCALE_H_

/* default values used by robtk_scale_new()
 * for calling robtk_scale_new_with_size()
 */
#define GSC_LENGTH 250
#define GSC_GIRTH 18

typedef struct {
	RobWidget *rw;

	float min;
	float max;
	float acc;
	float cur;
	float dfl;

	float drag_x, drag_y, drag_c;
	bool sensitive;
	bool prelight;

	bool (*cb) (RobWidget* w, void* handle);
	void* handle;

	void (*touch_cb) (void*, uint32_t, bool);
	void*    touch_hd;
	uint32_t touch_id;
	bool     touching;

	cairo_pattern_t* dpat;
	cairo_pattern_t* fpat;
	cairo_surface_t* bg;

	float w_width, w_height;
	bool horiz;

	char **mark_txt;
	float *mark_val;
	int    mark_cnt;
	bool mark_expose;
	PangoFontDescription *mark_font;
	float c_txt[4];
	float mark_space;

	pthread_mutex_t _mutex;

} RobTkScale;


static int robtk_scale_round_length(RobTkScale * d, float val) {
	if (d->horiz) {
		return rint((d->w_width - 8) *  (val - d->min) / (d->max - d->min));
	} else {
		return rint((d->w_height - 8) * (1.0 - (val - d->min) / (d->max - d->min)));
	}
}

static void robtk_scale_update_value(RobTkScale * d, float val) {
	if (val < d->min) val = d->min;
	if (val > d->max) val = d->max;
	if (val != d->cur) {
		float oldval = d->cur;
		d->cur = val;
		if (d->cb) d->cb(d->rw, d->handle);
		if (robtk_scale_round_length(d, oldval) != robtk_scale_round_length(d, val)) {
			val = robtk_scale_round_length(d, val);
			oldval = robtk_scale_round_length(d, oldval);
			cairo_rectangle_t rect;
			if (oldval > val) {

				if (d->horiz) {
					rect.x = 1 + val;
					rect.width = 9 + oldval - val;
					rect.y = d->rw->widget_scale * d->mark_space + 5;
					rect.height = d->w_height - 9 - d->mark_space * d->rw->widget_scale;
				} else {
					rect.x = 5;
					rect.width = d->w_width - 9 - d->mark_space * d->rw->widget_scale;
					rect.y = 1 + val;
					rect.height = 9 + oldval - val;
				}
			} else {
				if (d->horiz) {
					rect.x = 1 + oldval;
					rect.width = 9 + val - oldval;
					rect.y = d->rw->widget_scale * d->mark_space + 5;
					rect.height = d->w_height - 9 - d->mark_space * d->rw->widget_scale;
				} else {
					rect.x = 5;
					rect.width = d->w_width - 9 - d->mark_space * d->rw->widget_scale;
					rect.y = 1 + oldval;
					rect.height = 9 + val - oldval;
				}
			}
#ifdef GTK_BACKEND
			if (1 /* XXX is visible */) {
				queue_tiny_area(d->rw, rect.x, rect.y, rect.width, rect.height);
			}
#else
			if (d->rw->cached_position) {
				queue_tiny_area(d->rw, rect.x, rect.y, rect.width, rect.height);
			}
#endif
		}
	}
}

static RobWidget* robtk_scale_mousedown(RobWidget *handle, RobTkBtnEvent *event) {
	RobTkScale * d = (RobTkScale *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }
	if (d->touch_cb) {
		d->touch_cb (d->touch_hd, d->touch_id, true);
	}
	if (event->state & ROBTK_MOD_SHIFT) {
		robtk_scale_update_value(d, d->dfl);
	} else {
		d->drag_x = event->x;
		d->drag_y = event->y;
		d->drag_c = d->cur;
	}
	queue_draw(d->rw);
	return handle;
}

static RobWidget* robtk_scale_mouseup(RobWidget *handle, RobTkBtnEvent *event) {
	RobTkScale * d = (RobTkScale *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }
	d->drag_x = d->drag_y = -1;
	if (d->touch_cb) {
		d->touch_cb (d->touch_hd, d->touch_id, false);
	}
	queue_draw(d->rw);
	return NULL;
}

static RobWidget* robtk_scale_mousemove(RobWidget *handle, RobTkBtnEvent *event) {
	RobTkScale * d = (RobTkScale *)GET_HANDLE(handle);
	if (d->drag_x < 0 || d->drag_y < 0) return NULL;

	if (!d->sensitive) {
		d->drag_x = d->drag_y = -1;
		queue_draw(d->rw);
		return NULL;
	}
	float len;
	float diff;
	if (d->horiz) {
		len = d->w_width - 8;
		diff = (event->x - d->drag_x) / len;
	} else {
		len = d->w_height - 8;
		diff = (d->drag_y - event->y) / len;
	}
	diff = rint(diff * (d->max - d->min) / d->acc ) * d->acc;
	float val = d->drag_c + diff;

	/* snap to mark */
	const int snc = robtk_scale_round_length(d, val);
	// lock ?!
	for (int i = 0; i < d->mark_cnt; ++i) {
		int sn = robtk_scale_round_length(d, d->mark_val[i]);
		if (abs(sn-snc) < 3) {
			val = d->mark_val[i];
			break;
		}
	}

	robtk_scale_update_value(d, val);
	return handle;
}

static void robtk_scale_enter_notify(RobWidget *handle) {
	RobTkScale * d = (RobTkScale *)GET_HANDLE(handle);
	if (!d->prelight) {
		d->prelight = TRUE;
		queue_draw(d->rw);
	}
}

static void robtk_scale_leave_notify(RobWidget *handle) {
	RobTkScale * d = (RobTkScale *)GET_HANDLE(handle);
	if (d->touch_cb && d->touching) {
		d->touch_cb (d->touch_hd, d->touch_id, false);
		d->touching = FALSE;
	}
	if (d->prelight) {
		d->prelight = FALSE;
		queue_draw(d->rw);
	}
}

static void
robtk_scale_size_request(RobWidget* handle, int *w, int *h) {
	RobTkScale * d = (RobTkScale *)GET_HANDLE(handle);
	int rw, rh;
	if (d->horiz) {
		rh = GSC_GIRTH + (d->mark_cnt > 0 ? d->mark_space : 0);
		rh *= d->rw->widget_scale;
		rw = 250;
	} else {
		rw = GSC_GIRTH + (d->mark_cnt > 0 ? d->mark_space : 0);
		rw *= d->rw->widget_scale;
		rh = 250;
	}

	*w = d->w_width  = rw;
	*h = d->w_height = rh;
}

static void
robtk_scale_size_allocate(RobWidget* handle, int w, int h) {
	RobTkScale * d = (RobTkScale *)GET_HANDLE(handle);
	if (d->horiz) {
		d->w_width = w;
		d->w_height = d->rw->widget_scale * (GSC_GIRTH * + (d->mark_cnt > 0 ? d->mark_space : 0));
		if (d->w_height > h) {
			d->w_height = h;
		}
	} else {
		d->w_height = h;
		d->w_width = d->rw->widget_scale * (GSC_GIRTH + (d->mark_cnt > 0 ? d->mark_space : 0));
		if (d->w_width > w) {
			d->w_width = w;
		}
	}
	robwidget_set_size(handle, d->w_width, d->w_height);
	if (d->mark_cnt > 0) { d->mark_expose = TRUE; }
}

static RobWidget* robtk_scale_scroll(RobWidget *handle, RobTkBtnEvent *ev) {
	RobTkScale * d = (RobTkScale *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }

	if (!(d->drag_x < 0 || d->drag_y < 0)) {
		d->drag_x = d->drag_y = -1;
	}

	float val = d->cur;
	switch (ev->direction) {
		case ROBTK_SCROLL_RIGHT:
		case ROBTK_SCROLL_UP:
			val += d->acc;
			break;
		case ROBTK_SCROLL_LEFT:
		case ROBTK_SCROLL_DOWN:
			val -= d->acc;
			break;
		default:
			break;
	}

	if (d->touch_cb && !d->touching) {
		d->touch_cb (d->touch_hd, d->touch_id, true);
		d->touching = TRUE;
	}

	robtk_scale_update_value(d, val);
	return NULL;
}

static void create_scale_pattern(RobTkScale * d) {
	if (d->horiz) {
		d->dpat = cairo_pattern_create_linear (0.0, 0.0, 0.0, GSC_GIRTH);
	} else {
		d->dpat = cairo_pattern_create_linear (0.0, 0.0, GSC_GIRTH, 0);
	}
	cairo_pattern_add_color_stop_rgb (d->dpat, 0.0, .3, .3, .33);
	cairo_pattern_add_color_stop_rgb (d->dpat, 0.4, .5, .5, .55);
	cairo_pattern_add_color_stop_rgb (d->dpat, 1.0, .2, .2, .22);

	if (d->horiz) {
		d->fpat = cairo_pattern_create_linear (0.0, 0.0, 0.0, GSC_GIRTH);
	} else {
		d->fpat = cairo_pattern_create_linear (0.0, 0.0, GSC_GIRTH, 0);
	}
	cairo_pattern_add_color_stop_rgb (d->fpat, 0.0, .0, .0, .0);
	cairo_pattern_add_color_stop_rgb (d->fpat, 0.4,  1,  1,  1);
	cairo_pattern_add_color_stop_rgb (d->fpat, 1.0, .1, .1, .1);
}

#define SXX_W(minus) (d->w_width  + minus - d->rw->widget_scale * ((d->bg && !d->horiz) ? d->mark_space : 0))
#define SXX_H(minus) (d->w_height + minus - d->rw->widget_scale * ((d->bg &&  d->horiz) ? d->mark_space : 0))
#define SXX_T(plus)  (plus + d->rw->widget_scale * ((d->bg && d->horiz) ? d->mark_space : 0))

static void robtk_scale_render_metrics(RobTkScale* d) {
	if (d->bg) {
		cairo_surface_destroy(d->bg);
	}
	d->bg = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, d->w_width, d->w_height);
	cairo_t *cr = cairo_create (d->bg);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba (cr, .0, .0, .0, 0);
	cairo_rectangle (cr, 0, 0, d->w_width, d->w_height);
	cairo_fill (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba (cr, .7, .7, .7, 1.0);
	cairo_set_line_width (cr, 1.0);

	for (int i = 0; i < d->mark_cnt; ++i) {
		float v = 4.0 + robtk_scale_round_length(d, d->mark_val[i]);
		if (d->horiz) {
			if (d->mark_txt[i]) {
				cairo_save (cr);
				cairo_scale (cr, d->rw->widget_scale, d->rw->widget_scale);
				write_text_full(cr, d->mark_txt[i], d->mark_font, v / d->rw->widget_scale, d->rw->widget_scale, -M_PI/2, 1, d->c_txt);
				cairo_restore (cr);
			}
			cairo_move_to(cr, v+.5, SXX_T(1.5));
			cairo_line_to(cr, v+.5, SXX_T(0) + SXX_H(-.5));
		} else {
			if (d->mark_txt[i]) {
				cairo_save (cr);
				cairo_scale (cr, d->rw->widget_scale, d->rw->widget_scale);
				write_text_full(cr, d->mark_txt[i], d->mark_font, (d->w_width -2) / d->rw->widget_scale, v / d->rw->widget_scale, 0, 1, d->c_txt);
				cairo_restore (cr);
			}
			cairo_move_to(cr, 1.5, v+.5);
			cairo_line_to(cr, SXX_W(-.5) , v+.5);
		}
		cairo_stroke(cr);
	}
	cairo_destroy(cr);
}

static bool robtk_scale_expose_event (RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	RobTkScale * d = (RobTkScale *)GET_HANDLE(handle);
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	float c[4];
	get_color_from_theme(1, c);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgb (cr, c[0], c[1], c[2]);
	cairo_rectangle (cr, 0, 0, d->w_width, d->w_height);
	cairo_fill(cr);

	/* prepare tick mark surfaces */
	if (d->mark_cnt > 0 && d->mark_expose) {
		pthread_mutex_lock (&d->_mutex);
		d->mark_expose = FALSE;
		robtk_scale_render_metrics(d);
		pthread_mutex_unlock (&d->_mutex);
	}

	/* tick marks */
	if (d->bg) {
		if (!d->sensitive) {
			//cairo_set_operator (cr, CAIRO_OPERATOR_OVERLAY);
			cairo_set_operator (cr, CAIRO_OPERATOR_SOFT_LIGHT);
		} else {
			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		}
		cairo_set_source_surface(cr, d->bg, 0, 0);
		cairo_paint (cr);
	}

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	/* solid background */
	if (d->sensitive) {
		cairo_matrix_t matrix;
		cairo_matrix_init_translate(&matrix, 0.0, -SXX_T(0));
		cairo_pattern_set_matrix (d->dpat, &matrix);
		cairo_set_source(cr, d->dpat);
	} else {
		cairo_set_source_rgba (cr, .5, .5, .5, 1.0);
	}
	rounded_rectangle(cr, 4.5, SXX_T(4.5), SXX_W(-8), SXX_H(-8), C_RAD);
	cairo_fill_preserve(cr);

	if (d->sensitive) {
		cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
	} else {
		cairo_set_source_rgba (cr, .5, .5, .5, 1.0);
	}
	cairo_set_line_width(cr, .75);
	cairo_stroke_preserve (cr);
	cairo_clip (cr);


	float val = robtk_scale_round_length(d, d->cur);

	/* red area, left | top */
	if (d->sensitive) {
		cairo_set_source_rgba (cr, .5, .0, .0, .3);
	} else {
		cairo_set_source_rgba (cr, .5, .2, .2, .3);
	}
	if (d->horiz) {
		cairo_rectangle(cr, 3.0, SXX_T(5), val, SXX_H(-9));
	} else {
		cairo_rectangle(cr, 5, SXX_T(3) + val, SXX_W(-9), SXX_H(-7) - val);
	}
	cairo_fill(cr);

	/* green area, botom | right */
	if (d->sensitive) {
		cairo_set_source_rgba (cr, .0, .5, .0, .3);
	} else {
		cairo_set_source_rgba (cr, .2, .5, .2, .3);
	}
	if (d->horiz) {
		cairo_rectangle(cr, 3.0 + val, SXX_T(5), SXX_W(-7) - val, SXX_H(-9));
	} else {
		cairo_rectangle(cr, 5, SXX_T(3), SXX_W(-9), val);
	}
	cairo_fill(cr);

	/* value ring */
	if (d->sensitive) {
		cairo_matrix_t matrix;
		cairo_matrix_init_translate(&matrix, 0, -SXX_T(0));
		cairo_pattern_set_matrix (d->fpat, &matrix);
		cairo_set_source(cr, d->fpat);
	} else {
		cairo_set_source_rgba (cr, .7, .7, .7, .7);
	}
	if (d->horiz) {
		cairo_rectangle(cr, 3.0 + val, SXX_T(5), 3, SXX_H(-9));
	} else {
		cairo_rectangle(cr, 5, SXX_T(3) + val, SXX_W(-9), 3);
	}
	cairo_fill(cr);


	if (d->sensitive && (d->prelight || d->drag_x > 0)) {
		cairo_reset_clip (cr);
		cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
		cairo_clip (cr);
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, .1);
		rounded_rectangle(cr, 4.5, SXX_T(4.5), SXX_W(-8), SXX_H(-8), C_RAD);
		cairo_fill_preserve(cr);
		cairo_set_line_width(cr, .75);
		cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
		cairo_stroke(cr);
	}
	return TRUE;
}



/******************************************************************************
 * public functions
 */

static RobTkScale * robtk_scale_new_with_size(float min, float max, float step,
		int girth, int length, bool horiz) {

	assert(max > min);
	assert(step > 0);
	assert( (max - min) / step >= 1.0);

	RobTkScale *d = (RobTkScale *) malloc(sizeof(RobTkScale));

	d->mark_font = get_font_from_theme();
	get_color_from_theme(0, d->c_txt);

	pthread_mutex_init (&d->_mutex, 0);
	d->mark_space = 0.0; // XXX longest annotation text

	d->horiz = horiz;
	if (horiz) {
		d->w_width = length; d->w_height = girth;
	} else {
		d->w_width = girth; d->w_height = length;
	}

	d->rw = robwidget_new(d);
	ROBWIDGET_SETNAME(d->rw, "scale");

	d->mark_expose = FALSE;
	d->cb = NULL;
	d->handle = NULL;
	d->touch_cb = NULL;
	d->touch_hd = NULL;
	d->touch_id = 0;
	d->touching = FALSE;
	d->min = min;
	d->max = max;
	d->acc = step;
	d->cur = min;
	d->dfl = min;
	d->sensitive = TRUE;
	d->prelight = FALSE;
	d->drag_x = d->drag_y = -1;
	d->bg  = NULL;
	create_scale_pattern(d);

	d->mark_cnt = 0;
	d->mark_val = NULL;
	d->mark_txt = NULL;

	robwidget_set_size_request(d->rw, robtk_scale_size_request);
	robwidget_set_size_allocate(d->rw, robtk_scale_size_allocate);

	robwidget_set_expose_event(d->rw, robtk_scale_expose_event);
	robwidget_set_mouseup(d->rw, robtk_scale_mouseup);
	robwidget_set_mousedown(d->rw, robtk_scale_mousedown);
	robwidget_set_mousemove(d->rw, robtk_scale_mousemove);
	robwidget_set_mousescroll(d->rw, robtk_scale_scroll);
	robwidget_set_enter_notify(d->rw, robtk_scale_enter_notify);
	robwidget_set_leave_notify(d->rw, robtk_scale_leave_notify);

	return d;
}

static RobTkScale * robtk_scale_new(float min, float max, float step, bool horiz) {
	return robtk_scale_new_with_size(min, max, step, GSC_GIRTH, GSC_LENGTH, horiz);
}

static void robtk_scale_destroy(RobTkScale *d) {
	robwidget_destroy(d->rw);
	cairo_pattern_destroy(d->dpat);
	cairo_pattern_destroy(d->fpat);
	pthread_mutex_destroy(&d->_mutex);
	for (int i = 0; i < d->mark_cnt; ++i) {
		free(d->mark_txt[i]);
	}
	free(d->mark_txt);
	free(d->mark_val);
	pango_font_description_free(d->mark_font);
	free(d);
}

static RobWidget * robtk_scale_widget(RobTkScale *d) {
	return d->rw;
}

static void robtk_scale_set_callback(RobTkScale *d, bool (*cb) (RobWidget* w, void* handle), void* handle) {
	d->cb = cb;
	d->handle = handle;
}

static void robtk_scale_set_touch(RobTkScale *d, void (*cb) (void*, uint32_t, bool), void* handle, uint32_t id) {
	d->touch_cb = cb;
	d->touch_hd = handle;
	d->touch_id = id;
}

static void robtk_scale_set_value(RobTkScale *d, float v) {
	v = d->min + rint((v-d->min) / d->acc ) * d->acc;
	robtk_scale_update_value(d, v);
}

static void robtk_scale_set_sensitive(RobTkScale *d, bool s) {
	if (d->sensitive != s) {
		d->sensitive = s;
		queue_draw(d->rw);
	}
}

static float robtk_scale_get_value(RobTkScale *d) {
	return (d->cur);
}

static void robtk_scale_set_default(RobTkScale *d, float v) {
	assert(v >= d->min);
	assert(v <= d->max);
	d->dfl = v;
}

static void robtk_scale_add_mark(RobTkScale *d, float v, const char *txt) {
	int tw = 0;
	int th = 0;
	if (txt && strlen(txt)) {
		get_text_geometry(txt, d->mark_font, &tw, &th);
	}
	pthread_mutex_lock (&d->_mutex);
	if ((tw + 3) > d->mark_space) {
		d->mark_space = tw + 3;
	}
	d->mark_val = (float *) realloc(d->mark_val, sizeof(float) * (d->mark_cnt+1));
	d->mark_txt = (char **) realloc(d->mark_txt, sizeof(char*) * (d->mark_cnt+1));
	d->mark_val[d->mark_cnt] = v;
	d->mark_txt[d->mark_cnt] = txt ? strdup(txt): NULL;
	d->mark_cnt++;
	d->mark_expose = TRUE;
	pthread_mutex_unlock (&d->_mutex);
}
#endif
