/* Stereoscope -- Stereo-width / frequency
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
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

#define RTK_URI "http://gareus.org/oss/lv2/meters#"
#define RTK_GUI "stereoscopeui"

#define FFT_BINS_MAX 8192 // half of the FFT data-size

enum {
	SS_FFT = 6,
	SS_BAND,
	SS_SCREEN
};

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


/* various GUI pixel sizes */

#define SS_BORDER 5
#define SS_SIZE 360

#define MAKEUP_GAIN 6

#ifdef _WIN32
#define snprintf(s, l, ...) sprintf(s, __VA_ARGS__)
#endif

static const float c_ann[4] = {0.5, 0.5, 0.5, 1.0}; // text annotation color
static const float c_ahz[4] = {0.6, 0.6, 0.6, 0.5}; // frequency annotation
static const float c_grd[4] = {0.4, 0.4, 0.4, 1.0}; // grid color


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

	RobWidget* hbox1;
	RobWidget* hbox2;

	RobTkCBtn* btn_oct;
	RobTkSelect* sel_fft;
	RobTkDial* screen;
	RobTkLbl* lbl_fft;
	RobTkLbl* lbl_screen;
	RobTkSep* sep0;
	RobTkSep* sep2;

	cairo_surface_t* sf_dat;
	cairo_surface_t* sf_ann;

	PangoFontDescription *font[2];

	float lr[FFT_BINS_MAX];
	float level[FFT_BINS_MAX];

	pthread_mutex_t fft_lock;

	uint32_t fft_bins;
	uint32_t* freq_band;
	uint32_t  freq_bins;

	bool disable_signals;
	bool update_grid;
	bool clear_persistence;
	uint32_t width;
	uint32_t height;

	float log_rate;
	float log_base;

	float c_fg[4];
	float c_bg[4];
} SFSUI;


static void reinitialize_fft(SFSUI* ui, uint32_t fft_size) {
	pthread_mutex_lock (&ui->fft_lock);
	fftx_free(ui->fa);
	fftx_free(ui->fb);

	fft_size = MIN(8192, MAX(128, fft_size));
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
	ui->log_rate  = (1.0f - 10000.0f / ui->rate) / ((5000.0f / ui->rate) * (5000.0f / ui->rate));
	ui->log_base = log10f(1.0f + ui->log_rate);
	ui->update_grid = true;
	ui->clear_persistence = true;

	for (uint32_t i = 0; i < ui->fft_bins; i++) {
		ui->lr[i] = 0.5;
		ui->level[i] = -100;
	}

	int band = 0;
	uint32_t bin = 0;
	const double f_r = 1000;
	const double b = 12;
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
	SFSUI* ui = (SFSUI*)handle;

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
	SFSUI* ui = (SFSUI*)handle;
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

static void create_surfaces(SFSUI* ui) {
	cairo_t* cr;
	ui->sf_ann = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ui->width, ui->height);
	cr = cairo_create (ui->sf_ann);
	cairo_rectangle (cr, 0, 0, ui->width, ui->height);
	CairoSetSouerceRGBA(ui->c_bg);
	cairo_fill (cr);
	cairo_destroy (cr);

	ui->sf_dat = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ui->width, ui->height);
	cr = cairo_create (ui->sf_dat);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.0);
	cairo_rectangle (cr, 0, 0, ui->width, ui->height);
	cairo_fill (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
	rounded_rectangle (cr, SS_BORDER, SS_BORDER, SS_SIZE, SS_SIZE, SS_BORDER);
	cairo_fill (cr);
	cairo_destroy (cr);
}

