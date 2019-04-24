/* The Tuna Tuner UI
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "src/tuna.h"

#define RTK_URI TUNA_URI
#define RTK_GUI "ui"

#ifndef MIN
#define MIN(A,B) ( (A) < (B) ? (A) : (B) )
#endif
#ifndef MAX
#define MAX(A,B) ( (A) > (B) ? (A) : (B) )
#endif

/* widget, window size */
#define DAWIDTH  (400.)
#define DAHEIGHT (300.)

/* pixel layout */
#define L_BAR_X (20)
#define L_BAR_W (DAWIDTH - 40)

#define L_CNT_YT (DAHEIGHT - 150)
#define L_CNT_XC (DAWIDTH / 2)
#define L_CNT_H (20.)

#define L_STB_H (20.)
#define L_STB_YC (DAHEIGHT - 105)

#define L_LVL_YT (DAHEIGHT - 80)
#define L_LVL_H  (10)

#define L_ERR_YT (DAHEIGHT - 60)
#define L_ERR_H  (10)

#define L_NFO_YC (80.)
#define L_NFO_XL (100.)
#define L_NFO_XC (325.)

#define L_TUN_XC (160)
#define L_TUN_YC (125)


#define L_FOO_XC (DAWIDTH / 2)
#define L_FOO_YB (DAHEIGHT - 20)

typedef struct {
	LV2UI_Write_Function write;
	LV2UI_Controller controller;
	LV2_Atom_Forge forge;
	LV2_URID_Map* map;
	TunaLV2URIs uris;

	RobWidget *hbox, *ctable;
	RobWidget *darea;
	RobTkXYp  *xyp;

	RobWidget *btnbox;
	RobTkRBtn *disp[2];
	RobTkSep  *sep[3];
	RobTkLbl  *label[4];
	RobTkSpin *spb_tuning;
	RobTkSpin *spb_octave;
	RobTkSpin *spb_freq;
	RobTkSelect *sel_note;
	RobTkSelect *sel_mode;

	RobTkLbl  *lbl_debug[7];
	RobTkSpin *spb_debug[7];

	PangoFontDescription *font[4];
	cairo_surface_t *frontface;
	cairo_surface_t *spect_ann;
	cairo_pattern_t* meterpattern;

	float p_rms;
	float p_freq;
	float p_octave;
	float p_note;
	float p_cent;
	float p_error;

	/* smoothed values for display */
	float s_rms;
	float s_error;
	float s_cent;

	float strobe_tme;
	float strobe_dpy;
	float strobe_phase;

	bool disable_signals;
	bool spectr_enable;

	const char *nfo;
} TunaUI;

static const char notename[12][3] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

enum fontsize {
	F_M_MED = 0,
	F_S_LARGE,
	F_M_HUGE,
	F_M_SMALL,
};


/******************************************************************************
 * Look & Feel
 */


static inline float log_meter80 (float db) {
	float def = 0.0f;

	if (db < -80.0f) {
		def = 0.0f;
	} else if (db < -70.0f) {
		def = (db + 80.0f) * 0.50f;
	} else if (db < -60.0f) {
		def = (db + 70.0f) * 0.75f + 5.0f;
	} else if (db < -50.0f) {
		def = (db + 60.0f) * 1.00f + 12.5f;
	} else if (db < -40.0f) {
		def = (db + 50.0f) * 1.25f + 22.5f;
	} else if (db < -30.0f) {
		def = (db + 40.0f) * 1.50f + 35.0f;
	} else if (db < -20.0f) {
		def = (db + 30.0f) * 1.75f + 50.0f;
	} else if (db < 2.0f) {
		def = (db + 20.0f) * 2.00f + 67.5f;
	} else {
		def = 110.0f;
	}
	return (def/110.0f);
}

static int deflect(float val) {
	int lvl = rint(L_BAR_W * log_meter80(val));
	if (lvl < 2) lvl = 0;
	else if (lvl < 4) lvl = 4;
	if (lvl >= L_BAR_W) lvl = L_BAR_W;
	return lvl;
}

