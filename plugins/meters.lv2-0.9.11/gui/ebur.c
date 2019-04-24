/* ebu-r128 LV2 GUI
 *
 * Copyright 2012-2013 Robin Gareus <robin@gareus.org>
 * Copyright 2011-2012 David Robillard <d@drobilla.net>
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

#define RTK_URI "http://gareus.org/oss/lv2/meters#"
#define RTK_GUI "eburui"

#define GBT_W(PTR) robtk_cbtn_widget(PTR)
#define GRB_W(PTR) robtk_rbtn_widget(PTR)
#define GSP_W(PTR) robtk_spin_widget(PTR)
#define GPB_W(PTR) robtk_pbtn_widget(PTR)
#define GLB_W(PTR) robtk_lbl_widget(PTR)

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "src/uris.h"

#ifndef MAX
#define MAX(A,B) ( (A) > (B) ? (A) : (B) )
#endif

/*************************/
#define CX  (178.5f)
#define CY  (196.5f)

#define COORD_ALL_W 356
#define COORD_ALL_H 412

#define COORD_MTR_X 23
#define COORD_MTR_Y 52

#define COORD_MX_X 298  // max display X
#define COORD_TP_X 25   // true peak

#define COORD_ML_Y 11   // TOP

#define COORD_BI_X 8    // integrating X
#define COORD_BR_X (COORD_MX_X-65) // bottom right info
#define COORD_BI_Y 341  // integrating

#define COORD_LEVEL_X 118 //used w/ COORD_ML_Y
#define COORD_BINFO_Y 314 // used with COORD_BI_X

#define CLPX(X) (X+13)
#define CLPY(Y) (Y+6)

/* some width and radii, */

#define COORD_BINTG_W 340

#define COORD_BINFO_W 117
#define COORD_BINFO_H 40
#define COORD_LEVEL_W 120
#define COORD_LEVEL_H 24

#define RADIUS   (120.0f)
#define RADIUS1  (122.0f)
#define RADIUS5  (125.0f)
#define RADIUS10 (130.0f)
#define RADIUS19 (139.0f)
#define RADIUS22 (142.0f)
#define RADIUS23 (143.0f)

/*************************/

enum {
	FONT_M14 = 0,
	FONT_M12,
	FONT_M09,
	FONT_M08,
	FONT_S09,
	FONT_S08
};

typedef struct {
	LV2_Atom_Forge forge;

	LV2_URID_Map* map;
	EBULV2URIs   uris;

	LV2UI_Write_Function write;
	LV2UI_Controller     controller;

	RobWidget* box;

	RobTkCBtn* btn_start;
	RobTkPBtn* btn_reset;

	RobWidget* cbx_box;
	RobTkRBtn* cbx_lufs;
	RobTkRBtn* cbx_lu;
	RobTkRBtn* cbx_sc9;
	RobTkRBtn* cbx_sc18;
	RobTkRBtn* cbx_sc24;
	RobTkRBtn* cbx_ring_short;
	RobTkRBtn* cbx_ring_mom;
	RobTkRBtn* cbx_hist_short;
	RobTkRBtn* cbx_hist_mom;
	RobTkCBtn* cbx_transport;
	RobTkCBtn* cbx_autoreset;
	RobTkCBtn* cbx_truepeak;

	RobTkRBtn* cbx_radar;
	RobTkRBtn* cbx_histogram;

	RobTkSpin* spn_radartime;
	RobTkLbl* lbl_ringinfo;
	RobTkLbl* lbl_radarinfo;

	RobTkSep* sep_h0;
	RobTkSep* sep_h1;
	RobTkSep* sep_h2;
	RobTkSep* sep_v0;

	RobWidget* m0;

	cairo_pattern_t * cpattern;
	cairo_pattern_t * lpattern9;
	cairo_pattern_t * lpattern18;
	cairo_pattern_t * hpattern9;
	cairo_pattern_t * hpattern18;

	cairo_surface_t * level_surf;
	cairo_surface_t * radar_surf;
	cairo_surface_t * lvl_label;
	cairo_surface_t * hist_label;

	bool redraw_labels;
	bool fontcache;
	PangoFontDescription *font[6];

	bool disable_signals;

	/* current data */
	float lm, mm, ls, ms, il, rn, rx, it, tp;

	float *radarS;
	float *radarM;
	int radar_pos_cur;
	int radar_pos_max;

	int histS[HIST_LEN];
	int histM[HIST_LEN];
	int histLenS;
	int histLenM;

	/* displayed data */
	int radar_pos_disp;
	int circ_max;
	int circ_val;
	bool fullhist;
	int  fastradar;
	bool fasthist;

	bool fasttracked[5];
	float prev_lvl[5]; // ls,lm,mm,ms, tp
	const char *nfo;
} EBUrUI;


/******************************************************************************
 * fast math helpers
 */

static inline float fast_log2 (float val)
{
	union {float f; int i;} t;
	t.f = val;
	int * const    exp_ptr =  &t.i;
	int            x = *exp_ptr;
	const int      log_2 = ((x >> 23) & 255) - 128;
	x &= ~(255 << 23);
	x += 127 << 23;
	*exp_ptr = x;

	val = ((-1.0f/3) * t.f + 2) * t.f - 2.0f/3;

	return (val + log_2);
}

static inline float fast_log10 (const float val)
{
	return fast_log2(val) * 0.301029996f;
}

/******************************************************************************
 * meter colors
 */

static float radar_deflect(const float v, const float r) {
	if (v < -60) return 0;
	if (v > 0) return r;
	return (v + 60.0) * r / 60.0;
}

static void radar_color(cairo_t* cr, const float v) {
	const float alpha = 1.0;

	if (v < -70) {
		cairo_set_source_rgba (cr, .3, .3, .3, alpha);
	} else if (v < -53) {
		cairo_set_source_rgba (cr, .0, .0, .5, alpha);
	} else if (v < -47) {
		cairo_set_source_rgba (cr, .0, .0, .9, alpha);
	} else if (v < -35) {
		cairo_set_source_rgba (cr, .0, .6, .0, alpha);
	} else if (v < -23) {
		cairo_set_source_rgba (cr, .0, .9, .0, alpha);
	} else if (v < -11) {
		cairo_set_source_rgba (cr, .75, .75, .0, alpha);
	} else if (v < -7) {
		cairo_set_source_rgba (cr, .8, .4, .0, alpha);
	} else if (v < -3.5) {
		cairo_set_source_rgba (cr, .75, .0, .0, alpha);
	} else {
		cairo_set_source_rgba (cr, 1.0, .0, .0, alpha);
	}
}

static cairo_pattern_t * radar_pattern(cairo_t* crx, float cx, float cy, float rad) {
	cairo_pattern_t * pat = cairo_pattern_create_radial(cx, cy, 0, cx, cy, rad);
	cairo_pattern_add_color_stop_rgba(pat, 0.0 ,  .05, .05, .05, 1.0); // radar BG
	cairo_pattern_add_color_stop_rgba(pat, 0.05,  .0, .0, .0, 1.0); // -57

	cairo_pattern_add_color_stop_rgba(pat, radar_deflect(-53.0, 1.0),  .0, .0, .5, 1.0);

	cairo_pattern_add_color_stop_rgba(pat, radar_deflect(-48.0, 1.0),  .0, .0, .8, 1.0);
	cairo_pattern_add_color_stop_rgba(pat, radar_deflect(-46.5, 1.0),  .0, .5, .4, 1.0);

	cairo_pattern_add_color_stop_rgba(pat, radar_deflect(-36.0, 1.0),  .0, .6, .0, 1.0);
	cairo_pattern_add_color_stop_rgba(pat, radar_deflect(-34.0, 1.0),  .0, .8, .0, 1.0);

	cairo_pattern_add_color_stop_rgba(pat, radar_deflect(-24.0, 1.0),  .1, .9, .1, 1.0);
	cairo_pattern_add_color_stop_rgba(pat, radar_deflect(-22.0, 1.0), .75,.75, .0, 1.0);

	cairo_pattern_add_color_stop_rgba(pat, radar_deflect(-11.5, 1.0),  .8, .4, .0, 1.0);
	cairo_pattern_add_color_stop_rgba(pat, radar_deflect(-10.5, 1.0),  .7, .1, .1, 1.0);

	cairo_pattern_add_color_stop_rgba(pat, radar_deflect( -4.5, 1.0),  .9, .0, .0, 1.0);
	cairo_pattern_add_color_stop_rgba(pat, radar_deflect( -3.5, 1.0),   1, .1, .0, 1.0);
	cairo_pattern_add_color_stop_rgba(pat, 1.0 ,                        1, .1, .0, 1.0);
	return pat;
}