static void update_grid(SFSUI* ui) {
	cairo_t *cr = cairo_create (ui->sf_ann);

	cairo_rectangle (cr, 0, 0, ui->width, ui->height);
	CairoSetSouerceRGBA(ui->c_bg);
	cairo_fill (cr);

	cairo_set_line_width (cr, 1.0);

	rounded_rectangle (cr, SS_BORDER, SS_BORDER, SS_SIZE, SS_SIZE, SS_BORDER);
	cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
	cairo_fill(cr);
	rounded_rectangle (cr, SS_BORDER-.5, SS_BORDER-.5, SS_SIZE+1, SS_SIZE+1, SS_BORDER);
	CairoSetSouerceRGBA(c_g90);
	cairo_stroke(cr);

	const double dash1[] = {1.0, 2.0};
	cairo_set_dash(cr, dash1, 2, 0);

	CairoSetSouerceRGBA(c_grd);

#define FREQ_ANN(FRQ, TXT) { \
	const float py = rintf(SS_BORDER + SS_SIZE * (1.0 - fast_log10(1.0 + 2 * FRQ * ui->log_rate / ui->rate) / ui->log_base)) + .5; \
	cairo_move_to(cr, SS_BORDER, py); \
	cairo_line_to(cr, SS_BORDER + SS_SIZE, py); \
	cairo_stroke(cr); \
	write_text_full(cr, TXT, ui->font[0], SS_SIZE, py, 0, -1, c_ahz); \
	}

	float freq = 62.5;
	while (freq < ui->rate / 2) {
		char txt[16];
		if (freq < 1000) {
			snprintf(txt, 16, "%d Hz", (int)ceil(freq));
		} else {
			snprintf(txt, 16, "%d KHz", (int)ceil(freq/1000.f));
		}
		FREQ_ANN(freq, txt);
		freq *= 2.0;
	}

#define LEVEL_ANN(LVL, TXT) { \
	const float dx =  .5 * SS_SIZE * (1 - powf(10, .05 * (LVL))); \
	const float p0 = .5 + rintf(SS_BORDER + .5 * SS_SIZE + dx); \
	const float p1 = .5 + rintf(SS_BORDER + .5 * SS_SIZE - dx); \
	cairo_move_to(cr, p0, SS_BORDER); \
	cairo_line_to(cr, p0, SS_BORDER + SS_SIZE); \
	cairo_stroke(cr); \
	cairo_move_to(cr, p1, SS_BORDER); \
	cairo_line_to(cr, p1, SS_BORDER + SS_SIZE); \
	cairo_stroke(cr); \
	write_text_full(cr, TXT, ui->font[0], p0, SS_BORDER + 3, -.5 * M_PI, -1, c_ahz); \
	write_text_full(cr, TXT, ui->font[0], p1, SS_BORDER + 3, -.5 * M_PI, -1, c_ahz); \
	}

	LEVEL_ANN(-1, "1dB");
	LEVEL_ANN(-3, "3dB");
	LEVEL_ANN(-6, "6dB");
	LEVEL_ANN(-10, "10dB");
	LEVEL_ANN(-20, "20dB");


	const double dash2[] = {1.0, 3.0};
	cairo_set_line_width(cr, 3.5);
	cairo_set_dash(cr, dash2, 2, 2);

	const float xmid = rintf(SS_BORDER + SS_SIZE *.5) + .5;
	cairo_move_to(cr, xmid, SS_BORDER);
	cairo_line_to(cr, xmid, SS_BORDER + SS_SIZE);
	cairo_stroke(cr);

	write_text_full(cr, "L",  ui->font[1], SS_BORDER + 6,           SS_BORDER + 12, 0, -2, c_ann);
	write_text_full(cr, "R",  ui->font[1], SS_BORDER + SS_SIZE - 6, SS_BORDER + 12, 0, -2, c_ann);

	cairo_destroy (cr);
}

static void plot_data_fft(SFSUI* ui) {
	cairo_t* cr;
	cr = cairo_create (ui->sf_dat);

	rounded_rectangle (cr, SS_BORDER, SS_BORDER, SS_SIZE, SS_SIZE, SS_BORDER);
	cairo_clip_preserve (cr);

	const float persistence = robtk_dial_get_value(ui->screen);
	float transp;
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	if (persistence > 0) {
		cairo_set_source_rgba(cr, 0, 0, 0, .25 - .0025 * persistence);
		transp = 0.05;
	} else {
		cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
		transp = .5;
	}
	cairo_fill(cr);

	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_width (cr, 1.0);

	const float xmid = rintf(SS_BORDER + SS_SIZE *.5) + .5;
	const float dnum = SS_SIZE / ui->log_base;
	const float denom = ui->log_rate / (float)ui->fft_bins;

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	for (uint32_t i = 1; i < ui->fft_bins-1 ; ++i) {
		if (ui->level[i] < 0) continue;

		const float level = MAKEUP_GAIN + fftx_power_to_dB(ui->level[i]);
		if (level < -80) continue;

		const float y  = rintf(SS_BORDER + SS_SIZE - dnum * fast_log10(1.0 + i * denom)) + .5;
		const float y1 = rintf(SS_BORDER + SS_SIZE - dnum * fast_log10(1.0 + (i+1) * denom)) + .5;
		const float pk = level > 0.0 ? 1.0 : (80 + level) / 80.0;
		const float a_lr = ui->lr[i];

		float clr[3];
		hsl2rgb(clr, .70 - .72 * pk, .9, .3 + pk * .4);
		cairo_set_source_rgba(cr, clr[0], clr[1], clr[2], transp  + pk * .2);
		cairo_set_line_width (cr, MAX(1.0, (y - y1)));

		cairo_move_to(cr, xmid, y);
		cairo_line_to(cr, SS_BORDER + SS_SIZE * a_lr, y);
		cairo_stroke(cr);

	}
	cairo_destroy (cr);
}

