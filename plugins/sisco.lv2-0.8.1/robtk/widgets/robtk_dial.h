/* dial widget
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

#ifndef _ROB_TK_DIAL_H_
#define _ROB_TK_DIAL_H_

/* default values used by robtk_dial_new()
 * for calling robtk_dial_new_with_size()
 */
#define GED_WIDTH 55
#define GED_HEIGHT 30
#define GED_RADIUS 10
#define GED_CX 27.5
#define GED_CY 12.5

typedef struct _RobTkDial {
	RobWidget *rw;

	float min;
	float max;
	float acc;
	float cur;
	float dfl;
	float alt;
	float base_mult;
	float scroll_mult;
	float dead_zone_delta;
	int   n_detents;
	float *detent;
	bool  constrain_to_accuracy;

	int click_state;
	int click_states;
	int click_dflt;

	float scroll_accel;
#define ACCEL_THRESH 10
	struct timespec scroll_accel_timeout;
	int scroll_accel_thresh;
	bool with_scroll_accel;

	float drag_x, drag_y, drag_c;
	bool dragging;
	bool clicking;
	bool sensitive;
	bool prelight;
	int  displaymode;

	bool (*cb) (RobWidget* w, void* handle);
	void* handle;

	void (*ann) (struct _RobTkDial* d, cairo_t *cr, void* handle);
	void* ann_handle;

	void (*touch_cb) (void*, uint32_t, bool);
	void*    touch_hd;
	uint32_t touch_id;
	bool     touching;

	cairo_pattern_t* dpat;
	cairo_surface_t* bg;
	float bg_scale;

	float w_width, w_height;
	float w_cx, w_cy;
	float w_radius;
	float *scol;
	float dcol[4][4];

	bool threesixty;

} RobTkDial;

