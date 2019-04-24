/* spectrum analyzer LV2 GUI
 *
 * Copyright 2012-2015 Robin Gareus <robin@gareus.org>
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
#define RTK_GUI "dpmui"
#define MTR_URI RTK_URI

#define LVGL_RESIZEABLE

#define GM_TOP    (ui->display_freq ?  4.5f : 12.5 + GM_PEAK)
#define GM_LEFT   (ui->gm_left)
#define GM_GIRTH  (ui->gm_girth)
#define GM_WIDTH  (ui->gm_width)

#define GM_MINH   (396.0f)

//#define GM_HEIGHT (ui->display_freq ? 400.0f:460.0f)
#define GM_HEIGHT (ui->height)
#define GM_SCALE  (GM_TXT - GM_TOP - (ui->display_freq ? 8.5f : 12.5))
#define GM_PEAK   ceil(8 + 9.f * ui->scale)
#define FQ_ANN    ceil(51.f * ui->scale)
#define FQ_WIDTH  (13 + FQ_ANN)
#define AN_HEIGHT ceil(46.f * ui->scale)
#define AN_WIDTH  ceil(32.f * ui->scale)
#define GM_TXT    (GM_HEIGHT - (ui->display_freq ? FQ_ANN : GM_PEAK))

#define MA_WIDTH  ceil(30.0f * ui->scale)

#define MAX_CAIRO_PATH 32
#define MAX_METERS 31

#define	TOF ((GM_TOP           ) / GM_HEIGHT)
#define	BOF ((GM_TOP + GM_SCALE) / GM_HEIGHT)
#define	YVAL(x) ((GM_TOP + GM_SCALE - (x)) / GM_HEIGHT)
#define	YPOS(x) (GM_TOP + GM_SCALE - (x))

#define UINT_TO_RGB(u,r,g,b) { (*(r)) = ((u)>>16)&0xff; (*(g)) = ((u)>>8)&0xff; (*(b)) = (u)&0xff; }
#define UINT_TO_RGBA(u,r,g,b,a) { UINT_TO_RGB(((u)>>8),r,g,b); (*(a)) = (u)&0xff; }

/* val: .05 .. 15 1/s  <> 0..100 */
#define RESPSCALE(X) ((X) > 0.05 ? rint(400.0 * (1.3f + log10f(X)) )/ 10.0 : 0)
#define INV_RESPSCALE(X) powf(10, (X) * .025f - 1.3f)

static const char *freq_table [] = {
	  " 25 Hz", "31.5 Hz",  " 40 Hz",
	  " 50 Hz",  " 63 Hz",  " 80 Hz",
	  "100 Hz",  "125 Hz",  "160 Hz",
	  "200 Hz",  "250 Hz",  "315 Hz",
	  "400 Hz",  "500 Hz",  "630 Hz",
	  "800 Hz",  " 1 kHz", "1250 Hz",
	 "1.6 kHz",  " 2 kHz", "2.5 kHz",
	 "3150 Hz",  " 4 kHz",  " 5 kHz",
	 "6.3 kHz",  " 8 kHz",  "10 kHz",
	"12.5 kHz",  "16 kHz",  "20 kHz"
};

typedef struct {
	RobWidget *rw;

	LV2UI_Write_Function write;
	LV2UI_Controller     controller;

	RobWidget* m_box;
	RobWidget* c_box;
	RobWidget* m0;
	RobTkScale* fader;
	RobTkLbl* lbl_speed;
	RobTkCBtn* btn_peaks;
	RobTkDial* spn_speed;
	RobTkSep* sep_h0;

	cairo_surface_t* sf[MAX_METERS];
	cairo_surface_t* an[MAX_METERS];
	cairo_surface_t* ma[2];
	cairo_surface_t* dial;
	cairo_pattern_t* mpat;
	PangoFontDescription *font[4];

	float val[MAX_METERS];
	int   val_def[MAX_METERS];
	int   val_vis[MAX_METERS];

	float peak_val[MAX_METERS];
	int   peak_def[MAX_METERS];
	int   peak_vis[MAX_METERS];

	bool disable_signals;
	float gain;
	uint32_t num_meters;
	bool display_freq;
	bool reset_toggle;
	int  initialize;
	bool metrics_changed;
	bool size_changed;
	bool show_peaks;
	bool show_peaks_changed;
	uint32_t misc_state;

	float cache_sf;
	float cache_ma;
	int highlight;

	float gm_width;
	float gm_girth;
	float gm_left;
	int min_width;
	int cur_width;

	int width;
	int height;

	float _min_w;
	float _min_h;

	float c_txt[4];
	float c_bgr[4];
	float scale;

} SAUI;

static void invalidate_meter(SAUI* ui, int mtr, float val, float peak);

/******************************************************************************
 * meter deflection
 */

static inline float
log_meter (float db)
{
         float def = 0.0f; /* Meter deflection %age */

         if (db < -70.0f) {
                 def = 0.0f;
         } else if (db < -60.0f) {
                 def = (db + 70.0f) * 0.25f;
         } else if (db < -50.0f) {
                 def = (db + 60.0f) * 0.5f + 2.5f;
         } else if (db < -40.0f) {
                 def = (db + 50.0f) * 0.75f + 7.5f;
         } else if (db < -30.0f) {
                 def = (db + 40.0f) * 1.5f + 15.0f;
         } else if (db < -20.0f) {
                 def = (db + 30.0f) * 2.0f + 30.0f;
         } else if (db < 6.0f) {
                 def = (db + 20.0f) * 2.5f + 50.0f;
         } else {
		 def = 115.0f;
	 }

	 /* 115 is the deflection %age that would be
	    when db=6.0. this is an arbitrary
	    endpoint for our scaling.
	 */

  return (def/115.0f);
}

static int deflect(SAUI* ui, float val) {
	int lvl = rint(GM_SCALE * log_meter(val));
	if (lvl < 2) lvl = 2;
	if (lvl >= GM_SCALE) lvl = GM_SCALE;
	return lvl;
}