static void plot_data_oct(SFSUI* ui) {
	cairo_t* cr;
	cr = cairo_create (ui->sf_dat);
	rounded_rectangle (cr, SS_BORDER, SS_BORDER, SS_SIZE, SS_SIZE, SS_BORDER);
	cairo_clip_preserve (cr);

	const float persistence = robtk_dial_get_value(ui->screen);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	if (persistence > 0) {
		cairo_set_source_rgba(cr, 0, 0, 0, .33 - .0033 * persistence);
	} else {
		cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
	}
	cairo_fill(cr);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

	const float xmid = rintf(SS_BORDER + SS_SIZE *.5) + .5;
	const float dnum = SS_SIZE / ui->log_base;
	const float denom = 2.0 * ui->log_rate / ui->rate;

	uint32_t fi = 1;
	for (uint32_t i = 0; i < ui->freq_bins; ++i) {
		float a_lr = 0;
		float a_level = 0;
		float a_freq = 0;
		uint32_t a_cnt = 0;

		while(fi < ui->freq_band[i]) {
			if (ui->level[fi] < 0) { fi++; continue; }

			a_freq += fi * ui->fa->freq_per_bin;
			a_level += ui->level[fi];
			a_lr += ui->lr[fi];
			a_cnt++;
			fi++;
		}
		if (a_cnt == 0) continue;
		a_level = MAKEUP_GAIN + fftx_power_to_dB (a_level);
		if (a_level < -80) continue;

		a_freq /= (float)a_cnt;
		a_lr /= (float)a_cnt;

		const float y = rintf(SS_BORDER + SS_SIZE - dnum * fast_log10(1.0 + a_freq * denom)) + .5;
		const float pk = a_level > 0.0 ? 1.0 : (80 + a_level) / 80.0;

		float clr[3];
		hsl2rgb(clr, .70 - .72 * pk, .9, .3 + pk * .4);
		cairo_set_source_rgba(cr, clr[0], clr[1], clr[2], 0.8);

		if (fabsf(a_lr -.5f) < .05f) {
			cairo_set_line_width (cr, 3.0);
		} else {
			cairo_set_line_width (cr, 1.0);
		}

		cairo_move_to(cr, xmid, y);
		cairo_line_to(cr, SS_BORDER + SS_SIZE * a_lr, y);
		cairo_stroke(cr);
	}
	cairo_destroy (cr);
}