static bool robtk_dial_expose_event (RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	RobTkDial * d = (RobTkDial *)GET_HANDLE(handle);
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);
	cairo_scale (cr, d->rw->widget_scale, d->rw->widget_scale);

	float c[4];
	get_color_from_theme(1, c);
	cairo_set_source_rgb (cr, c[0], c[1], c[2]);

	if ((d->displaymode & 16) == 0) {
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle (cr, 0, 0, d->w_width, d->w_height);
		cairo_fill(cr);
	}

	if (d->bg) {
		if (!d->sensitive) {
			cairo_set_operator (cr, CAIRO_OPERATOR_SOFT_LIGHT);
		} else {
			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		}
		cairo_save (cr);
		cairo_scale (cr, 1.0 / d->bg_scale, 1.0 / d->bg_scale);
		cairo_set_source_surface(cr, d->bg, 0, 0);
		cairo_paint (cr);
		cairo_restore (cr);
		cairo_set_source_rgb (cr, c[0], c[1], c[2]);
	}

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	if (d->sensitive) {
		cairo_set_source(cr, d->dpat);
	}
	cairo_arc (cr, d->w_cx, d->w_cy, d->w_radius, 0, 2.0 * M_PI);
	cairo_fill_preserve (cr);
	cairo_set_line_width(cr, .75);
	cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
	cairo_stroke (cr);

	if (d->sensitive && d->click_state > 0) {
		CairoSetSouerceRGBA(&d->scol[4*(d->click_state-1)]);
		cairo_arc (cr, d->w_cx, d->w_cy, d->w_radius-1, 0, 2.0 * M_PI);
		cairo_fill(cr);
	}

	if (d->sensitive) {
		CairoSetSouerceRGBA(d->dcol[0]);
	} else {
		CairoSetSouerceRGBA(d->dcol[1]);
	}

	float ang;
	if (d->threesixty) {
		ang = (.5 * M_PI) + (2.0 * M_PI) * (d->cur - d->min) / (d->max - d->min);
	} else {
		ang = (.75 * M_PI) + (1.5 * M_PI) * (d->cur - d->min) / (d->max - d->min);
	}

	if ((d->displaymode & 1) == 0) {
		/* line from center */
		cairo_set_line_width(cr, 1.5);
		cairo_move_to(cr, d->w_cx, d->w_cy);
		float wid = M_PI * 2 / 180.0;
		cairo_arc (cr, d->w_cx, d->w_cy, d->w_radius, ang-wid, ang+wid);
		cairo_stroke (cr);
	} else {
		/* dot */
		cairo_save(cr);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_translate(cr, d->w_cx, d->w_cy);
		cairo_rotate (cr, ang);

		cairo_set_line_width(cr, 3.5);
		cairo_move_to(cr, d->w_radius - 5.0, 0);
		cairo_close_path(cr);
		cairo_stroke (cr);

		if (d->displaymode & 2) {
			/* small shade in dot */
			cairo_set_source_rgba (cr, .2, .2, .2, .1);
			cairo_set_line_width(cr, 1.5);
			cairo_move_to(cr, d->w_radius - 4.75, 0);
			cairo_close_path(cr);
			cairo_stroke (cr);
		}
		cairo_restore(cr);
	}

	if ((d->displaymode & 4) && !d->threesixty) {
		cairo_set_line_width(cr, 1.5);
		CairoSetSouerceRGBA(d->dcol[3]);
		cairo_arc (cr, d->w_cx, d->w_cy, d->w_radius + 1.5, (.75 * M_PI), (2.25 * M_PI));
		cairo_stroke (cr);
		if (d->sensitive) {
			CairoSetSouerceRGBA(d->dcol[2]);
		} else {
			CairoSetSouerceRGBA(d->dcol[3]);
		}
		if (d->displaymode & 8) {
			float dfl = (.75 * M_PI) + (1.5 * M_PI) * (d->dfl - d->min) / (d->max - d->min);
			if (dfl < ang) {
				cairo_arc (cr, d->w_cx, d->w_cy, d->w_radius + 1.5, dfl, ang);
				cairo_stroke (cr);
			} else if (dfl > ang) {
				cairo_arc (cr, d->w_cx, d->w_cy, d->w_radius + 1.5, ang, dfl);
				cairo_stroke (cr);
			}
		} else {
			cairo_arc (cr, d->w_cx, d->w_cy, d->w_radius + 1.5, (.75 * M_PI), ang);
			cairo_stroke (cr);
		}
	}

	if (d->sensitive && (d->prelight || d->dragging)) {
		if (ISBRIGHT(c)) {
			cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, .15);
		} else {
			cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, .15);
		}
		cairo_arc (cr, d->w_cx, d->w_cy, d->w_radius-1, 0, 2.0 * M_PI);
		cairo_fill(cr);
		if (d->ann) d->ann(d, cr, d->ann_handle);
	}
	return TRUE;
}

static void robtk_dial_update_state(RobTkDial * d, int state) {
	if (state < 0) state = 0;
	if (state > d->click_states) state = d->click_states;
	if (state != d->click_state) {
		 d->click_state = state;
		if (d->cb) d->cb(d->rw, d->handle);
		queue_draw(d->rw);
	}
}

static void robtk_dial_update_value(RobTkDial * d, float val) {
	if (d->threesixty) {
		// TODO use fmod(), but retain accuracy constraint
		while (val < d->min) val += (d->max - d->min);
		while (val > d->max) val -= (d->max - d->min);
		assert (val >= d->min && val <= d->max);
	} else {
		if (val < d->min) val = d->min;
		if (val > d->max) val = d->max;
	}
	if (d->constrain_to_accuracy) {
		val = d->min + rintf((val-d->min) / d->acc ) * d->acc;
	}
	if (val != d->cur) {
		d->cur = val;
		if (d->cb) d->cb(d->rw, d->handle);
		queue_draw(d->rw);
	}
}

static RobWidget* robtk_dial_mousedown(RobWidget* handle, RobTkBtnEvent *ev) {
	RobTkDial * d = (RobTkDial *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }
	if (d->touch_cb) {
		d->touch_cb (d->touch_hd, d->touch_id, true);
	}
	if (ev->state & ROBTK_MOD_SHIFT) {
		robtk_dial_update_value(d, d->dfl);
		robtk_dial_update_state(d, d->click_dflt);
	} else if (ev->button == 3) {
		if (d->cur == d->dfl) {
			robtk_dial_update_value(d, d->alt);
		} else {
			d->alt = d->cur;
			robtk_dial_update_value(d, d->dfl);
		}
	} else if (ev->button == 1) {
		d->dragging = TRUE;
		d->clicking = TRUE;
		d->drag_x = ev->x;
		d->drag_y = ev->y;
		d->drag_c = d->cur;
	}
	queue_draw(d->rw);
	return handle;
}

