/* kmeter LV2 GUI
 *
 * Copyright 2012-2013 Robin Gareus <robin@gareus.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#define RTK_URI "http://gareus.org/oss/lv2/meters#"
#define RTK_GUI "kmeterui"
#define MTR_URI RTK_URI

#define LVGL_RESIZEABLE

#define GM_HEIGHT (396.0f)

// TODO cache those
#define GM_TOP    (.5 + ceil (25. * ui->height / GM_HEIGHT))
#define GM_BOTTOM (4.5 + floor(7.f * ui->height / GM_HEIGHT))
#define GM_LEFT   (.5 + floor(4.5f * ui->height / GM_HEIGHT))
#define GM_GIRTH  (ceil(10.0f * ui->height / GM_HEIGHT))
#define GM_WIDTH  (GM_GIRTH + GM_LEFT + GM_LEFT)
#define MA_WIDTH  (ceil(4.0f + 17 * ui->height / GM_HEIGHT))
#define PK_WIDTH  (ceil (28.0f * ui->height / GM_HEIGHT))
#define PK_HEIGHT (ceil (10 + (6. * ui->height / GM_HEIGHT)))
#define PK_TOP    (floor((GM_TOP - PK_HEIGHT)/2))

#define GM_SCALE  (ui->height - GM_TOP - GM_BOTTOM - 2.0)

#define MAX_METERS 2

#define	TOF ((GM_TOP           ) / ui->height)
#define	BOF ((GM_TOP + GM_SCALE) / ui->height)
#define	YVAL(x) ((GM_TOP + GM_SCALE - (x)) / ui->height)
#define	YPOS(x) (GM_TOP + GM_SCALE - (x))

#define UINT_TO_RGB(u,r,g,b) { (*(r)) = ((u)>>16)&0xff; (*(g)) = ((u)>>8)&0xff; (*(b)) = (u)&0xff; }
#define UINT_TO_RGBA(u,r,g,b,a) { UINT_TO_RGB(((u)>>8),r,g,b); (*(a)) = (u)&0xff; }

typedef struct {
	RobWidget *rw;

	LV2UI_Write_Function write;
	LV2UI_Controller     controller;

  RobWidget* m0;

	cairo_surface_t* sf[MAX_METERS];
	cairo_surface_t* an[MAX_METERS];
	cairo_surface_t* ma[2];
	cairo_pattern_t* mpat;
	cairo_surface_t* lb[2];
	PangoFontDescription *font;

	float val[MAX_METERS];
	int   val_def[MAX_METERS];
	int   val_vis[MAX_METERS];

	float peak_val[MAX_METERS];
	int   peak_def[MAX_METERS];
	int   peak_vis[MAX_METERS];
	float peak_max;

	uint32_t num_meters;
	bool metrics_changed;
	bool size_changed;

	int  kstandard;
	int  initialize;
	bool reset_toggle;
	bool dBFS;

	int width;
	int height;

	float c_txt[4];
	float c_bgr[4];

} KMUI;

/******************************************************************************
 * meter deflection
 */

static inline float meter_deflect_k (float db, float krange) {
  db+=krange;
  if (db < -40.0f) {
		return (db > -90.0f ? pow (10.0f, db * 0.05f) : 0.0f) * 500.0f / (krange + 45.0f);
  } else {
    const float rv = (db + 45.0f) / (krange + 45.0f);
    if (rv < 1.0) {
      return rv;
    } else {
      return 1.0;
    }
  }
}

static int deflect(KMUI* ui, float val) {
	int lvl = rint(GM_SCALE * meter_deflect_k(val, ui->kstandard));
	if (lvl < 2) lvl = 2;
	if (lvl >= GM_SCALE) lvl = GM_SCALE;
	return lvl;
}

/******************************************************************************
 * Drawing
 */

static void render_meter(KMUI*, int, int, int, int, int);