/******************************************************************************
 * Drawing
 */

static void render_meter(SAUI*, int, int, int, int, int);

enum {
	FONT_S06 = 0,
	FONT_S08,
	FONT_M07,
	FONT_M08
};

static void reinitialize_font_cache(SAUI* ui) {
	char fontname[24];
	for (int i=0; i < 4; ++i) {
		if (ui->font[i]) {
			pango_font_description_free(ui->font[i]);
		}
	}

	sprintf (fontname, "Sans %dpx", (int)floor(10 * ui->scale));
	ui->font[FONT_S08] = pango_font_description_from_string(fontname);
	sprintf (fontname, "Sans %dpx", (int)floor(8 * ui->scale));
	ui->font[FONT_S06] = pango_font_description_from_string(fontname);
	sprintf (fontname, "Mono %dpx", (int)floor(9 * ui->scale));
	ui->font[FONT_M07] = pango_font_description_from_string(fontname);
	sprintf (fontname, "Mono %dpx", (int)floor(10 * ui->scale));
	ui->font[FONT_M08] = pango_font_description_from_string(fontname);
}


static void create_meter_pattern(SAUI* ui) {
	const int width = GM_WIDTH;
	const int height = GM_HEIGHT;

	if (ui->mpat) cairo_pattern_destroy(ui->mpat);
	int clr[12];
	float stp[5];

	stp[4] = deflect(ui,  0);
	stp[3] = deflect(ui, -3);
	stp[2] = deflect(ui, -9);
	stp[1] = deflect(ui,-18);
	stp[0] = deflect(ui,-40);

	if (ui->display_freq) {
		clr[ 0]=0x004488ff; clr[ 1]=0x1188bbff;
		clr[ 2]=0x228888ff; clr[ 3]=0x00bb00ff;
	} else {
		clr[ 0]=0x008844ff; clr[ 1]=0x009922ff;
		clr[ 2]=0x00aa00ff; clr[ 3]=0x00bb00ff;
	}
	clr[ 4]=0x00ff00ff; clr[ 5]=0x00ff00ff;
	clr[ 6]=0xfff000ff; clr[ 7]=0xfff000ff;
	clr[ 8]=0xff8000ff; clr[ 9]=0xff8000ff;
	clr[10]=0xff0000ff; clr[11]=0xff0000ff;

	guint8 r,g,b,a;
	const double onep  =  1.0 / (double) GM_SCALE;
	const double softT =  2.0 / (double) GM_SCALE;
	const double softB =  2.0 / (double) GM_SCALE;

	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, height);

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
		cairo_pattern_t* shade_pattern = cairo_pattern_create_linear (0.0, 0.0, width, 0.0);
		cairo_pattern_add_color_stop_rgba (shade_pattern,  .0, 0.0, 0.0, 0.0, 0.90);
		cairo_pattern_add_color_stop_rgba (shade_pattern, (GM_LEFT-1.0) / GM_WIDTH,   0.0, 0.0, 0.0, 0.55);
		cairo_pattern_add_color_stop_rgba (shade_pattern, (GM_LEFT + GM_GIRTH * .35) / GM_WIDTH, 1.0, 1.0, 1.0, 0.12);
		cairo_pattern_add_color_stop_rgba (shade_pattern, (GM_LEFT + GM_GIRTH * .53) / GM_WIDTH, 0.0, 0.0, 0.0, 0.05);
		cairo_pattern_add_color_stop_rgba (shade_pattern, (GM_LEFT+1.0+GM_GIRTH) / GM_WIDTH,  0.0, 0.0, 0.0, 0.55);
		cairo_pattern_add_color_stop_rgba (shade_pattern, 1.0, 0.0, 0.0, 0.0, 0.90);

		cairo_surface_t* surface;
		cairo_t* tc = 0;
		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
		tc = cairo_create (surface);
		cairo_set_source (tc, pat);
		cairo_rectangle (tc, 0, 0, width, height);
		cairo_fill (tc);
		cairo_pattern_destroy (pat);

		cairo_set_source (tc, shade_pattern);
		cairo_rectangle (tc, 0, 0, width, height);
		cairo_fill (tc);
		cairo_pattern_destroy (shade_pattern);

		// LED stripes
		cairo_save (tc);
		cairo_set_line_width(tc, 1.0);
		cairo_set_source_rgba(tc, .0, .0, .0, 0.6);
		for (float y=0.5; y < height; y+= 2.0) {
			cairo_move_to(tc, 0, y);
			cairo_line_to(tc, width, y);
			cairo_stroke (tc);
		}
		cairo_restore (tc);

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
		const float ang, const int align,
		const float * const col) {
	write_text_full(cr, txt, font, x, y, ang, align, col);
}


#define FONT_LBL ui->font[FONT_S08]
#define FONT_MTR ui->font[FONT_S06]
#define FONT_VAL ui->font[FONT_M07]
#define FONT_SPK ui->font[FONT_M08]

#define INIT_ANN_BG(VAR, WIDTH, HEIGHT) \
	if (VAR) cairo_surface_destroy(VAR); \
	VAR = cairo_image_surface_create (CAIRO_FORMAT_RGB24, WIDTH, HEIGHT); \
	cr = cairo_create (VAR);

#define INIT_BLACK_BG(VAR, WIDTH, HEIGHT) \
	INIT_ANN_BG(VAR, WIDTH, HEIGHT) \
	CairoSetSouerceRGBA(c_blk); \
	cairo_rectangle (cr, 0, 0, WIDTH, WIDTH); \
	cairo_fill (cr);

