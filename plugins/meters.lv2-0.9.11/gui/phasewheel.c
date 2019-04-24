/* FFT Phase-Wheel Display
 *
 * Copyright (C) 2013-2014 Robin Gareus <robin@gareus.org>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* various GUI pixel sizes */
#define LVGL_RESIZEABLE

#define PH_RAD   (160) ///< radius of main data display
#define PH_POINT (ui->pscale * 3.0) ///< radius a single data point

#define XOFF 5
#define YOFF 5

#define MIN_CUTOFF (-80.f) // dB

/* level range annotation */
#define ANN_W (2 * (PH_RAD + XOFF))
#define ANN_H 32
#define ANN_B 25 ///< offset from bottom

/* phase correlation meter width/height */
#define PC_BOUNDW ( 60.0f)
#define PC_BOUNDH (ui->height)

/* phase correlation meter inner sizes */
#define PC_TOP       (  5.0f)
#define PC_LEFT      ( 19.0f)
#define PC_BLOCK     ( 10.0f)
#define PC_WIDTH     ( 22.0f)
#define PC_HEIGHT    (PC_BOUNDH - 2 * PC_TOP)
#define PC_BLOCKSIZE (PC_HEIGHT - PC_BLOCK)

static const float c_ann[4] = {0.5, 0.5, 0.5, 1.0}; // text annotation color
static const float c_ahz[4] = {0.6, 0.6, 0.6, 0.6}; // frequency annotation
static const float c_grd[4] = {0.4, 0.4, 0.4, 1.0}; // grid color

#ifdef _WIN32
#define snprintf(s, l, ...) sprintf(s, __VA_ARGS__)
#endif

/******************************************************************************
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "../src/uri2.h"
#include "fft.c"

#ifndef MIN
#define MIN(A,B) ( (A) < (B) ? (A) : (B) )
#endif
#ifndef MAX
#define MAX(A,B) ( (A) > (B) ? (A) : (B) )
#endif

#define RTK_URI "http://gareus.org/oss/lv2/meters#"
#define RTK_GUI "phasewheelui"

#define FFT_BINS_MAX 8192 // half of the FFT data-size

#ifdef _WIN32
#define snprintf(s, l, ...) sprintf(s, __VA_ARGS__)
#endif

enum {
	MF_PHASE = 6,
	MF_GAIN,
	MF_CUTOFF,
	MF_FFT,
	MF_BAND,
	MF_NORM,
	MF_SCREEN
};


typedef struct {
	LV2_Atom_Forge forge;
	LV2_URID_Map*  map;
	XferLV2URIs uris;

	LV2UI_Write_Function write;
	LV2UI_Controller controller;

	float rate;

	struct FFTAnalysis *fa;
	struct FFTAnalysis *fb;

	RobWidget* rw;
	RobWidget* m0;
	RobWidget* m1;
	RobWidget* m2;

	RobWidget* hbox1;
	RobWidget* hbox2;
	RobWidget* hbox3;

	RobTkDial* screen;
	RobTkDial* gain;
	RobTkCBtn* btn_oct;
	RobTkCBtn* btn_norm;
	RobTkSelect* sel_fft;
	RobTkLbl* lbl_fft;
	RobTkLbl* lbl_screen;
	RobTkSep* sep0;
	RobTkSep* sep1;
	RobTkSep* sep2;
	RobTkSep* sep3;
	RobTkSep* sep4;

	cairo_surface_t* sf_dat;
	cairo_surface_t* sf_ann;
	cairo_surface_t* sf_nfo;

	PangoFontDescription *font[2];
	cairo_surface_t* sf_dial;
	cairo_surface_t* sf_gain;
	cairo_surface_t* sf_pc[2];

	float db_cutoff;
	float db_thresh;
	float cor;

	float phase[FFT_BINS_MAX];
	float level[FFT_BINS_MAX];
	float peak;
	float pgain;

	pthread_mutex_t fft_lock;

	uint32_t fft_bins;
	uint32_t* freq_band;
	uint32_t  freq_bins;

	bool disable_signals;
	bool update_annotations;
	bool update_grid;
	uint32_t width;
	uint32_t height;
	uint32_t m0_width;
	uint32_t m0_height;

	float log_rate;
	float log_base;

	int32_t drag_cutoff_x;
	float   drag_cutoff_db;
	bool prelight_cutoff;

	float c_fg[4];
	float c_bg[4];
	float scale;
	float pscale;

	const char *nfo;
} MF2UI;


static void reinitialize_fft(MF2UI* ui, uint32_t fft_size) {
	pthread_mutex_lock (&ui->fft_lock);
	fftx_free(ui->fa);
	fftx_free(ui->fb);

	fft_size = MIN(8192, MAX(64, fft_size));
	fft_size--;
	fft_size |= 0x3f;
	fft_size |= fft_size >> 2;
	fft_size |= fft_size >> 4;
	fft_size |= fft_size >> 8;
	fft_size |= fft_size >> 16;
	fft_size++;
	fft_size = MIN(FFT_BINS_MAX, fft_size);
	ui->fft_bins = fft_size;

	ui->fa = (struct FFTAnalysis*) malloc(sizeof(struct FFTAnalysis));
	ui->fb = (struct FFTAnalysis*) malloc(sizeof(struct FFTAnalysis));
	fftx_init(ui->fa, ui->fft_bins * 2, ui->rate, 25);
	fftx_init(ui->fb, ui->fft_bins * 2, ui->rate, 25);
	ui->log_rate  = (1.0f - 10000.0f / ui->rate) / ((2000.0f / ui->rate) * (2000.0f / ui->rate));
	ui->log_base = log10f(1.0f + ui->log_rate);
	ui->update_grid = true;

	for (uint32_t i = 0; i < ui->fft_bins; i++) {
		ui->phase[i] = 0;
		ui->level[i] = -100;
	}

	int band = 0;
	uint32_t bin = 0;
	const double f_r = 1000;
	const double b = ui->fft_bins < 128 ? 6 : 12;
	const double f2f = pow(2,  1. / (2. * b));

	assert(ui->fa->freq_per_bin < f_r);
	const int b_l = floorf(b * logf(ui->fa->freq_per_bin / f_r) / logf(2));
	const int b_u = ceilf(b * logf(.5 * ui->rate / f_r) / logf(2));
	ui->freq_bins = b_u - b_l - 1;

	free(ui->freq_band);
	ui->freq_band = (uint32_t*) malloc(ui->freq_bins * sizeof(uint32_t));

	for (uint32_t i = 0; i < ui->fft_bins; i++) {
		double f_m = pow(2, (band + b_l) / b) * f_r;
		double f_2 = f_m * f2f;
		if (f_2 > i * ui->fa->freq_per_bin) {
			continue;
		}
		while (f_2 < i * ui->fa->freq_per_bin) {
			band++;
			f_m = pow(2, (band + b_l) / b) * f_r;
			f_2 = f_m * f2f;
		}
		ui->freq_band[bin++] = i;
	}
	ui->freq_band[bin++] = ui->fft_bins;
	ui->freq_bins = bin;

	pthread_mutex_unlock (&ui->fft_lock);
}

/******************************************************************************
 * Communication with DSP backend -- send/receive settings
 */