static cairo_pattern_t * histogram_pattern(cairo_t* crx, float cx, float cy, float rad, bool plus9) {
	cairo_pattern_t * pat = cairo_pattern_create_radial(rad, rad, 0, rad, rad, rad);
	cairo_pattern_add_color_stop_rgba(pat, 0.00,  .0, .0, .0, 0.0);
	cairo_pattern_add_color_stop_rgba(pat, 0.06,  .0, .0, .0, 0.0);
	cairo_pattern_add_color_stop_rgba(pat, 0.09,  .1, .1, .1, 1.0);
	cairo_pattern_add_color_stop_rgba(pat, 0.20,  .2, .2, .2, 1.0);
	cairo_pattern_add_color_stop_rgba(pat, 0.50,  .4, .4, .4, 1.0);
	cairo_pattern_add_color_stop_rgba(pat, 0.91,  .5, .5, .5, 1.0);
	cairo_pattern_add_color_stop_rgba(pat, 1.0 , 1.0, .2, .2, 1.0);

	cairo_surface_t *sfx = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rad*2, rad*2);
	cairo_t *cr = cairo_create (sfx);
	CairoSetSouerceRGBA(c_trs);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr, 0, 0, rad*2, rad*2);
	cairo_fill (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	cairo_move_to(cr, rad, rad);
	cairo_set_source (cr, pat);
	cairo_move_to(cr, rad, rad);
	cairo_arc(cr, rad, rad, rad, 0.5 * M_PI, 2.0 * M_PI);
	cairo_fill(cr);
	cairo_pattern_destroy (pat);

	//cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_operator (cr, CAIRO_OPERATOR_HSL_COLOR);
#define PIE(START, END, CR, CG, CB, CA) \
	cairo_set_source_rgba(cr, CR, CG, CB, CA); \
	cairo_move_to(cr, rad, rad); \
	cairo_arc (cr, rad, rad, rad, (.5+(START)) * M_PI, (.5 + (END)) * M_PI); \
	cairo_close_path (cr); \
	cairo_fill (cr);

	if (plus9) {
		PIE(  0/6.0,   2/6.0,  .0,  .4, .0, .6);
		PIE(  2/6.0,   6/6.0,  .0,  .8, .0, .6);
		PIE(  6/6.0,   9/6.0, .75, .75, .0, .6);
	} else {
		// -36..+18
		PIE(  0/6.0,   1/6.0,  .0,  .0, .4, .6);
		PIE(  1/6.0,   2/6.0,  .0,  .0, .8, .6);
		PIE(  2/6.0,   4/6.0,  .0,  .4, .0, .6);
		PIE(  4/6.0,   6/6.0,  .0,  .8, .0, .6);
		PIE(  6/6.0,   8/6.0, .75, .75, .0, .6);
		PIE(  8/6.0, 8.5/6.0,  .8,  .4, .0, .6);
		PIE(8.5/6.0,   9/6.0, 1.0,  .0, .0, .6);
	}

	//cairo_translate (cr, cx-rad, cy-rad);

	cairo_surface_flush(sfx);
	cairo_destroy (cr);

	pat =  cairo_pattern_create_for_surface (sfx);
	cairo_matrix_t m;
	cairo_matrix_init_translate (&m, -(cx-rad), -(cy-rad));
	cairo_pattern_set_matrix (pat, &m);
	cairo_surface_destroy (sfx);

	return pat;
}

/******************************************************************************
 * custom visuals
 */

#define LUFS(V) ((V) < -100 ? -INFINITY : (lufs ? (V) : (V) + 23.0))
#define FONT(A) ui->font[(A)]

static char *format_lufs(char *buf, const float val) {
	if (val < -100)
		sprintf(buf, "   -\u221E");
	else
		sprintf(buf, "%+5.1f", val);
	return buf;
}

static void write_text(
		cairo_t* cr,
		const char *txt,
		PangoFontDescription *font, //const char *font,
		const float x, const float y,
		const float ang, const int align,
		const float * const col) {
	write_text_full(cr, txt, font, x, y, ang, align, col);
}

static cairo_surface_t * hlabel_surface(EBUrUI* ui) {
	cairo_surface_t *sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, RADIUS*2, RADIUS*2);
	cairo_t *cr = cairo_create (sf);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	CairoSetSouerceRGBA(c_gry);
	cairo_set_line_width(cr, 1.0);

#define CIRCLABEL(RDS,LBL) \
	{ \
	cairo_arc (cr, RADIUS, RADIUS, RADIUS * RDS, 0.5 * M_PI, 2.0 * M_PI); \
	cairo_stroke (cr); \
	write_text(cr, LBL, FONT(FONT_M08), RADIUS + RADIUS * RDS, RADIUS + 14.5, M_PI * -.5, 2, c_gry);\
	}
	CIRCLABEL(.301, "20%")
	CIRCLABEL(.602, "40%")
	CIRCLABEL(.903, "80%")
	cairo_destroy (cr);
	return sf;
}

static cairo_surface_t * clabel_surface(EBUrUI* ui, bool plus9, bool plus24, bool lufs) {
	char buf[128];
	cairo_surface_t *sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, COORD_ALL_W, COORD_ALL_H);
	cairo_t *cr = cairo_create (sf);

	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

#define SIN60 0.866025404
#define CLABEL(PT, XS, YS, AL) \
		sprintf(buf, "%+.0f", LUFS(PT)); \
		write_text(cr, buf, FONT(FONT_M08), CX + RADIUS23 * XS, CY + RADIUS23 *YS , 0, AL, c_gry);

	if (plus9) {
		CLABEL(-41,    0.0,   1.0, 8)
		CLABEL(-38,   -0.5, SIN60, 7)
		CLABEL(-35, -SIN60,   0.5, 1)
		CLABEL(-32,   -1.0,   0.0, 1)
		CLABEL(-29, -SIN60,  -0.5, 1)
		CLABEL(-26,   -0.5,-SIN60, 4)
		CLABEL(-23,    0.0,  -1.0, 5)
		CLABEL(-20,    0.5,-SIN60, 6)
		CLABEL(-17,  SIN60,  -0.5, 3)
		CLABEL(-14,    1.0,   0.0, 3)
	} else { // plus18
		CLABEL(-59,    0.0,   1.0, 8)
		CLABEL(-53,   -0.5, SIN60, 7)
		CLABEL(-47, -SIN60,   0.5, 1)
		CLABEL(-41,   -1.0,   0.0, 1)
		CLABEL(-35, -SIN60,  -0.5, 1)
		CLABEL(-29,   -0.5,-SIN60, 4)
		CLABEL(-23,    0.0,  -1.0, 5)
		CLABEL(-17,    0.5,-SIN60, 6)
		CLABEL(-11,  SIN60,  -0.5, 3)
		CLABEL( -5,    1.0,   0.0, 3)
		if (plus24) { // plus24
			CLABEL(1,  SIN60,   0.5, 3)
		}
	}
	cairo_destroy (cr);
	return sf;
}

static void ring_leds(EBUrUI* ui, int *l, int *m) {
	const bool rings = robtk_rbtn_get_active(ui->cbx_ring_short);
	const bool plus9 = robtk_rbtn_get_active(ui->cbx_sc9);

	const float clr = rings ? ui->ls : ui->lm;
	const float cmr = rings ? ui->ms : ui->mm;
	*l = rint(plus9 ? (clr + 41.0f) * 4 : (clr + 59.0f) * 2.0);
	*m = rint(plus9 ? (cmr + 41.0f) * 4 : (cmr + 59.0f) * 2.0);
}

static void initialize_font_cache(EBUrUI* ui) {
	ui->fontcache = true;
	ui->font[FONT_M14] = pango_font_description_from_string("Mono 18px");
	ui->font[FONT_M12] = pango_font_description_from_string("Mono 14px");
	ui->font[FONT_M09] = pango_font_description_from_string("Mono 12px");
	ui->font[FONT_M08] = pango_font_description_from_string("Mono 10px");
	ui->font[FONT_S09] = pango_font_description_from_string("Sans 12px");
	ui->font[FONT_S08] = pango_font_description_from_string("Sans 10px");
	assert(ui->font[FONT_M14]);
	assert(ui->font[FONT_M12]);
	assert(ui->font[FONT_M09]);
	assert(ui->font[FONT_M08]);
	assert(ui->font[FONT_S09]);
	assert(ui->font[FONT_S08]);
}

/******************************************************************************
 * Areas for partial exposure
 */

static void leveldisplaypath(cairo_t *cr) {
	cairo_move_to(cr,CLPX( 37), CLPY(190));
	cairo_line_to(cr,CLPX( 55), CLPY(124));
	cairo_line_to(cr,CLPX( 98), CLPY( 80));
	cairo_line_to(cr,CLPX(165), CLPY( 61));
	cairo_line_to(cr,CLPX(231), CLPY( 80));
	cairo_line_to(cr,CLPX(276), CLPY(126));
	cairo_line_to(cr,CLPX(292), CLPY(190));
	cairo_line_to(cr,CLPX(292), CLPY(200));
	cairo_line_to(cr,CLPX(268), CLPY(268));
	cairo_line_to(cr,CLPX(315), CLPY(268));
	cairo_line_to(cr,CLPX(330), CLPY(200));
	cairo_line_to(cr,CLPX(330), CLPY(185));
	cairo_line_to(cr,CLPX(310), CLPY(112));
	cairo_line_to(cr,CLPX(242), CLPY( 47));
	cairo_line_to(cr,CLPX(165), CLPY( 33));
	cairo_line_to(cr,CLPX( 87), CLPY( 47));
	cairo_line_to(cr,CLPX( 20), CLPY(113));
	cairo_line_to(cr,CLPX(  0), CLPY(190));

#if 0 // outside
	cairo_line_to(cr,  0, 0);
	cairo_line_to(cr,  COORD_ALL_W, 0);
	cairo_line_to(cr,  COORD_ALL_W, COORD_ALL_H);
	cairo_line_to(cr,  0, COORD_ALL_H);
	//cairo_line_to(cr,  0, 0);
	cairo_line_to(cr,  CLP(0), CLP(190));
#endif

	cairo_line_to(cr, CLPX( 25), CLPY(267));
	cairo_line_to(cr, CLPX( 80), CLPY(332));
	cairo_line_to(cr, CLPX(165), CLPY(345));
	cairo_line_to(cr, CLPX(180), CLPY(345));
	cairo_line_to(cr, CLPX(180), CLPY(315));
	cairo_line_to(cr, CLPX(160), CLPY(316));
	cairo_line_to(cr, CLPX(101), CLPY(301));
	cairo_line_to(cr, CLPX( 54), CLPY(254));
	cairo_close_path(cr);

}

const static cairo_rectangle_t rect_is_level = {COORD_MTR_X, COORD_MTR_Y, 320, 290}; // match w/COORD_BINFO
const static cairo_rectangle_t rect_is_radar = {CX-RADIUS-9, CY-RADIUS-9, 2*RADIUS+12, 2*RADIUS+12};

#define RDR_INV_X (CX-RADIUS -2.5) // 43
#define RDR_INV_Y (CY-RADIUS -2.5) // 68
#define RDR_INV_W (2*RADIUS + 5)
#define RDR_INV_H (2*RADIUS + 5)

static cairo_pattern_t * level_pattern (const float cx, const float cy, const float rad, bool plus9) {
	cairo_surface_t *sfx = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rad * 2, rad * 2);
	cairo_t *cr = cairo_create (sfx);