static void alloc_annotations(SAUI* ui) {
	cairo_t* cr;
	if (ui->display_freq) {
		/* frequecy table */
		for (uint32_t i = 0; i < ui->num_meters; ++i) {
			INIT_BLACK_BG(ui->an[i], 24, FQ_WIDTH)
			write_text(cr, freq_table[i], FONT_LBL, 0, 0, -M_PI/2, 7, c_g90);
			cairo_destroy (cr);
		}
	}

	if (ui->dial) {
		return; // XXX TODO *dynamic* widget scaling
		cairo_surface_destroy(ui->dial);
	}
	ui->dial = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2 * (GED_WIDTH), 2 * (GED_HEIGHT+10));
	cr = cairo_create (ui->dial);
	cairo_scale (cr, 2.0, 2.0);
	CairoSetSouerceRGBA(c_trs);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr, 0, 0, GED_WIDTH, GED_HEIGHT+10);
	cairo_fill (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	float xlp, ylp;
#define RESPLABLEL(V) \
	{ \
		float ang = (-.75 * M_PI) + (1.5 * M_PI) * (RESPSCALE(V) - RESPSCALE(.05)) / (RESPSCALE(8.) - RESPSCALE(.05)); \
		xlp = GED_CX + .5 + sinf (ang) * (GED_RADIUS + 3.0); \
		ylp = GED_CY + 10.5 - cosf (ang) * (GED_RADIUS + 3.0); \
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); \
		CairoSetSouerceRGBA(ui->c_txt); \
		cairo_set_line_width(cr, 1.5); \
		cairo_move_to(cr, rint(xlp)-.5, rint(ylp)-.5); \
		cairo_close_path(cr); \
		cairo_stroke(cr); \
		xlp = GED_CX + .5 + sinf (ang) * (GED_RADIUS + 9.5); \
		ylp = GED_CY + 10.5 - cosf (ang) * (GED_RADIUS + 9.5); \
	}

	RESPLABLEL(8.0); write_text(cr, "1/8", FONT_MTR, xlp, ylp, 0, 2, ui->c_txt);
	RESPLABLEL(4.0); write_text(cr, " 1/4", FONT_MTR, xlp, ylp, 0, 2, ui->c_txt);
	RESPLABLEL(2.0); write_text(cr, " 1/2", FONT_MTR, xlp, ylp, 0, 2, ui->c_txt);
	RESPLABLEL(1.0); write_text(cr, "1", FONT_MTR, xlp, ylp, 0, 2, ui->c_txt);
	RESPLABLEL(1/2.f); write_text(cr, "2", FONT_MTR, xlp, ylp, 0, 2, ui->c_txt);
	RESPLABLEL(1/4.f); write_text(cr, "4", FONT_MTR, xlp, ylp, 0, 2, ui->c_txt);
	RESPLABLEL(1/10.f); write_text(cr, "10 ", FONT_MTR, xlp, ylp, 0, 2, ui->c_txt);
	RESPLABLEL(1/20.f); write_text(cr, "20", FONT_MTR, xlp, ylp, 0, 2, ui->c_txt);
	cairo_destroy (cr);
}

static void realloc_metrics(SAUI* ui) {
	const float dboff = ui->gain;
	if (!ui->size_changed && rint(ui->cache_ma * 5) == rint(dboff * 5)) {
		return;
	}
	ui->cache_ma = dboff;
	cairo_t* cr;
#define DO_THE_METER(DB, TXT) \
	if (dboff + DB < 6.0 && dboff + DB >= -60) \
	write_text(cr,  TXT , FONT_MTR, MA_WIDTH - 3, YPOS(deflect(ui, dboff + DB)), 0, 1, c_g90);

#define DO_THE_METRICS \
	DO_THE_METER(  25, "+25dB") \
	DO_THE_METER(  20, "+20dB") \
	DO_THE_METER(  18, "+18dB") \
	DO_THE_METER(  15, "+15dB") \
	DO_THE_METER(   9,  "+9dB") \
	DO_THE_METER(   6,  "+6dB") \
	DO_THE_METER(   3,  "+3dB") \
	DO_THE_METER(   0,  " 0dB") \
	DO_THE_METER(  -3,  "-3dB") \
	DO_THE_METER(  -9,  "-9dB") \
	DO_THE_METER( -15, "-15dB") \
	DO_THE_METER( -18, "-18dB") \
	DO_THE_METER( -20, "-20dB") \
	DO_THE_METER( -25, "-25dB") \
	DO_THE_METER( -30, "-30dB") \
	DO_THE_METER( -40, "-40dB") \
	DO_THE_METER( -50, "-50dB") \
	DO_THE_METER( -60, "-60dB") \

	INIT_ANN_BG(ui->ma[0], MA_WIDTH, GM_HEIGHT);
	CairoSetSouerceRGBA(ui->c_bgr);
	cairo_rectangle (cr, 0, 0, MA_WIDTH, GM_HEIGHT);
	cairo_fill (cr);
	DO_THE_METRICS
	if (ui->display_freq) {
		write_text(cr,  "dBFS", FONT_MTR, MA_WIDTH - 5, GM_TXT - 5, 0, 1, c_g90);
	}
	cairo_destroy (cr);

	INIT_ANN_BG(ui->ma[1], MA_WIDTH, GM_HEIGHT)
	CairoSetSouerceRGBA(ui->c_bgr);
	cairo_rectangle (cr, 0, 0, MA_WIDTH, GM_HEIGHT);
	cairo_fill (cr);
	DO_THE_METRICS
	if (ui->display_freq) {
		write_text(cr,  "dBFS", FONT_MTR, MA_WIDTH - 5, GM_TXT - 5, 0, 1, c_g90);
	} else {
		write_text(cr,  "dBTP", FONT_MTR, MA_WIDTH - 5, GM_TXT - 5 + ceil(GM_PEAK * .5), 0, 1, c_g90);
	}
	cairo_destroy (cr);
}

static void prepare_metersurface(SAUI* ui) {
	const float dboff = ui->gain;

	if (!ui->size_changed && rint(ui->cache_sf * 5) == rint(dboff * 5)) {
		return;
	}
	ui->cache_sf = dboff;

	cairo_t* cr;
#define ALLOC_SF(VAR) \
	if (VAR) cairo_surface_destroy(VAR); \
	VAR = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, GM_WIDTH, GM_HEIGHT);\
	cr = cairo_create (VAR);\
	CairoSetSouerceRGBA(ui->c_bgr); \
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);\
	cairo_rectangle (cr, 0, 0, GM_WIDTH, GM_HEIGHT);\
	cairo_fill (cr);

