/* surround meter UI
 *
 * Copyright 2016 Robin Gareus <robin@gareus.org>
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#define WITH_SPLINE_KNOB

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define LVGL_RESIZEABLE

#define RTK_URI "http://gareus.org/oss/lv2/meters#"
#define RTK_GUI "surmeterui"
#define MTR_URI RTK_URI

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#ifndef MIN
#define MIN(A,B) ( (A) < (B) ? (A) : (B) )
#endif
#ifndef MAX
#define MAX(A,B) ( (A) > (B) ? (A) : (B) )
#endif

/*************************/
enum {
	FONT_MS = 0,
	FONT_M10,
	FONT_S10,
	FONT_LAST,
};

typedef struct {
	LV2UI_Write_Function write;
	LV2UI_Controller     controller;

	RobWidget* box;
	RobWidget* tbl;
	RobTkSep*  sep_h0;

	bool disable_signals;
	bool fontcache;
	PangoFontDescription *font[FONT_LAST];
	float c_fg[4];
	float c_bg[4];

	/* main drawing */
	RobWidget*       m0;
	uint32_t         width;
	uint32_t         height;
	cairo_surface_t* sf_ann;
	cairo_pattern_t* pat_peak;
	cairo_pattern_t* pat_rms;
	bool             update_grid;
	bool             update_pat_rms;

	/* correlation pairs UI*/
	RobWidget*       m_cor[4];
	uint32_t         cor_w;
	uint32_t         cor_h;
	cairo_surface_t* sf_cor;
	bool             update_cor;
	RobTkLbl*        lbl_cor[3];
	RobTkSelect*     sel_cor_a[4];
	RobTkSelect*     sel_cor_b[4];

	/* current data */
	float peak[8];
	float rms[8];
	float cor[4];

	RobTkCBtn*       btn_peak;
	/* RMS zoom/gain */
	cairo_surface_t* sf_bg_rms;
	RobTkDial*       spn_rms_gain;
	float            rms_gain;
#ifdef WITH_SPLINE_KNOB
	RobTkDial*       spn_spline;
	RobTkDial*       spn_bulb;
	cairo_surface_t* sf_bg_spline;
	cairo_surface_t* sf_bg_bulb;
#endif

	/* settings */
	uint8_t n_chn;
	uint8_t n_cor;
	const char *nfo;
} SURui;


static float meter_deflect (const float coeff) {
	return sqrtf (coeff);
}

static float db_deflect (const float dB) {
	return meter_deflect (powf (10, .05 * dB));
}

/******************************************************************************
 * custom visuals
 */

static void initialize_font_cache(SURui* ui) {
	ui->fontcache = true;
	ui->font[FONT_MS]  = pango_font_description_from_string("Mono 9px");
	ui->font[FONT_M10] = pango_font_description_from_string("Mono 10px");
	ui->font[FONT_S10] = pango_font_description_from_string("Sans 10px");
	assert(ui->font[FONT_M10]);
	assert(ui->font[FONT_S10]);
}

static void dial_annotation_db (RobTkDial* d, cairo_t *cr, void *data) {
	SURui* ui = (SURui*) (data);
	char txt[16];
	snprintf(txt, 16, "%+5.1fdB", d->cur);

	int tw, th;
	cairo_save(cr);
	PangoLayout * pl = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(pl, ui->font[FONT_M10]);
	pango_layout_set_text(pl, txt, -1);
	pango_layout_get_pixel_size(pl, &tw, &th);
	cairo_translate (cr, d->w_width / 2, d->w_height - 3);
	cairo_translate (cr, -tw / 2.0 , -th);
	cairo_set_source_rgba (cr, .0, .0, .0, .5);
	rounded_rectangle(cr, -1, -1, tw+3, th+1, 3);
	cairo_fill(cr);
	CairoSetSouerceRGBA(c_wht);
	pango_cairo_show_layout(cr, pl);
	g_object_unref(pl);
	cairo_restore(cr);
	cairo_new_path(cr);
}

static void prepare_faceplates (SURui* ui) {
	cairo_t *cr;
	float xlp, ylp;
	static const float c_dlf[4] = {0.8, 0.8, 0.8, 1.0}; // dial faceplate

#define INIT_DIAL_SF(VAR, W, H) \
	  VAR = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2 * (W), 2 * (H)); \
	  cr = cairo_create (VAR); \
	  cairo_scale (cr, 2.0, 2.0); \
	  CairoSetSouerceRGBA(c_trs); \
	  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE); \
	  cairo_rectangle (cr, 0, 0, W, H); \
	  cairo_fill (cr); \
	  cairo_set_operator (cr, CAIRO_OPERATOR_OVER); \

