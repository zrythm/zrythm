/* goniometer LV2 GUI
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
#define RTK_GUI "goniometerui"
#define LVGL_RESIZEABLE

#define CLIP_GM

typedef void Stcorrdsp;

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "../zita-resampler/resampler.h"
#include "../src/goniometer.h"

#define GED_W(PTR) robtk_dial_widget(PTR)
#define GBT_W(PTR) robtk_cbtn_widget(PTR)
#define GSP_W(PTR) robtk_spin_widget(PTR)
#define GLB_W(PTR) robtk_lbl_widget(PTR)

#define PC_BOUNDS ( 40.0f)

#define PC_TOP    ( 12.5f)
#define PC_BLOCK  (  9.0f)
#define PC_LEFT   ( 10.0f)
#define PC_WIDTH  ( 20.0f)
#define PC_HEIGHT (348.0f)

#define GM_BOUNDS (373.0f)
#define GM_CENTER (186.5f)

#define GM_RADIUS (200.0f)
#define GM_RAD2   (100.0f)

#define MAX_CAIRO_PATH 256
#define PC_BLOCKSIZE (PC_HEIGHT - PC_BLOCK - 2)

#define GAINSCALE(x) (x > .01 ? ((20 * log10f(x) + 40) / 6.60206) : 0)
#define INV_GAINSCALE(x) (powf(10, .05*((x * 6.602059) - 40)))

using namespace LV2M;

typedef struct {
	LV2_Handle instance;
	LV2UI_Write_Function write;
	LV2UI_Controller     controller;

  RobWidget* box;
  RobWidget* m0;
	RobWidget* b_box;
  RobWidget* c_tbl;

	RobTkCBtn* cbn_src;
	RobTkSpin* spn_src_fact;

	RobTkDial* spn_compress;
	RobTkDial* spn_gattack;
	RobTkDial* spn_gdecay;
	RobTkDial* spn_gtarget;
	RobTkDial* spn_grms;

	RobTkCBtn* cbn_autogain;
	RobTkCBtn* cbn_preferences;
	RobTkCBtn* cbn_lines;
	RobTkCBtn* cbn_xfade;

	RobTkSpin* spn_psize;
	RobTkSpin* spn_vfreq;
	RobTkDial* spn_alpha;

	RobTkSep* sep_h0;
	RobTkSep* sep_h1;
	RobTkSep* sep_v0;

	RobTkLbl* lbl_src_fact;
	RobTkLbl* lbl_psize;
	RobTkLbl* lbl_vfreq;
	RobTkLbl* lbl_compress;
	RobTkLbl* lbl_gattack;
	RobTkLbl* lbl_gdecay;
	RobTkLbl* lbl_gtarget;
	RobTkLbl* lbl_grms;

	RobTkScale* fader;

	bool initialized;
	float c_txt[4];

	int sfc;
	cairo_surface_t* sf[3];
	cairo_surface_t* an[7];
	cairo_surface_t* dial[4];
	cairo_surface_t* sf_nfo;

#ifdef CLIP_GM
	float crop_x0[3], crop_y0[3], crop_x1[3], crop_y1[3];
#endif

	float last_x, last_y;
	float lp0, lp1;
	float hpw;

	float cor;
	uint32_t ntfy_u, ntfy_b;

	float gain;
	bool disable_signals;

	float attack_pow;
	float decay_pow;
	float g_target;
	float g_rms;

	Resampler *src;
	float *scratch;
	float *resampl;
	float src_fact;

	int w_width;
	int w_height;
	int xrundisplay;

	const char *nfo;
} GMUI;

static bool cb_preferences(RobWidget *w, gpointer handle);

static void setup_src(GMUI* ui, float oversample, int hlen, float frel) {
	LV2gm* self = (LV2gm*) ui->instance;

	if (ui->src != 0) {
		delete ui->src;
		free(ui->scratch);
		free(ui->resampl);
		ui->src = 0;
		ui->scratch = 0;
		ui->resampl = 0;
		ui->hpw = expf(-2.0 * M_PI * 20 / self->rate);
	}

	if (oversample <= 1) {
		ui->src_fact = 1;
		return;
	}

	uint32_t bsiz = self->rate * 2;

	ui->hpw = expf(-2.0 * M_PI * 20 / (self->rate * oversample));
	ui->src_fact = oversample;
	ui->src = new Resampler();
	ui->src->setup(self->rate, self->rate * oversample, 2, hlen, frel);

	ui->scratch = (float*) calloc(bsiz, sizeof(float));
	ui->resampl = (float*) malloc(bsiz * oversample * sizeof(float));

	/* q/d initialize */
	ui->src->inp_count = 8192;
	ui->src->inp_data = ui->scratch;
	ui->src->out_count = 8192 * oversample;
	ui->src->out_data = ui->resampl;
	ui->src->process ();
}

/*****
 * drawing helpers
 */

static void write_text(
		cairo_t* cr,
		const char *txt, const char * font,
		const float x, const float y,
		const int align,
		const float * const col) {

	PangoFontDescription *fd;
	if (font) {
		fd = pango_font_description_from_string(font);
	} else {
		fd = get_font_from_theme();
	}
	write_text_full(cr, txt, fd, x, y, 0, align, col);
	pango_font_description_free(fd);
}

static void alloc_annotations(GMUI* ui) {
#define FONT_GM "Mono 22px"
#define FONT_PC "Mono 12px"
#define FONT_LB "Sans 8px"

#define INIT_BLACK_BG(ID, WIDTH, HEIGHT) \
	ui->an[ID] = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT); \
	cr = cairo_create (ui->an[ID]); \
	CairoSetSouerceRGBA(c_trs); \
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE); \
	cairo_rectangle (cr, 0, 0, WIDTH, WIDTH); \
	cairo_fill (cr);

	cairo_t* cr;

	INIT_BLACK_BG(0, 32, 32)
	write_text(cr, "L", FONT_GM, 16, 16, 2, c_grb);
	cairo_destroy (cr);

	INIT_BLACK_BG(1, 32, 32)
	write_text(cr, "R", FONT_GM, 16, 16, 2, c_grb);
	cairo_destroy (cr);

	INIT_BLACK_BG(2, 64, 32)
	write_text(cr, "Mono", FONT_GM, 32, 16, 2, c_grb);
	cairo_destroy (cr);

	INIT_BLACK_BG(3, 32, 32)
	write_text(cr, "+S", FONT_GM, 16, 16, 2, c_grb);
	cairo_destroy (cr);

	INIT_BLACK_BG(4, 32, 32)
	write_text(cr, "-S", FONT_GM, 16, 16, 2, c_grb);
	cairo_destroy (cr);

	INIT_BLACK_BG(5, 32, 32)
	write_text(cr, "+1", FONT_PC, 10, 10, 2, c_grb);
	cairo_destroy (cr);

	INIT_BLACK_BG(6, 32, 32)
	write_text(cr, "-1", FONT_PC, 10, 10, 2, c_grb);
	cairo_destroy (cr);