#define GAINLINE(DB) \
	if (dboff + DB < 5.99) { \
		const float yoff = GM_TOP + GM_SCALE - deflect(ui, dboff + DB); \
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
		GAINLINE(25);
		GAINLINE(20);
		GAINLINE(18);
		GAINLINE(15);
		GAINLINE(9);
		GAINLINE(6);
		GAINLINE(3);
		GAINLINE(0);
		GAINLINE(-3);
		GAINLINE(-9);
		GAINLINE(-15);
		GAINLINE(-18);
		GAINLINE(-20);
		GAINLINE(-25);
		GAINLINE(-30);
		GAINLINE(-40);
		GAINLINE(-50);
		GAINLINE(-60);
		cairo_destroy(cr);

		render_meter(ui, i, GM_SCALE, 2, 0, 0);
		ui->val_vis[i] = 2;
		ui->peak_vis[i] = 0;
	}
}

static void render_meter(SAUI* ui, int i, int v_old, int v_new, int m_old, int m_new) {
	cairo_t* cr = cairo_create (ui->sf[i]);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	CairoSetSouerceRGBA(c_blk);
	rounded_rectangle (cr, GM_LEFT-.5, GM_TOP, GM_GIRTH+1, GM_SCALE, 6);
	cairo_fill_preserve(cr);
	cairo_clip(cr);

	cairo_set_source(cr, ui->mpat);
	cairo_rectangle (cr, GM_LEFT, GM_TOP + GM_SCALE - v_new - 1, GM_GIRTH, v_new + 1);
	cairo_fill(cr);

	if (ui->show_peaks) {
		/* peak hold */
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_rectangle (cr, GM_LEFT, GM_TOP + GM_SCALE - m_new - 0.5, GM_GIRTH, 3);
		cairo_fill_preserve (cr);
		CairoSetSouerceRGBA(c_hlt);
		cairo_fill(cr);
	}

	/* border */
	cairo_reset_clip(cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	cairo_set_line_width(cr, .75);
	CairoSetSouerceRGBA(c_g60);
	rounded_rectangle (cr, GM_LEFT, GM_TOP, GM_GIRTH, GM_SCALE, 6);
	cairo_stroke(cr);

	cairo_destroy(cr);
}

/******************************************************************************
 * main drawing
 */

static void format_db(char *buf, const float val) {
	if (val > 99) {
		sprintf(buf, "++++");
	}
	else if (val > -10) {
		sprintf(buf, "%+.1f", val);
	}
	else if (val > -99.9) {
		sprintf(buf, "%.0f ", val);
	}
	else {
		sprintf(buf, " -\u221E ");
	}
}

static void format_val(char *buf, const float val) {
	if (val > 99) {
		sprintf(buf, "+++++");
	}
	else if (val > -99.9) {
		sprintf(buf, "%+5.1f", val);
	}
	else {
		sprintf(buf, " -\u221E  ");
	}
}


static bool expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev) {
	SAUI* ui = (SAUI*)GET_HANDLE(handle);

	if (ui->size_changed) {
		reinitialize_font_cache(ui);
		alloc_annotations(ui);
		//robtk_dial_set_surface(ui->spn_speed, ui->dial); // XXX
		create_meter_pattern(ui);
	}
	if (ui->metrics_changed || ui->size_changed) {
		realloc_metrics(ui);
		prepare_metersurface(ui);
		ui->metrics_changed = false;
		ui->size_changed = false;
	}

	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	/* metric areas */
	if (rect_intersect_a(ev, 0, 0, MA_WIDTH, GM_HEIGHT)) {
		cairo_set_source_surface(cr, ui->ma[0], 0, 0);
		cairo_paint (cr);
	}
	if (rect_intersect_a(ev, MA_WIDTH + GM_WIDTH * ui->num_meters, 0, MA_WIDTH, GM_HEIGHT)) {
		cairo_set_source_surface(cr, ui->ma[1], MA_WIDTH + GM_WIDTH * ui->num_meters, 0);
		cairo_paint (cr);
	}

	for (uint32_t i = 0; i < ui->num_meters ; ++i) {
		if (!rect_intersect_a(ev, MA_WIDTH + GM_WIDTH * i, 0, GM_WIDTH, GM_HEIGHT)) continue;

		const int v_old = ui->val_vis[i];
		const int v_new = ui->val_def[i];
		const int m_old = ui->peak_vis[i];
		const int m_new = ui->peak_def[i];

		if (v_old != v_new || m_old != m_new || ui->show_peaks_changed) {
			ui->val_vis[i] = v_new;
			ui->peak_vis[i] = m_new;
			render_meter(ui, i, v_old, v_new, m_old, m_new);
		}
		cairo_set_source_surface(cr, ui->sf[i], MA_WIDTH + GM_WIDTH * i, 0);
		cairo_paint (cr);
	}

	if (ev->width >= ui->width && ev->height >= ui->height) {
		/* full expose/redraw */
		ui->show_peaks_changed = false;
	}

	/* numerical peak and value */
	if (!ui->display_freq) {
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		for (uint32_t i = 0; i < ui->num_meters ; ++i) {
			char buf[8];
			if (rect_intersect_a(ev, MA_WIDTH + GM_WIDTH * i + 2, 5, GM_WIDTH-4, GM_PEAK)) {
				cairo_save(cr);
				rounded_rectangle (cr, MA_WIDTH + GM_WIDTH * i + 2, 5, GM_WIDTH-4, GM_PEAK, 4);
				if (ui->peak_val[i] >= -1.0) {
					CairoSetSouerceRGBA(c_ptr);
				} else {
					CairoSetSouerceRGBA(c_blk);
				}
				cairo_fill(cr);
				rounded_rectangle (cr, MA_WIDTH + GM_WIDTH * i + 1.5, 5.5, GM_WIDTH-3, GM_PEAK - 1, 4);
				cairo_set_line_width(cr, 1);
				CairoSetSouerceRGBA(c_g60);
				cairo_stroke_preserve (cr);
				cairo_clip (cr);
				format_db(buf, ui->peak_val[i]);

				write_text(cr, buf, FONT_VAL, MA_WIDTH + GM_WIDTH * (i + 1) - 5, 5 + ceil(GM_PEAK * .5), 0, 1, c_g90);
				cairo_restore(cr);
			}

			if (rect_intersect_a(ev, MA_WIDTH + GM_WIDTH * i + 2, GM_TXT - 5, GM_WIDTH-4, GM_PEAK)) {
				cairo_save(cr);
				rounded_rectangle (cr, MA_WIDTH + GM_WIDTH * i + 2, GM_TXT - 5, GM_WIDTH-4, GM_PEAK, 4);
				CairoSetSouerceRGBA(ui->c_bgr);
				cairo_fill(cr);
				rounded_rectangle (cr, MA_WIDTH + GM_WIDTH * i + 1.5, GM_TXT - 4.5, GM_WIDTH-3, GM_PEAK - 1, 4);
				cairo_set_line_width(cr, 1.0);
				CairoSetSouerceRGBA(c_g60);
				cairo_stroke_preserve (cr);
				cairo_clip (cr);
				format_db(buf, ui->val[i]);
				write_text(cr, buf, FONT_VAL, MA_WIDTH + GM_WIDTH * (i + 1) - 5, GM_TXT - 5 + ceil(GM_PEAK * .5), 0, 1, c_g90);
				cairo_restore(cr);
			}
		}
	}

	/* labels */
	if (ui->display_freq) {
		cairo_set_operator (cr, CAIRO_OPERATOR_SCREEN);
		for (uint32_t i = 0; i < ui->num_meters ; ++i) {
			if (!rect_intersect_a(ev, MA_WIDTH + GM_WIDTH * i, GM_TXT, 24, 64)) continue;
			cairo_set_source_surface(cr, ui->an[i], MA_WIDTH + GM_WIDTH * i + rintf(.5 * (GM_WIDTH - 13)), GM_TXT);
			cairo_paint (cr);
		}
	}

	/* highlight */
	if (ui->highlight >= 0 && ui->highlight < (int) ui->num_meters &&
			rect_intersect_a(ev, MA_WIDTH + GM_WIDTH * ui->highlight + GM_WIDTH/2 - AN_WIDTH, GM_TXT -4.5, 2 * AN_WIDTH, AN_HEIGHT)) {
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		const int i = ui->highlight;
		char buf[32], bufv[8], bufp[8];
		format_val(bufv, ui->val[i]);
		format_val(bufp, ui->peak_val[i]);
		sprintf(buf, "%s\nc:%s\np:%s"
				, freq_table[i], bufv, bufp);
		cairo_save(cr);
		cairo_set_line_width(cr, 0.75);
		CairoSetSouerceRGBA(c_g90);
		cairo_move_to(cr, MA_WIDTH + GM_WIDTH * i + GM_LEFT + GM_GIRTH/2, GM_TXT - 8.5);
		cairo_line_to(cr, MA_WIDTH + GM_WIDTH * i + GM_LEFT + GM_GIRTH/2, GM_TXT - 4.5);
		cairo_stroke(cr);
		rounded_rectangle (cr, MA_WIDTH + GM_WIDTH * i + GM_WIDTH/2 - AN_WIDTH, GM_TXT -4.5, 2 * AN_WIDTH, AN_HEIGHT, 3);
		CairoSetSouerceRGBA(c_xfb);
		cairo_fill_preserve (cr);
		CairoSetSouerceRGBA(c_g60);
		cairo_stroke_preserve (cr);
		cairo_clip (cr);
		write_text(cr, buf, FONT_SPK, MA_WIDTH + GM_WIDTH * i + GM_WIDTH/2, GM_TXT + 18 * ui->scale, 0, 2, c_g90);
		cairo_restore(cr);
	}

	return TRUE;
}