static RobWidget* robtk_dial_mouseup(RobWidget* handle, RobTkBtnEvent *ev) {
	RobTkDial * d = (RobTkDial *)GET_HANDLE(handle);
	if (!d->sensitive) {
		d->dragging = FALSE;
		d->clicking = FALSE;
		return NULL;
	}
	d->dragging = FALSE;
	if (d->clicking) {
		robtk_dial_update_state(d, (d->click_state + 1) % (d->click_states + 1));
	}
	d->clicking = FALSE;
	if (d->touch_cb) {
		d->touch_cb (d->touch_hd, d->touch_id, false);
	}
	queue_draw(d->rw);
	return NULL;
}

static RobWidget* robtk_dial_mousemove(RobWidget* handle, RobTkBtnEvent *ev) {
	RobTkDial * d = (RobTkDial *)GET_HANDLE(handle);
	if (!d->dragging) return NULL;

	d->clicking = FALSE;
	if (!d->sensitive) {
		d->dragging = FALSE;
		queue_draw(d->rw);
		return NULL;
	}

#ifndef ABSOLUTE_DRAGGING
	const float mult = (ev->state & ROBTK_MOD_CTRL) ? (d->base_mult * 0.1) : d->base_mult;
#else
	const float mult = d->base_mult;
#endif
	float diff = ((ev->x - d->drag_x) - (ev->y - d->drag_y));
	if (diff == 0) {
		return handle;
	}

#define DRANGE(X) (!d->threesixty ? X : (d->min + fmod ((X) - d->min, (d->max - d->min))))

	for (int i = 0; i < d->n_detents; ++i) {
		const float px_deadzone = 34.f - d->n_detents; // px
		if (DRANGE(d->cur - d->detent[i]) * DRANGE(d->cur - d->detent[i] + diff * mult) < 0) {
			/* detent */
			const int tozero = DRANGE(d->cur - d->detent[i]) * mult;
			int remain = diff - tozero;
			if (abs (remain) > px_deadzone) {
				/* slow down passing the default value */
				remain += (remain > 0) ? px_deadzone * -.5 : px_deadzone * .5;
				diff = tozero + remain;
				d->dead_zone_delta = 0;
			} else {
				robtk_dial_update_value(d, d->detent[i]);
				d->dead_zone_delta = remain / px_deadzone;
				d->drag_x = ev->x;
				d->drag_y = ev->y;
				goto out;
			}
		}

		if (fabsf (rintf(DRANGE(d->cur - d->detent[i]) / mult) + d->dead_zone_delta) < 1) {
			robtk_dial_update_value(d, d->detent[i]);
			d->dead_zone_delta += diff / px_deadzone;
			d->drag_x = ev->x;
			d->drag_y = ev->y;
			goto out;
		}
	}

	if (d->constrain_to_accuracy) {
		diff = rintf(diff * mult * (d->max - d->min) / d->acc ) * d->acc;
	} else {
		diff *=  mult;
	}

	if (diff != 0) {
		d->dead_zone_delta = 0;
	}
	robtk_dial_update_value(d, d->drag_c + diff);

out:
#ifndef ABSOLUTE_DRAGGING
	if (d->drag_c != d->cur) {
		d->drag_x = ev->x;
		d->drag_y = ev->y;
		d->drag_c = d->cur;
	}
#endif

	return handle;
}

static void robtk_dial_enter_notify(RobWidget *handle) {
	RobTkDial * d = (RobTkDial *)GET_HANDLE(handle);
	if (!d->prelight) {
		d->prelight = TRUE;
		queue_draw(d->rw);
	}
}