#define INIT_DIAL_SF(VAR, TXTL, TXTR) \
	VAR = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, GED_WIDTH, GED_HEIGHT); \
	cr = cairo_create (VAR); \
	CairoSetSouerceRGBA(c_trs); \
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE); \
	cairo_rectangle (cr, 0, 0, GED_WIDTH, GED_HEIGHT); \
	cairo_fill (cr); \
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER); \
	write_text(cr, TXTL, FONT_LB, 2, GED_HEIGHT - 1, 6, ui->c_txt); \
	write_text(cr, TXTR, FONT_LB, GED_WIDTH-1, GED_HEIGHT - 1, 4, ui->c_txt); \
	cairo_destroy (cr);

	INIT_DIAL_SF(ui->dial[0], "slow", "fast")
	INIT_DIAL_SF(ui->dial[1], "peak", "rms ")
	INIT_DIAL_SF(ui->dial[2], "  0%", "100%")
	INIT_DIAL_SF(ui->dial[3], " 15%", "600%")

	if (ui->nfo) {
		PangoFontDescription *fd = pango_font_description_from_string("Sans 10px");
		create_text_surface2(&ui->sf_nfo,
				GM_BOUNDS, 12,
				GM_BOUNDS -2, 0,
				ui->nfo, fd, 0, 7, c_g30);
		pango_font_description_free(fd);
	}
}

static void alloc_sf(GMUI* ui) {
	cairo_t* cr;
#define ALLOC_SF(VAR) \
	VAR = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, GM_BOUNDS, GM_BOUNDS);\
	cr = cairo_create (VAR);\
	CairoSetSouerceRGBA(c_blk); \
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);\
	cairo_rectangle (cr, 0, 0, GM_BOUNDS, GM_BOUNDS);\
	cairo_fill (cr);\
	cairo_destroy(cr);

	ALLOC_SF(ui->sf[0])
	ALLOC_SF(ui->sf[1])
	ALLOC_SF(ui->sf[2])
}


static void draw_rb(GMUI* ui, gmringbuf *rb) {
	float d0, d1;
	size_t n_samples = gmrb_read_space(rb);
	if (n_samples < 64) return;

	const bool composit = !robtk_cbtn_get_active(ui->cbn_xfade);
	const bool autogain = robtk_cbtn_get_active(ui->cbn_autogain);
	const bool lines = robtk_cbtn_get_active(ui->cbn_lines);
	const float line_width = robtk_spin_get_value(ui->spn_psize);
	const float persist = .5 + .005 * robtk_dial_get_value(ui->spn_alpha);
	const float attack_pow = ui->attack_pow;
	const float decay_pow = ui->decay_pow;
	const float g_target = ui->g_target;
	const float g_rms = ui->g_rms;
#ifdef WITH_INFLATE
	const float compress = .02 * robtk_dial_get_value(ui->spn_compress);
#endif

	if (composit) {
		ui->sfc = (ui->sfc + 1) % 3;
	}
	cairo_t* cr = cairo_create (ui->sf[ui->sfc]);

	if (composit) {
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba (cr, .0, .0, .0, .42); // fade out
		cairo_rectangle (cr, 0, 0, GM_BOUNDS, GM_BOUNDS);
		cairo_fill (cr);
	} else if (persist >= 1.0) {
		;
	} else if (persist >  0.5) {
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_rgba (cr, .0, .0, .0, 1.0 - persist);
		cairo_rectangle (cr, 0, 0, GM_BOUNDS, GM_BOUNDS);
		cairo_fill (cr);
	} else {
		CairoSetSouerceRGBA(c_blk);
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle (cr, 0, 0, GM_BOUNDS, GM_BOUNDS);
		cairo_fill (cr);
	}

	cairo_rectangle (cr, 0, 0, GM_BOUNDS, GM_BOUNDS);
	cairo_clip(cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	int cnt = 0;
	if (lines) {
		CairoSetSouerceRGBA(c_gml);
		//cairo_set_tolerance(cr, 1.0); // default .1
		cairo_set_line_width(cr, line_width);
		cairo_move_to(cr, ui->last_x, ui->last_y);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
	} else {
		CairoSetSouerceRGBA(c_gmp);
		cairo_set_line_width(cr, line_width);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	}

	bool os = false;
	size_t n_points = n_samples;
	if (ui->src_fact > 1) {
		size_t j=0;
		for (j=0; j < n_samples; ++j) {
			if (gmrb_read_one(rb, &ui->scratch[2*j], &ui->scratch[2*j+1])) break;
		}

		assert (j == n_samples);
		assert (n_samples > 0);

		ui->src->inp_count = n_samples;
		ui->src->inp_data = ui->scratch;
		ui->src->out_count = n_samples * ui->src_fact;
		ui->src->out_data = ui->resampl;
		ui->src->process ();
		n_points *= ui->src_fact;
		os = true;
	}

	float rms_0 = 0;
	float rms_1 = 0;
	int   rms_c = 0;
	float ag_xmax = 0;
	float ag_ymax = 0;
	float ag_xmin = 0;
	float ag_ymin = 0;
#ifdef CLIP_GM
	ui->crop_x1[ui->sfc] = 0;
	ui->crop_x0[ui->sfc] = GM_BOUNDS;
	ui->crop_y1[ui->sfc] = 0;
	ui->crop_y0[ui->sfc] = GM_BOUNDS;
#endif
	for (uint32_t i=0; i < n_points; ++i) {
		if (os) {
			d0 = ui->resampl[2*i];
			d1 = ui->resampl[2*i+1];
		} else {
			if (gmrb_read_one(rb, &d0, &d1)) break;
		}

#if 1 /* high pass filter */
		ui->lp0 += ui->hpw * (d0 - ui->lp0);
		ui->lp1 += ui->hpw * (d1 - ui->lp1);
		/* prevent denormals */
		ui->lp0 += 1e-12f;
		ui->lp1 += 1e-12f;
#else
		ui->lp0 = d0;
		ui->lp1 = d1;
#endif
		const float ax = (ui->lp0 - ui->lp1);
		const float ay = (ui->lp0 + ui->lp1);

		if (autogain) {
			if (ax > ag_xmax) ag_xmax = ax;
			if (ax < ag_xmin) ag_xmin = ax;
			if (ay > ag_ymax) ag_ymax = ay;
			if (ay < ag_ymin) ag_ymin = ay;
			rms_0 += ui->lp0 * ui->lp0;
			rms_1 += ui->lp1 * ui->lp1;
			rms_c++;
		}
#ifdef WITH_INFLATE
		float x, y;
		if (compress > 0.0 && compress <= 2.0) {
			const float volume = sqrt(ax * ax + ay * ay);
			float compr = 1.0;
			if (volume > 1.0) {
				compr = 1.0 / volume;
			} else if (volume > .001) {
				compr = 1.0 - compress * log10(volume);
			} else {
				compr = 1.0 + compress * 3.0;
			}
			x = GM_CENTER - ui->gain * ax * compr * GM_RAD2;
			y = GM_CENTER - ui->gain * ay * compr * GM_RAD2;
		} else {
			x = GM_CENTER - ui->gain * ax * GM_RAD2;
			y = GM_CENTER - ui->gain * ay * GM_RAD2;
		}
#else
		const float x = GM_CENTER - ui->gain * ax * GM_RAD2;
		const float y = GM_CENTER - ui->gain * ay * GM_RAD2;
#endif

		const float linelensquare = (ui->last_x - x) * (ui->last_x - x) + (ui->last_y - y) * (ui->last_y - y);
		if ( linelensquare < 2.0) continue;

		ui->last_x = x;
		ui->last_y = y;

#ifdef CLIP_GM
		if (x > ui->crop_x1[ui->sfc]) ui->crop_x1[ui->sfc] = x+1;
		if (x < ui->crop_x0[ui->sfc]) ui->crop_x0[ui->sfc] = x-1;
		if (y > ui->crop_y1[ui->sfc]) ui->crop_y1[ui->sfc] = y+1;
		if (y < ui->crop_y0[ui->sfc]) ui->crop_y0[ui->sfc] = y-1;
#endif

		if (lines) {
#if 0 // w/ MAX_CAIRO_PATH 1, expose every color
			if (linelensquare > 1600){
				cairo_set_source_rgba (cr, .88, .88, .15, .2); // GM COLOR
			} else {
				cairo_set_source_rgba (cr, .88, .88, .15, 1.0 - linelensquare / 2000); // GM COLOR
			}
#endif
			cairo_line_to(cr, ui->last_x, ui->last_y);
			cairo_move_to(cr, ui->last_x, ui->last_y);

		} else {
			cairo_rectangle(cr, rintf(ui->last_x - line_width * .5), rintf(ui->last_y - line_width * .5), line_width, line_width);
		}

		if (++cnt > MAX_CAIRO_PATH) {
			cnt = 0;
			if (lines) {
				cairo_stroke(cr);
				cairo_move_to(cr, ui->last_x, ui->last_y);
			} else {
				cairo_fill(cr);
			}
		}
	}

	if (cnt > 0) {
		if (lines) {
			cairo_stroke(cr);
		} else {
			cairo_fill(cr);
		}
	}

	cairo_destroy(cr);

	if (!isfinite(ui->lp0)) ui->lp0 = 0;
	if (!isfinite(ui->lp1)) ui->lp1 = 0;

	if (autogain) {
		LV2gm* self = (LV2gm*) ui->instance;
		float elapsed = n_samples / self->rate;
		const float xdif = (ag_xmax - ag_xmin);
		const float ydif = (ag_ymax - ag_ymin);
		float max  = sqrt(xdif * xdif + ydif * ydif);

		max *= .707;

		if (rms_c > 0 && g_rms > 0 && isfinite(g_rms)) {
			const float rms = 5.436 /* 2e */ * (rms_0 > rms_1 ? sqrtf(rms_0 / rms_c) : sqrtf(rms_1 / rms_c));
			//printf("max: %f <> rms %f (tgt:%f)\n", max, rms, g_target);
			max = max * (1.0 - g_rms) + rms * g_rms;
		}

		max *= g_target;

		if (!isfinite(max)) max = 0;
		float gain;
		if (max < .01) {
			gain = 100.0;
		} else if (max > 100.0) {
			gain = .02;
		} else {
			gain = 2.0 / max;
		}

		const float attack = gain < ui->gain ? attack_pow * (.31 + .1 * log10f(elapsed)) : decay_pow * (.03 + .007 * logf(elapsed));
		//printf(" %.3f  %.3f [max: %f %f] %f\n", ui->gain, gain, xdif, ydif, max);
		gain = ui->gain + attack * (gain - ui->gain);
		if (gain < .001) gain = .001;

		float fgain = gain;
		if (fgain > 20.0) fgain = 20.0;
		if (fgain < .03) fgain = .03;
		if (rint(500 * GAINSCALE(fgain)) != rint(500 * robtk_scale_get_value(ui->fader))) { // XXX
			robtk_scale_set_value(ui->fader, GAINSCALE(fgain));
		}
		//printf("autogain:  %+6.2f dB (*%f) (%f)\n", 20 * log10f(ui->gain), ui->gain, gain);
		ui->gain = gain;
	}
}

static void draw_gm_labels(GMUI* ui, cairo_t* cr) {
	cairo_save(cr);
	cairo_translate(cr, PC_BOUNDS, 0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SCREEN);

#define DRAW_LABEL(ID, XPOS, YPOS) \
	cairo_set_source_surface(cr, ui->an[ID], (XPOS)-16, (YPOS)-16); cairo_paint (cr);

	DRAW_LABEL(0, GM_CENTER - GM_RAD2, GM_CENTER - GM_RAD2)
	DRAW_LABEL(1, GM_CENTER + GM_RAD2, GM_CENTER - GM_RAD2);

	DRAW_LABEL(2, GM_CENTER - 16, GM_CENTER - GM_RADIUS * 3/4 - 12);
	DRAW_LABEL(3, GM_CENTER - GM_RADIUS * 3/4 - 12 , GM_CENTER - 1);
	DRAW_LABEL(4, GM_CENTER + GM_RADIUS * 3/4 + 12 , GM_CENTER - 1);

	const double dashed[] = {1.0, 2.0};
	cairo_set_line_width(cr, 3.5);
	CairoSetSouerceRGBA(c_grb);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);

	cairo_set_dash(cr, dashed, 2, 0);
	cairo_move_to(cr, GM_CENTER - GM_RADIUS * 0.7079, GM_CENTER);
	cairo_line_to(cr, GM_CENTER + GM_RADIUS * 0.7079, GM_CENTER);
	cairo_stroke(cr);

	cairo_move_to(cr, GM_CENTER, GM_CENTER - GM_RADIUS * 0.7079);
	cairo_line_to(cr, GM_CENTER, GM_CENTER + GM_RADIUS * 0.7079);
	cairo_stroke(cr);

	if (ui->sf_nfo) {
		cairo_set_source_surface(cr, ui->sf_nfo, 0, GM_BOUNDS - 12);
		cairo_paint (cr);
	}
	cairo_restore(cr);
}