static void xy_clip_fn(cairo_t *cr, void *data) {
	TunaUI* ui = (TunaUI*) data;
	rounded_rectangle (cr, 10, 10, DAWIDTH - 20, DAHEIGHT - 20, 10);
	cairo_clip(cr);

	/* global abs. threshold */
	{
		float y0 = 10 + (DAHEIGHT - 20.) * robtk_spin_get_value(ui->spb_debug[0]) / -92.;
		float y1 = 10 + (DAHEIGHT - 20.) * 92 / 92.;
		cairo_set_source_rgba(cr, 0.2, 0.2, 0.4, 0.5);
		cairo_rectangle (cr, 0, y0, DAWIDTH, y1-y0);
		cairo_fill(cr);
	}

	cairo_save(cr);
	if (ui->p_freq > 0) {
		/* note detected */

		if (ui->s_rms > -90) {
		/* RMS value and post-filter threshold */
			float xf = 10 + (DAWIDTH - 20.) * ui->p_freq / 1500.;
			const float y0 = 10 + (DAHEIGHT - 20.) * (-ui->s_rms) / 92.;
			const float y1 = 10 + (DAHEIGHT - 20.) * (-ui->s_rms - robtk_spin_get_value(ui->spb_debug[1])) / 92.;
			const float y2 = 10 + (DAHEIGHT - 20.) * (-ui->s_rms - robtk_spin_get_value(ui->spb_debug[2])) / 92.;
			cairo_set_source_rgba(cr, 0.6, 0.6, 0.8, 0.6);
			const double dash[] = {1.5};
			cairo_set_line_width(cr, 1.5);
			cairo_set_dash(cr, dash, 1, 0);
			cairo_move_to(cr, 0, rint(y0)-.5);
			cairo_line_to(cr, DAWIDTH, rint(y0)-.5);
			cairo_stroke(cr);
			cairo_set_dash(cr, NULL, 0, 0);

			cairo_set_source_rgba(cr, 0.1, 0.5, 0.1, 0.3);
			cairo_rectangle (cr, 0, y0, DAWIDTH, y1-y0);
			cairo_fill(cr);
			cairo_set_source_rgba(cr, 0.1, 0.5, 0.4, 0.4);
			cairo_rectangle (cr, xf-5.5, y0, 10, y2-y0);
			cairo_fill(cr);
		}

		if (robtk_rbtn_get_active(ui->disp[1])) {
			float pp = -100;
			for (uint32_t d=0; d < ui->xyp->n_points; ++d) {
				if (fabsf(ui->xyp->points_x[d] - ui->p_freq) < 10 && ui->xyp->points_y[d] > pp
						) pp = ui->xyp->points_y[d];
			}
			if (pp > -85) {
				const float y0 = 10 - (DAHEIGHT - 20.) * (pp) / 92.;
				const float y1 = 10 - (DAHEIGHT - 20.) * (pp + robtk_spin_get_value(ui->spb_debug[4])) / 92.;
				const float y2 = 10 - (DAHEIGHT - 20.) * (pp + robtk_spin_get_value(ui->spb_debug[3]) + robtk_spin_get_value(ui->spb_debug[4])) / 92.;
				const float y3 = 10 - (DAHEIGHT - 20.) * (pp + robtk_spin_get_value(ui->spb_debug[5])) / 92.;
				const float y4 =  0 - (DAHEIGHT - 20.) * (robtk_spin_get_value(ui->spb_debug[6])) / 92.;
				float x = (DAWIDTH - 20.) * ui->p_freq / 1500.;
				float x0 = 10 + x;

				cairo_set_source_rgba(cr, 0.5, 0.1, 0.1, 0.3);
				cairo_rectangle (cr, x0, y0, DAWIDTH-x0, y1-y0);
				cairo_fill(cr);
				cairo_set_source_rgba(cr, 0.8, 0.1, 0.1, 0.3);
				cairo_rectangle (cr, x0, y0, DAWIDTH-x0, y2-y0);
				cairo_fill(cr);
				cairo_set_source_rgba(cr, 0.1, 0.1, 0.6, 0.4);
				cairo_rectangle (cr, x0, y0, x, y3-y0);
				cairo_fill(cr);
				cairo_set_source_rgba(cr, 0.2, 0.1, 0.6, 0.3);
				cairo_rectangle (cr, x0+x, y0, DAWIDTH-x0-x, y4+y3-y0);
				cairo_fill(cr);
				cairo_set_source_rgba(cr, 0.2, 0.1, 0.6, 0.3);
				cairo_rectangle (cr, x0+3.f*x, y0, DAWIDTH-x0-3*x, 2.f*y4+y3-y0);
				cairo_fill(cr);

				/* draw cross */
				cairo_set_line_width(cr, 1.0);
				cairo_set_source_rgba(cr, 0.9, 0.9, 0.9, 0.8);
				cairo_move_to(cr, rintf(x0) - 3.5, rintf(y0) - 3);
				cairo_line_to(cr, rintf(x0) + 2.5, rintf(y0) + 3);
				cairo_stroke(cr);
				cairo_move_to(cr, rintf(x0) + 2.5, rintf(y0) - 3);
				cairo_line_to(cr, rintf(x0) - 3.5, rintf(y0) + 3);
				cairo_stroke(cr);
			}
		}

		/* octave lines */
		cairo_set_source_rgba (cr, .0, .9, .0, .6);
		cairo_set_line_width(cr, 3.5);
		float x = 10 + (DAWIDTH - 20.) * ui->p_freq / 1500.;
		cairo_move_to(cr, rintf(x) - .5, 10);
		cairo_line_to(cr, rintf(x) - .5, DAHEIGHT - 10);
		cairo_stroke(cr);

		const double dash[] = {1.5};
		cairo_set_dash(cr, dash, 1, 0);
		cairo_set_line_width(cr, 4.0);
		cairo_set_source_rgba (cr, .2, .8, .0, .6);
		x = 10 + (DAWIDTH - 20.) * 2.0 * ui->p_freq / 1500.;
		cairo_move_to(cr, rintf(x) - .0, 10);
		cairo_line_to(cr, rintf(x) - .0, DAHEIGHT - 10);
		cairo_stroke(cr);

		x = 10 + (DAWIDTH - 20.) * 4.0 * ui->p_freq / 1500.;
		cairo_move_to(cr, rintf(x) - .0, 10);
		cairo_line_to(cr, rintf(x) - .0, DAHEIGHT - 10);
		cairo_stroke(cr);

		x = 10 + (DAWIDTH - 20.) * 8.0 * ui->p_freq / 1500.;
		cairo_move_to(cr, rintf(x) - .0, 10);
		cairo_line_to(cr, rintf(x) - .0, DAHEIGHT - 10);
		cairo_stroke(cr);
	} else {
#if 0
		rounded_rectangle (cr, 10, 10, DAWIDTH - 20, DAHEIGHT - 20, 10);
		cairo_set_source_rgba(cr, 0.4, 0.4, 0.4, 0.2);
		cairo_fill(cr);
#endif

		if (ui->s_rms > -80) {
			const float y0 = 10 + (DAHEIGHT - 20.) * (-ui->s_rms) / 92.;
			cairo_set_source_rgba(cr, 0.6, 0.6, 0.8, 0.6);
			const double dash[] = {1.5};
			cairo_set_line_width(cr, 1.5);
			cairo_set_dash(cr, dash, 1, 0);
			cairo_move_to(cr, 0, rint(y0)-.5);
			cairo_line_to(cr, DAWIDTH, rint(y0)-.5);
			cairo_stroke(cr);
		}
	}
	cairo_restore(cr);
}