#define COLORPIE(A0, A1, R, G, B) \
	cairo_set_source_rgb (cr, R, G, B); \
	cairo_move_to(cr, rad, rad); \
	cairo_arc(cr, rad, rad, rad, M_PI * (.5 +(A0)) - .0218, M_PI * (.5 + (A1)) - 0.0218); \
	cairo_close_path (cr); \
	cairo_fill (cr);

	if (plus9) {
		COLORPIE(  0/6.0,   2/6.0,  .0,  .4, .0);
		COLORPIE(  2/6.0,   6/6.0,  .0,  .8, .0);
		COLORPIE(  6/6.0,   9/6.0, .75, .75, .0);
	} else {
		// -36..+18
		COLORPIE(  0/6.0,   1/6.0,  .0,  .0, .4);
		COLORPIE(  1/6.0,   2/6.0,  .0,  .0, .8);
		COLORPIE(  2/6.0,   4/6.0,  .0,  .4, .0);
		COLORPIE(  4/6.0,   6/6.0,  .0,  .8, .0);
		COLORPIE(  6/6.0,   8/6.0, .75, .75, .0);
		COLORPIE(  8/6.0, 8.5/6.0,  .8,  .4, .0);
		COLORPIE(8.5/6.0,   9/6.0, 1.0,  .0, .0);
		// and + 24
		COLORPIE(  9/6.0,  11/6.0, 1.0,  .0, .0);
	}

	cairo_surface_flush(sfx);
	cairo_destroy (cr);

	cairo_pattern_t * pat =  cairo_pattern_create_for_surface (sfx);

	cairo_matrix_t m;
	cairo_matrix_init_translate (&m, -cx + rad, -cy + rad);
	cairo_pattern_set_matrix (pat, &m);

	cairo_surface_destroy (sfx);
	return pat;
}

static void prepare_lvl_surface (EBUrUI* ui) {
	cairo_t *cr;
	assert (!ui->level_surf);
	ui->level_surf = cairo_image_surface_create(CAIRO_FORMAT_A8, COORD_ALL_W, ceil(CY) + RADIUS22);

	cr = cairo_create (ui->level_surf);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	cairo_set_line_width(cr, 2.5);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	CairoSetSouerceRGBA(c_g30);

	const int ulp = 120;
	for (int rng = 0; rng <= ulp; ++rng) {
		const float ang = 0.043633231 * rng + 1.570796327;
		float cc = sinf(ang);
		float sc = cosf(ang);
		cairo_move_to(cr,   CX + RADIUS10 * sc, CY + RADIUS10 * cc);
		cairo_line_to(cr, CX + RADIUS19 * sc, CY + RADIUS19 * cc);
		cairo_stroke (cr);
	}
	cairo_destroy (cr);
}

static void render_radar (EBUrUI* ui) {
	cairo_t *cr;
	if (!ui->radar_surf) {
		ui->radar_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, RADIUS * 2, RADIUS * 2);
		cr = cairo_create (ui->radar_surf);
		cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cr);
	} else {
		cr = cairo_create (ui->radar_surf);
	}

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_translate (cr, RADIUS, RADIUS);

	const bool hists = robtk_rbtn_get_active(ui->cbx_hist_short);
	const bool plus9 = robtk_rbtn_get_active(ui->cbx_sc9);

	if (robtk_rbtn_get_active(ui->cbx_histogram)) {
		/* ----- Histogram ----- */
		const int *rdr = hists ? ui->histS : ui->histM;
		const int  len = hists ? ui->histLenS : ui->histLenM;

		cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

		/* histogram background */
		if (len > 0) {
			CairoSetSouerceRGBA(c_g05);
		} else {
			CairoSetSouerceRGBA(c_rd2);
		}
		cairo_arc (cr, 0, 0, RADIUS, 0, 2.0 * M_PI);
		cairo_fill (cr);

		if (len > 0) {
			int amin, amax;
			//  lvlFS = (0.1f * (ang - 700))
			//  lvlFS =  .1 * ang - 70
			if (plus9) { // -41 .. -14 LUFS
				amin = 290; // -41LUFS
				amax = 560; // -14LUFS
			} else { // -59 .. -5 LUFS
				amin = 110; // -59LUFS
				amax = 650; // -5LUFS
			}
			const double astep = 1.5 * M_PI / (double) (amax - amin);
			const double aoff = (M_PI / 2.0) - amin * astep;

			cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
			cairo_set_line_width(cr, 0.75);
			if (plus9) {
				cairo_set_source (cr, ui->hpattern9);
			} else {
				cairo_set_source (cr, ui->hpattern18);
			}

			for (int ang = amin; ang < amax; ++ang) {
				if (rdr[ang] <= 0) continue;
				const float rad = (float) RADIUS * (1.0 + fast_log10(rdr[ang] / (float) len));
				if (rad < 5) continue;

				cairo_move_to(cr, 0, 0);
				cairo_arc (cr, 0, 0, rad,
						(double) (ang-1.0) * astep + aoff, (ang+1.0) * astep + aoff);
				cairo_close_path(cr);
			}
			cairo_fill(cr);

			cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);

			/* outer circle */
			cairo_set_line_width(cr, 1.0);
			CairoSetSouerceRGBA(c_g20);
			cairo_arc (cr, 0, 0, RADIUS, /*0.5 * M_PI */ 0, 2.0 * M_PI);
			cairo_stroke (cr);

			cairo_save(cr);
			cairo_set_source_surface(cr, ui->hist_label, -RADIUS, -RADIUS);
			cairo_paint(cr);
			cairo_restore(cr);

		} else {
			write_text(cr, "No integration\ndata available.", FONT(FONT_S08), RADIUS * .5 , 5, 0, 8, c_g80);
		}

		/* center circle */
		const float innercircle = 6;
		cairo_set_line_width(cr, 1.0);
		CairoSetSouerceRGBA(c_blk);
		cairo_arc (cr, 0, 0, innercircle, 0, 2.0 * M_PI);
		cairo_fill_preserve (cr);
		CairoSetSouerceRGBA(c_gry);
		cairo_stroke(cr);

		cairo_arc (cr, 0, 0, innercircle + 3, .5 * M_PI, 2.0 * M_PI);
		cairo_stroke(cr);

		/* gain lines */
		const double dashed[] = {3.0, 5.0};
		cairo_save(cr);
		cairo_set_dash(cr, dashed, 2, 4.0);
		cairo_set_line_width(cr, 1.5);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		for (int i = 3; i <= 12; ++i) {
			const float ang = .5235994f * i;
			float cc = sinf(ang);
			float sc = cosf(ang);
			cairo_move_to(cr, innercircle * sc, innercircle * cc);
			cairo_line_to(cr, RADIUS5 * sc, RADIUS5 * cc);
			cairo_stroke (cr);
		}
		cairo_restore(cr);

	} else {
		/* ----- History ----- */
		ui->radar_pos_disp = ui->radar_pos_cur;

		/* radar background */
		if (ui->fastradar < 0) {
			//printf("FAST RDR %d cur: %d / %d\n", ui->fastradar, ui->radar_pos_cur, ui->radar_pos_max);
			CairoSetSouerceRGBA(c_g05);
			cairo_arc (cr, 0, 0, RADIUS, 0, 2.0 * M_PI);
			cairo_fill (cr);
		}

		cairo_set_line_width(cr, 1.0);

		if (ui->radar_pos_max > 0) {
			float *rdr = hists ? ui->radarS : ui->radarM;
			const double astep = 2.0 * M_PI / (double) ui->radar_pos_max;

			int a0 = 0;
			int a1 = ui->radar_pos_max;

			if (ui->fastradar >= 0) {
				a0 = (ui->fastradar - 3 + ui->radar_pos_max) % ui->radar_pos_max;
				a1 = a0 + 10;

				cairo_move_to(cr, 0, 0);
				cairo_arc (cr, 0, 0, RADIUS, (double) (a0 + 1.0) * astep, (a0 + 9.f) * astep);
				cairo_close_path(cr);
				CairoSetSouerceRGBA(c_g05);
				cairo_fill (cr);
			}

			cairo_set_source (cr, ui->cpattern);

			for (int ang = a0; ang < a1; ++ang) {
				cairo_move_to(cr, 0, 0);
				cairo_arc (cr, 0, 0, radar_deflect(rdr[ang % ui->radar_pos_max], RADIUS),
						(double) ang * astep, (ang+1.5) * astep);
				cairo_close_path(cr);
			}
			cairo_fill(cr);

			/* fade-out values */
			for (int p = 0; p < 7; ++p) {
				float pos = ui->radar_pos_cur + 1 + p;
				cairo_set_source_rgba (cr, .0, .0, .0, 1.0 - ((p+1.0)/7.0));
				cairo_move_to(cr, 0, 0);
				cairo_arc (cr, 0, 0, RADIUS, pos * astep, (pos + 1.0) * astep);
				cairo_fill(cr);
			}

			/* current position */
			CairoSetSouerceRGBA(c_g7X); // XXX
			cairo_move_to(cr, 0, 0);
			cairo_arc (cr, 0, 0, RADIUS,
						(double) ui->radar_pos_cur * astep, ((double) ui->radar_pos_cur + 1.0) * astep);
			cairo_line_to(cr, 0, 0);
			cairo_fill (cr);
		}
	}
	cairo_destroy (cr);
}

static void draw_radar_ann (cairo_t* cr) {
	cairo_save (cr);
	cairo_translate (cr, CX, CY);
	/* radar lines */
	cairo_set_line_width(cr, 1.5);
	CairoSetSouerceRGBA(c_an0);
	cairo_arc (cr, 0, 0, radar_deflect(-23, RADIUS), 0, 2.0 * M_PI);
	cairo_stroke (cr);

	cairo_set_line_width(cr, 1.0);
	CairoSetSouerceRGBA(c_an1);
	cairo_arc (cr, 0, 0, radar_deflect(-47, RADIUS), 0, 2.0 * M_PI);
	cairo_stroke (cr);
	cairo_arc (cr, 0, 0, radar_deflect(-35, RADIUS), 0, 2.0 * M_PI);
	cairo_stroke (cr);
	cairo_arc (cr, 0, 0, radar_deflect(-11, RADIUS), 0, 2.0 * M_PI);
	cairo_stroke (cr);
	cairo_arc (cr, 0, 0, radar_deflect( 0, RADIUS), 0, 2.0 * M_PI);
	cairo_stroke (cr);

	const float innercircle = radar_deflect(-47, RADIUS);
	for (int i = 0; i < 12; ++i) {
		const float ang = .5235994f * i;
		float cc = sinf(ang);
		float sc = cosf(ang);
		cairo_move_to(cr, innercircle * sc, innercircle * cc);
		cairo_line_to(cr, RADIUS * sc, RADIUS * cc);
	}
	cairo_stroke (cr);
	cairo_restore (cr);
}