/** notfiy backend that UI is closed */
static void ui_disable(LV2UI_Handle handle)
{
	MF2UI* ui = (MF2UI*)handle;

	uint8_t obj_buf[64];
	lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 64);
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_frame_time(&ui->forge, 0);
	LV2_Atom* msg = (LV2_Atom*)x_forge_object(&ui->forge, &frame, 1, ui->uris.ui_off);
	lv2_atom_forge_pop(&ui->forge, &frame);
	ui->write(ui->controller, 0, lv2_atom_total_size(msg), ui->uris.atom_eventTransfer, msg);
}

/** notify backend that UI is active:
 * request state and enable data-transmission */
static void ui_enable(LV2UI_Handle handle)
{
	MF2UI* ui = (MF2UI*)handle;
	uint8_t obj_buf[64];
	lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 64);
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_frame_time(&ui->forge, 0);
	LV2_Atom* msg = (LV2_Atom*)x_forge_object(&ui->forge, &frame, 1, ui->uris.ui_on);
	lv2_atom_forge_pop(&ui->forge, &frame);
	ui->write(ui->controller, 0, lv2_atom_total_size(msg), ui->uris.atom_eventTransfer, msg);
}


/******************************************************************************
 * Drawing
 */

static void hsl2rgb(float c[3], const float hue, const float sat, const float lum) {
	const float cq = lum < 0.5 ? lum * (1 + sat) : lum + sat - lum * sat;
	const float cp = 2.f * lum - cq;
	c[0] = rtk_hue2rgb(cp, cq, hue + 1.f/3.f);
	c[1] = rtk_hue2rgb(cp, cq, hue);
	c[2] = rtk_hue2rgb(cp, cq, hue - 1.f/3.f);
}

/** prepare drawing surfaces, render fixed background */
static void m0_create_surfaces(MF2UI* ui) {
	cairo_t* cr;
	const double ccc = floor (ui->width / 2.0) + .5;
	const double rad = (ui->width - XOFF) * .5;

	if (ui->sf_ann) cairo_surface_destroy(ui->sf_ann);
	ui->sf_ann = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ui->width, ui->height);
	cr = cairo_create (ui->sf_ann);
	cairo_rectangle (cr, 0, 0, ui->width, ui->height);
	CairoSetSouerceRGBA(ui->c_bg);
	cairo_fill (cr);
	cairo_destroy (cr);

	if (ui->sf_dat) cairo_surface_destroy(ui->sf_dat);
	ui->sf_dat = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ui->width, ui->height);
	cr = cairo_create (ui->sf_dat);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.0);
	cairo_rectangle (cr, 0, 0, ui->width, ui->height);
	cairo_fill (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
	cairo_arc (cr, ccc, ccc, rad, 0, 2.0 * M_PI);
	cairo_fill (cr);
	cairo_destroy (cr);
}

static void m1_create_surfaces(MF2UI* ui) {
	cairo_t* cr;
	ui->sf_pc[0] = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, PC_WIDTH, 16);
	cr = cairo_create (ui->sf_pc[0]);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0, .0, 0, .0);
	cairo_rectangle (cr, 0, 0, PC_WIDTH, 20);
	cairo_fill (cr);
	write_text_full(cr, "+1", ui->font[1], PC_WIDTH / 2, 10, 0, 2, c_ann);
	cairo_destroy (cr);

	ui->sf_pc[1] = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, PC_WIDTH, 16);
	cr = cairo_create (ui->sf_pc[1]);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, .0, 0, 0, .0);
	cairo_rectangle (cr, 0, 0, PC_WIDTH, 20);
	cairo_fill (cr);
	write_text_full(cr, "-1", ui->font[1], PC_WIDTH / 2, 10, 0, 2, c_ann);
	cairo_destroy (cr);
}

static void m2_create_surfaces(MF2UI* ui) {
	cairo_t* cr;
	ui->sf_gain = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ANN_W, 40);

#define AMPLABEL(V, O, T, X) \
	{ \
		const float ang = (-.75 * M_PI) + (1.5 * M_PI) * ((V) + (O)) / (T); \
		xlp = X + .5 + sinf (ang) * (10 + 3.0); \
		ylp = 16.5 + .5 - cosf (ang) * (10 + 3.0); \
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); \
		CairoSetSouerceRGBA(ui->c_fg); \
		cairo_set_line_width(cr, 1.5); \
		cairo_move_to(cr, rint(xlp)-.5, rint(ylp)-.5); \
		cairo_close_path(cr); \
		cairo_stroke(cr); \
		xlp = X + .5 + sinf (ang) * (10 + 9.5); \
		ylp = 16.5 + .5 - cosf (ang) * (10 + 9.5); \
	}

	ui->sf_dial = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 40);
	cr = cairo_create (ui->sf_dial);
	float xlp, ylp;
	AMPLABEL(-40, 40., 80., 30.5); write_text_full(cr, "-40", ui->font[0], xlp, ylp, 0, 2, ui->c_fg);
	AMPLABEL(-30, 40., 80., 30.5);
	AMPLABEL(-20, 40., 80., 30.5);
	AMPLABEL(-10, 40., 80., 30.5);
	AMPLABEL(  0, 40., 80., 30.5);
	AMPLABEL( 10, 40., 80., 30.5);
	AMPLABEL( 20, 40., 80., 30.5);
	AMPLABEL( 30, 40., 80., 30.5);
	AMPLABEL( 40, 40., 80., 30.5); write_text_full(cr, "+40", ui->font[0], xlp, ylp, 0, 2, ui->c_fg); \
	cairo_destroy (cr);
}

/** draw frequency calibration circles
 * and on screen annotations - sample-rate dependent
 */