static void robtk_dial_leave_notify(RobWidget *handle) {
	RobTkDial * d = (RobTkDial *)GET_HANDLE(handle);
	if (d->touch_cb && d->touching) {
		d->touch_cb (d->touch_hd, d->touch_id, false);
		d->touching = FALSE;
	}
	if (d->prelight) {
		d->prelight = FALSE;
		d->scroll_accel = 1.0;
		d->scroll_accel_thresh = 0;
		queue_draw(d->rw);
	}
}


static RobWidget* robtk_dial_scroll(RobWidget* handle, RobTkBtnEvent *ev) {
	RobTkDial * d = (RobTkDial *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }
	if (d->dragging) { d->dragging = FALSE; }

	if (d->with_scroll_accel) {
		struct timespec now;
		rtk_clock_gettime(&now);
		int64_t ts0 =  now.tv_sec * 1000 + now.tv_nsec / 1000000;
		int64_t ts1 =  d->scroll_accel_timeout.tv_sec * 1000 + d->scroll_accel_timeout.tv_nsec / 1000000;
		if (ts0 - ts1 < 100) {
			if (abs(d->scroll_accel_thresh) > ACCEL_THRESH && d->scroll_accel < 4) {
				d->scroll_accel += .025;
			}
		} else {
			d->scroll_accel_thresh = 0;
			d->scroll_accel = 1.0;
		}

		d->scroll_accel_timeout.tv_sec = now.tv_sec;
		d->scroll_accel_timeout.tv_nsec = now.tv_nsec;
	} else {
		d->scroll_accel_thresh = 0;
		d->scroll_accel = 1.0;
	}

	const float delta = (ev->state & ROBTK_MOD_CTRL) ? d->acc : d->scroll_mult * d->acc;

	float val = d->cur;
	switch (ev->direction) {
		case ROBTK_SCROLL_RIGHT:
		case ROBTK_SCROLL_UP:
			if (d->scroll_accel_thresh < 0) {
				d->scroll_accel_thresh = 0;
				d->scroll_accel = 1.0;
			} else if (d->scroll_accel_thresh <= ACCEL_THRESH) {
				d->scroll_accel_thresh++;
			}
			val += delta * d->scroll_accel;
			break;
		case ROBTK_SCROLL_LEFT:
		case ROBTK_SCROLL_DOWN:
			if (d->scroll_accel_thresh > 0) {
				d->scroll_accel_thresh = 0;
				d->scroll_accel = 1.0;
			} else if (d->scroll_accel_thresh >= -ACCEL_THRESH) {
				d->scroll_accel_thresh--;
			}
			val -= delta * d->scroll_accel;
			break;
		default:
			break;
	}

	if (d->touch_cb && !d->touching) {
		d->touch_cb (d->touch_hd, d->touch_id, true);
		d->touching = TRUE;
	}

	robtk_dial_update_value(d, val);
	return NULL;
}

static void create_dial_pattern(RobTkDial * d, const float c_bg[4]) {
	if (d->dpat) {
		cairo_pattern_destroy(d->dpat);
	}

	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, d->w_height);

	const float pat_left   = (d->w_cx - d->w_radius) / (float) d->w_width;
	const float pat_right  = (d->w_cx + d->w_radius) / (float) d->w_width;
	const float pat_top    = (d->w_cy - d->w_radius) / (float) d->w_height;
	const float pat_bottom = (d->w_cy + d->w_radius) / (float) d->w_height;