//#define DEBUG_DRAW(WHAT) printf("expose: %s\n", WHAT);
#define DEBUG_DRAW(WHAT)

/******************************************************************************
 * Main drawing function
 */

static bool expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev) {
	EBUrUI* ui = (EBUrUI*)GET_HANDLE(handle);
	const bool lufs =  robtk_rbtn_get_active(ui->cbx_lufs);
	const bool rings = robtk_rbtn_get_active(ui->cbx_ring_short);
	const bool plus9 = robtk_rbtn_get_active(ui->cbx_sc9);
	const bool plus24= robtk_rbtn_get_active(ui->cbx_sc24);
	const bool dbtp  = robtk_cbtn_get_active(ui->cbx_truepeak);

	char buf[128];
	char lufb0[15], lufb1[15];
	int redraw_part = 0;
	/* initialized */
	if (!ui->cpattern) {
		ui->cpattern = radar_pattern(cr, 0, 0, RADIUS);
	}
	if (!ui->hpattern9) {
		ui->hpattern9 = histogram_pattern(cr, 0, 0, RADIUS, TRUE);
	}
	if (!ui->hpattern18) {
		ui->hpattern18 = histogram_pattern(cr, 0, 0, RADIUS, FALSE);
	}
	if (!ui->lpattern9) {
		ui->lpattern9 = level_pattern(CX, CY, RADIUS23, TRUE);
	}
	if (!ui->lpattern18) {
		ui->lpattern18 = level_pattern(CX, CY, RADIUS23, FALSE);
	}
	if (!ui->level_surf) {
		prepare_lvl_surface (ui);
	}

	if (!ui->lvl_label || ui->redraw_labels) {
		if (ui->lvl_label) cairo_surface_destroy(ui->lvl_label);
		ui->lvl_label = clabel_surface(ui, plus9, plus24, lufs);
		ui->redraw_labels = FALSE;
	}
	if (!ui->hist_label) {
		ui->hist_label = hlabel_surface(ui);
	}
	/* end initialization */

	DEBUG_DRAW("->>>START----");
#if 0 // DEBUG
	printf("IS: %.1f+%.1f  %.1fx%.1f\n", ev->x, ev->y, ev->width, ev->height);
#endif
	bool clip_set = false;

	if (ev->x == 0 && ev->y == 0 && ev->width == COORD_ALL_W && ev->height == COORD_ALL_H) {
		redraw_part = 3;
	} else if (ev->x == COORD_BR_X && ev->y == COORD_BINFO_Y-1
			&& ev->width == COORD_BINFO_W && ev->height == COORD_BINFO_H+1) {
		// bottom right info box (when integrating)
		redraw_part = 0;
	} else if (ev->x == COORD_MTR_X && ev->y == COORD_MTR_Y) {
		redraw_part = 1;
		if (ui->fullhist) {
			ui->fullhist = false;
			redraw_part |= 2;
		} else {
			leveldisplaypath(cr);
			cairo_clip (cr);
			clip_set = true;
		}
		//printf("NO RDR BUT LVL\n");
	} else if (ev->x >= CX - RADIUS5 && ev->x <= CX+RADIUS5 && ev->y >= CY - RADIUS5 && ev->y <= CY + RADIUS5) {
		/* ie. fastradar */
		redraw_part = 2;
		//printf("RADAR ONLY\n");
		cairo_arc (cr, CX, CY, RADIUS1, 0, 2.0 * M_PI);
		cairo_clip (cr);
		clip_set = true;
	} else {
		redraw_part = 0;
		if (rect_intersect(ev, &rect_is_radar)) {
			redraw_part |= 2;
		}
		if (rect_intersect(ev, &rect_is_level)) {
			redraw_part |= 1;
		}
	}

	if (!clip_set) {
		cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
		cairo_clip (cr);
	}

	ui->fasttracked[0] = ui->fasttracked[1] = ui->fasttracked[2] = ui->fasttracked[3] = ui->fasttracked[4] = false;
	ui->fasthist = false;

	/* fill background */
	rounded_rectangle (cr, 0, 4, COORD_ALL_W, COORD_ALL_H-6, 10);
	CairoSetSouerceRGBA(c_blk);
	cairo_fill_preserve (cr);
	cairo_set_line_width(cr, 0.75);
	CairoSetSouerceRGBA(c_g30);
	cairo_stroke(cr);

	if (rect_intersect_a(ev, 12, COORD_ML_Y, 10, 50)) {
		DEBUG_DRAW("version");
		write_text(cr,
				ui->nfo ? ui->nfo : "x42 EBU R128 LV2",
				FONT(FONT_S08), 1, 15, 1.5 * M_PI, 7, c_g30);
	}

	if (rect_intersect_a(ev, COORD_LEVEL_X, COORD_ML_Y, COORD_LEVEL_W, COORD_LEVEL_H)) {
		DEBUG_DRAW("Big Level Num");
		/* big level as text */
		ui->prev_lvl[0] = rings ? ui->ls : ui->lm;
		sprintf(buf, "%s %s", format_lufs(lufb0, LUFS(ui->prev_lvl[0])), lufs ? "LUFS" : "LU");
		write_text(cr, buf, FONT(FONT_M14), CX , COORD_ML_Y+4, 0, 8, c_wht);
	}

	int trw = lufs ? 87 : 75;

	if (rect_intersect_a(ev, COORD_MX_X, COORD_ML_Y+25, 40, 30)) {
		DEBUG_DRAW("Max Legend");
		/* max legend */
		CairoSetSouerceRGBA(c_g20);
		rounded_rectangle (cr, COORD_MX_X, COORD_ML_Y+25, 40, 30, 10);
		cairo_fill (cr);
		write_text(cr, !rings ? "Mom":"Short", FONT(FONT_S08), COORD_MX_X+20, COORD_ML_Y+25+15, 0, 8, c_wht);
	}

	if (rect_intersect_a(ev, COORD_MX_X+50-trw, COORD_ML_Y, trw, 38)) {
		DEBUG_DRAW("Max Level");
		/* max level background */
		CairoSetSouerceRGBA(c_g30);
		rounded_rectangle (cr, COORD_MX_X+50-trw, COORD_ML_Y, trw, 38, 10);
		cairo_fill (cr);
		/* display max level as text */
		ui->prev_lvl[1] = rings ? ui->ms: ui->mm;
		sprintf(buf, "Max:\n%s %s", format_lufs(lufb0, LUFS(ui->prev_lvl[1])), lufs ? "LUFS" : "LU");
		write_text(cr, buf, FONT(FONT_M09), COORD_MX_X+50-10, COORD_ML_Y+5, 0, 7, c_wht);
	}

	if (dbtp && rect_intersect_a(ev, COORD_TP_X+10, COORD_ML_Y+25, 40, 30)) {
		DEBUG_DRAW("dBTP Legend");
		/* true-peak legend */
		CairoSetSouerceRGBA(c_g20);
		rounded_rectangle (cr, COORD_TP_X+10, COORD_ML_Y+25, 40, 30, 10);
		cairo_fill (cr);
		write_text(cr, "True", FONT(FONT_S08), COORD_TP_X+30, COORD_ML_Y+25+15, 0, 8, c_wht);
	}

	if (dbtp && rect_intersect_a(ev, COORD_TP_X, COORD_ML_Y, 75, 38)) {
		DEBUG_DRAW("dBTP Level");
		/* true peak level */
		if (ui->tp >= 0.8912f) { // -1dBFS
			CairoSetSouerceRGBA(c_prd);
		} else {
			CairoSetSouerceRGBA(c_g30);
		}
		rounded_rectangle (cr, COORD_TP_X, COORD_ML_Y, 75, 38, 10);
		cairo_fill (cr);

		/* true-peak val */
		ui->prev_lvl[4] = ui->tp;
		sprintf(buf, "%s", format_lufs(lufb0, ui->tp));
		write_text(cr, buf, FONT(FONT_M09), COORD_TP_X+65, COORD_ML_Y+5, 0, 7, c_wht);
		write_text(cr, "dBTP", FONT(FONT_M09), COORD_TP_X+65, COORD_ML_Y+19, 0, 7, c_wht);
	}

#if 1
	if (!ui->radar_surf || (redraw_part & 2) == 2) {
		DEBUG_DRAW("RADAR");
		render_radar(ui);
		ui->fastradar = -1;
		cairo_set_source_surface (cr, ui->radar_surf, CX - RADIUS, CY - RADIUS);
		cairo_paint(cr);

		if (!robtk_rbtn_get_active(ui->cbx_histogram)) {
			draw_radar_ann (cr);
		}
	}
#endif

#define MPI108 1.599885148 // M_PI / 2 + M_PI / 108.0
#define MPI72 0.043633231  // M_PI / 72
#define MPI00 1.541707506  // M_PI / 2 - M_PI / 108.0

	if (redraw_part & 1) {
		DEBUG_DRAW("CIRC Level");

		int cl, cm;
		ring_leds(ui, &cl, &cm);
		ui->circ_max = cm;
		ui->circ_val = cl;

		const int ulp = plus24 ? 120 : 108;
		if (cl > ulp) cl = ulp;
		if (cm > ulp) cm = ulp;

		const float ang = MPI108 + MPI72 * (float) MAX(-1, cl);

		if (cl >= 0) {
			cairo_save (cr);
			cairo_move_to(cr, CX, CY);
			cairo_arc (cr, CX, CY, RADIUS22, MPI00, ang);
			cairo_close_path (cr);
			cairo_clip (cr);
			if (plus9) {
				cairo_set_source (cr, ui->lpattern9);
			} else {
				cairo_set_source (cr, ui->lpattern18);
			}
			cairo_mask_surface(cr, ui->level_surf, 0, 0);
			cairo_fill (cr);
			cairo_restore (cr);
		}

		if (cl < ulp) {
			const float tang = MPI108 + MPI72 * (float) ulp; // TODO + 24
			cairo_save (cr);
			cairo_move_to(cr, CX, CY);
			cairo_arc (cr, CX, CY, RADIUS22, ang, tang);
			cairo_close_path (cr);
			cairo_clip (cr);
			CairoSetSouerceRGBA(c_g30);
			cairo_mask_surface(cr, ui->level_surf, 0, 0);
			cairo_fill (cr);
			cairo_restore (cr);
		}

		if (cm > 0) {
			cairo_set_line_width(cr, 2.5);
			cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
			const float mang = M_PI * .5 + MPI72 * (float) cm;
			float cc = sinf(mang);
			float sc = cosf(mang);
			cairo_move_to(cr,   CX + RADIUS10 * sc, CY + RADIUS10 * cc);
			radar_color(cr, cm);
			cairo_line_to(cr, CX + RADIUS22 * sc, CY + RADIUS22 * cc);
			cairo_stroke (cr);
		}

		cairo_set_source_surface(cr, ui->lvl_label, 0, 0);
		cairo_paint(cr);
	}

	int bottom_max_offset = 50;

	if (ui->il > -60 || robtk_cbtn_get_active(ui->btn_start)) {
		bottom_max_offset = 3;
	}
	if (rect_intersect_a(ev, COORD_BI_X+10, COORD_BI_Y, 40, 30) && bottom_max_offset == 3) {
		DEBUG_DRAW("Integ. 'Long' Legend");
		CairoSetSouerceRGBA(c_g20);
		rounded_rectangle (cr, COORD_BI_X+10, COORD_BI_Y, 40, 30, 10);
		cairo_fill (cr);
		write_text(cr, "Long", FONT(FONT_S08), COORD_BI_X+30 , COORD_BI_Y+18, 0,  5, c_wht);
	}
	/* integrated level text display */
	if (rect_intersect_a(ev, COORD_BI_X, COORD_BI_Y+20, COORD_BINTG_W, 40) && bottom_max_offset == 3) {
		DEBUG_DRAW("Integrated Level");
		CairoSetSouerceRGBA(c_g30);
		rounded_rectangle (cr, COORD_BI_X, COORD_BI_Y+20, COORD_BINTG_W, 40, 10);
		cairo_fill (cr);

		if (ui->il > -60) {
			sprintf(buf, "Int:   %s %s", format_lufs(lufb0, LUFS(ui->il)), lufs ? "LUFS" : "LU");
			write_text(cr, buf, FONT(FONT_M09), COORD_BI_X+10 , COORD_BI_Y+40, 0,  6, c_wht);
		} else {
			sprintf(buf, "[Integrating over 5 sec]");
			write_text(cr, buf, FONT(FONT_S09), COORD_BI_X+10 , COORD_BI_Y+40, 0,  6, c_wht);
		}

		if (ui->rx > -60.0 && ui->rn > -60.0) {
			sprintf(buf, "Range: %s..%s %s (%4.1f)",
					format_lufs(lufb0, LUFS(ui->rn)), format_lufs(lufb1, LUFS(ui->rx)),
					lufs ? "LUFS" : "LU", (ui->rx - ui->rn));
			write_text(cr, buf, FONT(FONT_M09), COORD_BI_X+10 , COORD_BI_Y+55, 0,  6, c_wht);
		} else {
			sprintf(buf, "[Collecting 10 sec range.]");
			write_text(cr, buf, FONT(FONT_S09), COORD_BI_X+10 , COORD_BI_Y+55, 0,  6, c_wht);
		}

		/* clock */
		if (ui->it < 60) {
			sprintf(buf, "%.1f\"", ui->it);
		} else if (ui->it < 600) {
			int minutes = ui->it / 60;
			int seconds = ((int)floorf(ui->it)) % 60;
			int ds = 10*(ui->it - seconds - 60*minutes);
			sprintf(buf, "%d'%02d\"%d", minutes, seconds, ds);
		} else if (ui->it < 3600) {
			int minutes = ui->it / 60;
			int seconds = ((int)floorf(ui->it)) % 60;
			sprintf(buf, "%d'%02d\"", minutes, seconds);
		} else {
			int hours = ui->it / 3600;
			int minutes = ((int)floorf(ui->it / 60)) % 60;
			sprintf(buf, "%dh%02d'", hours, minutes);
		}
		write_text(cr, buf, FONT(FONT_M12), COORD_BINTG_W-7, COORD_BI_Y+50, 0,  4, c_wht);
	}

	if (rect_intersect_a(ev, COORD_BR_X+115-trw, COORD_BI_Y-50+bottom_max_offset, trw, 40) && redraw_part != 1) {
		DEBUG_DRAW("Bottom Level");
		/* bottom level text display */
		trw = lufs ? 117 : 105;
		//printf("BOTTOM LVL @ %d+%d %dx%d\n", COORD_BR_X+115-trw, 305+bottom_max_offset, trw, 40);
		CairoSetSouerceRGBA(c_g20);
		rounded_rectangle (cr, COORD_BR_X+64, COORD_BI_Y-50+bottom_max_offset, 40, 30, 10);
		cairo_fill (cr);
		CairoSetSouerceRGBA(c_g30);
		rounded_rectangle (cr, COORD_BR_X+115-trw, COORD_BI_Y-30+bottom_max_offset, trw, 40, 10);
		cairo_fill (cr);

		ui->prev_lvl[2] = !rings ? ui->ls : ui->lm;
		ui->prev_lvl[3] = !rings ? ui->ms : ui->mm;
		write_text(cr, rings ? "Mom":"Short", FONT(FONT_S08), COORD_BR_X+85, COORD_BI_Y-45+bottom_max_offset, 0, 8, c_wht);
		sprintf(buf, "%s %s", format_lufs(lufb0, LUFS(ui->prev_lvl[2])), lufs ? "LUFS" : "LU");
		write_text(cr, buf, FONT(FONT_M09), COORD_BR_X+105, COORD_BI_Y-25+bottom_max_offset, 0, 7, c_wht);
		sprintf(buf, "Max:%s %s", format_lufs(lufb0, LUFS(ui->prev_lvl[3])), lufs ? "LUFS" : "LU");
		write_text(cr, buf, FONT(FONT_M09), COORD_BR_X+105, COORD_BI_Y-10+bottom_max_offset, 0, 7, c_wht);
	}

#if 0
	{
		cairo_rectangle (cr, 0, 0, COORD_ALL_W, COORD_ALL_H);
		float c[3];
		c[0] = rand() / (float)RAND_MAX;
		c[1] = rand() / (float)RAND_MAX;
		c[2] = rand() / (float)RAND_MAX;
		cairo_set_source_rgba (cr, c[0], c[1], c[2], 0.1);
		cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
		cairo_fill (cr);
	}
#endif

	DEBUG_DRAW("----END----");
	return TRUE;
}