static void update_grid(MF2UI* ui) {
	const double ccc = floor (ui->width / 2.0) + .5;
	const double rad = (ui->width - XOFF) * .5;
	cairo_t *cr = cairo_create (ui->sf_ann);

	cairo_rectangle (cr, 0, 0, ui->width, ui->height);
	CairoSetSouerceRGBA(ui->c_bg);
	cairo_fill (cr);

	cairo_set_line_width (cr, 1.0);
	cairo_arc (cr, ccc, ccc, rad, 0, 2.0 * M_PI);
	cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
	cairo_fill_preserve(cr);
	CairoSetSouerceRGBA(c_g90);
	cairo_stroke(cr);

	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

	double lw = round (10. * rad / (double)PH_RAD) / 10.;
	cairo_set_line_width (cr, lw);
	double dash1[2];
	dash1[0] = 1;
	dash1[1] = 2 * lw;

	cairo_set_dash(cr, dash1, 2, 0);

	CairoSetSouerceRGBA(c_grd);

	char fn[32];
	sprintf (fn, "Mono %.0fpx", round (9.f * rad / (double)PH_RAD));
	PangoFontDescription *fd = pango_font_description_from_string(fn);

	float freq = 62.5;
	while (freq < ui->rate / 2) {
		char txt[16];
		if (freq < 1000) {
			snprintf(txt, 16, "%d Hz", (int)ceil(freq));
		} else {
			snprintf(txt, 16, "%d kHz", (int)ceil(freq/1000.f));
		}

		{
			const float dr = ui->scale * PH_RAD * fast_log10(1.0 + 2 * freq * ui->log_rate / ui->rate) / ui->log_base;
			cairo_arc (cr, ccc, ccc, dr, 0, 2.0 * M_PI);
			cairo_stroke(cr);
			const float px = ccc + dr * sinf(M_PI * -.75);
			const float py = ccc - dr * cosf(M_PI * -.75);
			write_text_full(cr, txt, fd, px, py, M_PI * -.75, -2, c_ahz);
		}

		freq *= 2.0;
	}

	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);

	/* horiz line */
	cairo_set_line_width(cr, lw * 1.5);
	cairo_move_to(cr, ccc, ccc);
	cairo_line_to(cr, ccc - rad, ccc);
	cairo_move_to(cr, ccc, ccc);
	cairo_line_to(cr, ccc + rad, ccc);
	cairo_stroke(cr);

	/* vert thicker line */
	cairo_set_line_width(cr, lw * 2.5);
	cairo_move_to(cr, ccc, ccc);
	cairo_line_to(cr, ccc, ccc - rad);
	cairo_move_to(cr, ccc, ccc);
	cairo_line_to(cr, ccc, ccc + rad);
	cairo_stroke(cr);

	cairo_set_dash(cr, NULL, 0, 0);

	write_text_full(cr, "+L",  fd, ccc, ccc - rad * .92, 0, -2, c_ann);
	write_text_full(cr, "-L",  fd, ccc, ccc + rad * .92, 0, -2, c_ann);
	write_text_full(cr, "0\u00B0",  fd, ccc, ccc - rad * .80, 0, -2, c_ann);
	write_text_full(cr, "180\u00B0",  fd, ccc, ccc + rad * .80, 0, -2, c_ann);

	write_text_full(cr, "-R",  fd, ccc - rad * .92, ccc, 0, -2, c_ann);
	write_text_full(cr, "+R",  fd, ccc + rad * .92, ccc, 0, -2, c_ann);
	write_text_full(cr, "-90\u00B0",  fd, ccc - rad * .80, ccc, 0, -2, c_ann);
	write_text_full(cr, "+90\u00B0",  fd, ccc + rad * .80, ccc, 0, -2, c_ann);

	pango_font_description_free(fd);
	cairo_destroy (cr);
}

/** draw level-range display
 * depends on gain (dial) and cutoff
 */
static void update_annotations(MF2UI* ui) {
	cairo_t* cr = cairo_create (ui->sf_gain);

	cairo_rectangle (cr, 0, 0, ANN_W, 40);
	CairoSetSouerceRGBA(ui->c_bg);
	cairo_fill (cr);

	rounded_rectangle (cr, 3, 3 , ANN_W - 6, ANN_H - 6, 6);
	if (ui->drag_cutoff_x >= 0 || ui->prelight_cutoff) {
		cairo_set_source_rgba(cr, .15, .15, .15, 1.0);
	} else {
		cairo_set_source_rgba(cr, .0, .0, .0, 1.0);
	}
	cairo_fill (cr);

	cairo_set_line_width (cr, 1.0);
	const uint32_t mxw = ANN_W - XOFF * 2 - 36;
	const uint32_t mxo = XOFF + 18;
	const float cutoff = ui->db_cutoff;
	const uint32_t   cutoff_m = floor(mxw * (MIN_CUTOFF - cutoff) / MIN_CUTOFF);
	assert(cutoff_m < mxw);
	const uint32_t   cutoff_w = mxw - cutoff_m;

	for (uint32_t i=0; i < mxw; ++i) {
		float clr[3];
		if (i < cutoff_m) {
			clr[0] = clr[1] = clr[2] = .1;
		} else {
			const float pk = (i-cutoff_m) / (float)cutoff_w;
			hsl2rgb(clr, .68 - .72 * pk, .9, .2 + pk * .4);
		}

		cairo_set_source_rgba(cr, clr[0], clr[1], clr[2], 1.0);

		cairo_move_to(cr, mxo + i + .5, ANN_B - 5);
		cairo_line_to(cr, mxo + i + .5, ANN_B);
		cairo_stroke(cr);
	}

	cairo_set_source_rgba(cr, .8, .8, .8, .8);

	const float gain = robtk_dial_get_value(ui->gain);
	for (int32_t db = MIN_CUTOFF; db <=0 ; db+= 10) {
		char dbt[16];
		if (db == 0) {
			snprintf(dbt, 16, "\u2265%+.0fdB", (db - gain));
		} else {
			snprintf(dbt, 16, "%+.0fdB", (db - gain));
		}
		write_text_full(cr, dbt, ui->font[0], mxo + rint(mxw * (-MIN_CUTOFF + db) / -MIN_CUTOFF), ANN_B - 14 , 0, 2, c_wht);
		cairo_move_to(cr, mxo + rint(mxw * (-MIN_CUTOFF + db) / -MIN_CUTOFF) + .5, ANN_B - 7);
		cairo_line_to(cr, mxo + rint(mxw * (-MIN_CUTOFF + db) / -MIN_CUTOFF) + .5, ANN_B);
		cairo_stroke(cr);
	}

	/* black overlay above low-end cutoff */
	if (ui->db_cutoff > MIN_CUTOFF && (ui->drag_cutoff_x >= 0 || ui->prelight_cutoff)) {
		const float cox = rint(mxw * (ui->db_cutoff - MIN_CUTOFF)/ -MIN_CUTOFF);
		cairo_rectangle(cr, mxo, 6, cox, ANN_B - 6);
		cairo_set_source_rgba(cr, .0, .0, .0, .7);
		cairo_fill(cr);

		cairo_set_line_width (cr, 1.0);
		cairo_set_source_rgba(cr, .9, .5, .5, .6);
		cairo_move_to(cr, mxo + cox + .5, ANN_B - 6);
		cairo_line_to(cr, mxo + cox + .5, ANN_B + 1);
		cairo_stroke(cr);
	}

	cairo_destroy (cr);
}

/** draw a data-point
 *
 * @param pk level-peak, normalized 0..1 according to cutoff range + gain
 * @param dx,dy  X,Y cartesian position
 * @param ccc circle radius (optional, show spread if >0)
 * @param dist, phase angular vector corresponding to X,Y (optional spread)
 */