static void create_meter_pattern(KMUI* ui) {
	if (ui->mpat) cairo_pattern_destroy (ui->mpat);

	int clr[12];
	float stp[5];

	stp[4] = deflect(ui,  4 - ui->kstandard);
	stp[3] = deflect(ui,  3 - ui->kstandard);
	stp[2] = deflect(ui,  0 - ui->kstandard);
	stp[1] = deflect(ui,-20 - ui->kstandard);
	stp[0] = deflect(ui,-40 - ui->kstandard);

	clr[ 0]=0x003399ff; clr[ 1]=0x009933ff;
	clr[ 2]=0x00aa00ff; clr[ 3]=0x00bb00ff;
	clr[ 4]=0x00ff00ff; clr[ 5]=0x00ff00ff;
	clr[ 6]=0xfff000ff; clr[ 7]=0xfff000ff;
	clr[ 8]=0xff8000ff; clr[ 9]=0xff8000ff;
	clr[10]=0xff0000ff; clr[11]=0xff0000ff;

	guint8 r,g,b,a;
	const double onep  =  1.0 / (double) GM_SCALE;
	const double softT =  2.0 / (double) GM_SCALE;
	const double softB =  2.0 / (double) GM_SCALE;

	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, ui->height);

	cairo_pattern_add_color_stop_rgb (pat,  .0, .0 , .0, .0);
	cairo_pattern_add_color_stop_rgb (pat, TOF - onep,      .0 , .0, .0);
	cairo_pattern_add_color_stop_rgb (pat, TOF, .5 , .5, .5);

	// top/clip
	UINT_TO_RGBA (clr[11], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, TOF + onep,
	                                  r/255.0, g/255.0, b/255.0);

	// -0dB
	UINT_TO_RGBA (clr[10], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, YVAL(stp[4]) - softT,
	                                  r/255.0, g/255.0, b/255.0);
	UINT_TO_RGBA (clr[9], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, YVAL(stp[4]) + softB,
	                                  r/255.0, g/255.0, b/255.0);

	// -3dB || -2dB
	UINT_TO_RGBA (clr[8], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, YVAL(stp[3]) - softT,
	                                  r/255.0, g/255.0, b/255.0);
	UINT_TO_RGBA (clr[7], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, YVAL(stp[3]) + softB,
	                                  r/255.0, g/255.0, b/255.0);

	// -9dB
	UINT_TO_RGBA (clr[6], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, YVAL(stp[2]) - softT,
	                                  r/255.0, g/255.0, b/255.0);
	UINT_TO_RGBA (clr[5], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, YVAL(stp[2]) + softB,
	                                  r/255.0, g/255.0, b/255.0);

	// -18dB
	UINT_TO_RGBA (clr[4], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, YVAL(stp[1]) - softT,
	                                  r/255.0, g/255.0, b/255.0);
	UINT_TO_RGBA (clr[3], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, YVAL(stp[1]) + softB,
	                                  r/255.0, g/255.0, b/255.0);

	// -40dB
	UINT_TO_RGBA (clr[2], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, YVAL(stp[0]) - softT,
	                                  r/255.0, g/255.0, b/255.0);
	UINT_TO_RGBA (clr[1], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, YVAL(stp[0]) + softB,
	                                  r/255.0, g/255.0, b/255.0);

	// -inf
	UINT_TO_RGBA (clr[0], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, BOF - 4 * onep - softT,
	                                  r/255.0, g/255.0, b/255.0);

	//Bottom
	cairo_pattern_add_color_stop_rgb (pat, BOF, .1 , .1, .1);
	cairo_pattern_add_color_stop_rgb (pat, BOF + onep, .0 , .0, .0);
	cairo_pattern_add_color_stop_rgb (pat, 1.0, .0 , .0, .0);

	if (!getenv("NO_METER_SHADE") || strlen(getenv("NO_METER_SHADE")) == 0) {
		cairo_pattern_t* shade_pattern = cairo_pattern_create_linear (0.0, 0.0, GM_WIDTH, 0.0);
		cairo_pattern_add_color_stop_rgba (shade_pattern, (GM_LEFT-1.0) / GM_WIDTH,   0.0, 0.0, 0.0, 0.15);
		cairo_pattern_add_color_stop_rgba (shade_pattern, (GM_LEFT + GM_GIRTH * .35) / GM_WIDTH, 1.0, 1.0, 1.0, 0.10);
		cairo_pattern_add_color_stop_rgba (shade_pattern, (GM_LEFT + GM_GIRTH * .53) / GM_WIDTH, 0.0, 0.0, 0.0, 0.05);
		cairo_pattern_add_color_stop_rgba (shade_pattern, (GM_LEFT+1.0+GM_GIRTH) / GM_WIDTH,  0.0, 0.0, 0.0, 0.25);

		cairo_surface_t* surface;
		cairo_t* tc = 0;
		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, GM_WIDTH, ui->height);
		tc = cairo_create (surface);
		cairo_set_source (tc, pat);
		cairo_rectangle (tc, 0, 0, GM_WIDTH, ui->height);
		cairo_fill (tc);
		cairo_pattern_destroy (pat);

		cairo_set_source (tc, shade_pattern);
		cairo_rectangle (tc, 0, 0, GM_WIDTH, ui->height);
		cairo_fill (tc);
		cairo_pattern_destroy (shade_pattern);