#define DIALDOTS(V, XADD, YADD) \
	float ang = (-.75 * M_PI) + (1.5 * M_PI) * (V); \
	xlp = GED_CX + XADD + sinf (ang) * (GED_RADIUS + 3.0); \
	ylp = GED_CY + YADD - cosf (ang) * (GED_RADIUS + 3.0); \
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); \
	CairoSetSouerceRGBA(c_dlf); \
	cairo_set_line_width(cr, 2.5); \
	cairo_move_to(cr, rint(xlp)-.5, rint(ylp)-.5); \
	cairo_close_path(cr); \
	cairo_stroke(cr);

	INIT_DIAL_SF(ui->sf_bg_rms, GED_WIDTH + 10, GED_HEIGHT + 12);
	{ DIALDOTS(  0.0, 5.5, 4.5) }
	{ DIALDOTS(1/6.f, 5.5, 4.5) }
	{ DIALDOTS(2/6.f, 5.5, 4.5) }
	{ DIALDOTS(3/6.f, 5.5, 4.5) }
	{ DIALDOTS(4/6.f, 5.5, 4.5) }
	{ DIALDOTS(5/6.f, 5.5, 4.5) }
	{ DIALDOTS(  1.0, 5.5, 4.5) }
	write_text_full (cr, "RMS Zoom", ui->font[FONT_MS], GED_CX + 5, GED_HEIGHT + 12, 0, 5, c_dlf);

#ifdef WITH_SPLINE_KNOB
	INIT_DIAL_SF(ui->sf_bg_spline, GED_WIDTH + 10, GED_HEIGHT + 12);
	{ DIALDOTS(  0.0, 5.5, 4.5) }
	{ DIALDOTS(  1.5 / (1. + (ui->n_chn * .5) - .5), 5.5, 4.5) }
	{ DIALDOTS(  1.0, 5.5, 4.5) }
	write_text_full (cr, "Shape", ui->font[FONT_MS], GED_CX + 5, GED_HEIGHT + 12, 0, 5, c_dlf);

	INIT_DIAL_SF(ui->sf_bg_bulb, GED_WIDTH + 10, GED_HEIGHT + 12);
	{ DIALDOTS(  0.0, 5.5, 4.5) }
	{ DIALDOTS(  .71, 5.5, 4.5) }
	{ DIALDOTS(  1.0, 5.5, 4.5) }
	write_text_full (cr, "Bulb", ui->font[FONT_MS], GED_CX + 5, GED_HEIGHT + 12, 0, 5, c_dlf);
#endif

	cairo_destroy (cr);
}


/******************************************************************************
 * Helpers for Drawing
 */

static void speaker(SURui* ui, cairo_t* cr, uint32_t n) {
	cairo_save (cr);
	cairo_rotate (cr, M_PI * -.5);
	cairo_scale (cr, 1.5, 1.5);
	cairo_move_to     (cr,  2, -.5);
	cairo_rel_line_to (cr,  0, -7);
	cairo_rel_line_to (cr,  5,  5);
	cairo_rel_line_to (cr,  5,  0);
	cairo_rel_line_to (cr,  0,  5);
	cairo_rel_line_to (cr, -5,  0);
	cairo_rel_line_to (cr, -5,  5);
	cairo_rel_line_to (cr,  0, -7);
	cairo_close_path (cr);

	char txt[16];
	CairoSetSouerceRGBA (c_g60);
	cairo_fill (cr);
	cairo_restore (cr);

	sprintf (txt,"%d", n);
	write_text_full (cr, txt, ui->font[FONT_S10], 0, -10, 0, 2, ui->c_bg);
}

static cairo_pattern_t* color_pattern (const float rad, const float g, const float a) {
	cairo_pattern_t* pat = cairo_pattern_create_radial (0, 0, 0, 0, 0, rad);

	cairo_pattern_add_color_stop_rgba(pat, 0.0,                  .05, .05, .05, a);

	cairo_pattern_add_color_stop_rgba(pat, g * db_deflect(-24.5),  .0,  .0,  .8, a);
	cairo_pattern_add_color_stop_rgba(pat, g * db_deflect(-23.5),  .0,  .5,  .4, a);

	cairo_pattern_add_color_stop_rgba(pat, g * db_deflect( -9.5),  .0,  .6,  .0, a);
	cairo_pattern_add_color_stop_rgba(pat, g * db_deflect( -8.5),  .0,  .8,  .0, a);

	cairo_pattern_add_color_stop_rgba(pat, g * db_deflect( -6.3),  .1,  .9,  .1, a);
	cairo_pattern_add_color_stop_rgba(pat, g * db_deflect( -5.7),  .6,  .9,  .0, a);

	cairo_pattern_add_color_stop_rgba(pat, g * db_deflect( -3.2), .75, .75,  .0, a);
	cairo_pattern_add_color_stop_rgba(pat, g * db_deflect( -2.8),  .8,  .4,  .1, a);

	cairo_pattern_add_color_stop_rgba(pat, g * db_deflect( -1.5),  .9,  .0,  .0, a);
	cairo_pattern_add_color_stop_rgba(pat, g * db_deflect( -0.5),   1,  .1,  .0, a);
	cairo_pattern_add_color_stop_rgba(pat, g,                       1,  .1,  .0, a);
	return pat;
}