/******************************************************************************
 * UI callbacks
 */

static RobWidget* cb_reset_peak (RobWidget* handle, RobTkBtnEvent *event) {
	SAUI* ui = (SAUI*)GET_HANDLE(handle);
	ui->reset_toggle = !ui->reset_toggle;
	/* reset peak-hold in backend */
	float temp = ui->reset_toggle ? 1.0 : 2.0;
	ui->write(ui->controller, ui->display_freq? 61 : 0,
			sizeof(float), 0, (const void*) &temp);

	for (uint32_t i=0; i < ui->num_meters ; ++i) {
		ui->peak_val[i] = -100;
		ui->peak_def[i] = deflect(ui, -100);
	}
	queue_draw(ui->m0);
	return NULL;
}

static bool set_gain(RobWidget* w, void* handle) {
	SAUI* ui = (SAUI*)handle;
	float oldgain = ui->gain;
	ui->gain = robtk_scale_get_value(ui->fader);
#if 1
	if (ui->gain < -12) ui->gain = -12;
	if (ui->gain >= 32.0) ui->gain = 32.0;
#endif
	if (oldgain == ui->gain) return TRUE;
	if (!ui->disable_signals) {
		ui->write(ui->controller, 62, sizeof(float), 0, (const void*) &ui->gain);
	}
	if (ui->display_freq) { // should actually always be true here
#if 0
		for (uint32_t pidx=0; pidx < ui->num_meters ; ++pidx) {
			invalidate_meter(ui, pidx, ui->val[pidx], ui->peak_val[pidx]);
		}
#else
		ui->initialize = 1;
		float temp = -3;
		ui->write(ui->controller, ui->display_freq? 61 : 0,
				sizeof(float), 0, (const void*) &temp);
#endif
	}
	ui->metrics_changed = true;
	queue_draw(ui->m0);
	return TRUE;
}