#define PAT_XOFF(VAL) (pat_left + 0.35 * 2.0 * d->w_radius)

	if (ISBRIGHT(c_bg)) {
		cairo_pattern_add_color_stop_rgb (pat, pat_top,    SHADE_RGB(c_bg, .95));
		cairo_pattern_add_color_stop_rgb (pat, pat_bottom, SHADE_RGB(c_bg, 2.4));
	} else {
		cairo_pattern_add_color_stop_rgb (pat, pat_top,    SHADE_RGB(c_bg, 2.4));
		cairo_pattern_add_color_stop_rgb (pat, pat_bottom, SHADE_RGB(c_bg, .95));
	}

	if (!getenv("NO_METER_SHADE") || strlen(getenv("NO_METER_SHADE")) == 0) {
		/* light from top-left */
		cairo_pattern_t* shade_pattern = cairo_pattern_create_linear (0.0, 0.0, d->w_width, 0.0);
		if (ISBRIGHT(c_bg)) {
			cairo_pattern_add_color_stop_rgba (shade_pattern, pat_left,       1.0, 1.0, 1.0, 0.15);
			cairo_pattern_add_color_stop_rgba (shade_pattern, PAT_XOFF(0.35), 0.0, 0.0, 0.0, 0.10);
			cairo_pattern_add_color_stop_rgba (shade_pattern, PAT_XOFF(0.53), 1.0, 1.0, 1.0, 0.05);
			cairo_pattern_add_color_stop_rgba (shade_pattern, pat_right,      1.0, 1.0, 1.0, 0.25);
		} else {
			cairo_pattern_add_color_stop_rgba (shade_pattern, pat_left,       0.0, 0.0, 0.0, 0.15);
			cairo_pattern_add_color_stop_rgba (shade_pattern, PAT_XOFF(0.35), 1.0, 1.0, 1.0, 0.10);
			cairo_pattern_add_color_stop_rgba (shade_pattern, PAT_XOFF(0.53), 0.0, 0.0, 0.0, 0.05);
			cairo_pattern_add_color_stop_rgba (shade_pattern, pat_right,      0.0, 0.0, 0.0, 0.25);
		}

		cairo_surface_t* surface;
		cairo_t* tc = 0;
		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, d->w_width, d->w_height);
		tc = cairo_create (surface);
		cairo_set_operator (tc, CAIRO_OPERATOR_SOURCE);
		cairo_set_source (tc, pat);
		cairo_rectangle (tc, 0, 0, d->w_width, d->w_height);
		cairo_fill (tc);
		cairo_pattern_destroy (pat);

		cairo_set_operator (tc, CAIRO_OPERATOR_OVER);
		cairo_set_source (tc, shade_pattern);
		cairo_rectangle (tc, 0, 0, d->w_width, d->w_height);
		cairo_fill (tc);
		cairo_pattern_destroy (shade_pattern);

		pat = cairo_pattern_create_for_surface (surface);
		cairo_destroy (tc);
		cairo_surface_destroy (surface);
	}

	d->dpat = pat;
}

/******************************************************************************
 * RobWidget stuff
 */

static void
robtk_dial_size_request(RobWidget* handle, int *w, int *h) {
	RobTkDial * d = (RobTkDial *)GET_HANDLE(handle);
	*w = d->w_width * d->rw->widget_scale;
	*h = d->w_height * d->rw->widget_scale;
}


/******************************************************************************
 * public functions
 */