static void draw_grid (SURui* ui) {
	const double mwh = MIN (ui->width, ui->height) *.9;
	const double rad = rint (mwh / 2.0) + .5;
	const double ccx = rint (ui->width / 2.0) + .5;
	const double ccy = rint (ui->height / 2.0) + .5;

	if (ui->sf_ann) cairo_surface_destroy(ui->sf_ann);
	ui->sf_ann = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ui->width, ui->height);
	cairo_t* cr = cairo_create (ui->sf_ann);

	cairo_rectangle (cr, 0, 0, ui->width, ui->height);
	CairoSetSouerceRGBA(ui->c_bg);
	cairo_fill (cr);

	cairo_set_line_width (cr, 1.0);

	cairo_arc (cr, ccx, ccy, rad, 0, 2.0 * M_PI);
	cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
	cairo_fill_preserve (cr);
	CairoSetSouerceRGBA (c_g90);
	cairo_stroke (cr);

	cairo_translate (cr, ccx, ccy);

#if 1 // beta
	PangoFontDescription *font = pango_font_description_from_string("Mono 32px");
	write_text_full (cr, "beta-version", font, 0, 0, -.23, 2, c_g20);
	pango_font_description_free(font);
#endif

	float sc = mwh / 360.0;
	for (uint32_t i = 0; i < ui->n_chn; ++i) {
		cairo_save (cr);
		cairo_rotate (cr, (float) i * 2.0 * M_PI / (float) ui->n_chn);
		cairo_translate (cr, 0, -rad);

		cairo_scale (cr, sc, sc);
		speaker (ui, cr, i + 1);
		cairo_restore (cr);
	}

	const double dash2[] = {1.5, 3.5};
	cairo_set_dash(cr, dash2, 2, 2);
	cairo_set_operator (cr, CAIRO_OPERATOR_ADD);

	if (ui->pat_peak) cairo_pattern_destroy(ui->pat_peak);
	ui->pat_peak = color_pattern (rad, .99 * ui->rms_gain, 1.0);
	cairo_set_line_width (cr, 1.5);

#define ANNARC(dB) { \
	char txt[16]; \
	float ypos = ui->rms_gain * db_deflect (dB); \
	if (ypos > .2 && ypos < .95) { \
		sprintf (txt, "%d", dB); \
		cairo_arc (cr, 0, 0, .5 + ypos * rad, 0, 2.0 * M_PI); \
		cairo_set_source (cr, ui->pat_peak); \
		cairo_stroke (cr); \
		cairo_save (cr); \
		cairo_rotate (cr, M_PI / 3.0); \
		cairo_scale (cr, sc, sc); \
		write_text_full (cr, txt, ui->font[FONT_S10], 0, .5 + ypos * rad / sc, M_PI, -2, ui->c_fg); \
		cairo_restore (cr); \
	} \
}

	cairo_arc (cr, 0, 0, rad, 0, 2.0 * M_PI);
	cairo_clip (cr);

	ANNARC(0);
	ANNARC(-3);
	ANNARC(-6);
	ANNARC(-9);
	ANNARC(-13);
	ANNARC(-18);
	ANNARC(-24);
	ANNARC(-36);
	ANNARC(-48);

	cairo_destroy (cr);

	if (ui->pat_peak) cairo_pattern_destroy(ui->pat_peak);
	ui->pat_peak = color_pattern (rad, 1, .8);
}

static void draw_rms_pattern (SURui* ui) {
	const double mwh = MIN (ui->width, ui->height) *.9;
	const double rad = rint (mwh / 2.0) + .5;

	if (ui->pat_rms) cairo_pattern_destroy(ui->pat_rms);
	ui->pat_rms = color_pattern (rad, ui->rms_gain, .6);
	//ui->pat_rms = color_pattern (rad, 1., .6); // XXX
}