static void render_frontface(TunaUI* ui) {
	cairo_t *cr;
	if (!ui->frontface) {
		ui->frontface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, DAWIDTH, DAHEIGHT);
	}
	cr = cairo_create(ui->frontface);
	float c_bg[4];
	get_color_from_theme(1, c_bg);
	CairoSetSouerceRGBA(c_bg);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr, 0, 0, DAWIDTH, DAHEIGHT);
	cairo_fill (cr);

	rounded_rectangle (cr, 10, 10, DAWIDTH - 20, DAHEIGHT - 20, 10);
	CairoSetSouerceRGBA(c_blk);
	cairo_fill(cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_line_width(cr, 1.0);

	write_text_full(cr, "The Tuna Tuner Tube",
			ui->font[F_S_LARGE], DAWIDTH/2, 30, 0, 2, c_wht);

	/* Cent scale */
	rounded_rectangle (cr, (L_CNT_XC - L_BAR_W/2) - 2, L_CNT_YT - 2, L_BAR_W + 4 , L_CNT_H + 4, 4);
	CairoSetSouerceRGBA(c_g30);
	cairo_fill(cr);
	write_text_full(cr, "C E N T", ui->font[F_S_LARGE], L_CNT_XC, L_CNT_YT + L_CNT_H/2, 0, 2, c_g60);

	const float cnt_m10 = rintf(L_CNT_XC - L_BAR_W * .1);
	const float cnt_w10 = rintf(L_BAR_W * .2);
	cairo_pattern_t* cpat = cairo_pattern_create_linear (cnt_m10 -.5, 0.0, cnt_m10+cnt_w10, 0.0);
	cairo_pattern_add_color_stop_rgba (cpat, 0.00, .8, .8, .0, .0);
	cairo_pattern_add_color_stop_rgba (cpat, 0.10, .8, .8, .0, .3);
	cairo_pattern_add_color_stop_rgba (cpat, 0.25, .0, .8, .0, .3);
	cairo_pattern_add_color_stop_rgba (cpat, 0.50, .0, .8, .0, .4);
	cairo_pattern_add_color_stop_rgba (cpat, 0.75, .0, .8, .0, .3);
	cairo_pattern_add_color_stop_rgba (cpat, 0.90, .8, .8, .0, .3);
	cairo_pattern_add_color_stop_rgba (cpat, 1.00, .8, .8, .0, .0);

	cairo_set_source (cr, cpat);
	cairo_rectangle(cr, cnt_m10, L_CNT_YT, cnt_w10 + .5, L_CNT_H);
	cairo_fill(cr);
	cairo_pattern_destroy (cpat);

	for (int cent = -40; cent <= 40  ; cent +=5) {
		char tmp[16];
		snprintf(tmp, 16, "%+3d", cent);
		if (abs(cent) > 10) CairoSetSouerceRGBA(c_red);
		else if (abs(cent) > 0) CairoSetSouerceRGBA(c_nyl);
		else CairoSetSouerceRGBA(c_grn);

		float ylen = (cent % 10) ? 4.5 : 1.5 ;
		const double dash[] = {1.5};
		cairo_save(cr);
		cairo_set_dash(cr, dash, 1, 0);
		cairo_move_to(cr, rintf(L_CNT_XC + L_BAR_W * cent / 100.) -.5, L_CNT_YT + ylen);
		cairo_line_to(cr, rintf(L_CNT_XC + L_BAR_W * cent / 100.) -.5, L_CNT_YT + L_CNT_H - ylen - .5);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
	write_text_full(cr, "-45", ui->font[F_M_SMALL],
			rint(L_CNT_XC + L_BAR_W * -45 / 100.)-.5, L_CNT_YT + L_CNT_H/2, 0 * M_PI, 2, c_g60);
	write_text_full(cr, "+45", ui->font[F_M_SMALL],
			rint(L_CNT_XC + L_BAR_W *  45 / 100.)-.5, L_CNT_YT + L_CNT_H/2, 0 * M_PI, 2, c_g60);

	/* Strobe background */
	CairoSetSouerceRGBA(c_g30);
	rounded_rectangle (cr, L_BAR_X - 2, L_STB_YC - 12, L_BAR_W + 4 , L_STB_H + 4, 4);
	cairo_fill(cr);
	write_text_full(cr, "S T R O B E", ui->font[F_S_LARGE], L_CNT_XC, L_STB_YC, 0, 2, c_g60);

	/* Level background */
	CairoSetSouerceRGBA(c_g30);
	rounded_rectangle (cr, L_BAR_X - 2, L_LVL_YT - 2, L_BAR_W + 4 , L_LVL_H + 4, 4);
	cairo_fill(cr);
	cairo_save(cr);
	//const double dash[] = {1.5};
	//cairo_set_dash(cr, dash, 1, 0);
	for (int db= -72; db <= 0; db+=6) {
		if (db >= -30 && db <= -24) continue; // text  on top
		if (db <= -60) {
			CairoSetSouerceRGBA(c_gry);
		} else if (db < -18) {
			CairoSetSouerceRGBA(c_g60);
		} else if (db < 0) {
			CairoSetSouerceRGBA(c_g80);
		} else {
			CairoSetSouerceRGBA(c_red);
		}
		cairo_move_to(cr, rintf(L_BAR_X + deflect(db)) -.5, L_LVL_YT + 1.5);
		cairo_line_to(cr, rintf(L_BAR_X + deflect(db)) -.5, L_LVL_YT + L_LVL_H - 1.5);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
	write_text_full(cr, "signal level", ui->font[F_M_SMALL], L_CNT_XC, L_LVL_YT + L_LVL_H/2, 0, 2, c_g60);

	/* Accuracy background */
	CairoSetSouerceRGBA(c_g30);
	rounded_rectangle (cr, L_BAR_X - 2, L_ERR_YT - 2, L_BAR_W + 4 , L_ERR_H + 4, 4);
	cairo_fill(cr);
	write_text_full(cr, "accuracy", ui->font[F_M_SMALL], L_CNT_XC, L_ERR_YT + L_ERR_H/2, 0, 2, c_g60);

#if 1 /* version info */
	write_text_full(cr, ui->nfo ? ui->nfo : "x42 tuna " VERSION, ui->font[F_M_SMALL],
			15, 20, 1.5 * M_PI, 7, c_g20);
#endif

	cairo_destroy(cr);

	/* draw spectrogram faceplate */

	robtk_xydraw_set_surface(ui->xyp, NULL);
	if (ui->spect_ann) {
		cairo_surface_destroy (ui->spect_ann);
	}
	ui->spect_ann = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, DAWIDTH, DAWIDTH);
	cr = cairo_create (ui->spect_ann);

	CairoSetSouerceRGBA(c_bg);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr, 0, 0, DAWIDTH, DAHEIGHT);
	cairo_fill (cr);

	cairo_save(cr);
	rounded_rectangle (cr, 10, 10, DAWIDTH - 20, DAHEIGHT - 20, 10);
	CairoSetSouerceRGBA(c_blk);
	cairo_fill_preserve(cr);
	cairo_clip(cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

#if 0
	{
		float y0 = 10 + (DAHEIGHT - 20.) * 70 / 92.;
		float y1 = 10 + (DAHEIGHT - 20.) * 92 / 92.;
		cairo_set_source_rgba(cr, 0.2, 0.2, 0.4, 0.5);
		cairo_rectangle (cr, 0, y0, DAWIDTH, y1-y0);
		cairo_fill(cr);
	}
#endif

	cairo_set_line_width (cr, 1.0);
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	for (float dB = 6; dB < 92; dB += 6) {
		char tmp[16];
		const double dash[] = {3.0, 1.5};
		cairo_set_dash(cr, dash, 2, 0);
		sprintf(tmp, "%+0.0fdB", -dB);
		float y = 10 + (DAHEIGHT - 20.) * dB / 92.;
		cairo_move_to(cr, 10, rintf(y) + .5);
		cairo_line_to(cr, DAWIDTH - 10, rintf(y) + .5);
		cairo_stroke(cr);
		write_text_full(cr, tmp, ui->font[F_M_SMALL],
				DAWIDTH - 15, y, 0, 1, c_g60);
	}

	for (int fq = 50; fq < 1500; fq += 50) {
		float x = 10 + (DAWIDTH - 20.) * fq / 1500.;
		const double dash[] = {1.5};
		if (fq%100) {
			cairo_set_dash(cr, dash, 1, 0);
		} else {
			cairo_set_dash(cr, NULL, 0, 0);
		}

		cairo_move_to(cr, rintf(x) - .5, 10);
		cairo_line_to(cr, rintf(x) - .5, DAHEIGHT - 10);
		cairo_stroke(cr);

		if (fq%100 == 0 && fq <= 1300) {
			char tmp[16];
			if (fq < 1000.0) {
				sprintf(tmp, "%dHz", fq);
			} else {
				sprintf(tmp, "%0.1fkHz", fq/1000.0);
			}
			write_text_full(cr, tmp, ui->font[F_M_SMALL],
					x, 10, 1.5 * M_PI, 1, c_g60);
		}
	}
	cairo_restore(cr);
	cairo_destroy(cr);
	robtk_xydraw_set_surface(ui->xyp, ui->spect_ann);

	/* meter pattern */

	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0, L_BAR_W, 0.0);
	cairo_pattern_add_color_stop_rgba (pat, .0,               .0, .0, .0, .0);
	cairo_pattern_add_color_stop_rgba (pat, log_meter80(-70), .0, .0, .5, .6);
	cairo_pattern_add_color_stop_rgba (pat, log_meter80(-55), .1, .2, .8, .6);
	cairo_pattern_add_color_stop_rgba (pat, log_meter80(-45), .0, .7, .1, .6);
	cairo_pattern_add_color_stop_rgba (pat, log_meter80(-18), .0, .8, .0, .6);
	cairo_pattern_add_color_stop_rgba (pat, log_meter80(-6),  .4, .8, .0, .6);
	cairo_pattern_add_color_stop_rgba (pat, log_meter80(-3),  .8, .8, .0, .6);
	cairo_pattern_add_color_stop_rgba (pat, log_meter80(-1),  .8, .4, .0, .6);
	cairo_pattern_add_color_stop_rgba (pat, 1.0,              .8, .0, .0, .6);

#if 0
	if (!getenv("NO_METER_SHADE") || strlen(getenv("NO_METER_SHADE")) == 0) {
		cairo_pattern_t* shade_pattern = cairo_pattern_create_linear (0.0, 0.0, 0.0, L_LVL_H);
		cairo_pattern_add_color_stop_rgba (shade_pattern, 0.0, 0.0, 0.0, 0.0, 0.15);
		cairo_pattern_add_color_stop_rgba (shade_pattern, .35, 1.0, 1.0, 1.0, 0.10);
		cairo_pattern_add_color_stop_rgba (shade_pattern, .53, 0.0, 0.0, 0.0, 0.05);
		cairo_pattern_add_color_stop_rgba (shade_pattern, 1.0, 0.0, 0.0, 0.0, 0.25);

		cairo_surface_t* surface;
		cairo_t* tc = 0;
		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, L_BAR_W, L_LVL_H);
		tc = cairo_create (surface);

		cairo_set_operator (tc, CAIRO_OPERATOR_SOURCE);
		cairo_set_source (tc, pat);
		cairo_rectangle (tc, 0, 0, L_BAR_W, L_LVL_H);
		cairo_fill (tc);
		cairo_pattern_destroy (pat);

		cairo_set_operator (tc, CAIRO_OPERATOR_OVER);
		cairo_set_source (tc, shade_pattern);
		cairo_rectangle (tc, 0, 0, L_BAR_W, L_LVL_H);
		cairo_fill (tc);
		cairo_pattern_destroy (shade_pattern);

		pat = cairo_pattern_create_for_surface (surface);
		cairo_destroy (tc);
		cairo_surface_destroy (surface);
	}