static void draw_pc_annotation(GMUI* ui, cairo_t* cr) {
	cairo_set_operator (cr, CAIRO_OPERATOR_SCREEN);
	CairoSetSouerceRGBA(c_grb);
	cairo_set_line_width(cr, 1.5);

#define PC_ANNOTATION(YPOS) \
	cairo_move_to(cr, PC_LEFT + 2.0, rintf(PC_TOP + YPOS) - .5); \
	cairo_line_to(cr, PC_LEFT + PC_WIDTH - 2.0, rintf(PC_TOP + YPOS) - .5);\
	cairo_stroke(cr);

	PC_ANNOTATION(PC_HEIGHT * 0.1);
	PC_ANNOTATION(PC_HEIGHT * 0.2);
	PC_ANNOTATION(PC_HEIGHT * 0.3);
	PC_ANNOTATION(PC_HEIGHT * 0.4);
	PC_ANNOTATION(PC_HEIGHT * 0.6);
	PC_ANNOTATION(PC_HEIGHT * 0.7);
	PC_ANNOTATION(PC_HEIGHT * 0.8);
	PC_ANNOTATION(PC_HEIGHT * 0.9);

	DRAW_LABEL(5, PC_LEFT + 16, PC_TOP + 14.5);
	DRAW_LABEL(6, PC_LEFT + 16, PC_TOP + PC_HEIGHT - 2.5);

	CairoSetSouerceRGBA(c_glr);
	cairo_set_line_width(cr, 1.5);
	PC_ANNOTATION(PC_HEIGHT * 0.5);
}

static bool cclip(GMUI* ui, cairo_t* cr, int sfc) {
#ifdef CLIP_GM
	if (ui->crop_x1[sfc] <= ui->crop_x0[sfc]) return FALSE;
	if (ui->crop_y1[sfc] <= ui->crop_y0[sfc]) return FALSE;
	cairo_save(cr);
	cairo_rectangle(cr, PC_BOUNDS + ui->crop_x0[sfc], ui->crop_y0[sfc], ui->crop_x1[sfc] - ui->crop_x0[sfc], ui->crop_y1[sfc] - ui->crop_y0[sfc]);
	cairo_clip (cr);
	return TRUE;
#else
	return TRUE;
#endif
}