static void draw_cor_bg (SURui* ui) {
	if (ui->sf_cor) cairo_surface_destroy(ui->sf_cor);
	ui->sf_cor = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ui->cor_w, ui->cor_h);
	cairo_t* cr = cairo_create (ui->sf_cor);

	cairo_rectangle (cr, 0, 0, ui->cor_w, ui->cor_h);
	CairoSetSouerceRGBA(ui->c_bg);
	cairo_fill (cr);

	rounded_rectangle (cr, 4, 4, ui->cor_w - 8, ui->cor_h - 8, 5);
	CairoSetSouerceRGBA(c_g60);
	cairo_stroke_preserve (cr);
	CairoSetSouerceRGBA(c_g30);
	cairo_fill_preserve (cr);
	cairo_clip (cr);

	CairoSetSouerceRGBA(c_g60);
	cairo_set_line_width (cr, 1.0);

	const double dash2[] = {1.0, 2.0};
	cairo_set_dash(cr, dash2, 2, 2);

	for (uint32_t i = 1; i < 10; ++i) {
		if (i == 5) continue;
		const float px = 10.5f + rint ((ui->cor_w - 20.f) * i / 10.f);
		cairo_move_to (cr, px, 5);
		cairo_line_to (cr, px, ui->cor_h - 5);
		cairo_stroke (cr);
	}

	const float sc = ui->box->widget_scale;
	cairo_scale (cr, sc, sc);
	write_text_full (cr, "-1", ui->font[FONT_S10], 8. / sc , ui->cor_h * .5 / sc, 0, 3, ui->c_fg);
	write_text_full (cr,  "0", ui->font[FONT_S10], rintf(ui->cor_w *.5 / sc), ui->cor_h * .5 / sc, 0, 2, ui->c_fg);
	write_text_full (cr, "+1", ui->font[FONT_S10], (ui->cor_w - 8.f) / sc, ui->cor_h * .5 / sc, 0, 1, ui->c_fg);
	cairo_destroy (cr);
}

/******************************************************************************
 * Main drawing function
 */