/******************************************************************************
 * partial exposure
 */

#define INVALIDATE_RECT(XX,YY,WW,HH) queue_draw_area(ui->m0, XX, YY, WW, HH)
#define MIN2(A,B) ( (A) < (B) ? (A) : (B) )
#define MAX2(A,B) ( (A) > (B) ? (A) : (B) )
#define MIN3(A,B,C) (  (A) < (B)  ? MIN2 (A,C) : MIN2 (B,C) )
#define MAX3(A,B,C) (  (A) > (B)  ? MAX2 (A,C) : MAX2 (B,C) )

static void invalidate_changed(EBUrUI* ui, int what) {
	cairo_rectangle_t rect;

	if (what == -1) {
		queue_draw(ui->m0);
		ui->fasttracked[0] = ui->fasttracked[1] = ui->fasttracked[2] = ui->fasttracked[3] = ui->fasttracked[4] = true;
		ui->fasthist = true;
		return;
	}

	if (what == 0 && !ui->fasttracked[0]) {
		// main level display
		const bool rings = robtk_rbtn_get_active(ui->cbx_ring_short);
		const float pl = rings ? ui->ls : ui->lm;
		if (rintf(pl * 10.0f) != rintf(ui->prev_lvl[0] * 10.0f)) {
			ui->fasttracked[0] = true;
			queue_tiny_area(ui->m0, COORD_LEVEL_X, COORD_ML_Y, COORD_LEVEL_W, COORD_LEVEL_H);
		}
	}

	if (what == 0 && !ui->fasttracked[4] && robtk_cbtn_get_active(ui->cbx_truepeak)) {
		// true peak display
		if (rintf(ui->tp * 10.0f) != rintf(ui->prev_lvl[4] * 10.0f)) {
			ui->fasttracked[4] = true;
			queue_tiny_area(ui->m0, COORD_TP_X, COORD_ML_Y, 75, 38);    // top left side
			//queue_tiny_area(ui->m0, COORD_TP_X+10, COORD_ML_Y+25, 40, 30); // top left side tab
		}
	}

	if (what == 0 && !ui->fasttracked[1]) {
		// main max display
		const bool rings = robtk_rbtn_get_active(ui->cbx_ring_short);
		const float pl = rings ? ui->ms : ui->mm;
		if (rintf(pl * 10.0f) != rintf(ui->prev_lvl[1] * 10.0f)) {
			ui->fasttracked[1] = true;
			queue_tiny_area(ui->m0, COORD_MX_X-32, COORD_ML_Y, 87, 38);  // top side w/o LU/FS
			//queue_tiny_area(ui->m0, COORD_MX_X, COORD_ML_Y+25, 30, 30);// top side tab
		}
	}

	if (what == 0) {
		// TODO only when iteg. (time changed) or integ'ed value changed
		if (ui->il > -60 || robtk_cbtn_get_active(ui->btn_start)) {
		 // BIG -- when integrating
			if (!ui->fasttracked[3]) {
				ui->fasttracked[3] = true;
				queue_tiny_area(ui->m0, COORD_BI_X, COORD_BI_Y+20, COORD_BINTG_W, 45);   // bottom bar
			}
		}

		// bottom lvl+max
		if (!ui->fasttracked[2]) {
			const bool rings = robtk_rbtn_get_active(ui->cbx_ring_short);
			const float pl = !rings ? ui->ls : ui->lm;
			const float pm = !rings ? ui->ms : ui->mm;
			if (rintf(pl * 10.0f) != rintf(ui->prev_lvl[2] * 10.0f)
					|| rintf(pm * 10.0f) != rintf(ui->prev_lvl[3] * 10.0f)) {
				ui->fasttracked[2] = true;
				if (ui->il > -60 || robtk_cbtn_get_active(ui->btn_start)) {
					queue_tiny_area(ui->m0, COORD_BR_X, COORD_BINFO_Y, COORD_BINFO_W, COORD_BINFO_H); // bottom side
				} else {
					queue_tiny_area(ui->m0, COORD_BR_X, COORD_BI_Y+20, 117, 40); // bottom right space w/o tab
				}
			}
		}
	}

	if ((what & 1) ||
			(robtk_rbtn_get_active(ui->cbx_radar)
			 && ui->radar_pos_cur != ui->radar_pos_disp
			 && ui->fastradar == -1 /* != ui->radar_pos_cur */
			 )
			) {

		if ((what & 2) == 0 && ui->radar_pos_max > 0) {
			float ang0 = 2.0 * M_PI * (ui->radar_pos_cur - 1) / (float) ui->radar_pos_max;
			int dx0 = rintf(CX + RADIUS1 * cosf(ang0));
			int dy0 = rintf(CY + RADIUS1 * sinf(ang0));

			float ang1 = 2.0 * M_PI * (ui->radar_pos_cur + 13) / (float) ui->radar_pos_max;
			int dx1 = rint(CX + RADIUS1 * cosf(ang1));
			int dy1 = rint(CY + RADIUS1 * sinf(ang1));

			rect.x = MIN3(CX, dx0, dx1) -1;
			rect.y = MIN3(CY, dy0, dy1) -1;

			rect.width  = 2 + MAX3(CX, dx0, dx1) - rect.x;
			rect.height = 2 + MAX3(CY, dy0, dy1) - rect.y;

			ui->fastradar = ui->radar_pos_cur;
			queue_tiny_area(ui->m0, floorf(rect.x), floorf(rect.y), ceilf(rect.width), ceilf(rect.height));
		} else {
			/// XXX may be ignored IFF coincides with ring-lvl, hence:
			ui->fullhist = true;
			INVALIDATE_RECT(RDR_INV_X, RDR_INV_Y, RDR_INV_W, RDR_INV_H);
		}
	}

	if (what == 0) {
		int cl, cm;
		ring_leds(ui, &cl, &cm);

		if (ui->circ_max != cm || ui->circ_val != cl) {
			// TODO invalidate changed parts only
			// -> also update ring-lvl filter in expose_area
			INVALIDATE_RECT(COORD_MTR_X, COORD_MTR_Y, 320, 290); // XXX
		}
	}
}