#if 0 // LED stripes
		cairo_save (tc);
		cairo_set_line_width(tc, 1.0);
		cairo_set_source_rgba(tc, .0, .0, .0, 0.4);
		for (float y=0.5; y < ui->height; y+= 2.0) {
			cairo_move_to(tc, 0, y);
			cairo_line_to(tc, GM_WIDTH, y);
			cairo_stroke (tc);
		}
		cairo_restore (tc);
#endif

		pat = cairo_pattern_create_for_surface (surface);
		cairo_destroy (tc);
		cairo_surface_destroy (surface);
	}

	ui->mpat= pat;
}

static void write_text(
		cairo_t* cr,
		const char *txt,
		PangoFontDescription *font,
		const float x, const float y,
		const int align,
		const float * const col) {
	write_text_full(cr, txt, font, x, y, 0, align, col);
}

#define INIT_ANN_BG(VAR, WIDTH, HEIGHT) \
	if (VAR) cairo_surface_destroy(VAR);\
	VAR = cairo_image_surface_create (CAIRO_FORMAT_RGB24, WIDTH, HEIGHT); \
	cr = cairo_create (VAR);

#define INIT_ANN_LB(VAR, WIDTH, HEIGHT) \
	if (VAR) cairo_surface_destroy(VAR);\
	VAR = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT); \
	cr = cairo_create (VAR); \
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE); \
	cairo_set_source_rgba(cr, .0, .0, .0, 0.0); \
	cairo_rectangle (cr, 0, 0, WIDTH, HEIGHT); \
	cairo_fill (cr); \
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);


#define INIT_BLACK_BG(VAR, WIDTH, HEIGHT) \
	INIT_ANN_BG(VAR, WIDTH, HEIGHT) \
	CairoSetSouerceRGBA(c_blk); \
	cairo_rectangle (cr, 0, 0, WIDTH, WIDTH); \
	cairo_fill (cr);