#endif
	ui->meterpattern = pat;
}



static bool expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t *ev)
{
	TunaUI* ui = (TunaUI*) GET_HANDLE(handle);

	/* limit cairo-drawing to exposed area */
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip(cr);
	cairo_set_source_surface(cr, ui->frontface, 0, 0);
	cairo_paint (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	char txt[255];
	const float tuning = robtk_spin_get_value(ui->spb_tuning);


	/* HUGE info: note, ocatave, cent */
	snprintf(txt, 255, "%-2s%.0f", notename[(int)ui->p_note], ui->p_octave);
	write_text_full(cr, txt, ui->font[F_M_HUGE], L_NFO_XL, L_NFO_YC, 0, 3, c_wht);

	if (fabsf(ui->s_cent) < 100) {
		snprintf(txt, 255, "%+6.2f\u00A2", ui->s_cent);
		write_text_full(cr, txt, ui->font[F_M_MED], L_NFO_XC, L_NFO_YC, 0, 1, c_wht);
	}

	float note = ui->p_note + (ui->p_octave+1) * 12.f;
	if (note >= 0 && note < 128) {
		const float note_freq = tuning * powf(2.0, (note - 69.f) / 12.f);
		snprintf(txt, 255, "%7.2fHz @ %5.1fHz", note_freq, tuning);
	} else {
		snprintf(txt, 255, "@ %5.1fHz\n", tuning);
	}
	/* settings */
	write_text_full(cr, txt, ui->font[F_M_SMALL], L_TUN_XC, L_TUN_YC, 0, 2, c_wht);

	/* footer, Frequency || no-signal */
	if (ui->p_freq > 0) {
		snprintf(txt, 255, "%.2fHz", ui->p_freq );
		write_text_full(cr, txt,
				ui->font[F_M_MED], L_FOO_XC, L_FOO_YB, 0, 5, c_wht);
		//write_text_full(cr, txt, ui->font[F_M_MED], L_NFO_XC, L_TUN_YC, 0, 4, c_wht);
	} else {
		write_text_full(cr, " -- no signal -- ",
				ui->font[F_M_MED], L_FOO_XC, L_FOO_YB, 0, 5, c_g60);
	}

	/* cent bar graph */
	if (ui->p_freq > 0) {
		if (fabsf(ui->s_cent) <= 5.0) {
			cairo_set_source_rgba (cr, .0, .8, .0, .7);
		} else {
			cairo_set_source_rgba (cr, .8, .0, .0, .7);
		}
		cairo_rectangle (cr, L_CNT_XC -.5, L_CNT_YT,
				L_BAR_W * ui->s_cent / 100., L_CNT_H);
		cairo_fill(cr);
#if 0
		if (fabsf(ui->s_cent) <= 5.0) {
			/* cent triangles */
			cairo_set_line_width(cr, 1.0);
			float x1 = rint(L_CNT_XC - L_BAR_W *.05) -.5;
			float x2 = rint(L_CNT_XC - L_BAR_W *.05 - L_CNT_H *.7) -.5;
			cairo_move_to(cr, x1, L_CNT_YT + L_CNT_H *.5);
			cairo_line_to(cr, x2, L_CNT_YT );
			cairo_line_to(cr, x2, L_CNT_YT + L_CNT_H);
			cairo_close_path(cr);
			cairo_fill_preserve(cr);
			CairoSetSouerceRGBA(c_blk);
			cairo_stroke(cr);

			x1 = rint(L_CNT_XC + L_BAR_W *.05) +.5;
			x2 = rint(L_CNT_XC + L_BAR_W *.05 + L_CNT_H *.7) +.5;
			cairo_set_source_rgba (cr, .0, .8, .0, .7);
			cairo_move_to(cr, x1, L_CNT_YT + L_CNT_H *.5);
			cairo_line_to(cr, x2, L_CNT_YT );
			cairo_line_to(cr, x2, L_CNT_YT + L_CNT_H);
			cairo_close_path(cr);
			cairo_fill_preserve(cr);
			CairoSetSouerceRGBA(c_blk);
			cairo_stroke(cr);
		}
#endif
	}

	/* level bar graph */
	float level = deflect(ui->s_rms);
	if (level > 4) {
		cairo_set_source(cr, ui->meterpattern);
		rounded_rectangle (cr, L_BAR_X, L_LVL_YT, deflect(ui->s_rms + 6.0), L_LVL_H, 3);
		cairo_fill(cr);
	}

	/* accuracy bar graph */
	if (ui->p_freq == 0) {
		; // no data
	} else if (fabsf(ui->s_error) < 10) {
		cairo_set_source_rgba (cr, .0, .8, .0, .4);
		rounded_rectangle (cr, L_CNT_XC-40, L_ERR_YT, 80, L_ERR_H, 4);
		cairo_fill(cr);
		if (fabsf(ui->s_error) > 2) {
			cairo_set_source_rgba (cr, .0, .0, .9, .2);
			cairo_rectangle (cr, L_CNT_XC, L_ERR_YT,
					L_BAR_W * ui->s_error / 150.0, L_ERR_H);
			cairo_fill(cr);
		}
	} else if (ui->s_error > -25 && ui->s_error < 25) {
		/* normal range -50..+50 blue */
		cairo_set_source_rgba (cr, .2, .3, .9, .7);
		cairo_rectangle (cr, L_CNT_XC, L_ERR_YT,
				L_BAR_W * ui->s_error / 150.0, L_ERR_H);
		cairo_fill(cr);
	} else if (ui->s_error > -50 && ui->s_error < 50) {
		/* normal range -50..+50 yellow*/
		cairo_set_source_rgba (cr, .6, .6, .2, .7);
		cairo_rectangle (cr, L_CNT_XC, L_ERR_YT,
				L_BAR_W * ui->s_error / 150.0, L_ERR_H);
		cairo_fill(cr);
	} else if (ui->s_error > -100 && ui->s_error < 100) {
		/* out of phase */
		cairo_set_source_rgba (cr, .9, .3, .2, .7);
		cairo_rectangle (cr, L_CNT_XC, L_ERR_YT,
				L_BAR_W * ((ui->s_error + ((ui->s_error>0)?33.3:-33.3)) / 266.6), L_ERR_H);
		cairo_fill(cr);
	} else if (ui->s_error >= 100) {
		cairo_set_source_rgba (cr, .9, .0, .0, .7);
		cairo_rectangle (cr, L_CNT_XC, L_ERR_YT,
				L_BAR_W * .5 , L_ERR_H);
		cairo_fill(cr);
	} else if (ui->s_error <= -100) {
		cairo_set_source_rgba (cr, .9, .0, .0, .7);
		cairo_rectangle (cr, L_CNT_XC, L_ERR_YT,
				L_BAR_W * -.5 , L_ERR_H);
		cairo_fill(cr);
	}

	/* strobe setup */
	cairo_set_source_rgba (cr, .5, .5, .5, .8);
	if (ui->strobe_dpy != ui->strobe_tme) {
		if (ui->strobe_tme > ui->strobe_dpy) {
			float tdiff = ui->strobe_tme - ui->strobe_dpy;
			ui->strobe_phase += tdiff * ui->p_cent * 4;
			cairo_set_source_rgba (cr, .8, .8, .0, .8);
		}
		ui->strobe_dpy = ui->strobe_tme;
	}

	/* render strobe */
	cairo_save(cr);
	const double dash1[] = {8.0};
	const double dash2[] = {16.0};

	cairo_set_dash(cr, dash1, 1, ui->strobe_phase * -2.);
	cairo_set_line_width(cr, 8.0);
	cairo_move_to(cr, 20, L_STB_YC);
	cairo_line_to(cr, DAWIDTH-20, L_STB_YC);
	cairo_stroke (cr);

	cairo_set_dash(cr, dash2, 1, -ui->strobe_phase);
	cairo_set_line_width(cr, 16.0);
	cairo_move_to(cr, 20, L_STB_YC);
	cairo_line_to(cr, DAWIDTH-20, L_STB_YC);
	cairo_stroke (cr);
	cairo_restore(cr);


	return TRUE;
}

/******************************************************************************
 * UI callbacks
 */

static bool cb_disp_changed (RobWidget* handle, void *data) {
	TunaUI* ui = (TunaUI*) (data);
	bool ten = robtk_rbtn_get_active(ui->disp[0]);
	if (ten) {
		for (uint32_t i = 0; i < 7; ++i) {
			robwidget_hide(ui->spb_debug[i]->rw, false);
			robwidget_hide(ui->lbl_debug[i]->rw, false);
		}
		for (uint32_t i = 0; i < 4; ++i) {
			robwidget_show(ui->label[i]->rw, false);
		}
		robwidget_show(ui->spb_tuning->rw, false);
		robwidget_show(ui->spb_octave->rw, false);
		robwidget_show(ui->spb_freq->rw, false);
		robwidget_show(ui->sel_note->rw, false);
		robwidget_show(ui->sel_mode->rw, false);
		robwidget_show(ui->sep[1]->rw, false);
		robwidget_show(ui->sep[2]->rw, true);
	} else {
		robwidget_hide(ui->spb_tuning->rw, false);
		robwidget_hide(ui->spb_octave->rw, false);
		robwidget_hide(ui->spb_freq->rw, false);
		robwidget_hide(ui->sel_note->rw, false);
		robwidget_hide(ui->sel_mode->rw, false);
		for (uint32_t i = 0; i < 7; ++i) {
#ifdef GTK_BACKEND
		gtk_widget_set_no_show_all(ui->spb_debug[i]->rw->c, false);
		gtk_widget_set_no_show_all(ui->lbl_debug[i]->rw->c, false);
#endif
			robwidget_show(ui->spb_debug[i]->rw, false);
			robwidget_show(ui->lbl_debug[i]->rw, false);
		}
		for (uint32_t i = 0; i < 4; ++i) {
			robwidget_hide(ui->label[i]->rw, false);
		}
		robwidget_hide(ui->sep[1]->rw, false);
		robwidget_hide(ui->sep[2]->rw, true);
	}
	return TRUE;
}

static bool cb_set_mode (RobWidget* handle, void *data) {
	TunaUI* ui = (TunaUI*) (data);
	float mode = 0;
	switch(robtk_select_get_item(ui->sel_mode)) {
		default:
		case 0:
			robtk_select_set_sensitive(ui->sel_note, false);
			robtk_spin_set_sensitive(ui->spb_octave, false);
			robtk_spin_set_sensitive(ui->spb_freq,   false);
			break;
		case 1: /* freq */
			robtk_select_set_sensitive(ui->sel_note, false);
			robtk_spin_set_sensitive(ui->spb_octave, false);
			robtk_spin_set_sensitive(ui->spb_freq,   true);
			mode = robtk_spin_get_value(ui->spb_freq);
			break;
		case 2: /* note */
			robtk_select_set_sensitive(ui->sel_note, true);
			robtk_spin_set_sensitive(ui->spb_octave, true);
			robtk_spin_set_sensitive(ui->spb_freq,   false);
			mode = -1
				- rintf(robtk_spin_get_value(ui->spb_octave)+1) * 12.
				- robtk_select_get_value(ui->sel_note);
			break;
	}
	if (!ui->disable_signals) {
		ui->write(ui->controller, TUNA_MODE, sizeof(float), 0, (const void*) &mode);
	}
	return TRUE;
}

static bool cb_set_tuning (RobWidget* handle, void *data) {
	TunaUI* ui = (TunaUI*) (data);
	if (!ui->disable_signals) {
		float val = robtk_spin_get_value(ui->spb_tuning);
		ui->write(ui->controller, TUNA_TUNING, sizeof(float), 0, (const void*) &val);
	}
	queue_draw(ui->darea);
	return TRUE;
}

static bool cb_set_debug (RobWidget* handle, void *data) {
	TunaUI* ui = (TunaUI*) (data);
	if (!ui->disable_signals) {
		for (uint32_t i = 0; i < 7; ++i) {
			float val = robtk_spin_get_value(ui->spb_debug[i]);
			ui->write(ui->controller, TUNA_T_RMS + i, sizeof(float), 0, (const void*) &val);
		}
	}
	return TRUE;
}

/******************************************************************************
 * RobWidget
 */


static void ui_disable(LV2UI_Handle handle)
{
	TunaUI* ui = (TunaUI*)handle;
	uint8_t obj_buf[64];
	if (!ui->spectr_enable) return;
	lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 64);
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_frame_time(&ui->forge, 0);
	LV2_Atom* msg = (LV2_Atom*)x_forge_object(&ui->forge, &frame, 1, ui->uris.ui_off);
	lv2_atom_forge_pop(&ui->forge, &frame);
	ui->write(ui->controller, 0, lv2_atom_total_size(msg), ui->uris.atom_eventTransfer, msg);
}