static bool expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev) {
	SFSUI* ui = (SFSUI*)GET_HANDLE(handle);

	if (ui->update_grid) {
		update_grid(ui);
		ui->update_grid = false;
	}
	if (ui->clear_persistence) {
		cairo_t* crx;
		crx = cairo_create (ui->sf_dat);
		rounded_rectangle (crx, SS_BORDER, SS_BORDER, SS_SIZE, SS_SIZE, SS_BORDER);
		cairo_set_source_rgba(crx, 0, 0, 0, 1.0);
		cairo_fill(crx);
		cairo_destroy(crx);
		ui->clear_persistence = false;
	}

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


/******************************************************************************
 * UI callbacks  - FFT Bins
 */

static bool cb_set_fft (RobWidget* handle, void *data) {
	SFSUI* ui = (SFSUI*) (data);
	const float fft_size = 2 * robtk_select_get_value(ui->sel_fft);
	uint32_t fft_bins = floorf(fft_size / 2.0);
	if (ui->fft_bins == fft_bins) return TRUE;
	reinitialize_fft(ui, fft_bins);
	ui->write(ui->controller, SS_FFT, sizeof(float), 0, (const void*) &fft_size);
	return TRUE;
}

static bool cb_set_oct (RobWidget* handle, void *data) {
	SFSUI* ui = (SFSUI*) (data);
	ui->clear_persistence = true;
	if (ui->disable_signals) return TRUE;
	float val = robtk_cbtn_get_active(ui->btn_oct) ? 1.0 : 0.0;
	ui->write(ui->controller, SS_BAND, sizeof(float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_set_persistence (RobWidget* handle, void *data) {
	SFSUI* ui = (SFSUI*) (data);
	const float val = robtk_dial_get_value(ui->screen);
	if (ui->disable_signals) return TRUE;
	ui->write(ui->controller, SS_SCREEN, sizeof(float), 0, (const void*) &val);
	return TRUE;
}

/******************************************************************************
 * widget hackery
 */

static void
size_request(RobWidget* handle, int *w, int *h) {
	SFSUI* ui = (SFSUI*)GET_HANDLE(handle);
	*w = ui->width;
	*h = ui->height;
}

static RobWidget * toplevel(SFSUI* ui, void * const top)
{
	/* main widget: layout */
	ui->rw = rob_vbox_new(FALSE, 0);
	robwidget_make_toplevel(ui->rw, top);

	ui->hbox1 = rob_hbox_new(FALSE, 0);
	ui->hbox2 = rob_hbox_new(FALSE, 0);
	ui->sep2 = robtk_sep_new(true);

	rob_vbox_child_pack(ui->rw, ui->hbox1, FALSE, FALSE);
	rob_vbox_child_pack(ui->rw, robtk_sep_widget(ui->sep2), FALSE, FALSE);
	rob_vbox_child_pack(ui->rw, ui->hbox2, FALSE, FALSE);

	ui->font[0] = pango_font_description_from_string("Mono 9px");
	ui->font[1] = pango_font_description_from_string("Mono 10px");
	get_color_from_theme(0, ui->c_fg);
	get_color_from_theme(1, ui->c_bg);
	create_surfaces(ui);

	/* main drawing area */
	ui->m0 = robwidget_new(ui);
	ROBWIDGET_SETNAME(ui->m0, "stsco (m0)");
	robwidget_set_expose_event(ui->m0, expose_event);
	robwidget_set_size_request(ui->m0, size_request);
	rob_hbox_child_pack(ui->hbox1, ui->m0, FALSE, FALSE);

	/* screen persistence dial */
	ui->lbl_screen = robtk_lbl_new("Persistence:");
	ui->screen = robtk_dial_new_with_size(0.0, 100.0, 1,
			22, 22, 10.5, 10.5, 10);
	robtk_dial_set_alignment(ui->screen, 1.0, 0.5);
	robtk_dial_set_value(ui->screen, 50);
	robtk_dial_set_default(ui->screen, 50.0);
	robtk_dial_set_callback(ui->screen, cb_set_persistence, ui);

	/* fft bins */
	ui->lbl_fft = robtk_lbl_new("FFT Samples:");
	ui->sel_fft = robtk_select_new();
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

	/* explicit alignment */
	ui->sep0 = robtk_sep_new(true);
	robtk_sep_set_linewidth(ui->sep0, 0);

	rob_hbox_child_pack(ui->hbox2, robtk_lbl_widget(ui->lbl_screen), FALSE, FALSE);
	rob_hbox_child_pack(ui->hbox2, robtk_dial_widget(ui->screen), FALSE, FALSE);
	rob_hbox_child_pack(ui->hbox2, robtk_lbl_widget(ui->lbl_fft), FALSE, FALSE);
	rob_hbox_child_pack(ui->hbox2, robtk_select_widget(ui->sel_fft), FALSE, FALSE);
	rob_hbox_child_pack(ui->hbox2, robtk_sep_widget(ui->sep0), TRUE, FALSE);
	rob_hbox_child_pack(ui->hbox2, robtk_cbtn_widget(ui->btn_oct), FALSE, FALSE);

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
	SFSUI* ui = (SFSUI*) calloc(1,sizeof(SFSUI));
	*widget = NULL;
	ui->map = NULL;

	if (!strcmp(plugin_uri, MTR_URI "stereoscope")) { ; }
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

	map_xfer_uris(ui->map, &ui->uris);
	lv2_atom_forge_init(&ui->forge, ui->map);

	ui->write      = write_function;
	ui->controller = controller;

	ui->rate = 48000;
	ui->disable_signals = false;
	ui->update_grid = false;
	ui->clear_persistence = false;
	ui->fft_bins = 512;
	ui->freq_band = NULL;
	ui->freq_bins = 0;

	ui->width  = SS_SIZE + 2 * SS_BORDER;
	ui->height = SS_SIZE + 2 * SS_BORDER;

	pthread_mutex_init(&ui->fft_lock, NULL);
	*widget = toplevel(ui, ui_toplevel);
	reinitialize_fft(ui, ui->fft_bins);
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
	SFSUI* ui = (SFSUI*)handle;

	ui_disable(ui);

	pango_font_description_free(ui->font[0]);
	pango_font_description_free(ui->font[1]);

	cairo_surface_destroy(ui->sf_ann);
	cairo_surface_destroy(ui->sf_dat);

	robtk_select_destroy(ui->sel_fft);
	robtk_dial_destroy(ui->screen);
	robtk_lbl_destroy(ui->lbl_fft);
	robtk_lbl_destroy(ui->lbl_screen);
	robtk_sep_destroy(ui->sep0);
	robtk_sep_destroy(ui->sep2);
	robtk_cbtn_destroy(ui->btn_oct);

	robwidget_destroy(ui->m0);

	rob_box_destroy(ui->hbox1);
	rob_box_destroy(ui->hbox2);
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

/******************************************************************************/

static void process_audio(SFSUI* ui, const size_t n_elem, float const * const left, float const * const right) {
	pthread_mutex_lock(&ui->fft_lock);

	fftx_run(ui->fa, n_elem, left);
	bool display = !fftx_run(ui->fb, n_elem, right);

	if (display) {
		assert (fftx_bins(ui->fa) == ui->fft_bins);
		const float db_thresh = 1e-20;

		for (uint32_t i = 1; i < ui->fft_bins-1; i++) {
			if (ui->fa->power[i] < db_thresh && ui->fb->power[i] < db_thresh) {
				ui->lr[i] = 0.5;
				ui->level[i] = 0;
				continue;
			}
			const float lv = MAX(ui->fa->power[i], ui->fb->power[i]);
#if 1
			const float lr = .5 + .5 * (sqrtf(ui->fb->power[i]) - sqrtf(ui->fa->power[i])) / sqrtf(lv);
#else
			//XXX TODO log-scale / deflection of fraction
			//const float lr = .5 + .25 * fast_log10(ui->fb->power[i] / ui->fa->power[i]);
			float lr;
			if (ui->fb->power[i] < ui->fa->power[i]) {
				lr = .5 + .5 * fast_log10(ui->fb->power[i] / ui->fa->power[i]);
			} else {
				lr = .5 - .5 * fast_log10(ui->fa->power[i] / ui->fb->power[i]);
			}
#endif

			ui->level[i] += .1 * (lv - ui->level[i]) + 1e-20;
			ui->lr[i] += .1 * (lr - ui->lr[i]) + 1e-10;
		}
		queue_draw(ui->m0);
	}
	pthread_mutex_unlock(&ui->fft_lock);
}

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
	SFSUI* ui = (SFSUI*)handle;
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

	if (port_index == SS_FFT) {
		float val = *(float *)buffer;
		uint32_t fft_bins = floorf(val / 2.0);
		if (ui->fft_bins != fft_bins) {
			reinitialize_fft(ui, fft_bins);
			robtk_select_set_value(ui->sel_fft, ui->fft_bins);
		}
	}
	else if (port_index == SS_BAND) {
		float val = *(float *)buffer;
		ui->disable_signals = true;
		robtk_cbtn_set_active(ui->btn_oct, val != 0);
		ui->disable_signals = false;
	}
	else if (port_index == SS_SCREEN) {
		ui->disable_signals = true;
		robtk_dial_set_value(ui->screen, *(float *)buffer);
		ui->disable_signals = false;
	}
}