static inline void draw_point(MF2UI* ui, cairo_t *cr,
		const float pk,
		const float dx, const float dy,
		const float ccc, const float dist, float phase)
{
		float clr[3];
		hsl2rgb(clr, .68 - .72 * pk, .9, .2 + pk * .4);

		cairo_set_line_width (cr, PH_POINT);
		cairo_set_source_rgba(cr, clr[0], clr[1], clr[2], 0.3 + pk * .7);
		cairo_new_path (cr);
		cairo_move_to(cr, dx, dy);
		cairo_close_path(cr);
		if (ccc == 0) {
			cairo_stroke_preserve(cr);
			cairo_set_source_rgba(cr, clr[0], clr[1], clr[2], .1);
			cairo_set_line_width (cr, 2.f * PH_POINT + 1);
		}
		cairo_stroke(cr);

		if (ccc > 0 && dist > 0) {
			const float dev = .01256 * ccc / dist; // .004 * M_PI * ccc / dist
			cairo_set_line_width(cr, PH_POINT);
			cairo_set_source_rgba(cr, clr[0], clr[1], clr[2], .1);
			float pp = phase - .5 * M_PI;
			cairo_arc (cr, ccc, ccc, dist, (pp-dev), (pp+dev));
			cairo_stroke(cr);
		}
}

/* linear FFT data display */
static void plot_data_fft(MF2UI* ui) {
	cairo_t* cr;
	const double ccc = floor (ui->width / 2.0) + .5;
	const double rad = (ui->width - XOFF) * .5;
	const float gain = robtk_dial_get_value(ui->gain);
	const float persistence = robtk_dial_get_value(ui->screen);

	cr = cairo_create (ui->sf_dat);
	cairo_arc (cr, ccc, ccc, rad, 0, 2.0 * M_PI);
	cairo_clip_preserve (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	if (persistence > 0) {
		cairo_set_source_rgba(cr, 0, 0, 0, .3 - .003 * persistence);
	} else {
		cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
	}
	cairo_fill(cr);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	const float dnum = ui->scale * PH_RAD / ui->log_base;
	const float denom = ui->log_rate / (float)ui->fft_bins;
	const float cutoff = ui->db_cutoff;
	for (uint32_t i = 1; i < ui->fft_bins-1 ; ++i) {
		if (ui->level[i] < 0) continue;
		const float level = gain + fftx_power_to_dB(ui->level[i]);
		if (level < cutoff) continue;

		const float dist = dnum * fast_log10(1.0 + i * denom);
		const float dx = ccc + dist * sinf(ui->phase[i]);
		const float dy = ccc - dist * cosf(ui->phase[i]);
		const float pk = level > 0.0 ? 1.0 : (cutoff - level) / cutoff;

		draw_point(ui, cr, pk, dx, dy, ccc, dist, ui->phase[i]);
	}
	cairo_destroy (cr);
}

/* 1/Octave data display */
static void plot_data_oct(MF2UI* ui) {
	cairo_t* cr;
	const double ccc = floor (ui->width / 2.0) + .5;
	const double rad = (ui->width - XOFF) * .5;
	const float gain = robtk_dial_get_value(ui->gain);
	const float persistence = robtk_dial_get_value(ui->screen);

	cr = cairo_create (ui->sf_dat);
	cairo_arc (cr, ccc, ccc, rad, 0, 2.0 * M_PI);
	cairo_clip_preserve (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	if (persistence > 0) {
		cairo_set_source_rgba(cr, 0, 0, 0, .33 - .0033 * persistence);
	} else {
		cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
	}
	cairo_fill(cr);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

	const float dnum = ui->scale * PH_RAD / ui->log_base;
	const float denom = 2.0 * ui->log_rate / ui->rate;
	const float cutoff = ui->db_cutoff;

	uint32_t fi = 1;
	for (uint32_t i = 0; i < ui->freq_bins; ++i) {
		float ang_x = 0;
		float ang_y = 0;
		float a_level = 0;
		float a_freq = 0;
		uint32_t a_cnt = 0;

		while(fi < ui->freq_band[i]) {
			if (ui->level[fi] < 0) { fi++; continue; }
			a_freq += fi * ui->fa->freq_per_bin;
			a_level += ui->level[fi];
			ang_x += sinf(ui->phase[fi]);
			ang_y += cosf(ui->phase[fi]);
			a_cnt++;
			fi++;
		}
		if (a_cnt == 0) continue;
		a_level = gain + fftx_power_to_dB (a_level);
		if (a_level < cutoff) continue;

		a_freq /= (float)a_cnt;
		const float dist = dnum * fast_log10(1.0 + a_freq * denom);
		const float pk = a_level > 0.0 ? 1.0 : (cutoff - a_level) / cutoff;

		float dx, dy;
		if (a_cnt == 1) {
			dx = ccc + dist * ang_x;
			dy = ccc - dist * ang_y;
		} else {
			const float phase = atan2f(ang_x, ang_y);
			dx = ccc + dist * sinf(phase);
			dy = ccc - dist * cosf(phase);
		}

		draw_point(ui, cr, pk, dx, dy, 0, 0, 0);
	}

	cairo_destroy (cr);
}

/* main drawing callback */
static bool expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev) {
	MF2UI* ui = (MF2UI*)GET_HANDLE(handle);

	if (ui->update_grid) {
		ui->width  = floor(ui->scale * 2 * (PH_RAD + XOFF));
		ui->height = floor(ui->scale * 2 * (PH_RAD + YOFF));

		m0_create_surfaces(ui);
		update_grid(ui);
		ui->update_grid = false;
	}

	cairo_translate(cr,
			floor((ui->m0_width - ui->width) * .5),
			floor((ui->m0_height - ui->height) * .5));

	if (pthread_mutex_trylock (&ui->fft_lock) == 0 ) {
		if (robtk_cbtn_get_active(ui->btn_oct)) {
			plot_data_oct(ui);
		} else {
			plot_data_fft(ui);
		}
		pthread_mutex_unlock (&ui->fft_lock);
	}

	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	cairo_set_source_surface(cr, ui->sf_ann, 0, 0);
	cairo_paint (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
	cairo_set_source_surface(cr, ui->sf_dat, 0, 0);
	cairo_paint (cr);

	return TRUE;
}

/* level range scale */
static bool ga_expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev) {
	MF2UI* ui = (MF2UI*)GET_HANDLE(handle);

	if (ui->update_annotations) {
		update_annotations(ui);
		ui->update_annotations = false;
	}

	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	cairo_set_source_surface(cr, ui->sf_gain, 0, 0);
	cairo_paint (cr);

	return TRUE;
}

/* stereo-phase correlation display */
static bool pc_expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev) {
	MF2UI* ui = (MF2UI*)GET_HANDLE(handle);

	if (!ui->sf_nfo && ui->nfo) {
		PangoFontDescription *fd = pango_font_description_from_string("Sans 10px");
		create_text_surface2(&ui->sf_nfo,
				12, PC_BOUNDH,
				0, PC_TOP,
				ui->nfo, fd, M_PI * -.5, 7, c_g60);
			pango_font_description_free(fd);
	}

	cairo_save(cr);
	cairo_translate(cr, 0, rint((ui->m0_height - ui->height) * .5));

	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	/* display phase-correlation */
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	/* PC meter backgroud */
	CairoSetSouerceRGBA(ui->c_bg);
	cairo_rectangle (cr, 0, 0, PC_BOUNDW, PC_BOUNDH);
	cairo_fill(cr);

	CairoSetSouerceRGBA(c_blk);
	cairo_set_line_width(cr, 1.0);
	rounded_rectangle (cr, PC_LEFT-1, PC_TOP + 1.0, PC_WIDTH+2, PC_HEIGHT - 2.0, 3);
	cairo_fill_preserve(cr);
	cairo_save(cr);
	cairo_clip(cr);

	/* value */
	CairoSetSouerceRGBA(c_glb);
	const float c = rintf(PC_TOP + PC_BLOCKSIZE * ui->cor);
	rounded_rectangle (cr, PC_LEFT, c, PC_WIDTH, PC_BLOCK, 4);
	cairo_fill(cr);

	/* labels w/ background */
	cairo_set_source_surface(cr, ui->sf_pc[0], PC_LEFT, PC_TOP + 5);
	cairo_paint (cr);
	cairo_set_source_surface(cr, ui->sf_pc[1], PC_LEFT, PC_TOP + PC_HEIGHT - 25);
	cairo_paint (cr);

	cairo_restore(cr);

	rounded_rectangle (cr, PC_LEFT - .5, PC_TOP + .5, PC_WIDTH + 1, PC_HEIGHT - 1, 3);
	CairoSetSouerceRGBA(c_g90);
	cairo_stroke(cr);

	/* annotations */
	cairo_set_operator (cr, CAIRO_OPERATOR_SCREEN);
	CairoSetSouerceRGBA(c_grd);
	cairo_set_line_width(cr, 1.0);

#define PC_ANNOTATION(YPOS, OFF) \
	cairo_move_to(cr, PC_LEFT + OFF, rintf(PC_TOP + YPOS) + 0.5); \
	cairo_line_to(cr, PC_LEFT + PC_WIDTH - OFF, rintf(PC_TOP + YPOS) + 0.5);\
	cairo_stroke(cr);

	PC_ANNOTATION(PC_HEIGHT * 0.1, 4.0);
	PC_ANNOTATION(PC_HEIGHT * 0.2, 4.0);
	PC_ANNOTATION(PC_HEIGHT * 0.3, 4.0);
	PC_ANNOTATION(PC_HEIGHT * 0.4, 4.0);
	PC_ANNOTATION(PC_HEIGHT * 0.6, 4.0);
	PC_ANNOTATION(PC_HEIGHT * 0.7, 4.0);
	PC_ANNOTATION(PC_HEIGHT * 0.8, 4.0);
	PC_ANNOTATION(PC_HEIGHT * 0.9, 4.0);

	CairoSetSouerceRGBA(c_glr);
	cairo_set_line_width(cr, 1.5);
	PC_ANNOTATION(PC_HEIGHT * 0.5, 1.5);

	cairo_restore(cr);
	if (ui->sf_nfo) {
		cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
		cairo_clip (cr);
		cairo_set_source_surface(cr, ui->sf_nfo, PC_BOUNDW - 14, 0);
		cairo_paint (cr);
	}

	return TRUE;
}