static void ui_enable(LV2UI_Handle handle)
{
	TunaUI* ui = (TunaUI*)handle;
	uint8_t obj_buf[64];
	if (!ui->spectr_enable) return;
	lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 64);
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_frame_time(&ui->forge, 0);
	LV2_Atom* msg = (LV2_Atom*)x_forge_object(&ui->forge, &frame, 1, ui->uris.ui_on);
	lv2_atom_forge_pop(&ui->forge, &frame);
	ui->write(ui->controller, 0, lv2_atom_total_size(msg), ui->uris.atom_eventTransfer, msg);
}

static void
size_request(RobWidget* handle, int *w, int *h) {
	//TunaUI* ui = (TunaUI*)GET_HANDLE(handle);
	*w = DAWIDTH;
	*h = DAHEIGHT;
}

static RobWidget * toplevel(TunaUI* ui, void * const top)
{
	ui->hbox = rob_hbox_new(FALSE, 2);
	robwidget_make_toplevel(ui->hbox, top);
	ROBWIDGET_SETNAME(ui->hbox, "tuna");

	ui->darea = robwidget_new(ui);
	robwidget_set_alignment(ui->darea, 0, 0);
	robwidget_set_expose_event(ui->darea, expose_event);
	robwidget_set_size_request(ui->darea, size_request);

	ui->xyp = robtk_xydraw_new(DAWIDTH, DAHEIGHT);
	//ui->xyp->rw->position_set = plot_position_right;
	robtk_xydraw_set_alignment(ui->xyp, 0, 0);
	robtk_xydraw_set_linewidth(ui->xyp, 1.5);
	robtk_xydraw_set_drawing_mode(ui->xyp, RobTkXY_ymax_zline);
	robtk_xydraw_set_mapping(ui->xyp, 1./1500., 0, 1./92., 1.0);
	robtk_xydraw_set_area(ui->xyp, 10, 10, DAWIDTH - 20, DAHEIGHT -20.5);
	robtk_xydraw_set_clip_callback(ui->xyp, xy_clip_fn, ui);
	robtk_xydraw_set_color(ui->xyp, 1.0, .0, .2, 1.0);

	ui->ctable = rob_table_new(/*rows*/3, /*cols*/ 2, FALSE);

	ui->btnbox  = rob_hbox_new(FALSE, 2);
	ui->disp[0] = robtk_rbtn_new("Tuning", NULL);
	ui->disp[1] = robtk_rbtn_new("Debug", robtk_rbtn_group(ui->disp[0]));

	ui->sep[0] = robtk_sep_new(TRUE);
	ui->sep[1] = robtk_sep_new(TRUE);
	ui->sep[2] = robtk_sep_new(TRUE);
	robtk_sep_set_linewidth(ui->sep[0], 0);
	robtk_sep_set_linewidth(ui->sep[1], 0);
	robtk_sep_set_linewidth(ui->sep[2], 1);

	ui->spb_tuning = robtk_spin_new(220, 880, .5);
	ui->spb_octave = robtk_spin_new(-1, 10, 1);
	ui->spb_freq   = robtk_spin_new(20, 1000, .5); // TODO log-map
	ui->sel_mode   = robtk_select_new();
	ui->sel_note   = robtk_select_new();

	ui->spb_debug[0] = robtk_spin_new(-100, 0, .5);
	ui->spb_debug[1] = robtk_spin_new(-50, 0, .5);
	ui->spb_debug[2] = robtk_spin_new(-50, 10, .5);
	ui->spb_debug[3] = robtk_spin_new(  0, 40, .5);
	ui->spb_debug[4] = robtk_spin_new(  0, 60, .5);
	ui->spb_debug[5] = robtk_spin_new(-100, 0, .5);
	ui->spb_debug[6] = robtk_spin_new(-100, 0, .5);

	ui->lbl_debug[0] = robtk_lbl_new("Threshold (abs)");
	ui->lbl_debug[1] = robtk_lbl_new("Threshold (rms)");
	ui->lbl_debug[2] = robtk_lbl_new("Fund. (rms)");
	ui->lbl_debug[3] = robtk_lbl_new("1st over (fund)");
	ui->lbl_debug[4] = robtk_lbl_new("1st cut  (fund)");
	ui->lbl_debug[5] = robtk_lbl_new("Filt 1st (fund)");
	ui->lbl_debug[6] = robtk_lbl_new("Filt Nth (n-1)");

#define SPIN_DFTNVAL(SPB, VAL) \
	robtk_spin_set_default(SPB, VAL); \
	robtk_spin_set_value(SPB, VAL);

	SPIN_DFTNVAL(ui->spb_debug[0], -75)
	SPIN_DFTNVAL(ui->spb_debug[1], -45)
	SPIN_DFTNVAL(ui->spb_debug[2], -40)
	SPIN_DFTNVAL(ui->spb_debug[3],  20)
	SPIN_DFTNVAL(ui->spb_debug[4],   5)
	SPIN_DFTNVAL(ui->spb_debug[5], -30)
	SPIN_DFTNVAL(ui->spb_debug[6], -15)

	robtk_select_add_item(ui->sel_mode,  0 , "Auto");
	robtk_select_add_item(ui->sel_mode,  1 , "Freq");
	robtk_select_add_item(ui->sel_mode,  2 , "Note");

	robtk_select_add_item(ui->sel_note,  0 , "C");
	robtk_select_add_item(ui->sel_note,  1 , "C#");
	robtk_select_add_item(ui->sel_note,  2 , "D");
	robtk_select_add_item(ui->sel_note,  3 , "D#");
	robtk_select_add_item(ui->sel_note,  4 , "E");
	robtk_select_add_item(ui->sel_note,  5 , "F");
	robtk_select_add_item(ui->sel_note,  6 , "F#");
	robtk_select_add_item(ui->sel_note,  7 , "G");
	robtk_select_add_item(ui->sel_note,  8 , "G#");
	robtk_select_add_item(ui->sel_note,  9 , "A");
	robtk_select_add_item(ui->sel_note, 10 , "A#");
	robtk_select_add_item(ui->sel_note, 11 , "B");

	ui->label[0] = robtk_lbl_new("Tuning");
	ui->label[1] = robtk_lbl_new("Octave");
	ui->label[2] = robtk_lbl_new("Note");
	ui->label[3] = robtk_lbl_new("Freq");

	/* default values */
	robtk_spin_set_default(ui->spb_tuning, 440);
	robtk_spin_set_value(ui->spb_tuning, 440);
	robtk_spin_set_default(ui->spb_freq, 440);
	robtk_spin_set_value(ui->spb_freq, 440);
	robtk_spin_set_default(ui->spb_octave, 4);
	robtk_spin_set_value(ui->spb_octave, 4);
	robtk_select_set_default_item(ui->sel_note, 9);
	robtk_select_set_item(ui->sel_note, 9);
	robtk_select_set_default_item(ui->sel_mode, 0);
	robtk_select_set_item(ui->sel_mode, 0);

	robtk_select_set_sensitive(ui->sel_note, false);
	robtk_spin_set_sensitive(ui->spb_octave, false);
	robtk_spin_set_sensitive(ui->spb_freq,   false);

	/* layout alignments */
	robtk_spin_set_alignment(ui->spb_octave, 0, 0.5);
	robtk_spin_label_width(ui->spb_octave, -1, 20);
	robtk_spin_set_label_pos(ui->spb_octave, 2);
	robtk_spin_set_alignment(ui->spb_tuning, 1.0, 0.5);
	robtk_spin_label_width(ui->spb_tuning, -1, 0);
	robtk_spin_set_label_pos(ui->spb_tuning, 2);
	robtk_spin_set_alignment(ui->spb_freq, 0, 0.5);
	robtk_spin_label_width(ui->spb_freq, -1, 0);
	robtk_spin_set_label_pos(ui->spb_freq, 2);
	robtk_select_set_alignment(ui->sel_note, 0, .5);
	robtk_lbl_set_alignment(ui->label[1], 0, .5);
	robtk_lbl_set_alignment(ui->label[2], 0, .5);
	robtk_lbl_set_alignment(ui->label[3], 0, .5);

	for (uint32_t i = 0; i < 2; ++i) {
		rob_hbox_child_pack(ui->btnbox, robtk_rbtn_widget(ui->disp[i]), FALSE, FALSE);
		robtk_rbtn_set_callback(ui->disp[i], cb_disp_changed, ui);
	}
	/* table layout */
	int row = 0;
#define TBLADDSS(WIDGET, X0, X1, Y0, Y1) \
	rob_table_attach(ui->ctable, WIDGET, X0, X1, Y0, Y1, 2, 2, RTK_SHRINK, RTK_SHRINK);

#define TBLADDES(WIDGET, X0, X1, Y0, Y1) \
	rob_table_attach(ui->ctable, WIDGET, X0, X1, Y0, Y1, 2, 2, RTK_EXANDF, RTK_SHRINK);

	if (ui->spectr_enable) {
		TBLADDES(ui->btnbox, 0, 2, row, row+1); row++;

		rob_table_attach(ui->ctable, robtk_sep_widget(ui->sep[0])
				, 0, 2, row, row+1, 2, 2, RTK_SHRINK, RTK_EXANDF);
		row++;

		for (uint32_t i = 0; i < 7; ++i) {
			robtk_spin_set_alignment(ui->spb_debug[i], 0, 0.5);
			robtk_lbl_set_alignment(ui->lbl_debug[i], 0, 0.5);
			TBLADDES(robtk_lbl_widget(ui->lbl_debug[i]), 0, 1, row, row+1);
			TBLADDES(robtk_spin_widget(ui->spb_debug[i]), 1, 2, row, row+1);
#ifdef GTK_BACKEND
			gtk_widget_set_no_show_all(ui->spb_debug[i]->rw->c, true);
			gtk_widget_set_no_show_all(ui->lbl_debug[i]->rw->c, true);
#endif
			row++;
			robtk_spin_set_callback(ui->spb_debug[i], cb_set_debug, ui);
		}
	}

	TBLADDSS(robtk_lbl_widget(ui->label[0]), 0, 1, row, row+1);
	TBLADDSS(robtk_spin_widget(ui->spb_tuning), 1, 2, row, row+1);
	row++;

	rob_table_attach(ui->ctable, robtk_sep_widget(ui->sep[1])
			, 0, 2, row, row+1, 2, 2, RTK_SHRINK, RTK_EXANDF);
	row++;

	TBLADDES(robtk_select_widget(ui->sel_mode), 0, 2, row, row+1);
	row++;

	TBLADDSS(robtk_lbl_widget(ui->label[2]), 0, 1, row, row+1);
	TBLADDES(robtk_select_widget(ui->sel_note), 1, 2, row, row+1);
	row++;

	TBLADDSS(robtk_lbl_widget(ui->label[1]), 0, 1, row, row+1);
	TBLADDSS(robtk_spin_widget(ui->spb_octave), 1, 2, row, row+1);
	row++;

	TBLADDES(robtk_sep_widget(ui->sep[2]), 1, 2, row, row+1);
	row++;
	TBLADDSS(robtk_lbl_widget(ui->label[3]), 0, 1, row, row+1);
	TBLADDSS(robtk_spin_widget(ui->spb_freq), 1, 2, row, row+1);

	/* global layout */
	rob_hbox_child_pack(ui->hbox, ui->darea, FALSE, FALSE);
	if (ui->spectr_enable) {
		rob_hbox_child_pack(ui->hbox, robtk_xydraw_widget(ui->xyp), FALSE, FALSE);
	}
	rob_hbox_child_pack(ui->hbox, ui->ctable, FALSE, FALSE);

	/* signal callbacks */
	robtk_select_set_callback(ui->sel_mode, cb_set_mode, ui);
	robtk_select_set_callback(ui->sel_note, cb_set_mode, ui);
	robtk_spin_set_callback(ui->spb_freq, cb_set_mode, ui);
	robtk_spin_set_callback(ui->spb_octave, cb_set_mode, ui);
	robtk_spin_set_callback(ui->spb_tuning, cb_set_tuning, ui);

	/* misc */
	ui->font[0] = pango_font_description_from_string("Mono 18px");
	ui->font[1] = pango_font_description_from_string("Sans 14px");
	ui->font[2] = pango_font_description_from_string("Mono 56px");
	ui->font[3] = pango_font_description_from_string("Mono 11px");

	if (ui->spectr_enable) {
		robtk_rbtn_set_active(ui->disp[0], TRUE);
		cb_disp_changed(NULL, ui);
	}
	return ui->hbox;
}