static void create_metrics(KMUI* ui) {
	cairo_t* cr;
	char fontname[24];
	sprintf (fontname, "Mono %dpx", (int)ceil(3. + 6. * ui->height / GM_HEIGHT));

	pango_font_description_free(ui->font);
	ui->font = pango_font_description_from_string(fontname);

	sprintf (fontname, "Sans %dpx", (int)ceil(2 + 6. * ui->height / GM_HEIGHT));

	PangoFontDescription *font = pango_font_description_from_string(fontname);

	INIT_ANN_LB(ui->lb[0], ui->width, GM_BOTTOM);
	char kstd[10];
	snprintf(kstd, 9, "K%d/RMS", ui->kstandard);
	write_text(cr, kstd , font, ui->width - 3, GM_BOTTOM - 1, 4, c_blk);
	cairo_destroy (cr);

	INIT_ANN_LB(ui->lb[1], ui->width, GM_TOP);
	if (ui->num_meters < 2) {
		write_text(cr, "pe\nak" , font, (ui->width - PK_WIDTH)/2.0f - 4, GM_TOP/2, 1, c_g90);
		if (ui->dBFS)
			write_text(cr, "dB\nFS" , font, (ui->width + PK_WIDTH)/2.0f + 3, GM_TOP/2, 3, c_g90);
		else
			write_text(cr, "dB" , font, (ui->width + PK_WIDTH)/2.0f + 3, GM_TOP/2, 3, c_g90);
	} else {
		write_text(cr, "peak" , font, (ui->width - PK_WIDTH)/2.0f - 4, GM_TOP/2, 1, c_g90);
		if (ui->dBFS)
			write_text(cr, "dBFS" , font, (ui->width + PK_WIDTH)/2.0f + 3, GM_TOP/2, 3, c_g90);
		else
			write_text(cr, "dB " , font, (ui->width + PK_WIDTH)/2.0f + 3, GM_TOP/2, 3, c_g90);
	}
	cairo_destroy (cr);


#define DO_THE_METER(DB, TXT) \
	if (DB <= ui->kstandard) { \
		write_text(cr, TXT , font, MA_WIDTH - 3, YPOS(deflect(ui, DB - ui->kstandard)), 1, \
				(DB >= 4 ? c_red : (DB >= 0 ? c_nyl : c_g90)) ); \
	}

#define DO_THE_METRICS \
	DO_THE_METER(  20, "+20") \
	if (ui->kstandard == 14) { DO_THE_METER( 14, "+14") } else { DO_THE_METER( 15, "+15") } \
	DO_THE_METER(  12, "+12") \
	DO_THE_METER(   9,  "+9") \
	DO_THE_METER(   6,  "+6") \
	DO_THE_METER(   3,  "+3") \
	DO_THE_METER(   0,  " 0") \
	DO_THE_METER(  -3,  "-3") \
	DO_THE_METER(  -9,  "-9") \
	DO_THE_METER( -15, "-15") \
	DO_THE_METER( -20, "-20") \
	DO_THE_METER( -25, "-25") \
	DO_THE_METER( -30, "-30") \
	DO_THE_METER( -35, "-35") \
	DO_THE_METER( -40, "-40") \
	DO_THE_METER( -60, "-60") \

	INIT_ANN_BG(ui->ma[0], MA_WIDTH, ui->height);
	CairoSetSouerceRGBA(ui->c_bgr);
	cairo_rectangle (cr, 0, 0, MA_WIDTH, ui->height);
	cairo_fill (cr);
	DO_THE_METRICS
	cairo_destroy (cr);

	INIT_ANN_BG(ui->ma[1], MA_WIDTH, ui->height)
	CairoSetSouerceRGBA(ui->c_bgr);
	cairo_rectangle (cr, 0, 0, MA_WIDTH, ui->height);
	cairo_fill (cr);
	DO_THE_METRICS

	cairo_destroy (cr);
	pango_font_description_free(font);

#define ALLOC_SF(VAR) \
	if (VAR) cairo_surface_destroy(VAR);\
	VAR = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, GM_WIDTH, ui->height);\
	cr = cairo_create (VAR);\
	CairoSetSouerceRGBA(ui->c_bgr); \
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);\
	cairo_rectangle (cr, 0, 0, GM_WIDTH, ui->height);\
	cairo_fill (cr);