static bool ccclip(GMUI* ui, cairo_t* cr, int sfc0, int sfc1) {
#ifdef CLIP_GM
	if (ui->crop_x1[sfc1] <= ui->crop_x0[sfc1]) return cclip(ui, cr, sfc0);
	if (ui->crop_y1[sfc1] <= ui->crop_y0[sfc1]) return cclip(ui, cr, sfc0);
	cairo_rectangle_t r0, r1;
	r0.x = ui->crop_x0[sfc0];
	r0.y = ui->crop_y0[sfc0];
	r0.width  = ui->crop_x1[sfc0] - ui->crop_x0[sfc0];
	r0.height = ui->crop_y1[sfc0] - ui->crop_y0[sfc0];

	r1.x = ui->crop_x0[sfc1];
	r1.y = ui->crop_y0[sfc1];
	r1.width  = ui->crop_x1[sfc1] - ui->crop_x0[sfc1];
	r1.height = ui->crop_y1[sfc1] - ui->crop_y0[sfc1];

	rect_combine(&r0, &r1, &r0);

	cairo_save(cr);
	cairo_rectangle(cr, PC_BOUNDS + r0.x, r0.y, r0.width, r0.height);
	cairo_clip (cr);
	return TRUE;
#else
	return TRUE;
#endif
}

static bool cccclip(GMUI* ui, cairo_t* cr, int sfc0, int sfc1, int sfc2) {
#ifdef CLIP_GM
	if (ui->crop_x1[sfc2] <= ui->crop_x0[sfc1]) return ccclip(ui, cr, sfc0, sfc1);
	if (ui->crop_y1[sfc2] <= ui->crop_y0[sfc1]) return ccclip(ui, cr, sfc0, sfc1);
	cairo_rectangle_t r0, r1, r2;
	r0.x = ui->crop_x0[sfc0];
	r0.y = ui->crop_y0[sfc0];
	r0.width  = ui->crop_x1[sfc0] - ui->crop_x0[sfc0];
	r0.height = ui->crop_y1[sfc0] - ui->crop_y0[sfc0];

	r1.x = ui->crop_x0[sfc1];
	r1.y = ui->crop_y0[sfc1];
	r1.width  = ui->crop_x1[sfc1] - ui->crop_x0[sfc1];
	r1.height = ui->crop_y1[sfc1] - ui->crop_y0[sfc1];

	r2.x = ui->crop_x0[sfc2];
	r2.y = ui->crop_y0[sfc2];
	r2.width  = ui->crop_x1[sfc2] - ui->crop_x0[sfc2];
	r2.height = ui->crop_y1[sfc2] - ui->crop_y0[sfc2];

	rect_combine(&r0, &r1, &r0);
	rect_combine(&r0, &r2, &r0);

	cairo_save(cr);
	cairo_rectangle(cr, PC_BOUNDS + r0.x, r0.y, r0.width, r0.height);
	cairo_clip (cr);
	return TRUE;
#else
	return TRUE;
#endif
}

static void ccrestore(cairo_t* cr) {
#ifdef CLIP_GM

#ifdef VISIBLE_EXPOSE
			cairo_set_source_rgba (cr, .0, .0, .5, .25);
			cairo_rectangle (cr, PC_BOUNDS, 0, GM_BOUNDS, GM_BOUNDS);
			cairo_fill(cr);
#endif

			cairo_restore(cr);
#endif
}


/******************************************************************************
 * main drawing
 */
static bool expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev) {
	GMUI* ui = (GMUI*)GET_HANDLE(handle);
	LV2gm* self = (LV2gm*) ui->instance;

	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	if (rect_intersect_a(ev, PC_BOUNDS, 0, GM_BOUNDS, GM_BOUNDS)) {
		/* process and draw goniometer data */
		if (ui->ntfy_b != ui->ntfy_u) {
			//printf("%d -> %d\n", ui->ntfy_u, ui->ntfy_b);
			ui->ntfy_u = ui->ntfy_b;
			draw_rb(ui, self->rb);
		}

		/* display goniometer */
		const bool composit = !robtk_cbtn_get_active(ui->cbn_xfade);
		if (!composit) {
#if 0 // CLIP_GM
			const float persist = .5 + .005 * robtk_dial_get_value(ui->spn_alpha);
			if (persist >= 1.0) {
				cclip(ui, cr, ui->sfc);
			}
#endif
#if 0 // requires clear when switching to CRT w/ persist >0
			// but no regular clear is required here, then.
			cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
#else
			cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
			CairoSetSouerceRGBA(c_blk);
			cairo_rectangle (cr, PC_BOUNDS, 0, GM_BOUNDS, GM_BOUNDS);
			cairo_fill(cr);

			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
#endif
			cairo_set_source_surface(cr, ui->sf[ui->sfc], PC_BOUNDS, 0);
			cairo_paint (cr);
#if 0 // CLIP_GM -- TODO minimalize draw area
			if (persist >= 1.0) {
				ccrestore(cr);
			}
#endif
		} else {
			// TODO tweak and optimize overlay compositing

			CairoSetSouerceRGBA(c_blk);
			cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle (cr, PC_BOUNDS, 0, GM_BOUNDS, GM_BOUNDS);
			cairo_fill(cr);

			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

			if (cclip(ui, cr, (ui->sfc + 1)%3)) {
				cairo_set_source_surface(cr, ui->sf[(ui->sfc + 1)%3], PC_BOUNDS, 0);
				cairo_paint (cr);
				ccrestore(cr);
			}

			if (ccclip(ui, cr, (ui->sfc + 0)%3, (ui->sfc + 1)%3)) {
				cairo_set_source_surface(cr, ui->sf[(ui->sfc + 0)%3], PC_BOUNDS, 0);
				cairo_paint (cr);
				ccrestore(cr);
			}

			if (cccclip(ui, cr, (ui->sfc + 2)%3, (ui->sfc + 0)%3, (ui->sfc + 1)%3)) {
				cairo_set_source_surface(cr, ui->sf[(ui->sfc + 2)%3], PC_BOUNDS, 0);
				cairo_paint (cr);
				ccrestore(cr);
			}
		}
		draw_gm_labels(ui, cr);

		if (ui->xrundisplay < 0) { ui->xrundisplay++; }
		else if (self->rb_overrun) { if (!getenv("X42_GONIOMETER_NO_WARN")) ui->xrundisplay = 36; }
		else if (ui->xrundisplay > 0) ui->xrundisplay--;
		self->rb_overrun = false;

		if (ui->xrundisplay > 0) {
			cairo_save(cr);
			cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
			float c_warn[4] = {1.0, 0.0, 0.0, 0.75};
			if (ui->xrundisplay < 30) {
				c_warn[3] = ui->xrundisplay * .025;
			}
			CairoSetSouerceRGBA(c_warn);
			rounded_rectangle (cr, PC_BOUNDS + GM_BOUNDS - 175, GM_BOUNDS - 33, 170, 28, 6);
			cairo_fill(cr);
			write_text(cr, "Display buffer overflow\nYour system is not fast enough.",
					FONT_LB, GM_BOUNDS - 102, GM_BOUNDS - 27,
					0, c_g90);

			CairoSetSouerceRGBA(c_blk);
			cairo_set_line_width(cr, 1.5);
			float tx,ty;
			tx = PC_BOUNDS + GM_BOUNDS - 157.5;
			ty = GM_BOUNDS - 27.5;
			cairo_move_to(cr, tx, ty);
			cairo_line_to(cr, tx-10, ty+17);
			cairo_line_to(cr, tx+10, ty+17);
			cairo_close_path (cr);
			cairo_stroke(cr);
			cairo_move_to(cr, tx, ty+6);
			cairo_line_to(cr, tx, ty+11);
			cairo_stroke(cr);
			cairo_move_to(cr, tx, ty+14);
			cairo_line_to(cr, tx, ty+14);
			cairo_stroke(cr);
			cairo_restore(cr);
		}
	}

	if (rect_intersect_a(ev, 0, 0, PC_BOUNDS, GM_BOUNDS)) {
		/* display phase-correlation */
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		/* PC meter backgroud */
		CairoSetSouerceRGBA(c_g20);
		cairo_rectangle (cr, 0, 0, PC_BOUNDS, GM_BOUNDS);
		cairo_fill(cr);

		CairoSetSouerceRGBA(c_blk);
		rounded_rectangle (cr, PC_LEFT -1.0, PC_TOP - 2.0 , PC_WIDTH + 2.0, PC_HEIGHT + 4.0, 6);
		cairo_fill(cr);

		/* value */
		CairoSetSouerceRGBA(c_glb);
		const float c = .5 + rintf(PC_BLOCKSIZE * ui->cor);
		rounded_rectangle (cr, PC_LEFT, PC_TOP + c, PC_WIDTH, PC_BLOCK, 2);
		cairo_fill(cr);

		draw_pc_annotation(ui, cr);
	}
	return TRUE;
}