/******************************************************************************
 * LV2
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
	TunaUI* ui = (TunaUI*)calloc(1, sizeof(TunaUI));

	if (!ui) {
		fprintf(stderr, "Tuna.lv2 UI: out of memory\n");
		return NULL;
	}

	ui->strobe_dpy = 0;
	ui->strobe_tme = 0;
	ui->strobe_phase = 0;
	ui->s_rms = 0;
	ui->s_cent = 0;
	ui->s_error = 0;

	*widget = NULL;

	if (!strncmp(plugin_uri, TUNA_URI "one", 31 + 3 )) {
		ui->spectr_enable = false;
	} else if (!strncmp(plugin_uri, TUNA_URI "two", 31 + 3 )) {
		ui->spectr_enable = true;
	} else {
		free(ui);
		return NULL;
	}

	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
			ui->map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!ui->map) {
		fprintf(stderr, "Tuna.lv2 UI: Host does not support urid:map\n");
		free(ui);
		return NULL;
	}

	/* initialize private data structure */
	ui->write      = write_function;
	ui->controller = controller;
	map_tuna_uris(ui->map, &ui->uris);
	lv2_atom_forge_init(&ui->forge, ui->map);
	ui->nfo = robtk_info(ui_toplevel);

	*widget = toplevel(ui, ui_toplevel);
	render_frontface(ui);
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
	TunaUI* ui = (TunaUI*)handle;
	ui_disable(ui);

	robwidget_destroy(ui->darea);
	robtk_xydraw_set_surface(ui->xyp, NULL);
	cairo_surface_destroy (ui->spect_ann);
	robtk_xydraw_destroy(ui->xyp);

	for (uint32_t i = 0; i < 2; ++i) {
		robtk_sep_destroy(ui->sep[i]);
	}
	for (uint32_t i = 0; i < 4; ++i) {
		robtk_lbl_destroy(ui->label[i]);
	}
	for (uint32_t i = 0; i < 7; ++i) {
		robtk_spin_destroy(ui->spb_debug[i]);
		robtk_lbl_destroy(ui->lbl_debug[i]);
	}
	for (uint32_t i = 0; i < 2; ++i) {
		robtk_rbtn_destroy(ui->disp[i]);
	}

	robtk_spin_destroy(ui->spb_tuning);
	robtk_spin_destroy(ui->spb_octave);
	robtk_spin_destroy(ui->spb_freq);
	robtk_select_destroy(ui->sel_note);
	robtk_select_destroy(ui->sel_mode);

	rob_box_destroy(ui->btnbox);
	rob_box_destroy(ui->hbox);

	cairo_surface_destroy(ui->frontface);
	cairo_pattern_destroy(ui->meterpattern);

	for (uint32_t i = 0; i < 4; ++i) {
		pango_font_description_free(ui->font[i]);
	}
	free(ui);
}