static bool set_speed(RobWidget* w, void* handle) {
	SAUI* ui = (SAUI*)handle;
	if (!ui->disable_signals) {
		float val = INV_RESPSCALE(robtk_dial_get_value(ui->spn_speed));
		//printf("set_speed %f -> %f\n", robtk_dial_get_value(ui->spn_speed), val);
		ui->write(ui->controller, 60, sizeof(float), 0, (const void*) &val);
	}
	return TRUE;
}

static bool set_peakdisplay(RobWidget* w, void* handle) {
	SAUI* ui = (SAUI*)handle;
	bool show_peaks = robtk_cbtn_get_active(ui->btn_peaks);
	ui->misc_state &=~1;
	if (show_peaks) ui->misc_state |=~1;
	ui->show_peaks = show_peaks;
	ui->show_peaks_changed = true;
	if (!ui->disable_signals) {
		float misc_state = ui->misc_state;
		ui->write(ui->controller, 63, sizeof(float), 0, (const void*) &misc_state);
	}
	queue_draw(ui->m0);
	return TRUE;
}

static RobWidget* mousemove(RobWidget* handle, RobTkBtnEvent *event) {
	SAUI* ui = (SAUI*)GET_HANDLE(handle);
	if (event->y < GM_TOP || event->y > (GM_TOP + GM_SCALE)) {
		if (ui->highlight != -1) { queue_draw(ui->m0); }
		ui->highlight = -1;
		return NULL;
	}
	const int x = event->x - MA_WIDTH;
	const int remain =  x % ((int) GM_WIDTH);
	if (remain < GM_LEFT || remain > GM_LEFT + GM_GIRTH) {
		if (ui->highlight != -1) { queue_draw(ui->m0); }
		ui->highlight = -1;
		return NULL;
	}

	const uint32_t mtr = x / ((int) GM_WIDTH);
	if (mtr < ui->num_meters) {
		if (ui->highlight != (int) mtr) { queue_draw(ui->m0); }
		ui->highlight = mtr;
	} else {
		if (ui->highlight != -1) { queue_draw(ui->m0); }
		ui->highlight = -1;
	}
	return handle;
}

/******************************************************************************
 * widget hackery
 */

static void
size_request(RobWidget* handle, int *w, int *h) {
	SAUI* ui = (SAUI*)GET_HANDLE(handle);
	*w = ui->min_width;
	*h = GM_MINH;
}

static void
top_size_allocate(RobWidget* rw, int w, int h) {
	assert (rw->childcount == 3);
	SAUI* ui = (SAUI*)GET_HANDLE(rw->children[0]->children[0]);
	GLrobtkLV2UI * const self = (GLrobtkLV2UI*) robwidget_get_toplevel_handle(rw);

	if (ui->_min_w == 0 && ui->_min_h == 0) {
		if (rw->widget_scale == 1.0) {
			ui->_min_w = rw->area.width;
			ui->_min_h = rw->area.height;
		} else {
			rhbox_size_allocate (rw, w, h);
			return;
		}
	}
	assert (ui->_min_w > 1 && ui->_min_h > 1);

	const float scale_x = w / ui->_min_w;
	const float scale_y = h / ui->_min_h;
	rw->widget_scale =  MAX(1.0, MIN(2.0, floor (MIN(scale_y, scale_x) * 10) / 10));

	if (self->queue_widget_scale != rw->widget_scale) {
		self->queue_widget_scale = rw->widget_scale;
		puglPostResize(self->view);
		queue_draw (rw);
	}
	rhbox_size_allocate (rw, w, h);
}

static void
size_allocate(RobWidget* handle, int w, int h) {
	SAUI* ui = (SAUI*)GET_HANDLE(handle);
	ui->height = floor(h/2) * 2;
	ui->width = w;
	ui->size_changed = true;

	float scale_y = ui->height / GM_MINH;
	float scale_x = (float) ui->width / ui->min_width;
	ui->scale = MAX(1.0, MIN(2.5, MIN(scale_y, scale_x)));

	if (ui->display_freq) {
		ui->gm_width = MIN(40, floor ((w - 2.0 * MA_WIDTH) / ui->num_meters));
		ui->gm_girth = rintf (ui->gm_width * .75);
		ui->gm_left = .5 + floor(.5 * (ui->gm_width - ui->gm_girth));
		ui->cur_width = 2.0 * MA_WIDTH + ui->num_meters * GM_WIDTH;
	} else {
		ui->gm_width = floor ((w - 2.0 * MA_WIDTH) / ui->num_meters);
		if (ui->gm_width > 60) ui->gm_width = 60;
		ui->gm_girth = rintf (ui->gm_width * .42);
		ui->gm_left = .5 + floor(.5 * (ui->gm_width - ui->gm_girth));
		ui->cur_width = 2.0 * MA_WIDTH + ui->num_meters * GM_WIDTH;
	}
	robwidget_set_size(handle, MIN(ui->width, ui->cur_width), h);
	queue_draw(ui->m0);
}