/******************************************************************************
 * UI callbacks  - Dial
 */

static bool cb_set_gain (RobWidget* handle, void *data) {
	MF2UI* ui = (MF2UI*) (data);

	const float val = robtk_dial_get_value(ui->gain);
	if (rintf(ui->pgain) != rintf(val)) {
		ui->pgain = val;
		ui->update_annotations = true;
		queue_draw(ui->m2);
	}
#ifdef __USE_GNU
	const float thresh = exp10f(.05 * (MIN_CUTOFF - val));
#else
	const float thresh = powf(10, .05 * (MIN_CUTOFF - val));
#endif
	ui->db_thresh = thresh * thresh;
	if (ui->disable_signals) return TRUE;
	if (robtk_cbtn_get_active(ui->btn_norm)) return TRUE;
	ui->write(ui->controller, MF_GAIN, sizeof(float), 0, (const void*) &val);
	return TRUE;
}

static void annotation_txt(MF2UI *ui, RobTkDial * d, cairo_t *cr, const char *txt) {
	int tw, th;
	cairo_save(cr);
	PangoLayout * pl = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(pl, ui->font[1]);
	pango_layout_set_text(pl, txt, -1);
	pango_layout_get_pixel_size(pl, &tw, &th);
	cairo_translate (cr, d->w_cx, d->w_height);
	cairo_translate (cr, -tw/2.0 - 0.5, -th);
	cairo_set_source_rgba (cr, .0, .0, .0, .7);
	rounded_rectangle(cr, -1, -1, tw+3, th+1, 3);
	cairo_fill(cr);
	CairoSetSouerceRGBA(c_wht);
	pango_cairo_layout_path(cr, pl);
	cairo_fill(cr);
	g_object_unref(pl);
	cairo_restore(cr);
	cairo_new_path(cr);
}

static void dial_annotation_db(RobTkDial * d, cairo_t *cr, void *data) {
	MF2UI* ui = (MF2UI*) (data);
	char tmp[16];
	snprintf(tmp, 16, "%+4.1fdB", d->cur);
	annotation_txt(ui, d, cr, tmp);
}

/******************************************************************************
 * UI callbacks - Level Range Widget
 */

static RobWidget* m2_mousedown(RobWidget* handle, RobTkBtnEvent *event) {
	MF2UI* ui = (MF2UI*)GET_HANDLE(handle);
	if (event->state & ROBTK_MOD_SHIFT) {
		ui->db_cutoff = -45;
		ui->update_annotations = true;
		queue_draw(ui->m2);
		return NULL;
	}

	ui->drag_cutoff_db = ui->db_cutoff;
	ui->drag_cutoff_x = event->x;

	ui->update_annotations = true;
	queue_draw(ui->m2);

	return handle;
}

static RobWidget* m2_mouseup(RobWidget* handle, RobTkBtnEvent *event) {
	MF2UI* ui = (MF2UI*)GET_HANDLE(handle);
	ui->drag_cutoff_x = -1;
	ui->update_annotations = true;
	queue_draw(ui->m2);
	return NULL;
}

static RobWidget* m2_mousemove(RobWidget* handle, RobTkBtnEvent *event) {
	MF2UI* ui = (MF2UI*)GET_HANDLE(handle);
	if (ui->drag_cutoff_x < 0) return NULL;
	const float mxw = -MIN_CUTOFF / (float) (ANN_W - XOFF * 2 - 36);
	const float diff = (event->x - ui->drag_cutoff_x) * mxw;
	float cutoff = ui->drag_cutoff_db + diff;
	if (cutoff <= MIN_CUTOFF) cutoff = MIN_CUTOFF;
	if (cutoff >= -10) cutoff = -10;
	if (ui->db_cutoff != cutoff) {
		ui->db_cutoff = cutoff;
		ui->update_annotations = true;
		queue_draw(ui->m2);
		ui->write(ui->controller, MF_CUTOFF, sizeof(float), 0, (const void*) &cutoff);
	}
	return handle;
}