static RobTkDial * robtk_dial_new_with_size(float min, float max, float step,
		int width, int height, float cx, float cy, float radius) {

	assert(max > min);
	assert(step > 0);
	//assert( (max - min) / step <= 2048.0); // TODO if > 250, use acceleration, mult
	assert( (max - min) / step >= 1.0);

	assert( (cx  + radius) < width);
	assert( (cx  - radius) > 0);
	assert( (cy  + radius) < height);
	assert( (cy  - radius) > 0);

	RobTkDial *d = (RobTkDial *) malloc(sizeof(RobTkDial));

	d->w_width = width; d->w_height = height;
	d->w_cx = cx; d->w_cy = cy;
	d->w_radius = radius;

	d->rw = robwidget_new(d);
	ROBWIDGET_SETNAME(d->rw, "dial");
	robwidget_set_expose_event(d->rw, robtk_dial_expose_event);
	robwidget_set_size_request(d->rw, robtk_dial_size_request);
	robwidget_set_mouseup(d->rw, robtk_dial_mouseup);
	robwidget_set_mousedown(d->rw, robtk_dial_mousedown);
	robwidget_set_mousemove(d->rw, robtk_dial_mousemove);
	robwidget_set_mousescroll(d->rw, robtk_dial_scroll);
	robwidget_set_enter_notify(d->rw, robtk_dial_enter_notify);
	robwidget_set_leave_notify(d->rw, robtk_dial_leave_notify);

	d->cb = NULL;
	d->handle = NULL;
	d->ann = NULL;
	d->ann_handle = NULL;
	d->touch_cb = NULL;
	d->touch_hd = NULL;
	d->touch_id = 0;
	d->touching = FALSE;
	d->min = min;
	d->max = max;
	d->acc = step;
	d->cur = min;
	d->dfl = min;
	d->alt = min;
	d->n_detents = 0;
	d->detent = NULL;
	d->constrain_to_accuracy = TRUE;
	d->dead_zone_delta = 0;
	d->sensitive = TRUE;
	d->prelight = FALSE;
	d->dragging = FALSE;
	d->clicking = FALSE;
	d->threesixty = FALSE;
	d->displaymode = 0;
	d->click_states = 0;
	d->click_state = 0;
	d->click_dflt = 0;
	d->drag_x = d->drag_y = 0;
	d->scroll_accel = 1.0;
	d->base_mult = (((d->max - d->min) / d->acc) < 12) ? (d->acc * 12.0 / (d->max - d->min)) : 1.0;
	d->base_mult *= 0.004; // 250px
	d->scroll_mult = 1.0;
	d->scroll_accel_thresh = 0;
	d->with_scroll_accel = true;
	rtk_clock_gettime(&d->scroll_accel_timeout);
	d->bg  = NULL;
	d->dpat = NULL;
	d->bg_scale = 1.0;

	float c_bg[4]; get_color_from_theme(1, c_bg);
	create_dial_pattern(d, c_bg);

	d->scol = (float*) malloc(3 * 4 * sizeof(float));
	d->scol[0*4] = 1.0; d->scol[0*4+1] = 0.0; d->scol[0*4+2] = 0.0; d->scol[0*4+3] = 0.2;
	d->scol[1*4] = 0.0; d->scol[1*4+1] = 1.0; d->scol[1*4+2] = 0.0; d->scol[1*4+3] = 0.2;
	d->scol[2*4] = 0.0; d->scol[2*4+1] = 0.0; d->scol[2*4+2] = 1.0; d->scol[2*4+3] = 0.25;

	float c[4]; get_color_from_theme(1, c);
	if (ISBRIGHT(c)) {
		d->dcol[0][0] = .05; d->dcol[0][1] = .05; d->dcol[0][2] = .05; d->dcol[0][3] = 1.0;
		d->dcol[1][0] = .45; d->dcol[1][1] = .45; d->dcol[1][2] = .45; d->dcol[1][3] = 0.7;
	} else {
		d->dcol[0][0] = .95; d->dcol[0][1] = .95; d->dcol[0][2] = .95; d->dcol[0][3] = 1.0;
		d->dcol[1][0] = .55; d->dcol[1][1] = .55; d->dcol[1][2] = .55; d->dcol[1][3] = 0.7;
	}

	d->dcol[2][0] = .0;  d->dcol[2][1] = .75; d->dcol[2][2] = 1.0; d->dcol[2][3] = 0.8;
	d->dcol[3][0] = .50; d->dcol[3][1] = .50; d->dcol[3][2] = .50; d->dcol[3][3] = 0.5;

	return d;
}

static RobTkDial * robtk_dial_new(float min, float max, float step) {
	return robtk_dial_new_with_size(min, max, step,
			GED_WIDTH, GED_HEIGHT, GED_CX, GED_CY, GED_RADIUS);
}

static void robtk_dial_destroy(RobTkDial *d) {
	robwidget_destroy(d->rw);
	cairo_pattern_destroy(d->dpat);
	free(d->scol);
	free(d->detent);
	free(d);
}

static void robtk_dial_set_alignment(RobTkDial *d, float x, float y) {
	robwidget_set_alignment(d->rw, x, y);
}

static RobWidget * robtk_dial_widget(RobTkDial *d) {
	return d->rw;
}

static void robtk_dial_set_callback(RobTkDial *d, bool (*cb) (RobWidget* w, void* handle), void* handle) {
	d->cb = cb;
	d->handle = handle;
}