#define GAINLINE(DB) \
	if (DB < ui->kstandard) { \
		const float yoff = GM_TOP + GM_SCALE - deflect(ui, DB - ui->kstandard); \
		cairo_move_to(cr, 0, yoff); \
		cairo_line_to(cr, GM_WIDTH, yoff); \
		cairo_stroke(cr); \
}

	for (uint32_t i = 0; i < ui->num_meters; ++i) {
		ALLOC_SF(ui->sf[i])

		/* metric background */
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_set_line_width(cr, 1.0);
		CairoSetSouerceRGBA(c_g80);
		GAINLINE(20);
		if (ui->kstandard == 14) {GAINLINE(14);} else {GAINLINE(15);}
		GAINLINE(15);
		GAINLINE(12);
		GAINLINE(10);
		GAINLINE(9);
		GAINLINE(6);
		GAINLINE(4);
		GAINLINE(3);
		GAINLINE(0);
		GAINLINE(-3);
		GAINLINE(-6);
		GAINLINE(-9);
		GAINLINE(-10);
		GAINLINE(-15);
		GAINLINE(-20);
		GAINLINE(-25);
		GAINLINE(-30);
		GAINLINE(-35);
		GAINLINE(-40);
		GAINLINE(-45);
		GAINLINE(-50);
		GAINLINE(-55);
		GAINLINE(-60);
		cairo_destroy(cr);

		render_meter(ui, i, GM_SCALE, 2, 0, 0);
		ui->val_vis[i] = 2;
		ui->peak_vis[i] = 0;
	}
}