/******************************************************************************
 * internal helpers/callbacks
 */

static void save_state(GMUI* ui) {
	LV2gm* self = (LV2gm*) ui->instance;
	self->s_autogain   = robtk_cbtn_get_active(ui->cbn_autogain);
	self->s_oversample = robtk_cbtn_get_active(ui->cbn_src);
	self->s_line       = robtk_cbtn_get_active(ui->cbn_lines);
	self->s_persist    = robtk_cbtn_get_active(ui->cbn_xfade);
	self->s_preferences= robtk_cbtn_get_active(ui->cbn_preferences);

	self->s_sfact = robtk_spin_get_value(ui->spn_src_fact);
	if (self->s_line) {
		self->s_linewidth = robtk_spin_get_value(ui->spn_psize);
	} else {
		self->s_pointwidth = robtk_spin_get_value(ui->spn_psize);
	}
	self->s_persistency = robtk_dial_get_value(ui->spn_alpha);
	self->s_max_freq = robtk_spin_get_value(ui->spn_vfreq);

	self->s_gattack = robtk_dial_get_value(ui->spn_gattack);
	self->s_gdecay = robtk_dial_get_value(ui->spn_gdecay);
	self->s_compress = robtk_dial_get_value(ui->spn_compress);

	self->s_gtarget = robtk_dial_get_value(ui->spn_gtarget);
	self->s_grms = robtk_dial_get_value(ui->spn_grms);
}

/******************************************************************************
 * child callbacks
 */
static bool cb_save_state(RobWidget *w, gpointer handle) {
	GMUI* ui = (GMUI*)handle;
	save_state(ui);
	return TRUE;
}

static bool set_gain(RobWidget* w, gpointer handle) {
	GMUI* ui = (GMUI*)handle;
	ui->gain = INV_GAINSCALE(robtk_scale_get_value(ui->fader));
	const bool autogain = robtk_cbtn_get_active(ui->cbn_autogain);
	if (!ui->disable_signals && !autogain) {
		ui->write(ui->controller, 4, sizeof(float), 0, (const void*) &ui->gain);
	}
	return TRUE;
}

static bool cb_autogain(RobWidget *w, gpointer handle) {
	GMUI* ui = (GMUI*)handle;
	const bool autogain = robtk_cbtn_get_active(ui->cbn_autogain);
	if (autogain) {
		robtk_scale_set_sensitive(ui->fader, false);
		robtk_dial_set_sensitive(ui->spn_gattack, true);
		robtk_dial_set_sensitive(ui->spn_gdecay, true);
		robtk_dial_set_sensitive(ui->spn_gtarget, true);
		robtk_dial_set_sensitive(ui->spn_grms, true);
	} else {
		robtk_scale_set_sensitive(ui->fader, true);
		robtk_dial_set_sensitive(ui->spn_gattack, false);
		robtk_dial_set_sensitive(ui->spn_gdecay, false);
		robtk_dial_set_sensitive(ui->spn_gtarget, false);
		robtk_dial_set_sensitive(ui->spn_grms, false);
		ui->write(ui->controller, 4, sizeof(float), 0, (const void*) &ui->gain);
	}
	save_state(ui);
	return TRUE;
}

static bool cb_autosettings(RobWidget *w, gpointer handle) {
	GMUI* ui = (GMUI*)handle;
	float g_attack = robtk_dial_get_value(ui->spn_gattack);
	float g_decay = robtk_dial_get_value(ui->spn_gdecay);
	g_attack = 0.1 * exp(0.06 * g_attack) - .09;
	g_decay  = 0.1 * exp(0.06 * g_decay ) - .09;
	if (g_attack < .01) g_attack = .01;
	if (g_decay  < .01) g_decay = .01;
	ui->attack_pow = g_attack;
	ui->decay_pow = g_decay;
	//
	float g_rms = .01 * robtk_dial_get_value(ui->spn_grms);
	ui->g_rms = g_rms;

	float g_target = robtk_dial_get_value(ui->spn_gtarget);
	g_target = exp(1.8 * (-.02 * g_target + 1.0));
	if (g_target < .15) g_target = .15;
	ui->g_target = g_target;
	save_state(ui);
	return TRUE;
}

static bool cb_expose(RobWidget *w, gpointer handle) {
	GMUI* ui = (GMUI*)handle;
	queue_draw(ui->m0);
	save_state(ui);
	return TRUE;
}

static bool cb_lines(RobWidget *w, gpointer handle) {
	GMUI* ui = (GMUI*)handle;
	LV2gm* self = (LV2gm*) ui->instance;
	const bool nowlines = robtk_cbtn_get_active(ui->cbn_lines);
	if (!nowlines) {
		robtk_lbl_set_text(ui->lbl_psize, "Point Size [px]:");
		self->s_linewidth = robtk_spin_get_value(ui->spn_psize);
		robtk_spin_set_default(ui->spn_psize, 1.75);
	} else {
		robtk_lbl_set_text(ui->lbl_psize, "Line Width [px]:");
		self->s_pointwidth = robtk_spin_get_value(ui->spn_psize);
		robtk_spin_set_default(ui->spn_psize, 0.75);
	}
	robtk_spin_set_value(ui->spn_psize, nowlines ? self->s_linewidth : self->s_pointwidth);
	return cb_expose(w, handle);
}

static bool cb_xfade(RobWidget *w, gpointer handle) {
	GMUI* ui = (GMUI*)handle;
	if(!robtk_cbtn_get_active(ui->cbn_xfade)) {
		robtk_dial_set_sensitive(ui->spn_alpha, false);
	} else {
		robtk_dial_set_sensitive(ui->spn_alpha, true);
	}
	return cb_expose(w, handle);
}

static bool cb_vfreq(RobWidget *w, gpointer handle) {
	GMUI* ui = (GMUI*)handle;
	LV2gm* self = (LV2gm*) ui->instance;
	float v = robtk_spin_get_value(ui->spn_vfreq);
	if (v < 10) {
		robtk_spin_set_value(ui->spn_vfreq, 10);
		return TRUE;
	}
	if (v > 100) {
		robtk_spin_set_value(ui->spn_vfreq, 100);
		return TRUE;
	}

	v = rint(self->rate / v);

	self->apv = v;
	save_state(ui);
	return TRUE;
}

static bool cb_src(RobWidget *w, gpointer handle) {
	GMUI* ui = (GMUI*)handle;
	if (robtk_cbtn_get_active(ui->cbn_src)) {
		setup_src(ui, robtk_spin_get_value(ui->spn_src_fact), 12, 1.0);
	} else {
		setup_src(ui, 0, 0, 0);
	}
	save_state(ui);
	return TRUE;
}