static void robtk_dial_annotation_callback(RobTkDial *d, void (*cb) (RobTkDial* d, cairo_t *cr, void* handle), void* handle) {
	d->ann = cb;
	d->ann_handle = handle;
}

static void robtk_dial_set_touch(RobTkDial *d, void (*cb) (void*, uint32_t, bool), void* handle, uint32_t id) {
	d->touch_cb = cb;
	d->touch_hd = handle;
	d->touch_id = id;
}

static void robtk_dial_set_default(RobTkDial *d, float v) {
	if (d->constrain_to_accuracy) {
		v = d->min + rintf((v-d->min) / d->acc ) * d->acc;
	}
	assert(v >= d->min);
	assert(v <= d->max);
	d->dfl = v;
	d->alt = v;
}

static void robtk_dial_set_value(RobTkDial *d, float v) {
	robtk_dial_update_value(d, v);
}

static void robtk_dial_set_sensitive(RobTkDial *d, bool s) {
	if (d->sensitive != s) {
		d->sensitive = s;
		queue_draw(d->rw);
	}
}

static float robtk_dial_get_value(RobTkDial *d) {
	return (d->cur);
}

static void robtk_dial_enable_states(RobTkDial *d, int s) {
	if (s < 0) s = 0;
	if (s > 3) s = 3; // TODO realloc d->scol, allow N states
	d->click_states = s;
	robtk_dial_update_state(d, d->click_state % (d->click_states + 1));
}

static void robtk_dial_set_state(RobTkDial *d, int s) {
	robtk_dial_update_state(d, s);
}

static int robtk_dial_get_state(RobTkDial *d) {
	return (d->click_state);
}

static void robtk_dial_set_default_state(RobTkDial *d, int s) {
	assert(s >= 0);
	assert(s <= d->click_states);
	d->click_dflt = s;
}

static void robtk_dial_set_state_color(RobTkDial *d, int s, float r, float g, float b, float a) {
	assert(s > 0);
	assert(s <= d->click_states);
	d->scol[(s-1)*4+0] = r;
	d->scol[(s-1)*4+1] = g;
	d->scol[(s-1)*4+2] = b;
	d->scol[(s-1)*4+3] = a;
	if (d->click_state == s) {
		queue_draw(d->rw);
	}
}

static void robtk_dial_set_scroll_mult(RobTkDial *d, float v) {
	d->scroll_mult = v;
}

static void robtk_dial_set_detents(RobTkDial *d, const int n, const float *p) {
	free(d->detent);
	assert (n < 15); // XXX
	d->n_detents = n;
	d->detent = (float*) malloc(n * sizeof(float));
	memcpy(d->detent, p, n * sizeof(float));
}

static void robtk_dial_set_detent_default(RobTkDial *d, bool v) {
	free(d->detent);
	d->n_detents = 1;
	d->detent = (float*) malloc(sizeof(float));
	d->detent[0] = d->dfl;
}

static void robtk_dial_set_constained(RobTkDial *d, bool v) {
	d->constrain_to_accuracy = v;
}

static void robtk_dial_set_scaled_surface_scale(RobTkDial* d, cairo_surface_t* b, const float s) {
	d->bg = b;
	d->bg_scale = s;
}

static void robtk_dial_set_surface(RobTkDial *d, cairo_surface_t *s) {
	d->bg = s;
	d->bg_scale = 1.0;
}

static bool robtk_dial_update_range (RobTkDial *d, float min, float max, float step) {
	if (max <= min || step <= 0) return FALSE;
	if ((max - min) / step < 1.0) return FALSE;
	//assert( (max - min) / step <= 2048.0);

	d->min = min;
	d->max = max;
	d->acc = step;
	d->base_mult = (((d->max - d->min) / d->acc) < 12) ? (d->acc * 12.0 / (d->max - d->min)) : 1.0;
	d->base_mult *= 0.004;

	if (d->dfl < min) d->dfl = min;
	if (d->dfl > max) d->dfl = max;
	robtk_dial_update_value(d, d->cur);

	return TRUE;
}
#endif