static void render_meter(KMUI* ui, int i, int v_old, int v_new, int m_old, int m_new) {
	cairo_t* cr = cairo_create (ui->sf[i]);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	CairoSetSouerceRGBA(c_blk);
	rounded_rectangle (cr, GM_LEFT-1, GM_TOP, GM_GIRTH+2, GM_SCALE, 6);
	cairo_fill_preserve(cr);
	cairo_clip(cr);

	/* rms value */
	cairo_set_source(cr, ui->mpat);
	cairo_rectangle (cr, GM_LEFT, GM_TOP + GM_SCALE - v_new - 1, GM_GIRTH, v_new + 1);
	cairo_fill(cr);

	/* peak */
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_rectangle (cr, GM_LEFT, GM_TOP + GM_SCALE - m_new - 0.5, GM_GIRTH, 3);
	cairo_fill_preserve (cr);
	CairoSetSouerceRGBA(c_hlt);
	cairo_fill(cr);

	/* border */
	cairo_reset_clip(cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	cairo_set_line_width(cr, 0.75);
	CairoSetSouerceRGBA(c_g60);

#if 0
	/* clear top area left by rounded rect
	 * but it's performance hog
	 * new feature -> display all time peak :)
	 */
	cairo_move_to(cr, GM_LEFT + GM_GIRTH/2, GM_TOP + GM_SCALE + 2);
	cairo_line_to(cr, GM_LEFT + GM_GIRTH/2, GM_TOP + GM_SCALE + 8);
	cairo_stroke(cr);
#endif

	rounded_rectangle (cr, GM_LEFT-1, GM_TOP, GM_GIRTH+2, GM_SCALE, 6);
	cairo_stroke(cr);

	cairo_destroy(cr);
}

/******************************************************************************
 * main drawing
 */

static bool expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev) {
	KMUI* ui = (KMUI*)GET_HANDLE(handle);

	if (ui->size_changed) {
		create_metrics(ui);
		create_meter_pattern(ui);
		ui->size_changed = false;
		ui->metrics_changed = false;
	}
	if (ui->metrics_changed) {
		ui->metrics_changed = false;
		create_metrics(ui);
	}

	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	/* metric areas */
	cairo_set_source_surface(cr, ui->ma[0], 0, 0);
	cairo_paint (cr);
	cairo_set_source_surface(cr, ui->ma[1], MA_WIDTH + GM_WIDTH * ui->num_meters, 0);
	cairo_paint (cr);

	for (uint32_t i = 0; i < ui->num_meters ; ++i) {
		if (!rect_intersect_a(ev, MA_WIDTH + GM_WIDTH * i, 0, GM_WIDTH, ui->height)) continue;

		const int v_old = ui->val_vis[i];
		const int v_new = ui->val_def[i];
		const int m_old = ui->peak_vis[i];
		const int m_new = ui->peak_def[i];

		if (v_old != v_new || m_old != m_new) {
			ui->val_vis[i] = v_new;
			ui->peak_vis[i] = m_new;
			render_meter(ui, i, v_old, v_new, m_old, m_new);
		}

		cairo_set_source_surface(cr, ui->sf[i], MA_WIDTH + GM_WIDTH * i, 0);
		cairo_paint (cr);
	}

	/* numerical peak */
	if (rect_intersect_a(ev, (ui->width - PK_WIDTH) / 2.0f, PK_TOP, PK_WIDTH, PK_HEIGHT)) {
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_save(cr);
		rounded_rectangle (cr, (ui->width - PK_WIDTH) / 2.0f, PK_TOP, PK_WIDTH, PK_HEIGHT, 4);

		if (ui->peak_max >= -1.0) {
			CairoSetSouerceRGBA(c_ptr);
		} else if (ui->peak_max > -90.0) {
			CairoSetSouerceRGBA(c_blk);
		} else {
			CairoSetSouerceRGBA(c_g30);
		}

		cairo_fill_preserve (cr);
		cairo_set_line_width(cr, 0.75);
		CairoSetSouerceRGBA(c_g60);
		cairo_stroke_preserve (cr);
		cairo_clip (cr);

		const float peak_num = ui->dBFS ? ui->peak_max : ui->peak_max + ui->kstandard;
		char buf[24];
		if (ui->peak_max > 99) {
			sprintf(buf, "++++");
		} else if (ui->peak_max <= -90) {
			sprintf(buf, " -\u221E ");
		} else if (fabsf(peak_num) > 9.94) {
			sprintf(buf, "%+.0f ", peak_num);
		} else {
			sprintf(buf, "%+.1f", peak_num);
		}
		write_text(cr, buf, ui->font, (ui->width + PK_WIDTH) / 2.0f - 4, GM_TOP/2, 1, c_g90);
		cairo_restore(cr);
	}

	/* labels */
	cairo_set_source_surface(cr, ui->lb[1], 0, 0);
	cairo_paint (cr);
	cairo_set_source_surface(cr, ui->lb[0], 0, ui->height - GM_BOTTOM);
	cairo_paint (cr);

	return TRUE;
}

/******************************************************************************
 * UI callbacks
 */

static RobWidget* cb_reset_peak (RobWidget* handle, RobTkBtnEvent *event) {
	KMUI* ui = (KMUI*)GET_HANDLE(handle);
	if (event->state & ROBTK_MOD_CTRL) {
		ui->dBFS = !ui->dBFS;
		ui->metrics_changed = true;
		queue_draw(ui->m0);
		float temp = (ui->dBFS) ? -4 : 4;
		ui->write(ui->controller, 0, sizeof(float), 0, (const void*) &temp);
		return NULL;
	}

	ui->reset_toggle = !ui->reset_toggle;
	float temp = ui->reset_toggle ? 1.0 : 2.0;
	if (ui->dBFS) temp *= -1;
	ui->write(ui->controller, 0, sizeof(float), 0, (const void*) &temp);
	return NULL;
}

/******************************************************************************
 * widget hackery
 */

static void
size_request(RobWidget* handle, int *w, int *h) {
	KMUI* ui = (KMUI*)GET_HANDLE(handle);
	*h = GM_HEIGHT;
	*w = 2.0 * MA_WIDTH + ui->num_meters * GM_WIDTH; // recursive with allocted height
}