static bool cb_preferences(RobWidget *w, gpointer handle) {
	GMUI* ui = (GMUI*)handle;
	if (robtk_cbtn_get_active(ui->cbn_preferences)) {
		robwidget_show(ui->c_tbl, true);
	} else {
		robwidget_hide(ui->c_tbl, true);
	}
	queue_draw(ui->box);
	save_state(ui);
	return TRUE;
}

static void restore_state(GMUI* ui) {
	LV2gm* self = (LV2gm*) ui->instance;
	ui->disable_signals = true;
	robtk_spin_set_value(ui->spn_src_fact, self->s_sfact);
	robtk_spin_set_value(ui->spn_psize, self->s_line ? self->s_linewidth : self->s_pointwidth);
	robtk_spin_set_value(ui->spn_vfreq, self->s_max_freq);
	robtk_dial_set_value(ui->spn_alpha, self->s_persistency);

	robtk_dial_set_value(ui->spn_gattack, self->s_gattack);
	robtk_dial_set_value(ui->spn_gdecay, self->s_gdecay);
	robtk_dial_set_value(ui->spn_compress, self->s_compress);
	robtk_dial_set_value(ui->spn_gtarget, self->s_gtarget);
	robtk_dial_set_value(ui->spn_grms, self->s_grms);

	robtk_cbtn_set_active(ui->cbn_autogain,    self->s_autogain);
	robtk_cbtn_set_active(ui->cbn_src,         self->s_oversample);
	robtk_cbtn_set_active(ui->cbn_lines,       self->s_line);
	robtk_cbtn_set_active(ui->cbn_xfade,       self->s_persist);
	robtk_cbtn_set_active(ui->cbn_preferences, self->s_preferences);
	// TODO optimize, temp disable save during these
	cb_autogain(NULL, ui);
	cb_src(NULL, ui);
	cb_vfreq(NULL, ui);
	cb_xfade(NULL, ui);
	cb_autosettings(NULL, ui);
	ui->disable_signals = false;
}

/* instance connection */
#if (defined USE_GUI_THREAD && defined THREADSYNC)
static void expose_goniometer(void* ptr) {
	GMUI* ui = (GMUI*) ptr;
	ui->ntfy_b = (ui->ntfy_b + 1 )% 10000;
	queue_draw_area(ui->m0, PC_BOUNDS, 0, GM_BOUNDS, GM_BOUNDS);
}
#endif

/******************************************************************************
 * widget hackery
 */