static RobWidget * toplevel(SAUI* ui, void * const top)
{
	/* main widget: layout */
	ui->rw = rob_hbox_new(FALSE, 2);
	robwidget_make_toplevel(ui->rw, top);
	if (ui->display_freq) {
		robwidget_toplevel_enable_scaling (ui->rw);
	}

	/* DPM main drawing area */
	ui->m0 = robwidget_new(ui);
	ROBWIDGET_SETNAME(ui->m0, "dpm (m0)");

	robwidget_set_expose_event(ui->m0, expose_event);
	robwidget_set_size_request(ui->m0, size_request);
	robwidget_set_size_allocate(ui->m0, size_allocate);
	robwidget_set_mousedown(ui->m0, cb_reset_peak);
	if (ui->display_freq) {
		robwidget_set_mousemove(ui->m0, mousemove);
	}
	ui->sep_h0 = robtk_sep_new(FALSE);

	/* vbox on the right */
	ui->c_box = rob_vbox_new(FALSE, 2);

	ui->fader     = robtk_scale_new(-12, 32.0, .05, FALSE);
	ui->lbl_speed = robtk_lbl_new("Response [s]");
	ui->spn_speed = robtk_dial_new_with_size(RESPSCALE(.05), RESPSCALE(8), .1, GED_WIDTH, GED_HEIGHT+10, GED_CX, GED_CY+10, GED_RADIUS);
	ui->btn_peaks = robtk_cbtn_new("Peak Hold", GBT_LED_LEFT, true);
	robtk_cbtn_set_active(ui->btn_peaks, true);
	robtk_dial_set_default(ui->spn_speed, RESPSCALE(1.0f));
	robtk_dial_set_scaled_surface_scale (ui->spn_speed, ui->dial, 2.0);

	robtk_scale_set_default(ui->fader, 0);
	robtk_scale_set_value(ui->fader, 0);
	robtk_scale_add_mark(ui->fader,  -9,  "-9dB");
	robtk_scale_add_mark(ui->fader,  -6,  "-6dB");
	robtk_scale_add_mark(ui->fader,  -3,  "-3dB");
	robtk_scale_add_mark(ui->fader,   0,   "0dB");
	robtk_scale_add_mark(ui->fader,   3,  "+3dB");
	robtk_scale_add_mark(ui->fader,   6,  "+6dB");
	robtk_scale_add_mark(ui->fader,   9,  "+9dB");
	robtk_scale_add_mark(ui->fader,  10,  "");
	robtk_scale_add_mark(ui->fader,  12, "+12dB");
	robtk_scale_add_mark(ui->fader,  15, "+15dB");
	robtk_scale_add_mark(ui->fader,  18, "+18dB");
	robtk_scale_add_mark(ui->fader,  20, "+20dB");
	robtk_scale_add_mark(ui->fader,  25, "+25dB");
	robtk_scale_add_mark(ui->fader,  30, "+30dB");

	/* layout */

	ui->m_box = rob_vbox_new(FALSE, 0);
	rob_vbox_child_pack(ui->m_box, ui->m0, TRUE, TRUE);
	rob_hbox_child_pack(ui->rw, ui->m_box, TRUE, TRUE);

	if (ui->display_freq) {
		rob_hbox_child_pack(ui->rw, robtk_sep_widget(ui->sep_h0), FALSE, TRUE);
		rob_hbox_child_pack(ui->rw, ui->c_box, FALSE, TRUE);

		rob_vbox_child_pack(ui->c_box, robtk_scale_widget(ui->fader), TRUE, TRUE);
#if 1 // display speed, value-LPF config
		rob_vbox_child_pack(ui->c_box, robtk_lbl_widget(ui->lbl_speed), FALSE, FALSE);
		rob_vbox_child_pack(ui->c_box, robtk_dial_widget(ui->spn_speed), FALSE, FALSE);
#endif
		rob_vbox_child_pack(ui->c_box, robtk_cbtn_widget(ui->btn_peaks), FALSE, FALSE);
	}

	/* callbacks */
	robtk_scale_set_callback(ui->fader, set_gain, ui);
	robtk_dial_set_callback(ui->spn_speed, set_speed, ui);
	robtk_cbtn_set_callback(ui->btn_peaks, set_peakdisplay, ui);

	/* change _after_ packing, (packing checks allocate fn ptr) */
	if (ui->display_freq) {
		ui->rw->size_allocate = top_size_allocate;
	}

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
	SAUI* ui = (SAUI*) calloc(1,sizeof(SAUI));
	*widget = NULL;

	if      (!strcmp(plugin_uri, MTR_URI "spectr30mono")) { ui->num_meters = 30; ui->display_freq = true; }
	else if (!strcmp(plugin_uri, MTR_URI "spectr30stereo")) { ui->num_meters = 30; ui->display_freq = true; }
	else if (!strcmp(plugin_uri, MTR_URI "dBTPmono")) { ui->num_meters = 1; ui->display_freq = false; }
	else if (!strcmp(plugin_uri, MTR_URI "dBTPstereo")) { ui->num_meters = 2; ui->display_freq = false; }
	else {
		free(ui);
		return NULL;
	}
	ui->write      = write_function;
	ui->controller = controller;
	ui->scale = 1.0;

	get_color_from_theme(0, ui->c_txt);
	get_color_from_theme(1, ui->c_bgr);

	ui->gain = 0;
	ui->cache_sf = -100;
	ui->cache_ma = -100;
	ui->highlight = -1;
	ui->metrics_changed = true;
	ui->size_changed = true;

	ui->_min_w = 0;
	ui->_min_h = 0;

	ui->show_peaks_changed = false;
	ui->show_peaks = true;
	ui->misc_state = 1;

	for (uint32_t i=0; i < ui->num_meters ; ++i) {
		ui->val[i] = -100.0;
		ui->val_def[i] = deflect(ui, -100);
		ui->peak_val[i] = -100.0;
		ui->peak_def[i] = deflect(ui, -100);
	}
	ui->disable_signals = false;

	if (ui->display_freq) {
		ui->gm_width = 13.f;
		ui->gm_girth = 10.f;
		ui->gm_left  = 1.5f;
	} else {
		ui->gm_width = 28.f;
		ui->gm_girth = 12.f;
		ui->gm_left  = 8.5f;
	}
	ui->min_width = 2.0 * MA_WIDTH + ui->num_meters * GM_WIDTH;
	ui->width = ui->min_width;
	ui->height = GM_MINH;

	reinitialize_font_cache(ui);
	alloc_annotations(ui);

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
	SAUI* ui = (SAUI*)handle;
	for (uint32_t i=0; i < ui->num_meters ; ++i) {
		cairo_surface_destroy(ui->sf[i]);
		cairo_surface_destroy(ui->an[i]);
	}
	for (int i=0; i < 4; ++i) {
		pango_font_description_free(ui->font[i]);
	}
	cairo_pattern_destroy(ui->mpat);
	cairo_surface_destroy(ui->ma[0]);
	cairo_surface_destroy(ui->ma[1]);
	cairo_surface_destroy(ui->dial);

	robtk_scale_destroy(ui->fader);
	robtk_lbl_destroy(ui->lbl_speed);
	robtk_dial_destroy(ui->spn_speed);
	robtk_cbtn_destroy(ui->btn_peaks);
	robtk_sep_destroy(ui->sep_h0);
	rob_box_destroy(ui->c_box);

	robwidget_destroy(ui->m0);
	rob_box_destroy(ui->m_box);
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
static void invalidate_meter(SAUI* ui, int mtr, float val, float peak) {
	const int v_old = ui->val_def[mtr];
	const int v_new = deflect(ui, val + ui->gain);

	const int m_old = ui->peak_def[mtr];
	const int m_new = ceilf(deflect(ui, peak + ui->gain) / 2.0) * 2.0;
	int t, h;

#define INVALIDATE_RECT(XX,YY,WW,HH) \
		queue_tiny_area(ui->m0, XX, YY, WW, HH);

	if (rintf(ui->val[mtr] * 10.0f) != rintf(val * 10.0f) && !ui->display_freq) {
		INVALIDATE_RECT(mtr * GM_WIDTH + MA_WIDTH, GM_TXT - 5, GM_WIDTH, GM_PEAK + 1);
	}

	if (ui->highlight == mtr && ui->display_freq &&
			(rintf(ui->val[mtr] * 10.0f) != rintf(val * 10.0f) || rintf(m_old * 10.0f) != rintf(m_new * 10.0f))) {
		queue_tiny_area(ui->m0, mtr * GM_WIDTH + MA_WIDTH + GM_WIDTH/2 - AN_WIDTH -.5, GM_TXT - 8, 1 + 2 * AN_WIDTH, FQ_ANN);
	}

	if (rintf(ui->peak_val[mtr] * 10.0f) != rintf(peak * 10.0f) && !ui->display_freq) {
		INVALIDATE_RECT(mtr * GM_WIDTH + MA_WIDTH, 5, GM_WIDTH, GM_PEAK + 1);
	}

	ui->val[mtr] = val;
	ui->val_def[mtr] = v_new;
	ui->peak_val[mtr] = peak;
	ui->peak_def[mtr] = m_new;

	if (v_old != v_new) {
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

	if (m_old != m_new && ui->show_peaks) {
		if (m_old > m_new) {
			t = m_old;
			h = m_old - m_new;
		} else {
			t = m_new;
			h = m_new - m_old;
		}

		INVALIDATE_RECT(
				mtr * GM_WIDTH + MA_WIDTH + GM_LEFT - 1,
				GM_TOP + GM_SCALE - t - 1,
				GM_GIRTH + 2, h+4);
	}
}

/******************************************************************************
 * handle data from backend
 */

static void handle_spectrum_connections(SAUI* ui, uint32_t port_index, float v) {

	if (port_index == 62) {
		if (v >= -12 && v <= 32.0) {
			ui->disable_signals = true;
			robtk_scale_set_value(ui->fader, v);
			ui->disable_signals = false;
		}
	} else
	if (port_index == 63) {
		if (v >= 0 && v <= 256.0) {
			ui->disable_signals = true;
			robtk_cbtn_set_active(ui->btn_peaks, (((int)v)&1) == 1);
			ui->disable_signals = false;
		}
	} else
	if (port_index == 60) {
		ui->disable_signals = true;
		if (v > 0 && v < 15) {
			robtk_dial_set_value(ui->spn_speed, RESPSCALE(v));
		}
		ui->disable_signals = false;
	} else
	if (v > -500 && port_index < 30) {
		int pidx = port_index;
		float np = ui->peak_val[pidx];
		invalidate_meter(ui, pidx, v, np);
	}
	if (v > -500 && port_index >= 30  && port_index < 60) {
		int pidx = port_index - 30;
		float nv = ui->val[pidx];
		invalidate_meter(ui, pidx, nv, v);
	}
}

static void handle_meter_connections(SAUI* ui, uint32_t port_index, float v) {
	if (v <= -500) return;
	v = v > .00001f ? 20.0 * log10f(v) : -100.0;
	if (port_index == 3) {
		invalidate_meter(ui, 0, v, ui->peak_val[0]);
	}
	else if (port_index == 6) {
		invalidate_meter(ui, 1, v, ui->peak_val[1]);
	}

	/* peak data from backend */
	if (ui->num_meters == 1) {
		if (port_index == 4) {
			float np = ui->peak_val[0];
			if (v != np)  { np = v; }
			invalidate_meter(ui, 0, ui->val[0], np);
		}
	} else if (ui->num_meters == 2) {
		if (port_index == 7) {
			float np = ui->peak_val[0];
			if (v != np)  { np = v; }
			invalidate_meter(ui, 0, ui->val[0], np);
		}
		else if (port_index == 8) {
			float np = ui->peak_val[1];
			if (v != np)  { np = v; }
			invalidate_meter(ui, 1, ui->val[1], np);
		}
	}
}

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
	SAUI* ui = (SAUI*)handle;
	if (format != 0) return;

	if (ui->initialize == 0 && port_index == (ui->display_freq? 61 : 0)) {
		ui->initialize = 1;
		float temp = -3;
		ui->write(ui->controller, ui->display_freq? 61 : 0,
				sizeof(float), 0, (const void*) &temp);
	} else if (ui->initialize == 1 && *(float *)buffer <= -500 && (
			(ui->display_freq && port_index >= 30 && port_index < 60)
			|| (!ui->display_freq && ui->num_meters == 1 && port_index == 4)
			|| (!ui->display_freq && ui->num_meters == 2 && port_index == 7)
			)) {
		ui->initialize = 2;
		float temp = -4;
		ui->write(ui->controller, ui->display_freq? 61 : 0,
				sizeof(float), 0, (const void*) &temp);
	}

	if (ui->display_freq) {
		handle_spectrum_connections(ui, port_index, *(float *)buffer);
	} else {
		handle_meter_connections(ui, port_index, *(float *)buffer);
	}

}