static bool
m0_expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev) {
	SURui* ui = (SURui*)GET_HANDLE(handle);

	if (ui->update_grid) {
		draw_grid (ui);
		ui->update_grid = false;
	}
	if (ui->update_pat_rms) {
		draw_rms_pattern (ui);
		ui->update_pat_rms = false;
	}

	cairo_rectangle (cr, 0, 0, ui->width, ui->height);
	cairo_clip (cr);
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(cr, ui->sf_ann, 0, 0);
	cairo_paint (cr);

	const double mwh = MIN (ui->width, ui->height) *.9;
	const double rad = rint (mwh / 2.0) + .5;
	const double ccx = rint (ui->width / 2.0) + .5;
	const double ccy = rint (ui->height / 2.0) + .5;

	cairo_set_line_width (cr, 1.0);
	cairo_arc (cr, ccx, ccy, rad, 0, 2.0 * M_PI);
	cairo_clip (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_translate (cr, ccx, ccy);

	// pre-calculate angles
	float x[8], y[8];
	for (uint32_t i = 0; i < ui->n_chn; ++i) {
		const float ang = (float) i * 2.f * M_PI / (float) ui->n_chn;
		x[i] = rad * sin (ang);
		y[i] = rad * cos (ang);
	}

	// tangential (for splines)
#ifdef WITH_SPLINE_KNOB
	const float d_ang = robtk_dial_get_value(ui->spn_spline) / ui->n_chn;
	const float _tn = robtk_dial_get_value(ui->spn_bulb) / cos (d_ang);
#else
	const float d_ang = 2.0 / ui->n_chn;
	const float _tn = 1.0 / cos (d_ang);
#endif

	for (uint32_t i = 0; i < ui->n_chn; ++i) {
		const int n = (i + 1 + ui->n_chn) % ui->n_chn;
		const float pk0 = ui->rms[i] * ui->rms_gain;
		const float pk1 = ui->rms[n] * ui->rms_gain;

		const float a0 =  d_ang + (float)  i * 2.f * M_PI / (float) ui->n_chn;
		const float a1 = -d_ang + (float)  n * 2.f * M_PI / (float) ui->n_chn;
		const float _sa0 = rad * sin (a0) * _tn;
		const float _ca0 = rad * cos (a0) * _tn;
		const float _sa1 = rad * sin (a1) * _tn;
		const float _ca1 = rad * cos (a1) * _tn;

		{
			const float r0 = ui->rms[i] * ui->rms_gain;
			const float r1 = ui->rms[n] * ui->rms_gain;
			cairo_move_to (cr, 0, 0);
			cairo_line_to (cr, pk0 * x[i], -pk0 * y[i]);
			cairo_curve_to (cr,
					r0 * _sa0, -r0 * _ca0,
					r1 * _sa1, -r1 * _ca1,
					pk1 * x[n], -pk1 * y[n]);
			cairo_close_path (cr);
		}
	}

	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgba (cr, .6, .6, .6, .8);
	cairo_stroke_preserve (cr);
	cairo_set_source (cr, ui->pat_rms);
	cairo_fill (cr);

	if (robtk_cbtn_get_active(ui->btn_peak)) {
		const float lw = ceilf (5.f * rad / 200.f);
		const float lc =  (rad - .5 * lw) / rad;
		cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
		for (uint32_t i = 0; i < ui->n_chn; ++i) {
			float pk = ui->peak[i];
			if (pk > 1) pk = 1.0;
			pk *= lc; // rounded line overshoot

			cairo_move_to (cr, 0, 0);
			cairo_line_to (cr, pk * x[i], -pk * y[i]);

			cairo_set_line_width (cr, lw);
			cairo_set_source_rgba(cr, .1, .1, .1, .8);
			cairo_stroke_preserve (cr);
			cairo_set_source (cr, ui->pat_peak);
			cairo_set_line_width (cr, lw - 2.f);
			cairo_stroke (cr);
		}
	}

	return TRUE;
}


static bool
cor_expose_event(RobWidget* rw, cairo_t* cr, cairo_rectangle_t *ev) {
	SURui* ui = (SURui*)GET_HANDLE(rw);

	if (ui->update_cor) {
		draw_cor_bg (ui);
		ui->update_cor = false;
	}

	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	cairo_set_source_surface(cr, ui->sf_cor, 0, 0);
	cairo_paint (cr);

	const float ww = rw->area.width;
	const float hh = rw->area.height;

	rounded_rectangle (cr, 4, 4, ww - 8, hh - 8, 6);
	cairo_clip (cr);

	uint32_t pn = ui->n_chn;
	for (uint32_t cc = 0; cc < ui->n_cor; ++cc) {
		if (rw == ui->m_cor[cc]) { pn = cc; break; }
	}

	if (pn == ui->n_chn) {
		return TRUE;
	}

	cairo_set_line_width (cr, 13.0);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

	const float px = 10.5f + (ww - 20.f) * ui->cor[pn];
	cairo_move_to (cr, px, 9);
	cairo_line_to (cr, px, hh - 9);

	if (ui->cor[pn] < .35) {
		cairo_set_source_rgba(cr, .8, .1, .1, .7);
	} else if (ui->cor[pn] < .65) {
		cairo_set_source_rgba(cr, .75, .75, 0, .7);
	} else {
		cairo_set_source_rgba(cr, .1, .8, .1, .7);
	}
	cairo_stroke(cr);

	return TRUE;
}

/******************************************************************************
 * UI callbacks
 */
static bool cb_set_port (RobWidget* rw, void *data) {
	SURui* ui = (SURui*)(data);
	if (ui->disable_signals) return TRUE;

	uint32_t pn = 0;
	for (uint32_t cc = 0; cc < ui->n_chn; ++cc) {
		if (rw->self == ui->sel_cor_a[cc]) { pn = 1 + cc * 3; break; }
		if (rw->self == ui->sel_cor_b[cc]) { pn = 2 + cc * 3; break; }
	}
	const float pv = robtk_select_get_value((RobTkSelect*)rw->self);
	if (pn > 0) {
		ui->write(ui->controller, pn, sizeof(float), 0, (const void*) &pv);
	}
	return TRUE;
}

static bool cb_set_rms_gain (RobWidget* handle, void *data) {
	SURui* ui = (SURui*)(data);
	const float val = robtk_dial_get_value(ui->spn_rms_gain);
	ui->rms_gain = db_deflect (val);
	ui->update_grid = true;
	ui->update_pat_rms = true;
	queue_draw (ui->m0);
	if (ui->disable_signals) return TRUE;
	ui->write(ui->controller, 0, sizeof(float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_queue_draw (RobWidget* rw, void *data) {
	SURui* ui = (SURui*)(data);
	queue_draw (ui->m0);
	return TRUE;
}

#ifdef WITH_SPLINE_KNOB
static bool cb_set_spline (RobWidget* handle, void *data) {
	SURui* ui = (SURui*)(data);
	queue_draw (ui->m0);
	return TRUE;
}
#endif

/******************************************************************************
 * widget sizes
 */

static void
m0_size_request(RobWidget* handle, int *w, int *h) {
	*w = 400;
	*h = 400;
}

static void
m0_size_allocate(RobWidget* rw, int w, int h) {
	SURui* ui = (SURui*)GET_HANDLE(rw);
	ui->width = w;
	ui->height = h;
	ui->update_grid = true;
	ui->update_pat_rms = true;
	robwidget_set_size(rw, w, h);
	queue_draw(rw);
}

static void
cor_size_request(RobWidget* handle, int *w, int *h) {
	*w = 80;
	*h = 28;
}

static void
cor_size_allocate(RobWidget* rw, int w, int h) {
	SURui* ui = (SURui*)GET_HANDLE(rw);
	ui->cor_w = w;
	ui->cor_h = h;
	ui->update_cor = true;
	robwidget_set_size(rw, w, h);
	queue_draw(rw);
}

/******************************************************************************
 * LV2 callbacks
 */

static void ui_enable(LV2UI_Handle handle) { }
static void ui_disable(LV2UI_Handle handle) { }

static LV2UI_Handle
instantiate(
		void* const               ui_toplevel,
		const LV2UI_Descriptor*   descriptor,
		const char*               plugin_uri,
		const char*               bundle_path,
		LV2UI_Write_Function      write_function,
		LV2UI_Controller          controller,
		RobWidget**               widget,
		const LV2_Feature* const* features)
{
	SURui* ui = (SURui*)calloc(1,sizeof(SURui));
	ui->write      = write_function;
	ui->controller = controller;

	*widget = NULL;

	if      (!strcmp(plugin_uri, MTR_URI "surround8")) { ui->n_chn = 8; }
	else if (!strcmp(plugin_uri, MTR_URI "surround7")) { ui->n_chn = 7; }
	else if (!strcmp(plugin_uri, MTR_URI "surround6")) { ui->n_chn = 6; }
	else if (!strcmp(plugin_uri, MTR_URI "surround5")) { ui->n_chn = 5; }
	else if (!strcmp(plugin_uri, MTR_URI "surround4")) { ui->n_chn = 4; }
	else if (!strcmp(plugin_uri, MTR_URI "surround3")) { ui->n_chn = 3; }
	else {
		free(ui);
		return NULL;
	}

	ui->n_cor = ui->n_chn > 3 ? 4 : 3;
	ui->nfo = robtk_info(ui_toplevel);
	ui->width  = 400;
	ui->height = 400;
	ui->rms_gain = 1.0;
	ui->update_grid = true;
	ui->update_pat_rms = true;
	ui->update_cor = false;
	get_color_from_theme(0, ui->c_fg);
	get_color_from_theme(1, ui->c_bg);

	ui->box = rob_vbox_new(FALSE, 2);
	robwidget_make_toplevel(ui->box, ui_toplevel);
	robwidget_toplevel_enable_scaling (ui->box);
	ROBWIDGET_SETNAME(ui->box, "surmeter");

	ui->tbl = rob_table_new (/*rows*/7, /*cols*/3, FALSE);
	ROBWIDGET_SETNAME(ui->tbl, "surlayout");

	initialize_font_cache(ui);
	prepare_faceplates (ui);

	int row = 0;
	/* main drawing area */
	ui->m0 = robwidget_new(ui);
	ROBWIDGET_SETNAME(ui->m0, "sur(m0)");
	robwidget_set_alignment(ui->m0, .5, .5);
	robwidget_set_expose_event(ui->m0, m0_expose_event);
	robwidget_set_size_request(ui->m0, m0_size_request);
	robwidget_set_size_allocate(ui->m0, m0_size_allocate);
	rob_table_attach_defaults (ui->tbl, ui->m0, 0, 5, row, row + 1);

	++row;

	ui->sep_h0 = robtk_sep_new (TRUE);
	//rob_table_attach (ui->tbl, robtk_sep_widget(ui->sep_h0), 2, 3, row, row + 1, 0, 8, RTK_EXANDF, RTK_SHRINK);

	ui->btn_peak = robtk_cbtn_new("Peak", GBT_LED_LEFT, false);
	robtk_cbtn_set_active (ui->btn_peak, false);
	robtk_cbtn_set_callback (ui->btn_peak, cb_queue_draw, ui);
	//robtk_cbtn_set_color_on(ui->btn_oct,  .2, .8, .1);
	//robtk_cbtn_set_color_off(ui->btn_oct, .1, .3, .1);
	rob_table_attach (ui->tbl, robtk_cbtn_widget(ui->btn_peak),  3, 4, row, row + 1, 0, 0, RTK_SHRINK, RTK_SHRINK);

	ui->spn_rms_gain = robtk_dial_new_with_size (-3.0, 15.0, .1,
			GED_WIDTH + 10, GED_HEIGHT + 12, GED_CX + 5, GED_CY + 4, GED_RADIUS);
	robtk_dial_set_value(ui->spn_rms_gain, 0);
	robtk_dial_set_default(ui->spn_rms_gain, 0.0);
	robtk_dial_set_callback(ui->spn_rms_gain, cb_set_rms_gain, ui);

	robtk_dial_annotation_callback(ui->spn_rms_gain, dial_annotation_db, ui);
	robtk_dial_set_scaled_surface_scale (ui->spn_rms_gain, ui->sf_bg_rms, 2.0);
	robtk_dial_set_detent_default (ui->spn_rms_gain, true);
	robtk_dial_set_scroll_mult (ui->spn_rms_gain, 2.f);

	rob_table_attach (ui->tbl, robtk_dial_widget(ui->spn_rms_gain),  4, 5, row, row + 1, 0, 8, RTK_SHRINK, RTK_SHRINK);

#ifdef WITH_SPLINE_KNOB
	ui->spn_spline = robtk_dial_new_with_size (0.5, 1. + (ui->n_chn * .5), .05,
			GED_WIDTH + 10, GED_HEIGHT + 12, GED_CX + 5, GED_CY + 4, GED_RADIUS);
	robtk_dial_set_value(ui->spn_spline, 2.0);
	robtk_dial_set_scaled_surface_scale (ui->spn_spline, ui->sf_bg_spline, 2.0);
	robtk_dial_set_default(ui->spn_spline, 2.0);
	robtk_dial_set_callback(ui->spn_spline, cb_set_spline, ui);
	rob_table_attach (ui->tbl, robtk_dial_widget(ui->spn_spline),  0, 1, row, row + 1, 0, 8, RTK_SHRINK, RTK_SHRINK);

	ui->spn_bulb = robtk_dial_new_with_size (0.5, 1.2, .05,
			GED_WIDTH + 10, GED_HEIGHT + 12, GED_CX + 5, GED_CY + 4, GED_RADIUS);
	robtk_dial_set_value(ui->spn_bulb, 1.0);
	robtk_dial_set_scaled_surface_scale (ui->spn_bulb, ui->sf_bg_bulb, 2.0);
	robtk_dial_set_default(ui->spn_bulb, 1.0);
	robtk_dial_set_callback(ui->spn_bulb, cb_queue_draw, ui);
	rob_table_attach (ui->tbl, robtk_dial_widget(ui->spn_bulb),  1, 2, row, row + 1, 0, 8, RTK_SHRINK, RTK_SHRINK);
#endif

	++row;

	/* correlation headings */
	ui->lbl_cor[0]  = robtk_lbl_new("Stereo Pair Correlation");
	ui->lbl_cor[1]  = robtk_lbl_new("Chn");
	ui->lbl_cor[2]  = robtk_lbl_new("Chn");
	rob_table_attach (ui->tbl, robtk_lbl_widget(ui->lbl_cor[1]), 0, 1, row, row + 1, 0, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach (ui->tbl, robtk_lbl_widget(ui->lbl_cor[0]), 2, 3, row, row + 1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->tbl, robtk_lbl_widget(ui->lbl_cor[2]), 4, 5, row, row + 1, 0, 0, RTK_SHRINK, RTK_SHRINK);

	++row;
	/* correlation pairs */
	for (uint32_t c = 0; c < ui->n_cor; ++c) {
		ui->sel_cor_a[c] = robtk_select_new();
		ui->sel_cor_b[c] = robtk_select_new();
		for (uint32_t cc = 0; cc < ui->n_chn; ++cc) {
			char tmp[8];
			sprintf (tmp, "%d", cc + 1);
			robtk_select_add_item(ui->sel_cor_a[c], cc, tmp);
			robtk_select_add_item(ui->sel_cor_b[c], cc, tmp);
			robtk_select_set_default_item(ui->sel_cor_a[c], (2 * c) % ui->n_chn); // XXX
			robtk_select_set_default_item(ui->sel_cor_b[c], (2 * c + 1) % ui->n_chn); // XXX
			robtk_select_set_callback (ui->sel_cor_a[c], cb_set_port, ui);
			robtk_select_set_callback (ui->sel_cor_b[c], cb_set_port, ui);
		}

		/* correlation display area */
		ui->m_cor[c] = robwidget_new(ui);
		ROBWIDGET_SETNAME(ui->m_cor[c], "cor");
		robwidget_set_alignment(ui->m_cor[c], .5, .5);
		robwidget_set_expose_event(ui->m_cor[c], cor_expose_event);
		robwidget_set_size_request(ui->m_cor[c], cor_size_request);
		robwidget_set_size_allocate(ui->m_cor[c], cor_size_allocate);

		rob_table_attach (ui->tbl, robtk_select_widget(ui->sel_cor_a[c]), 0, 1, row, row + 1, 12, 0, RTK_SHRINK, RTK_SHRINK);
		rob_table_attach (ui->tbl, ui->m_cor[c],                          1, 4, row, row + 1,  0, 2, RTK_EXANDF, RTK_EXANDF);
		rob_table_attach (ui->tbl, robtk_select_widget(ui->sel_cor_b[c]), 4, 5, row, row + 1, 12, 0, RTK_SHRINK, RTK_SHRINK );
		++row;
	}

	/* global packing */
	rob_vbox_child_pack(ui->box, ui->tbl, TRUE, TRUE);

	*widget = ui->box;

	ui->disable_signals = false;

	return ui;
}

static enum LVGLResize
plugin_scale_mode(LV2UI_Handle handle)
{
	return LVGL_LAYOUT_TO_FIT;
}

static void
cleanup(LV2UI_Handle handle)
{
	SURui* ui = (SURui*)handle;

	if (ui->fontcache) {
		for (uint32_t i = 0; i < FONT_LAST; ++i) {
			pango_font_description_free(ui->font[i]);
		}
	}
	for (uint32_t i = 0; i < ui->n_cor; ++i) {
			robtk_select_destroy (ui->sel_cor_a[i]);
			robtk_select_destroy (ui->sel_cor_b[i]);
			robwidget_destroy(ui->m_cor[i]);
	}
	robtk_lbl_destroy(ui->lbl_cor[0]);
	robtk_lbl_destroy(ui->lbl_cor[1]);
	robtk_lbl_destroy(ui->lbl_cor[2]);
	robtk_dial_destroy(ui->spn_rms_gain);
	robtk_cbtn_destroy(ui->btn_peak);
#ifdef WITH_SPLINE_KNOB
	robtk_dial_destroy(ui->spn_spline);
	robtk_dial_destroy(ui->spn_bulb);
	cairo_surface_destroy(ui->sf_bg_spline);
	cairo_surface_destroy(ui->sf_bg_bulb);
#endif
	robtk_sep_destroy(ui->sep_h0);
	cairo_surface_destroy(ui->sf_ann);
	cairo_surface_destroy(ui->sf_bg_rms);
	cairo_surface_destroy(ui->sf_cor);
	cairo_pattern_destroy(ui->pat_peak);
	cairo_pattern_destroy(ui->pat_rms);
	robwidget_destroy(ui->m0);
	rob_table_destroy (ui->tbl);
	rob_box_destroy(ui->box);
	free(ui);
}

static const void*
extension_data(const char* uri)
{
	return NULL;
}

/******************************************************************************
 * handle data from backend
 */

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
	SURui* ui = (SURui*)handle;

	if ( format != 0 ) { return; }

	if (port_index == 0) {
		ui->disable_signals = true;
		robtk_dial_set_value (ui->spn_rms_gain, *(float *)buffer);
		ui->disable_signals = false;
	} else if (port_index > 0 && port_index <= 12 && port_index % 3 == 0) {
		// correlation data
		const uint32_t cc = (port_index - 1) / 3;
		if (cc < ui->n_cor) {
			ui->cor[cc] = .5f * (1.0f + *(float *)buffer);
			queue_draw (ui->m_cor[cc]);
		}
	} else if (port_index > 0 && port_index <= 12 && port_index % 3 == 1) {
		// correlation input A
		const uint32_t cc = (port_index - 1) / 3;
		if (cc < ui->n_cor) {
			uint32_t pn = rintf(*(float *)buffer);
			ui->disable_signals = true;
			robtk_select_set_value(ui->sel_cor_a[cc], pn);
			ui->disable_signals = false;
		}
	} else if (port_index > 0 && port_index <= 12 && port_index % 3 == 2) {
		// correlation input B
		const uint32_t cc = (port_index - 1) / 3;
		if (cc < ui->n_cor) {
			uint32_t pn = rintf(*(float *)buffer);
			ui->disable_signals = true;
			robtk_select_set_value(ui->sel_cor_b[cc], pn);
			ui->disable_signals = false;
		}
	} else if (port_index > 12 && port_index <= 12U + ui->n_chn * 4 && port_index % 4 == 3) {
		ui->rms[(port_index - 13) / 4] = meter_deflect (*(float *)buffer);
		queue_draw (ui->m0);
	} else if (port_index > 12 && port_index <= 12U + ui->n_chn * 4 && port_index % 4 == 0) {
		ui->peak[(port_index - 13) / 4] = meter_deflect(*(float *)buffer);
		queue_draw (ui->m0);
	}
}