static void
size_allocate(RobWidget* handle, int w, int h) {
	KMUI* ui = (KMUI*)GET_HANDLE(handle);
	ui->height = h;
	ui->width = MIN(w, 2.0 * MA_WIDTH + ui->num_meters * GM_WIDTH);
	ui->size_changed = true;
	assert(ui->width <= w);
	robwidget_set_size(handle, ui->width, ui->height);
	queue_draw(ui->m0);
}

static RobWidget * toplevel(KMUI* ui, void * const top)
{
	/* main widget: layout */
	ui->rw = rob_vbox_new(FALSE, 0);
	robwidget_make_toplevel(ui->rw, top);

	/* DPM main drawing area */
	ui->m0 = robwidget_new(ui);
	ROBWIDGET_SETNAME(ui->m0, "kmeter (m0)");

	robwidget_set_expose_event(ui->m0, expose_event);
	robwidget_set_size_request(ui->m0, size_request);
	robwidget_set_size_allocate(ui->m0, size_allocate);
	robwidget_set_mousedown(ui->m0, cb_reset_peak);

	rob_vbox_child_pack(ui->rw, ui->m0, TRUE, TRUE);
	return ui->rw;
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
	KMUI* ui = (KMUI*) calloc(1,sizeof(KMUI));
	*widget = NULL;

	if      (!strncmp(plugin_uri, MTR_URI "K12mono", 33 + 7))   { ui->num_meters = 1; ui->kstandard = 12;}
	else if (!strncmp(plugin_uri, MTR_URI "K14mono", 33 + 7))   { ui->num_meters = 1; ui->kstandard = 14;}
	else if (!strncmp(plugin_uri, MTR_URI "K20mono", 33 + 7))   { ui->num_meters = 1; ui->kstandard = 20;}
	else if (!strncmp(plugin_uri, MTR_URI "K12stereo", 33 + 9)) { ui->num_meters = 2; ui->kstandard = 12;}
	else if (!strncmp(plugin_uri, MTR_URI "K14stereo", 33 + 9)) { ui->num_meters = 2; ui->kstandard = 14;}
	else if (!strncmp(plugin_uri, MTR_URI "K20stereo", 33 + 9)) { ui->num_meters = 2; ui->kstandard = 20;}
	else {
		free(ui);
		return NULL;
	}
	ui->write      = write_function;
	ui->controller = controller;

	get_color_from_theme(0, ui->c_txt);
	get_color_from_theme(1, ui->c_bgr);

	ui->dBFS = true;
	ui->size_changed = true;
	ui->metrics_changed = true;

	ui->font = pango_font_description_from_string("Mono 9px");

	for (uint32_t i=0; i < ui->num_meters ; ++i) {
		ui->val[i] = -90.0;
		ui->val_def[i] = deflect(ui, -90);
		ui->peak_val[i] = -90.0;
		ui->peak_def[i] = deflect(ui, -90);
	}
	ui->peak_max = -90.0;

	ui->height = GM_HEIGHT;
	ui->width = 2.0 * MA_WIDTH + ui->num_meters * GM_WIDTH;

	*widget = toplevel(ui, ui_toplevel);

	ui->initialize = 0;
	ui->reset_toggle = false;

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
	KMUI* ui = (KMUI*)handle;
	for (uint32_t i=0; i < ui->num_meters ; ++i) {
		cairo_surface_destroy(ui->sf[i]);
		cairo_surface_destroy(ui->an[i]);
	}
	cairo_pattern_destroy(ui->mpat);
	cairo_surface_destroy(ui->ma[0]);
	cairo_surface_destroy(ui->ma[1]);
	cairo_surface_destroy(ui->lb[0]);
	cairo_surface_destroy(ui->lb[1]);
	pango_font_description_free(ui->font);

	robwidget_destroy(ui->m0);
	rob_box_destroy(ui->rw);

	free(ui);
}