static void m2_enter(RobWidget *handle) {
	MF2UI* ui = (MF2UI*)GET_HANDLE(handle);
	if (!ui->prelight_cutoff) {
		ui->prelight_cutoff = true;
		ui->update_annotations = true;
		queue_draw(ui->m2);
	}
}

static void m2_leave(RobWidget *handle) {
	MF2UI* ui = (MF2UI*)GET_HANDLE(handle);
	if (ui->prelight_cutoff) {
		ui->prelight_cutoff = false;
		ui->update_annotations = true;
		queue_draw(ui->m2);
	}
}

/******************************************************************************
 * UI callbacks  - FFT Bins and buttons
 */

static bool cb_set_fft (RobWidget* handle, void *data) {
	MF2UI* ui = (MF2UI*) (data);
	const float fft_size = 2 * robtk_select_get_value(ui->sel_fft);
	uint32_t fft_bins = floorf(fft_size / 2.0);
	if (ui->fft_bins == fft_bins) return TRUE;
	reinitialize_fft(ui, fft_bins);
	ui->write(ui->controller, MF_FFT, sizeof(float), 0, (const void*) &fft_size);
	return TRUE;
}

static bool cb_set_oct (RobWidget* handle, void *data) {
	MF2UI* ui = (MF2UI*) (data);
	if (ui->disable_signals) return TRUE;
	float val = robtk_cbtn_get_active(ui->btn_oct) ? 1.0 : 0.0;
	ui->write(ui->controller, MF_BAND, sizeof(float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_set_norm (RobWidget* handle, void *data) {
	MF2UI* ui = (MF2UI*) (data);
	float val = robtk_cbtn_get_active(ui->btn_norm) ? 1.0 : 0.0;
	robtk_dial_set_sensitive(ui->gain, val == 0.0);
	if (ui->disable_signals) return TRUE;
	ui->write(ui->controller, MF_NORM, sizeof(float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_set_persistence (RobWidget* handle, void *data) {
	MF2UI* ui = (MF2UI*) (data);
	const float val = robtk_dial_get_value(ui->screen);
	if (ui->disable_signals) return TRUE;
	ui->write(ui->controller, MF_SCREEN, sizeof(float), 0, (const void*) &val);
	return TRUE;
}

/******************************************************************************
 * widget hackery
 */

static enum LVGLResize
plugin_scale_mode(LV2UI_Handle handle)
{
	return LVGL_LAYOUT_TO_FIT;
}

static void
size_request(RobWidget* handle, int *w, int *h) {
	*w = 2 * (PH_RAD + XOFF);
	*h = 2 * (PH_RAD + YOFF);
}

static void
m0_set_scaling(RobWidget* rw, int w, int h) {
	MF2UI* ui = (MF2UI*)GET_HANDLE(rw);
	const float dflw = 2 * (PH_RAD + YOFF);
	const float dflh = 2 * (PH_RAD + YOFF);
	float scale = MIN(w/dflw, h/dflh);
	if (scale != ui->scale || (int)ui->m0_width != h || (int)ui->m0_height != h) {
		ui->m0_width = w;
		ui->m0_height = h;
		ui->scale  = scale;
		ui->pscale = sqrtf(scale);
		ui->update_grid = true;
	}
	queue_draw(rw);
}

static void
m0_size_allocate(RobWidget* rw, int w, int h) {
	m0_set_scaling(rw, w, h);
	robwidget_set_size(rw, w, h);
}

static void
pc_size_allocate(RobWidget* rw, int w, int h) {
	robwidget_set_size(rw, PC_BOUNDW, h);
	queue_draw(rw);
}

static void
pc_size_request(RobWidget* handle, int *w, int *h) {
	*w = PC_BOUNDW;
	*h = 2 * (PH_RAD + YOFF);
}

static void
ga_size_request(RobWidget* handle, int *w, int *h) {
	*w = ANN_W;
	*h = ANN_H;
}

/******************************************************************************
 * top-level widget layout and instantiation
 */

static RobWidget * toplevel(MF2UI* ui, void * const top)
{
	/* main widget: layout */
	ui->rw = rob_vbox_new(FALSE, 0);
	robwidget_make_toplevel(ui->rw, top);

	ui->hbox1 = rob_hbox_new(FALSE, 0);
	ui->hbox2 = rob_hbox_new(FALSE, 0);
	ui->hbox3 = rob_hbox_new(FALSE, 0);
	ui->sep2 = robtk_sep_new(true);

	rob_vbox_child_pack(ui->rw, ui->hbox1, TRUE, TRUE);
	rob_vbox_child_pack(ui->rw, ui->hbox2, FALSE, TRUE);
	rob_vbox_child_pack(ui->rw, robtk_sep_widget(ui->sep2), FALSE, TRUE);
	rob_vbox_child_pack(ui->rw, ui->hbox3, FALSE, TRUE);

	ui->font[0] = pango_font_description_from_string("Mono 9px");
	ui->font[1] = pango_font_description_from_string("Mono 10px");
	get_color_from_theme(0, ui->c_fg);
	get_color_from_theme(1, ui->c_bg);
	m1_create_surfaces(ui);
	m2_create_surfaces(ui);

	/* main drawing area */
	ui->m0 = robwidget_new(ui);
	ROBWIDGET_SETNAME(ui->m0, "mphase (m0)");
	robwidget_set_expose_event(ui->m0, expose_event);
	robwidget_set_size_request(ui->m0, size_request);
	robwidget_set_size_allocate(ui->m0, m0_size_allocate);
	rob_hbox_child_pack(ui->hbox1, ui->m0, TRUE, TRUE);

	/* phase correlation */
	ui->m1 = robwidget_new(ui);
	ROBWIDGET_SETNAME(ui->m1, "phase (m1)");
	robwidget_set_expose_event(ui->m1, pc_expose_event);
	robwidget_set_size_request(ui->m1, pc_size_request);
	robwidget_set_size_allocate(ui->m1, pc_size_allocate);
	rob_hbox_child_pack(ui->hbox1, ui->m1, FALSE, TRUE);

	/* gain box */
	ui->sep3 = robtk_sep_new(true);
	ui->sep4 = robtk_sep_new(true);
	robtk_sep_set_linewidth(ui->sep3, 0);
	robtk_sep_set_linewidth(ui->sep4, 0);

	/* gain annotation */
	ui->m2 = robwidget_new(ui);
	ROBWIDGET_SETNAME(ui->m2, "gain (m2)");
	robwidget_set_expose_event(ui->m2, ga_expose_event);
	robwidget_set_size_request(ui->m2, ga_size_request);

	rob_hbox_child_pack(ui->hbox2, robtk_sep_widget(ui->sep3), TRUE, TRUE);
	rob_hbox_child_pack(ui->hbox2, ui->m2, FALSE, FALSE);
	rob_hbox_child_pack(ui->hbox2, robtk_sep_widget(ui->sep4), TRUE, TRUE);

	robwidget_set_mousedown(ui->m2, m2_mousedown);
	robwidget_set_mouseup(ui->m2, m2_mouseup);
	robwidget_set_mousemove(ui->m2, m2_mousemove);
	robwidget_set_enter_notify(ui->m2, m2_enter);
	robwidget_set_leave_notify(ui->m2, m2_leave);

	/* gain dial */
	ui->gain = robtk_dial_new_with_size(-40.0, 40.0, .01,
			60, 40, 30.5, 16.5, 10);
	robtk_dial_set_alignment(ui->gain, 1.0, 0.5);
	robtk_dial_set_value(ui->gain, 0);
	robtk_dial_set_default(ui->gain, 20.0);
	robtk_dial_set_callback(ui->gain, cb_set_gain, ui);
	robtk_dial_set_surface(ui->gain,ui->sf_dial);
	robtk_dial_annotation_callback(ui->gain, dial_annotation_db, ui);
	rob_hbox_child_pack(ui->hbox2, robtk_dial_widget(ui->gain), FALSE, FALSE);

	/* fft bins */
	ui->lbl_fft = robtk_lbl_new("FFT:");
	ui->sel_fft = robtk_select_new();
	robtk_select_add_item(ui->sel_fft,   64, "128");
	robtk_select_add_item(ui->sel_fft,  128, "256");
	robtk_select_add_item(ui->sel_fft,  256, "512");
	robtk_select_add_item(ui->sel_fft,  512, "1024");
	robtk_select_add_item(ui->sel_fft, 1024, "2048");
	robtk_select_add_item(ui->sel_fft, 2048, "4096");
	robtk_select_add_item(ui->sel_fft, 4096, "8192");
	robtk_select_add_item(ui->sel_fft, 6144, "12288");
	robtk_select_add_item(ui->sel_fft, 8192, "16384");
	robtk_select_set_default_item(ui->sel_fft, 3);
	robtk_select_set_value(ui->sel_fft, 512);
	robtk_select_set_callback(ui->sel_fft, cb_set_fft, ui);

	/* N/octave */
	ui->btn_oct = robtk_cbtn_new("N/Octave Bands", GBT_LED_LEFT, false);
	robtk_cbtn_set_active(ui->btn_oct, false);
	robtk_cbtn_set_callback(ui->btn_oct, cb_set_oct, ui);

	robtk_cbtn_set_color_on(ui->btn_oct,  .2, .8, .1);
	robtk_cbtn_set_color_off(ui->btn_oct, .1, .3, .1);

	/* Normalize */
	ui->btn_norm = robtk_cbtn_new("Normalize", GBT_LED_LEFT, false);
	robtk_cbtn_set_active(ui->btn_norm, false);
	robtk_cbtn_set_callback(ui->btn_norm, cb_set_norm, ui);

	robtk_cbtn_set_color_on(ui->btn_norm,  .2, .8, .1);
	robtk_cbtn_set_color_off(ui->btn_norm, .1, .3, .1);

	/* screen persistence dial */
	ui->lbl_screen = robtk_lbl_new("Persistence:");
	ui->screen = robtk_dial_new_with_size(0.0, 100.0, 1,
			22, 22, 10.5, 10.5, 10);
	robtk_dial_set_alignment(ui->screen, 1.0, 0.5);
	robtk_dial_set_value(ui->screen, 33);
	robtk_dial_set_default(ui->screen, 33.0);
	robtk_dial_set_callback(ui->screen, cb_set_persistence, ui);

	/* explicit alignment */
	ui->sep0 = robtk_sep_new(true);
	robtk_sep_set_linewidth(ui->sep0, 0);
	ui->sep1 = robtk_sep_new(true);
	robtk_sep_set_linewidth(ui->sep1, 0);

	rob_hbox_child_pack(ui->hbox3, robtk_lbl_widget(ui->lbl_screen), FALSE, FALSE);
	rob_hbox_child_pack(ui->hbox3, robtk_dial_widget(ui->screen), FALSE, FALSE);
	rob_hbox_child_pack(ui->hbox3, robtk_lbl_widget(ui->lbl_fft), FALSE, FALSE);
	rob_hbox_child_pack(ui->hbox3, robtk_select_widget(ui->sel_fft), FALSE, FALSE);
	rob_hbox_child_pack(ui->hbox3, robtk_sep_widget(ui->sep0), TRUE, FALSE);
	rob_hbox_child_pack(ui->hbox3, robtk_cbtn_widget(ui->btn_oct), FALSE, FALSE);
	rob_hbox_child_pack(ui->hbox3, robtk_sep_widget(ui->sep1), TRUE, FALSE);
	rob_hbox_child_pack(ui->hbox3, robtk_cbtn_widget(ui->btn_norm), FALSE, FALSE);

	update_annotations(ui);
	return ui->rw;
}

/******************************************************************************
 * LV2 callbacks
 */

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
	MF2UI* ui = (MF2UI*) calloc(1,sizeof(MF2UI));
	*widget = NULL;
	ui->map = NULL;

	if (!strcmp(plugin_uri, MTR_URI "phasewheel")) { ; }
	else {
		free(ui);
		return NULL;
	}

	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
			ui->map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!ui->map) {
		fprintf(stderr, "meters.lv2 UI: Host does not support urid:map\n");
		free(ui);
		return NULL;
	}

	ui->nfo = robtk_info(ui_toplevel);
	map_xfer_uris(ui->map, &ui->uris);
	lv2_atom_forge_init(&ui->forge, ui->map);

	ui->write      = write_function;
	ui->controller = controller;

	ui->rate = 48000;
	ui->db_cutoff = -60;
	ui->db_thresh = 0.000001; // (-60dB)^2 // XXX
	ui->drag_cutoff_x = -1;
	ui->prelight_cutoff = false;
	ui->cor = 0.5;
	ui->disable_signals = false;
	ui->update_annotations = false;
	ui->update_grid = false;
	ui->fft_bins = 512;
	ui->freq_band = NULL;
	ui->freq_bins = 0;
	ui->pgain = -100;
	ui->peak = 0;
	ui->scale = 0.0;
	ui->pscale = 1.0;

	ui->width  = 2 * (PH_RAD + XOFF);
	ui->height = 2 * (PH_RAD + YOFF);

	pthread_mutex_init(&ui->fft_lock, NULL);
	*widget = toplevel(ui, ui_toplevel);
	reinitialize_fft(ui, ui->fft_bins);
	ui_enable(ui);
	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	MF2UI* ui = (MF2UI*)handle;

	ui_disable(ui);

	pango_font_description_free(ui->font[0]);
	pango_font_description_free(ui->font[1]);

	cairo_surface_destroy(ui->sf_nfo);
	cairo_surface_destroy(ui->sf_ann);
	cairo_surface_destroy(ui->sf_dat);
	cairo_surface_destroy(ui->sf_gain);
	cairo_surface_destroy(ui->sf_dial);
	cairo_surface_destroy(ui->sf_pc[0]);
	cairo_surface_destroy(ui->sf_pc[1]);

	robtk_select_destroy(ui->sel_fft);
	robtk_lbl_destroy(ui->lbl_fft);
	robtk_lbl_destroy(ui->lbl_screen);
	robtk_sep_destroy(ui->sep0);
	robtk_sep_destroy(ui->sep1);
	robtk_sep_destroy(ui->sep2);
	robtk_sep_destroy(ui->sep3);
	robtk_sep_destroy(ui->sep4);
	robtk_dial_destroy(ui->gain);
	robtk_dial_destroy(ui->screen);
	robtk_cbtn_destroy(ui->btn_oct);
	robtk_cbtn_destroy(ui->btn_norm);

	robwidget_destroy(ui->m0);
	robwidget_destroy(ui->m1);
	robwidget_destroy(ui->m2);

	rob_box_destroy(ui->hbox1);
	rob_box_destroy(ui->hbox2);
	rob_box_destroy(ui->hbox3);
	rob_box_destroy(ui->rw);

	fftx_free(ui->fa);
	fftx_free(ui->fb);
	free(ui->freq_band);

	pthread_mutex_destroy(&ui->fft_lock);

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

static void invalidate_pc(MF2UI* ui, const float val) {
	float c0, c1;
	if (rint(PC_BLOCKSIZE * ui->cor * 2) == rint (PC_BLOCKSIZE * val * 2)) return;
	c0 = PC_BLOCKSIZE * MIN(ui->cor, val);
	c1 = PC_BLOCKSIZE * MAX(ui->cor, val);
	ui->cor = val;
	queue_tiny_area(ui->m1, PC_LEFT, floorf(PC_TOP + c0), PC_WIDTH, ceilf(PC_BLOCK + 1 + c1 - c0));
}

/******************************************************************************/

static void process_audio(MF2UI* ui, const size_t n_elem, float const * const left, float const * const right) {
	pthread_mutex_lock(&ui->fft_lock);

	fftx_run(ui->fa, n_elem, left);
	bool display = !fftx_run(ui->fb, n_elem, right);

	if (display) {
		assert (fftx_bins(ui->fa) == ui->fft_bins);
		float peak = 0;
		const float db_thresh = ui->db_thresh;
		for (uint32_t i = 1; i < ui->fft_bins-1; i++) {
			if (ui->fa->power[i] < db_thresh || ui->fb->power[i] < db_thresh) {
				ui->phase[i] = 0;
				ui->level[i] = -100;
				continue;
			}
			const float phase0 = ui->fa->phase[i];
			const float phase1 = ui->fb->phase[i];
			float phase = phase1 - phase0;
			ui->phase[i] = phase;
			ui->level[i] = MAX(ui->fa->power[i], ui->fb->power[i]);
			if (ui->level[i] > peak) {
				peak = ui->level[i];
			}
		}

		ui->peak += .04 * (peak - ui->peak) + 1e-15;
		if (isnan (ui->peak)) { ui->peak = 0; }
		if (ui->peak > 1000) { ui->peak = 1000; }
		if (robtk_cbtn_get_active(ui->btn_norm)) {
			robtk_dial_set_value(ui->gain, - fftx_power_to_dB(ui->peak));
		}
		queue_draw(ui->m0);
	}
	pthread_mutex_unlock(&ui->fft_lock);
}

/******************************************************************************/

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
	MF2UI* ui = (MF2UI*)handle;
	LV2_Atom* atom = (LV2_Atom*)buffer;
	if (format == ui->uris.atom_eventTransfer
			&& (atom->type == ui->uris.atom_Blank|| atom->type == ui->uris.atom_Object))
	{
		/* cast the buffer to Atom Object */
		LV2_Atom_Object* obj = (LV2_Atom_Object*)atom;
		LV2_Atom *a0 = NULL;
		LV2_Atom *a1 = NULL;
		if (obj->body.otype == ui->uris.rawstereo
				&& 2 == lv2_atom_object_get(obj, ui->uris.audioleft, &a0, ui->uris.audioright, &a1, NULL)
				&& a0 && a1
				&& a0->type == ui->uris.atom_Vector
				&& a1->type == ui->uris.atom_Vector
			 )
		{
			LV2_Atom_Vector* left = (LV2_Atom_Vector*)LV2_ATOM_BODY(a0);
			LV2_Atom_Vector* right = (LV2_Atom_Vector*)LV2_ATOM_BODY(a1);
			if (left->atom.type == ui->uris.atom_Float && right->atom.type == ui->uris.atom_Float) {
				const size_t n_elem = (a0->size - sizeof(LV2_Atom_Vector_Body)) / left->atom.size;
				const float *l = (float*) LV2_ATOM_BODY(&left->atom);
				const float *r = (float*) LV2_ATOM_BODY(&right->atom);
				process_audio(ui, n_elem, l, r);
			}
		}
		else if (
				/* handle 'state/settings' data object */
				obj->body.otype == ui->uris.ui_state
				/* retrieve properties from object and
				 * check that there the [here] three required properties are set.. */
				&& 1 == lv2_atom_object_get(obj,
					ui->uris.samplerate, &a0, NULL)
				/* ..and non-null.. */
				&& a0
				/* ..and match the expected type */
				&& a0->type == ui->uris.atom_Float
				)
		{
			ui->rate = ((LV2_Atom_Float*)a0)->body;
			reinitialize_fft(ui, ui->fft_bins);
		}
	}
	else if (format != 0) return;

	if (port_index == MF_PHASE) {
		invalidate_pc(ui, 0.5f * (1.0f - *(float *)buffer));
	}
	else if (port_index == MF_GAIN) {
		ui->disable_signals = true;
		robtk_dial_set_value(ui->gain, *(float *)buffer);
		ui->disable_signals = false;
	}
	else if (port_index == MF_CUTOFF) {
		float val = *(float *)buffer;
		if (ui->drag_cutoff_x < 0 && val >= MIN_CUTOFF && val <= -10) {
			ui->db_cutoff = val;
			ui->update_annotations = true;
			queue_draw(ui->m2);
		}
	}
	else if (port_index == MF_FFT) {
		float val = *(float *)buffer;
		uint32_t fft_bins = floorf(val / 2.0);
		if (ui->fft_bins != fft_bins) {
			reinitialize_fft(ui, fft_bins);
			robtk_select_set_value(ui->sel_fft, ui->fft_bins);
		}
	}
	else if (port_index == MF_BAND) {
		float val = *(float *)buffer;
		ui->disable_signals = true;
		robtk_cbtn_set_active(ui->btn_oct, val != 0);
		ui->disable_signals = false;
	}
	else if (port_index == MF_NORM) {
		float val = *(float *)buffer;
		ui->disable_signals = true;
		robtk_cbtn_set_active(ui->btn_norm, val != 0);
		ui->disable_signals = false;
	}
	else if (port_index == MF_SCREEN) {
		ui->disable_signals = true;
		robtk_dial_set_value(ui->screen, *(float *)buffer);
		ui->disable_signals = false;
	}
}