static void invalidate_histogram_line(EBUrUI* ui, int p) {
	const bool plus9 = robtk_rbtn_get_active(ui->cbx_sc9);
	cairo_rectangle_t rect;

	// dup from expose_event()
	int amin, amax;
	if (plus9) {
		amin = 290;
		amax = 560;
	} else {
		amin = 110;
		amax = 650;
	}
	if (p < amin || p > amax) return;
	const double astep = 1.5 * M_PI / (double) (amax - amin);
	const double aoff = (M_PI / 2.0) - amin * astep;

	float ang0 = (float) (p-1) * astep + aoff;
	float ang1 = (float) (p+1) * astep + aoff;

	// see also "invalidate changed part of radar only" above
	int dx0 = rintf(CX + RADIUS1 * cosf(ang0));
	int dy0 = rintf(CY + RADIUS1 * sinf(ang0));
	int dx1 = rint(CX + RADIUS1 * cosf(ang1));
	int dy1 = rint(CY + RADIUS1 * sinf(ang1));

	rect.x = MIN3(CX, dx0, dx1) -1;
	rect.y = MIN3(CY, dy0, dy1) -1;

	rect.width  = 2 + MAX3(CX, dx0, dx1) - rect.x;
	rect.height = 2 + MAX3(CY, dy0, dy1) - rect.y;

	//printf("Q HIST: %.1f+%.1f %.1fx%.1f\n", rect.x, rect.y, rect.width, rect.height);
	if (!ui->fasthist) {
		queue_tiny_area(ui->m0, floorf(rect.x), floorf(rect.y), ceilf(rect.width), ceilf(rect.height));
		ui->fasthist = true;
	} else if (!ui->fullhist) {
		ui->fullhist = true;
		INVALIDATE_RECT(43, 68, 245, 245);
		//queue_draw(ui->m0); // trigger radar to be included..
		//queue_draw_area(ui->m0, rect.x, rect.y, rect.width, rect.height);
	}
}


/******************************************************************************
 * LV2 UI -> plugin communication
 */

static void forge_message_kv(EBUrUI* ui, LV2_URID uri, int key, float value) {
	uint8_t obj_buf[1024];
	if (ui->disable_signals) return;

	lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 1024);
	LV2_Atom* msg = forge_kvcontrolmessage(&ui->forge, &ui->uris, uri, key, value);
	ui->write(ui->controller, 0, lv2_atom_total_size(msg), ui->uris.atom_eventTransfer, msg);
}

/******************************************************************************
 * UI callbacks
 */

static bool btn_start(RobWidget *w, void* handle) {
	EBUrUI* ui = (EBUrUI*)handle;
	if (robtk_cbtn_get_active(ui->btn_start)) {
		forge_message_kv(ui, ui->uris.mtr_meters_cfg, CTL_START, 0);
	} else {
		forge_message_kv(ui, ui->uris.mtr_meters_cfg, CTL_PAUSE, 0);
	}
	invalidate_changed(ui, -1);
	return TRUE;
}

static bool btn_reset(RobWidget *w, void* handle) {
	EBUrUI* ui = (EBUrUI*)handle;
	forge_message_kv(ui, ui->uris.mtr_meters_cfg, CTL_RESET, 0);
	invalidate_changed(ui, -1);
	return TRUE;
}

static bool cbx_transport(RobWidget *w, void* handle) {
	EBUrUI* ui = (EBUrUI*)handle;
	if (robtk_cbtn_get_active(ui->cbx_transport)) {
		robtk_cbtn_set_sensitive(ui->btn_start, false);
		forge_message_kv(ui, ui->uris.mtr_meters_cfg, CTL_TRANSPORTSYNC, 1);
	} else {
		robtk_cbtn_set_sensitive(ui->btn_start, true);
		forge_message_kv(ui, ui->uris.mtr_meters_cfg, CTL_TRANSPORTSYNC, 0);
	}
	return TRUE;
}

static bool cbx_autoreset(RobWidget *w, void* handle) {
	EBUrUI* ui = (EBUrUI*)handle;
	if (robtk_cbtn_get_active(ui->cbx_autoreset)) {
		forge_message_kv(ui, ui->uris.mtr_meters_cfg, CTL_AUTORESET, 1);
	} else {
		forge_message_kv(ui, ui->uris.mtr_meters_cfg, CTL_AUTORESET, 0);
	}
	return TRUE;
}

static bool cbx_lufs(RobWidget *w, void* handle) {
	EBUrUI* ui = (EBUrUI*)handle;
	uint32_t v = 0;
	v |= robtk_rbtn_get_active(ui->cbx_lufs) ? 1 : 0;
	v |= robtk_rbtn_get_active(ui->cbx_sc9) ? 2 : 0;
	v |= robtk_rbtn_get_active(ui->cbx_sc24) ? 32 : 0;
	v |= robtk_rbtn_get_active(ui->cbx_ring_short) ? 4 : 0;
	v |= robtk_rbtn_get_active(ui->cbx_hist_short) ? 8 : 0;
	v |= robtk_rbtn_get_active(ui->cbx_histogram) ? 16 : 0;
	v |= robtk_cbtn_get_active(ui->cbx_truepeak) ? 64 : 0;
	forge_message_kv(ui, ui->uris.mtr_meters_cfg, CTL_UISETTINGS, (float)v);
	ui->redraw_labels = TRUE;
	invalidate_changed(ui, -1);
	return TRUE;
}

static bool spn_radartime(RobWidget *w, void* handle) {
	EBUrUI* ui = (EBUrUI*)handle;
	 float v = robtk_spin_get_value(ui->spn_radartime);
	forge_message_kv(ui, ui->uris.mtr_meters_cfg, CTL_RADARTIME, v);
	return TRUE;
}


/******************************************************************************
 * widget hackery
 */

static void
size_request(RobWidget* handle, int *w, int *h) {
	*w = COORD_ALL_W;
	*h = COORD_ALL_H;
}

/******************************************************************************
 * LV2 callbacks
 */

static void ui_enable(LV2UI_Handle handle) {
	EBUrUI* ui = (EBUrUI*)handle;
	forge_message_kv(ui, ui->uris.mtr_meters_on, 0, 0); // may be too early
}