static const void*
extension_data(const char* uri)
{
	return NULL;
}

/******************************************************************************
 * backend communication
 */

#define INVALIDATE_RECT(XX,YY,WW,HH) queue_tiny_area(ui->m0, floorf(XX), floorf(YY), WW, HH);

static void invalidate_meter(KMUI* ui, int mtr, float val) {
	const int v_old = ui->val_def[mtr];
	const int v_new = deflect(ui, val);

	ui->val[mtr] = val;
	ui->val_def[mtr] = v_new;

	if (v_old != v_new) {
		int t, h;
		if (v_old > v_new) {
			t = v_old;
			h = v_old - v_new;
		} else {
			t = v_new;
			h = v_new - v_old;
		}

		INVALIDATE_RECT(
				mtr * GM_WIDTH + MA_WIDTH + GM_LEFT - 1,
				GM_TOP + GM_SCALE - t - 1,
				GM_GIRTH + 2, h+3);
	}
}

static void invalidate_peak(KMUI* ui, int mtr, float val) {
	const int v_old = ui->peak_def[mtr];
	const int v_new = deflect(ui, val);

	ui->peak_val[mtr] = val;
	ui->peak_def[mtr] = v_new;

	if (v_old != v_new) {
		int t, h;
		if (v_old > v_new) {
			t = v_old;
			h = v_old - v_new;
		} else {
			t = v_new;
			h = v_new - v_old;
		}

		INVALIDATE_RECT(
				mtr * GM_WIDTH + MA_WIDTH + GM_LEFT - 1,
				GM_TOP + GM_SCALE - t - 1,
				GM_GIRTH + 2, h+4);
	}
}

static void invalidate_hold(KMUI* ui, float val) {
	if (ui->peak_max == val) return;
	ui->peak_max = val;
	INVALIDATE_RECT((ui->width - PK_WIDTH) / 2.0f - 1, GM_TOP/2 - 9, PK_WIDTH+2, 18);
}

/******************************************************************************
 * handle data from backend
 */

static void handle_meter_connections(KMUI* ui, uint32_t port_index, float v) {
	v = v > .000031623f ? 20.0 * log10f(v) : -90.0;
	if (port_index == 3) {
		invalidate_meter(ui, 0, v);
	}
	else if (port_index == 5 && ui->num_meters == 1) {
		invalidate_hold(ui, v);
	}
	else if (port_index == 6) {
		invalidate_meter(ui, 1, v);
	}
	else if (port_index == 4 && ui->num_meters == 1) {
		invalidate_peak(ui, 0, v);
	}
	else if (port_index == 7 && ui->num_meters == 2) {
		invalidate_peak(ui, 0, v);
	}
	else if (port_index == 8 && ui->num_meters == 2) {
		invalidate_peak(ui, 1, v);
	}
	else if (port_index == 9 && ui->num_meters == 2) {
		invalidate_hold(ui, v);
	}
}

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
	KMUI* ui = (KMUI*)handle;
	if (format != 0) return;

	if (port_index == 0) {
		int val = *(float *)buffer;
		/* re-request peak-value from backend */
		if (ui->initialize == 0) {
			ui->initialize = 1;
			if ((val < 0) ^ ui->dBFS) {
				ui->metrics_changed = true;
			}
			float temp = (val < 0) ? -3 : 3;
			ui->write(ui->controller, 0, sizeof(float), 0, (const void*) &temp);
		}
		ui->dBFS = (val < 0);
	} else if (ui->initialize == 1 && ((port_index == 5 && ui->num_meters == 1) || (port_index == 9 && ui->num_meters == 2))) {
			ui->initialize = 2;
			float temp = (ui->dBFS) ? -4 : 4;
			ui->write(ui->controller, 0, sizeof(float), 0, (const void*) &temp);
	}

	handle_meter_connections(ui, port_index, *(float *)buffer);
}