static void
port_event(LV2UI_Handle handle,
		uint32_t     port_index,
		uint32_t     buffer_size,
		uint32_t     format,
		const void*  buffer)
{
	TunaUI* ui = (TunaUI*)handle;

	LV2_Atom* atom = (LV2_Atom*)buffer;
	if (format == ui->uris.atom_eventTransfer
			&& (atom->type == ui->uris.atom_Blank || atom->type == ui->uris.atom_Object)
		 ) {
		LV2_Atom_Object* obj = (LV2_Atom_Object*)atom;
		LV2_Atom *a0 = NULL;
		LV2_Atom *a1 = NULL;
		if (
				obj->body.otype == ui->uris.spectrum
				&& 2 == lv2_atom_object_get(obj, ui->uris.spec_data_x, &a0, ui->uris.spec_data_y, &a1, NULL)
				&& a0 && a1
				&& a0->type == ui->uris.atom_Vector
				&& a1->type == ui->uris.atom_Vector
			 )
		{
			LV2_Atom_Vector* vof_x = (LV2_Atom_Vector*)LV2_ATOM_BODY(a0);
			LV2_Atom_Vector* vof_y = (LV2_Atom_Vector*)LV2_ATOM_BODY(a1);
			if (vof_x->atom.type == ui->uris.atom_Float
					&& vof_y->atom.type == ui->uris.atom_Float)
			{
				const size_t n_elem = (a0->size - sizeof(LV2_Atom_Vector_Body)) / vof_x->atom.size;
				const float *px = (float*) LV2_ATOM_BODY(&vof_x->atom);
				const float *py = (float*) LV2_ATOM_BODY(&vof_y->atom);
				robtk_xydraw_set_points(ui->xyp, n_elem, px, py);
			}
		}
		return;
	}

	if (format != 0) return;
	const float v = *(float *)buffer;
	switch (port_index) {
		/* I/O ports */
		case TUNA_TUNING:
			ui->disable_signals = true;
			robtk_spin_set_value(ui->spb_tuning, v);
			ui->disable_signals = false;
			break;
		case TUNA_MODE:
			ui->disable_signals = true;
			if (v > 0 && v <= 10000) {
				robtk_select_set_item(ui->sel_mode, 1);
				robtk_spin_set_value(ui->spb_freq, v);
			} else if (v <= -1 && v >= -128) {
				robtk_select_set_item(ui->sel_mode, 2);
				int note = -1 - v;
				robtk_spin_set_value(ui->spb_octave, (note / 12) -1);
				robtk_select_set_item(ui->sel_note, note % 12);
			} else {
				robtk_select_set_item(ui->sel_mode, 0);
			}
			ui->disable_signals = false;
			break;

		/* input ports */
		case TUNA_OCTAVE:   ui->p_octave = v; break;
		case TUNA_NOTE:     ui->p_note = MAX(0, MIN(11,v)); break;
		case TUNA_FREQ_OUT: ui->p_freq = v;
			if (v <= 0) ui->s_cent = 0;
			break;
		case TUNA_CENT:     ui->p_cent = v;
				ui->s_cent += .4 * (v - ui->s_cent) + 1e-12;
			break;
		case TUNA_RMS:      ui->p_rms = v;
			if (ui->p_rms < -90) {
				ui->s_rms = -90;
			} else {
				ui->s_rms += .3 * (v - ui->s_rms) + 1e-12;
			}
			if (ui->strobe_tme == 0 && v > -100) {
				queue_draw(ui->darea);
			}
			break;

		case TUNA_ERROR:    ui->p_error = v;
			if (ui->p_error==0) {
				ui->s_error = 0;
			} else {
				ui->s_error += .03 * (v - ui->s_error) + 1e-12;
			}
			break;

		case TUNA_STROBE:
			ui->strobe_tme = v;
			queue_draw(ui->darea);
			break;

		case TUNA_T_RMS:
			ui->disable_signals = true;
			robtk_spin_set_value(ui->spb_debug[0], v);
			ui->disable_signals = false;
			break;
		case TUNA_T_FLT:
			ui->disable_signals = true;
			robtk_spin_set_value(ui->spb_debug[1], v);
			ui->disable_signals = false;
			break;
		case TUNA_T_FFT:
			ui->disable_signals = true;
			robtk_spin_set_value(ui->spb_debug[2], v);
			ui->disable_signals = false;
			break;
		case TUNA_T_OVR:
			ui->disable_signals = true;
			robtk_spin_set_value(ui->spb_debug[3], v);
			ui->disable_signals = false;
			break;
		case TUNA_T_FUN:
			ui->disable_signals = true;
			robtk_spin_set_value(ui->spb_debug[4], v);
			ui->disable_signals = false;
			break;
		case TUNA_T_OCT:
			ui->disable_signals = true;
			robtk_spin_set_value(ui->spb_debug[5], v);
			ui->disable_signals = false;
			break;
		case TUNA_T_OVT:
			ui->disable_signals = true;
			robtk_spin_set_value(ui->spb_debug[6], v);
			ui->disable_signals = false;
			break;
		default:
			return;
	}
}

static const void*
extension_data(const char* uri)
{
	return NULL;
}

/* vi:set ts=2 sts=2 sw=2: */