static void
size_request(RobWidget* handle, int *w, int *h) {
	GMUI* ui = (GMUI*)GET_HANDLE(handle);
	robwidget_set_size(ui->m0, ui->w_width, ui->w_height);
	*w = ui->w_width;
	*h = ui->w_height;
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
	GMUI* ui = (GMUI*) calloc(1,sizeof(GMUI));
	*widget = NULL;

	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, "http://lv2plug.in/ns/ext/instance-access")) {
			ui->instance = (LV2_Handle*)features[i]->data;
		}
	}

	if (!ui->instance) {
		fprintf(stderr, "meters.lv2 error: Host does not support instance-access\n");
		free(ui);
		return NULL;
	}

	LV2gm* self = (LV2gm*) ui->instance;

	ui->write      = write_function;
	ui->controller = controller;
	ui->nfo        = robtk_info(ui_toplevel);

	get_color_from_theme(0, ui->c_txt);
	alloc_sf(ui);
	alloc_annotations(ui);

	ui->last_x = (GM_CENTER);
	ui->last_y = (GM_CENTER);
	ui->lp0 = 0;
	ui->lp1 = 0;
	ui->hpw = expf(-2.0 * M_PI * 20 / self->rate);
	ui->gain = 1.0;

	ui->cor = 0.5;
	ui->ntfy_b = 0;
	ui->ntfy_u = 1;
	ui->disable_signals = false;
	ui->src_fact = 1;
	ui->initialized = false;

	ui->box = rob_vbox_new(FALSE, 2);
	robwidget_make_toplevel(ui->box, ui_toplevel);
	ROBWIDGET_SETNAME(ui->box, "goni");

	ui->b_box = rob_hbox_new(TRUE, 6);

	ui->m0 = robwidget_new(ui);
	robwidget_set_expose_event(ui->m0, expose_event);
	robwidget_set_size_request(ui->m0, size_request);
	robwidget_set_alignment(ui->m0, .5 , 0);

	ui->fader = robtk_scale_new(3.0, 9.1, .01, TRUE);
	ui->cbn_autogain = robtk_cbtn_new("Auto Gain", GBT_LED_LEFT, false);


	ui->cbn_preferences = robtk_cbtn_new((const char*) "Settings", /* GBT_LED_OFF */ GBT_LED_LEFT, false);
	ui->c_tbl        = rob_table_new(/*rows*/6, /*cols*/ 5, FALSE);
	ui->cbn_src      = robtk_cbtn_new("Oversample", GBT_LED_LEFT, false);
	ui->spn_src_fact = robtk_spin_new(2, 32, 1);

	ui->spn_compress = robtk_dial_new(0.0, 100.0, 0.5);
	ui->spn_gattack  = robtk_dial_new(0.0, 100.0, 1.0);
	ui->spn_gdecay   = robtk_dial_new(0.0, 100.0, 1.0);

	ui->spn_gtarget  = robtk_dial_new(0.0, 100.0, 1.0);
	ui->spn_grms     = robtk_dial_new(0.0, 100.0, 1.0);

	ui->cbn_lines    = robtk_cbtn_new("Draw Lines", GBT_LED_LEFT, false);
	ui->cbn_xfade    = robtk_cbtn_new("CRT Persistency:", GBT_LED_LEFT, true);
	ui->spn_psize    = robtk_spin_new(.25, 5.25, .25);

	ui->spn_vfreq    = robtk_spin_new(10, 100, 5);
	ui->spn_alpha    = robtk_dial_new(0, 100, .5);

	robtk_cbtn_set_color_on(ui->cbn_preferences, .2, .8, .1);
	robtk_cbtn_set_color_off(ui->cbn_preferences, .1, .3, .1);
	robtk_cbtn_set_color_on(ui->cbn_lines, .1, .3, .8);
	robtk_cbtn_set_color_off(ui->cbn_lines, .1, .1, .3);

	robtk_cbtn_set_alignment(ui->cbn_preferences, .5, .5);
	robtk_cbtn_set_alignment(ui->cbn_autogain, .5, .5);
	robtk_cbtn_set_alignment(ui->cbn_src, .5, .5);
	robtk_cbtn_set_alignment(ui->cbn_lines, .5, .5);

	/* see also src/goniometerlv2.c -TOD  coax the defaults out of there */
	robtk_dial_set_default(ui->spn_compress, 0.0);
	robtk_dial_set_default(ui->spn_gattack, 54.0);
	robtk_dial_set_default(ui->spn_gdecay, 58.0);
	robtk_dial_set_default(ui->spn_gtarget, 40.0);
	robtk_dial_set_default(ui->spn_grms, 50.0);
	robtk_dial_set_default(ui->spn_alpha, 33.0);
	robtk_spin_set_default(ui->spn_vfreq, 50.0);
	robtk_spin_set_default(ui->spn_src_fact, 4.0);
	robtk_spin_set_default(ui->spn_psize, 1.25); // see also cb_lines()

	robtk_dial_set_value(ui->spn_compress, 0.0);
	robtk_dial_set_value(ui->spn_gattack, 1.0);
	robtk_dial_set_value(ui->spn_gdecay, 1.0);

	robtk_dial_set_value(ui->spn_gtarget, 50.0);
	robtk_dial_set_value(ui->spn_grms, 0.0);

	robtk_spin_set_value(ui->spn_src_fact, 4.0);
	robtk_spin_set_value(ui->spn_psize, 1.25);
	robtk_spin_set_value(ui->spn_vfreq, 25);
	robtk_dial_set_value(ui->spn_alpha, 0);

	robtk_spin_set_digits(ui->spn_psize, 2);

	ui->sep_h0        = robtk_sep_new(TRUE);
	ui->sep_h1        = robtk_sep_new(TRUE);
	ui->sep_v0        = robtk_sep_new(FALSE);

	ui->lbl_src_fact  = robtk_lbl_new("Oversampling:");
	ui->lbl_psize     = robtk_lbl_new("Line/Point [Px]:");
	ui->lbl_vfreq     = robtk_lbl_new("Limit FPS [Hz]:");
	ui->lbl_compress  = robtk_lbl_new("Inflate:");
	ui->lbl_gattack   = robtk_lbl_new("Attack Speed:");
	ui->lbl_gdecay    = robtk_lbl_new("Decay Speed:");
	ui->lbl_gtarget   = robtk_lbl_new("Target Zoom:");
	ui->lbl_grms      = robtk_lbl_new("RMS / Peak:");

	robtk_lbl_set_alignment(ui->lbl_vfreq,    1.0f, 0.5f);
	robtk_lbl_set_alignment(ui->lbl_compress, 1.0f, 0.5f);
	robtk_lbl_set_alignment(ui->lbl_gattack,  1.0f, 0.5f);
	robtk_lbl_set_alignment(ui->lbl_gdecay,   1.0f, 0.5f);
	robtk_lbl_set_alignment(ui->lbl_gtarget,  1.0f, 0.5f);
	robtk_lbl_set_alignment(ui->lbl_grms,     1.0f, 0.5f);
	robtk_lbl_set_alignment(ui->lbl_src_fact, 1.0f, 0.5f);
	robtk_lbl_set_alignment(ui->lbl_psize,    1.0f, 0.5f);
	robtk_cbtn_set_alignment(ui->cbn_xfade,   1.0f, 0.5f);

	robtk_spin_set_alignment(ui->spn_src_fact, 0.0, .5);
	robtk_spin_set_alignment(ui->spn_psize,    0.0, .5);
	robtk_spin_set_alignment(ui->spn_vfreq,    0.0, .5);
	robtk_dial_set_alignment(ui->spn_gattack,  0.0, .5);
	robtk_dial_set_alignment(ui->spn_gtarget,  0.0, .5);

	robtk_spin_label_width(ui->spn_src_fact, 12.0, 32);
	robtk_spin_label_width(ui->spn_psize,    12.0, 32);
	robtk_spin_label_width(ui->spn_vfreq,    12.0, 32);

	robtk_dial_set_surface(ui->spn_gattack,  ui->dial[0]);
	robtk_dial_set_surface(ui->spn_gdecay,   ui->dial[0]);
	robtk_dial_set_surface(ui->spn_gtarget,  ui->dial[3]);
	robtk_dial_set_surface(ui->spn_grms,     ui->dial[1]);
	robtk_dial_set_surface(ui->spn_compress, ui->dial[2]);
	robtk_dial_set_surface(ui->spn_alpha,    ui->dial[2]);

	/* fader init */
	robtk_scale_set_default(ui->fader, GAINSCALE(1.0));
	robtk_scale_set_value(ui->fader, GAINSCALE(1.0));

	robtk_scale_add_mark(ui->fader, GAINSCALE(7.9432), (const char*) "+18dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(5.6234), (const char*) "+15dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(3.9810), (const char*) "+12dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(2.8183), (const char*)  "+9dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(1.9952), (const char*)  "+6dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(1.4125), (const char*)  "+3dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(1.0000), (const char*)   "0dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(0.7079), (const char*)  "-3dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(0.5012), (const char*)  "-6dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(0.3548), (const char*)  "-9dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(0.2511), (const char*) "-12dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(0.1778), (const char*) "-15dB");
	robtk_scale_add_mark(ui->fader, GAINSCALE(0.1258), (const char*) "-18dB");

	ui->w_width = PC_BOUNDS + GM_BOUNDS;
	ui->w_height = GM_BOUNDS;

	/* layout */
	int row = 0;
	rob_table_attach((ui->c_tbl), robtk_sep_widget(ui->sep_h0), 0, 5, row, row+1, 0, 4, RTK_EXANDF, RTK_SHRINK);

	row++;
	rob_table_attach_defaults((ui->c_tbl), robtk_scale_widget(ui->fader), 0, 5, row, row+1);

	row++;
	rob_table_attach((ui->c_tbl), GLB_W(ui->lbl_gattack)          , 0, 1, row, row+1, 2, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach_defaults((ui->c_tbl), GED_W(ui->spn_gattack) , 1, 2, row, row+1);
	rob_table_attach((ui->c_tbl), GLB_W(ui->lbl_gdecay)           , 3, 4, row, row+1, 2, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach_defaults((ui->c_tbl), GED_W(ui->spn_gdecay)  , 4, 5, row, row+1);

	row++;
	rob_table_attach((ui->c_tbl), GLB_W(ui->lbl_gtarget)          , 0, 1, row, row+1, 2, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach_defaults((ui->c_tbl), GED_W(ui->spn_gtarget) , 1, 2, row, row+1);
	rob_table_attach((ui->c_tbl), GLB_W(ui->lbl_grms)             , 3, 4, row, row+1, 2, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach_defaults((ui->c_tbl), GED_W(ui->spn_grms)    , 4, 5, row, row+1);

	row++;
	rob_table_attach((ui->c_tbl), robtk_sep_widget(ui->sep_h1), 0, 5, row, row+1, 0, 2, RTK_EXANDF, RTK_SHRINK);

	row++;
	rob_table_attach((ui->c_tbl), robtk_sep_widget(ui->sep_v0), 2, 3, row, row+3, 3, 0, RTK_SHRINK, RTK_FILL);

	rob_table_attach((ui->c_tbl), GLB_W(ui->lbl_src_fact)         , 0, 1, row, row+1, 2, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach_defaults((ui->c_tbl), GSP_W(ui->spn_src_fact), 1, 2, row, row+1);

	rob_table_attach((ui->c_tbl), GBT_W(ui->cbn_xfade)            , 3, 4, row, row+1, 2, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach_defaults((ui->c_tbl), GED_W(ui->spn_alpha)   , 4, 5, row, row+1);

	row++;
	rob_table_attach((ui->c_tbl), GLB_W(ui->lbl_psize)            , 0, 1, row, row+1, 2, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach_defaults((ui->c_tbl), GSP_W(ui->spn_psize)   , 1, 2, row, row+1);

#ifdef WITH_INFLATE
	row++;
	rob_table_attach((ui->c_tbl), GLB_W(ui->lbl_vfreq)            , 0, 1, row, row+1, 2, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach_defaults((ui->c_tbl), GSP_W(ui->spn_vfreq)   , 1, 2, row, row+1);

	rob_table_attach((ui->c_tbl), GLB_W(ui->lbl_compress)         , 3, 4, row, row+1, 2, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach_defaults((ui->c_tbl), GED_W(ui->spn_compress), 4, 5, row, row+1);
#else
	rob_table_attach((ui->c_tbl), GLB_W(ui->lbl_vfreq)            , 3, 4, row, row+1, 2, 0, RTK_SHRINK, RTK_SHRINK);
	rob_table_attach_defaults((ui->c_tbl), GSP_W(ui->spn_vfreq)   , 4, 5, row, row+1);

	robtk_sep_set_linewidth(ui->sep_v0, 0);
#endif

	/* button box packing */
	rob_hbox_child_pack(ui->b_box, GBT_W(ui->cbn_preferences), FALSE, FALSE);
	rob_hbox_child_pack(ui->b_box, GBT_W(ui->cbn_autogain), FALSE, FALSE);
	rob_hbox_child_pack(ui->b_box, GBT_W(ui->cbn_src), FALSE, FALSE);
	rob_hbox_child_pack(ui->b_box, GBT_W(ui->cbn_lines), FALSE, FALSE);

	/* global packing */
	rob_vbox_child_pack(ui->box, ui->m0, FALSE, TRUE);
	rob_vbox_child_pack(ui->box, ui->b_box, FALSE, TRUE);
	rob_vbox_child_pack(ui->box, ui->c_tbl, FALSE, TRUE);

	robtk_cbtn_set_active(ui->cbn_preferences, FALSE);
	restore_state(ui);

	if (!robtk_cbtn_get_active(ui->cbn_preferences)) {
		robwidget_hide(ui->c_tbl, true);
	}

	robwidget_set_expose_event(ui->m0, expose_event);

	robtk_scale_set_callback(ui->fader, set_gain, ui);
	robtk_spin_set_callback(ui->spn_src_fact, cb_src, ui);

	robtk_cbtn_set_callback(ui->cbn_autogain, cb_autogain, ui);
	robtk_cbtn_set_callback(ui->cbn_src, cb_src, ui);

	robtk_dial_set_callback(ui->spn_gattack, cb_autosettings, ui);
	robtk_dial_set_callback(ui->spn_gdecay, cb_autosettings, ui);
	robtk_dial_set_callback(ui->spn_gtarget, cb_autosettings, ui);
	robtk_dial_set_callback(ui->spn_grms, cb_autosettings, ui);

	robtk_dial_set_callback(ui->spn_compress, cb_expose, ui);
	robtk_dial_set_callback(ui->spn_alpha, cb_save_state, ui);

	robtk_cbtn_set_callback(ui->cbn_lines, cb_lines, ui);
	robtk_cbtn_set_callback(ui->cbn_xfade, cb_xfade, ui);

	robtk_spin_set_callback(ui->spn_psize, cb_expose, ui);
	robtk_spin_set_callback(ui->spn_vfreq, cb_vfreq, ui);

	robtk_cbtn_set_callback(ui->cbn_preferences, cb_preferences, ui);

	*widget = ui->box;

	gmrb_read_clear(self->rb);
	ui->xrundisplay = -100;
	self->ui_active = true;

#if (defined USE_GUI_THREAD && defined THREADSYNC)
	GLrobtkLV2UI *glui = (GLrobtkLV2UI*) ui_toplevel;
	self->msg_thread_lock = &glui->msg_thread_lock;
	self->data_ready = &glui->data_ready;
	self->queue_display = &expose_goniometer;
	self->ui = ui;
#endif
	return ui;
}

static enum LVGLResize
plugin_scale_mode(LV2UI_Handle handle)
{
	GMUI* ui = (GMUI*)handle;
	if (robtk_cbtn_get_active(ui->cbn_preferences)) {
		return LVGL_LAYOUT_TO_FIT;
	} else {
		ui->box->resized = TRUE;
		return LVGL_ZOOM_TO_ASPECT;
	}
}

static void
cleanup(LV2UI_Handle handle)
{
	GMUI* ui = (GMUI*)handle;
	LV2gm* i = (LV2gm*)ui->instance;

	i->ui_active = false;

	for (int i=0; i < 3 ; ++i) {
		cairo_surface_destroy(ui->sf[i]);
	}

	for (int i=0; i < 7 ; ++i) {
		cairo_surface_destroy(ui->an[i]);
	}
	for (int i=0; i < 4 ; ++i) {
		cairo_surface_destroy(ui->dial[i]);
	}
	cairo_surface_destroy(ui->sf_nfo);

	robtk_cbtn_destroy(ui->cbn_autogain);
	robtk_cbtn_destroy(ui->cbn_src);
	robtk_spin_destroy(ui->spn_src_fact);
	robtk_dial_destroy(ui->spn_compress);
	robtk_dial_destroy(ui->spn_gattack);
	robtk_dial_destroy(ui->spn_gdecay);
	robtk_dial_destroy(ui->spn_gtarget);
	robtk_dial_destroy(ui->spn_grms);
	robtk_cbtn_destroy(ui->cbn_lines);
	robtk_cbtn_destroy(ui->cbn_xfade);
	robtk_spin_destroy(ui->spn_psize);
	robtk_spin_destroy(ui->spn_vfreq);
	robtk_dial_destroy(ui->spn_alpha);

	robtk_scale_destroy(ui->fader);
	robtk_lbl_destroy(ui->lbl_src_fact);
	robtk_lbl_destroy(ui->lbl_psize);
	robtk_lbl_destroy(ui->lbl_vfreq);
	robtk_lbl_destroy(ui->lbl_compress);
	robtk_lbl_destroy(ui->lbl_gattack);
	robtk_lbl_destroy(ui->lbl_gdecay);
	robtk_lbl_destroy(ui->lbl_gtarget);
	robtk_lbl_destroy(ui->lbl_grms);
	robtk_sep_destroy(ui->sep_h0);
	robtk_sep_destroy(ui->sep_h1);
	robtk_sep_destroy(ui->sep_v0);

	robtk_cbtn_destroy(ui->cbn_preferences);

	robwidget_destroy(ui->m0);
	rob_box_destroy(ui->b_box);
	rob_table_destroy(ui->c_tbl);
	rob_box_destroy(ui->box);

	delete ui->src;
	free(ui->scratch);
	free(ui->resampl);

	i->msg_thread_lock = NULL;
	free(ui);
}

static const void*
extension_data(const char* uri)
{
	return NULL;
}

static void invalidate_gm(GMUI* ui) {
	queue_draw_area(ui->m0, PC_BOUNDS, 0, GM_BOUNDS, GM_BOUNDS);
}

/******************************************************************************
 * backend communication
 */
static void invalidate_pc(GMUI* ui, const float val) {
	float c0, c1;
	if (rint(PC_BLOCKSIZE * ui->cor * 2) == rint (PC_BLOCKSIZE * val * 2)) return;
	c0 = PC_BLOCKSIZE * MIN(ui->cor, val);
	c1 = PC_BLOCKSIZE * MAX(ui->cor, val);
	ui->cor = val;
	queue_tiny_area(ui->m0, PC_LEFT, floorf(PC_TOP + c0), PC_WIDTH, ceilf(PC_BLOCK + 1 + c1 - c0));
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
	GMUI* ui = (GMUI*)handle;
	if (format != 0) return;
	if (port_index == 4) {
		float v = *(float *)buffer;
		if (v >= 0.001 && v <= 20.0) {
			ui->disable_signals = true;
			robtk_scale_set_value(ui->fader, GAINSCALE(v));
			ui->disable_signals = false;
		}
	} else
	if (port_index == 5) {
		invalidate_pc(ui, 0.5f * (1.0f - *(float *)buffer));
	} else
	if (port_index == 6) {
		ui->ntfy_b = (uint32_t) (*(float *)buffer);
		invalidate_gm(ui);
	}
}