static void ui_disable(LV2UI_Handle handle) {
	EBUrUI* ui = (EBUrUI*)handle;
	forge_message_kv(ui, ui->uris.mtr_meters_off, 0, 0);
}

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
	EBUrUI* ui = (EBUrUI*)calloc(1,sizeof(EBUrUI));
	ui->write      = write_function;
	ui->controller = controller;
	ui->fastradar = -1;

	*widget = NULL;

	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
			ui->map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!ui->map) {
		fprintf(stderr, "UI: Host does not support urid:map\n");
		free(ui);
		return NULL;
	}

	ui->nfo = robtk_info(ui_toplevel);
	map_eburlv2_uris(ui->map, &ui->uris);

	lv2_atom_forge_init(&ui->forge, ui->map);

	ui->box = rob_vbox_new(FALSE, 2);
	robwidget_make_toplevel(ui->box, ui_toplevel);
	ROBWIDGET_SETNAME(ui->box, "ebur128");

	ui->m0 = robwidget_new(ui);
	robwidget_set_alignment(ui->m0, .5, .5);
	robwidget_set_expose_event(ui->m0, expose_event);
	robwidget_set_size_request(ui->m0, size_request);

	ui->btn_start = robtk_cbtn_new("Integrate", GBT_LED_OFF, false);
	ui->btn_reset = robtk_pbtn_new("Reset");

	ui->cbx_box = rob_table_new(/*rows*/6, /*cols*/ 5, FALSE);
	ui->cbx_lu         = robtk_rbtn_new("LU", NULL);
	ui->cbx_lufs       = robtk_rbtn_new("LUFS", robtk_rbtn_group(ui->cbx_lu));

	ui->cbx_ring_mom   = robtk_rbtn_new("Momentary", NULL);
	ui->cbx_ring_short = robtk_rbtn_new("Short", robtk_rbtn_group(ui->cbx_ring_mom));

	ui->cbx_hist_short = robtk_rbtn_new("Short", NULL);
	ui->cbx_hist_mom   = robtk_rbtn_new("Momentary", robtk_rbtn_group(ui->cbx_hist_short));

	ui->cbx_sc18       = robtk_rbtn_new("-36..+18LU", NULL);
	ui->cbx_sc9        = robtk_rbtn_new("-18..+9LU", robtk_rbtn_group(ui->cbx_sc18));
	ui->cbx_sc24       = robtk_rbtn_new("-36..+24LU", robtk_rbtn_group(ui->cbx_sc18));

	ui->cbx_transport  = robtk_cbtn_new("Host Transport", GBT_LED_LEFT, true);
	ui->cbx_autoreset  = robtk_cbtn_new("Reset on Start", GBT_LED_LEFT, true);
	ui->spn_radartime  = robtk_spin_new(30, 600, 15);
	ui->lbl_radarinfo  = robtk_lbl_new("History Length [s]:");
	ui->lbl_ringinfo   = robtk_lbl_new("Level Display");
#ifdef EASTER_EGG
	ui->cbx_truepeak   = robtk_cbtn_new("True-Peak", GBT_LED_LEFT, true);
#else
	ui->cbx_truepeak   = robtk_cbtn_new("Compute True-Peak", GBT_LED_LEFT, true);
#endif

	ui->sep_h0         = robtk_sep_new(TRUE);
	ui->sep_h1         = robtk_sep_new(TRUE);
	ui->sep_h2         = robtk_sep_new(TRUE);
	ui->sep_v0         = robtk_sep_new(FALSE);

	ui->cbx_radar      = robtk_rbtn_new("History", NULL);
	ui->cbx_histogram  = robtk_rbtn_new("Histogram", robtk_rbtn_group(ui->cbx_radar));

	robtk_sep_set_linewidth(ui->sep_h2, 0);
	robtk_lbl_set_alignment(ui->lbl_radarinfo, 0.0f, 0.5f);
	robtk_spin_set_label_pos(ui->spn_radartime, 1);
	robtk_spin_set_alignment(ui->spn_radartime, 1.0f, 0.5f);
	robtk_pbtn_set_alignment(ui->btn_reset, 0.5, 0.5);
	robtk_cbtn_set_alignment(ui->btn_start, 0.5, 0.5);
	robtk_spin_set_default(ui->spn_radartime, 120);
	robtk_spin_set_value(ui->spn_radartime, 120);
	robtk_spin_label_width(ui->spn_radartime, 32.0, -1);

	int row = 0; // left side
	rob_table_attach((ui->cbx_box), GLB_W(ui->lbl_ringinfo), 0, 2, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	row++;
	rob_table_attach_defaults(ui->cbx_box, robtk_sep_widget(ui->sep_h0), 0, 2, row, row+1);
	row++;
	rob_table_attach(ui->cbx_box, GRB_W(ui->cbx_lu)   , 0, 1, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->cbx_box, GRB_W(ui->cbx_lufs) , 1, 2, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	row++;
	rob_table_attach(ui->cbx_box, GRB_W(ui->cbx_sc18) , 0, 1, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->cbx_box, GRB_W(ui->cbx_sc9)  , 1, 2, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	row++;
#ifdef EASTER_EGG
	rob_table_attach(ui->cbx_box, GRB_W(ui->cbx_sc24)      , 1, 2, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->cbx_box, GBT_W(ui->cbx_truepeak)  , 0, 1, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
#else
	rob_table_attach(ui->cbx_box, GBT_W(ui->cbx_truepeak)  , 0, 2, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
#endif
	row++;
	rob_table_attach(ui->cbx_box, GRB_W(ui->cbx_ring_mom)  , 0, 1, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->cbx_box, GRB_W(ui->cbx_ring_short), 1, 2, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);

	rob_table_attach_defaults(ui->cbx_box, robtk_sep_widget(ui->sep_v0), 2, 3, 0, 6);

	row = 0; // right side
	rob_table_attach(ui->cbx_box, GRB_W(ui->cbx_histogram) , 3, 4, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->cbx_box, GRB_W(ui->cbx_radar)     , 4, 5, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	row++;
	rob_table_attach_defaults(ui->cbx_box, robtk_sep_widget(ui->sep_h1), 3, 5, row, row+1);
	row++;
	rob_table_attach(ui->cbx_box, GLB_W(ui->lbl_radarinfo), 3, 4, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->cbx_box, GSP_W(ui->spn_radartime), 4, 5, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	row++;
	rob_table_attach(ui->cbx_box, GBT_W(ui->cbx_autoreset), 3, 4, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->cbx_box, GPB_W(ui->btn_reset), 4, 5, row, row+1, 0, 0, RTK_FILL, RTK_SHRINK);
	row++;
	rob_table_attach(ui->cbx_box, GBT_W(ui->cbx_transport), 3, 4, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->cbx_box, GBT_W(ui->btn_start), 4, 5, row, row+1, 0, 0, RTK_FILL, RTK_SHRINK);
	row++;
	rob_table_attach(ui->cbx_box, GRB_W(ui->cbx_hist_mom)  , 3, 4, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach(ui->cbx_box, GRB_W(ui->cbx_hist_short), 4, 5, row, row+1, 0, 0, RTK_EXANDF, RTK_SHRINK);


	/* global packing */
	rob_vbox_child_pack(ui->box, ui->m0, FALSE, FALSE);
	rob_vbox_child_pack(ui->box, ui->cbx_box, FALSE, FALSE);
	rob_vbox_child_pack(ui->box, robtk_sep_widget(ui->sep_h2), TRUE, FALSE);

	/* signals */
	robtk_cbtn_set_callback(ui->btn_start, btn_start, ui);
	robtk_pbtn_set_callback_up(ui->btn_reset, btn_reset, ui);

	robtk_spin_set_callback(ui->spn_radartime, spn_radartime, ui);

	robtk_rbtn_set_callback(ui->cbx_lufs, cbx_lufs, ui);
	robtk_rbtn_set_callback(ui->cbx_sc18, cbx_lufs, ui);

	robtk_rbtn_set_callback(ui->cbx_hist_short, cbx_lufs, ui);
	robtk_rbtn_set_callback(ui->cbx_ring_short, cbx_lufs, ui);
	robtk_rbtn_set_callback(ui->cbx_histogram, cbx_lufs, ui);
	robtk_cbtn_set_callback(ui->cbx_truepeak, cbx_lufs, ui);

	robtk_cbtn_set_callback(ui->cbx_transport, cbx_transport, ui);
	robtk_cbtn_set_callback(ui->cbx_autoreset, cbx_autoreset, ui);

	*widget = ui->box;

	initialize_font_cache(ui);
	ui->redraw_labels = TRUE;

	ui_enable(ui);
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
	EBUrUI* ui = (EBUrUI*)handle;
	ui_disable(handle);
	if (ui->cpattern) {
		cairo_pattern_destroy (ui->cpattern);
	}
	if (ui->lpattern9) {
		cairo_pattern_destroy (ui->lpattern9);
	}
	if (ui->lpattern18) {
		cairo_pattern_destroy (ui->lpattern18);
	}
	if (ui->hpattern9) {
		cairo_pattern_destroy (ui->hpattern9);
	}
	if (ui->hpattern18) {
		cairo_pattern_destroy (ui->hpattern18);
	}
	if (ui->level_surf) {
		cairo_surface_destroy(ui->level_surf);
	}
	if (ui->radar_surf) {
		cairo_surface_destroy(ui->radar_surf);
	}
	if (ui->lvl_label) {
		cairo_surface_destroy(ui->lvl_label);
	}
	if (ui->hist_label) {
		cairo_surface_destroy(ui->hist_label);
	}

	if (ui->fontcache) {
		for (int i=0; i < 6; ++i) {
			pango_font_description_free(ui->font[i]);
		}
	}

	free(ui->radarS);
	free(ui->radarM);

	robtk_rbtn_destroy(ui->cbx_lufs);
	robtk_rbtn_destroy(ui->cbx_lu);
	robtk_rbtn_destroy(ui->cbx_sc9);
	robtk_rbtn_destroy(ui->cbx_sc18);
	robtk_rbtn_destroy(ui->cbx_sc24);
	robtk_rbtn_destroy(ui->cbx_ring_short);
	robtk_rbtn_destroy(ui->cbx_ring_mom);
	robtk_rbtn_destroy(ui->cbx_hist_short);
	robtk_rbtn_destroy(ui->cbx_hist_mom);
	robtk_cbtn_destroy(ui->cbx_transport);
	robtk_cbtn_destroy(ui->cbx_autoreset);
	robtk_cbtn_destroy(ui->cbx_truepeak);
	robtk_spin_destroy(ui->spn_radartime);
	robtk_cbtn_destroy(ui->btn_start);
	robtk_pbtn_destroy(ui->btn_reset);
	robtk_lbl_destroy(ui->lbl_ringinfo);
	robtk_lbl_destroy(ui->lbl_radarinfo);
	robtk_sep_destroy(ui->sep_h0);
	robtk_sep_destroy(ui->sep_h1);
	robtk_sep_destroy(ui->sep_h2);
	robtk_sep_destroy(ui->sep_v0);
	robtk_rbtn_destroy(ui->cbx_radar);
	robtk_rbtn_destroy(ui->cbx_histogram);

	robwidget_destroy(ui->m0);
	rob_table_destroy(ui->cbx_box);
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

#define PARSE_CHANGED_FLOAT(var, dest) \
	if (var && var->type == uris->atom_Float) { \
		float val = ((LV2_Atom_Float*)var)->body; \
		if (val != dest) { \
			dest = val; \
			changed = true; \
		} \
	}

#define PARSE_A_FLOAT(var, dest) \
	if (var && var->type == uris->atom_Float) { \
		dest = ((LV2_Atom_Float*)var)->body; \
	}

#define PARSE_A_INT(var, dest) \
	if (var && var->type == uris->atom_Int) { \
		dest = ((LV2_Atom_Int*)var)->body; \
	}

static bool parse_ebulevels(EBUrUI* ui, const LV2_Atom_Object* obj) {
	const EBULV2URIs* uris = &ui->uris;
	bool changed = false;
	LV2_Atom *lm = NULL;
	LV2_Atom *mm = NULL;
	LV2_Atom *ls = NULL;
	LV2_Atom *ms = NULL;
	LV2_Atom *il = NULL;
	LV2_Atom *rn = NULL;
	LV2_Atom *rx = NULL;
	LV2_Atom *ii = NULL;
	LV2_Atom *it = NULL;
	LV2_Atom *tp = NULL;

	lv2_atom_object_get(obj,
			uris->ebu_loudnessM, &lm,
			uris->ebu_maxloudnM, &mm,
			uris->ebu_loudnessS, &ls,
			uris->ebu_maxloudnS, &ms,
			uris->ebu_integrated, &il,
			uris->ebu_range_min, &rn,
			uris->ebu_range_max, &rx,
			uris->mtr_truepeak, &tp,
			uris->ebu_integrating, &ii,
			uris->ebu_integr_time, &it,
			NULL
			);


	const float old_time = ui->it;
	PARSE_CHANGED_FLOAT(it, ui->it)

	if (old_time < ui->it && ui->it - old_time < .2) {
		ui->it = old_time;
		changed = false;
	}

	PARSE_CHANGED_FLOAT(lm, ui->lm)
	PARSE_CHANGED_FLOAT(mm, ui->mm)
	PARSE_CHANGED_FLOAT(ls, ui->ls)
	PARSE_CHANGED_FLOAT(ms, ui->ms)
	PARSE_CHANGED_FLOAT(il, ui->il)
	PARSE_CHANGED_FLOAT(rn, ui->rn)
	PARSE_CHANGED_FLOAT(rx, ui->rx)
	PARSE_CHANGED_FLOAT(tp, ui->tp)

	if (ii && ii->type == uris->atom_Bool) {
		bool ix = ((LV2_Atom_Bool*)ii)->body;
	  bool bx = robtk_cbtn_get_active(ui->btn_start);
		if (ix != bx) {
			changed = true;
			ui->disable_signals = true;
			robtk_cbtn_set_active(ui->btn_start, ix);
			ui->disable_signals = false;
		}
	}
	return changed;
}

static void parse_radarinfo(EBUrUI* ui, const LV2_Atom_Object* obj) {
	const EBULV2URIs* uris = &ui->uris;

	LV2_Atom *lm = NULL;
	LV2_Atom *ls = NULL;
	LV2_Atom *pp = NULL;
	LV2_Atom *pc = NULL;
	LV2_Atom *pm = NULL;

	float xlm, xls;
	int p,c,m;

	xlm = xls = -INFINITY;
	p=c=m=-1;

	lv2_atom_object_get(obj,
			uris->ebu_loudnessM, &lm,
			uris->ebu_loudnessS, &ls,
			uris->rdr_pointpos, &pp,
			uris->rdr_pos_cur, &pc,
			uris->rdr_pos_max, &pm,
			NULL
			);

	PARSE_A_FLOAT(lm, xlm)
	PARSE_A_FLOAT(ls, xls)
	PARSE_A_INT(pp, p);
	PARSE_A_INT(pc, c);
	PARSE_A_INT(pm, m);

	if (m < 1 || c < 0 || p < 0) return;

	if (m != ui->radar_pos_max) {
		ui->radarS = (float*) realloc((void*) ui->radarS, sizeof(float) * m);
		ui->radarM = (float*) realloc((void*) ui->radarM, sizeof(float) * m);
		ui->radar_pos_max = m;
		for (int i=0; i < ui->radar_pos_max; ++i) {
			ui->radarS[i] = -INFINITY;
			ui->radarM[i] = -INFINITY;
		}
	}
	ui->radarM[p] = xlm;
	ui->radarS[p] = xls;
	ui->radar_pos_cur = c;
}

static void parse_histogram(EBUrUI* ui, const LV2_Atom_Object* obj) {
	const EBULV2URIs* uris = &ui->uris;
	LV2_Atom *lm = NULL;
	LV2_Atom *ls = NULL;
	LV2_Atom *pp = NULL;

	int p = -1;

	lv2_atom_object_get(obj,
			uris->ebu_loudnessM, &lm,
			uris->ebu_loudnessS, &ls,
			uris->rdr_pointpos, &pp,
			NULL
			);

	PARSE_A_INT(pp, p);
	if (p < 0 || p > HIST_LEN) return;
	const int oldM = ui->histM[p];
	const int oldS = ui->histS[p];
	PARSE_A_INT(lm, ui->histM[p]);
	PARSE_A_INT(ls, ui->histS[p]);

	if (robtk_rbtn_get_active(ui->cbx_histogram)) {
		const bool hists = robtk_rbtn_get_active(ui->cbx_hist_short);
		if ((hists && oldS != ui->histS[p]) || (!hists && oldM != ui->histM[p])) {
			invalidate_histogram_line(ui, p);
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
	EBUrUI* ui = (EBUrUI*)handle;
	const EBULV2URIs* uris = &ui->uris;

	if (format == uris->atom_eventTransfer) {
		LV2_Atom* atom = (LV2_Atom*)buffer;

		if (atom->type == uris->atom_Blank || atom->type == uris->atom_Object) {
			LV2_Atom_Object* obj = (LV2_Atom_Object*)atom;
			if (obj->body.otype == uris->mtr_ebulevels) {
				if (parse_ebulevels(ui, obj)) {
					invalidate_changed(ui, 0);
				}
			} else if (obj->body.otype == uris->mtr_control) {
				int k; float v;
				get_cc_key_value(&ui->uris, obj, &k, &v);
				if (k == CTL_LV2_FTM) {
					int vv = v;
					ui->disable_signals = true;
					robtk_cbtn_set_active(ui->cbx_autoreset, (vv&2)==2);
					robtk_cbtn_set_active(ui->cbx_transport, (vv&1)==1);
					ui->disable_signals = false;
				} else if (k == CTL_LV2_RADARTIME) {
					ui->disable_signals = true;
					robtk_spin_set_value(ui->spn_radartime, v);
					ui->disable_signals = false;
				} else if (k == CTL_LV2_RESETRADAR) {
					ui->radar_pos_cur = 0;
					ui->fastradar = -1;
					ui->tp = 0;
					for (int i=0; i < ui->radar_pos_max; ++i) {
						ui->radarS[i] = -INFINITY;
						ui->radarM[i] = -INFINITY;
					}
					ui->histLenM = 0;
					ui->histLenS = 0;
					for (int i=0; i < HIST_LEN; ++i) {
						ui->histM[i] = 0;
						ui->histS[i] = 0;
					}
					invalidate_changed(ui, -1);
				} else if (k == CTL_LV2_RESYNCDONE) {
					invalidate_changed(ui, -1);
				} else if (k == CTL_UISETTINGS) {
					uint32_t vv = v;
					ui->disable_signals = true;
					if ((vv & 1)) {
						robtk_rbtn_set_active(ui->cbx_lufs, true);
					} else {
						robtk_rbtn_set_active(ui->cbx_lu, true);
					}
					if ((vv & 2)) {
						robtk_rbtn_set_active(ui->cbx_sc9, true);
					} else {
#ifdef EASTER_EGG
						if ((vv & 32)) {
							robtk_rbtn_set_active(ui->cbx_sc24, true);
						} else {
							robtk_rbtn_set_active(ui->cbx_sc18, true);
						}
#else
						robtk_rbtn_set_active(ui->cbx_sc18, true);
#endif
					}
					if ((vv & 4)) {
						robtk_rbtn_set_active(ui->cbx_ring_short, true);
					} else {
						robtk_rbtn_set_active(ui->cbx_ring_mom, true);
					}
					if ((vv & 8)) {
						robtk_rbtn_set_active(ui->cbx_hist_short, true);
					} else {
						robtk_rbtn_set_active(ui->cbx_hist_mom, true);
					}
					if ((vv & 16)) {
						robtk_rbtn_set_active(ui->cbx_histogram, true);
					} else {
						robtk_rbtn_set_active(ui->cbx_radar, true);
					}
					robtk_cbtn_set_active(ui->cbx_truepeak, (vv & 64) ? true: false);
					ui->disable_signals = false;
				}
			} else if (obj->body.otype == uris->rdr_radarpoint) {
				parse_radarinfo(ui, obj);
				if (robtk_rbtn_get_active(ui->cbx_radar)) {
					invalidate_changed(ui, 4);
				}
			} else if (obj->body.otype == uris->rdr_histpoint) {
				parse_histogram(ui, obj);
			} else if (obj->body.otype == uris->rdr_histogram) {
				LV2_Atom *lm = NULL;
				LV2_Atom *ls = NULL;
				lv2_atom_object_get(obj,
						uris->ebu_loudnessM, &lm,
						uris->ebu_loudnessS, &ls,
				NULL
				);
				PARSE_A_INT(lm, ui->histLenM);
				PARSE_A_INT(ls, ui->histLenS);
				if (robtk_rbtn_get_active(ui->cbx_histogram)) {
					invalidate_changed(ui, 3);
				}
			} else {
				fprintf(stderr, "UI: Unknown control message.\n");
			}
		} else {
			fprintf(stderr, "UI: Unknown message type.\n");
		}
	}
}
