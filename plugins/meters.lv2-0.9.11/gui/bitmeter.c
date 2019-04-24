/* bitmeter LV2 GUI
 *
 * Copyright 2015 Robin Gareus <robin@gareus.org>
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

#define LVGL_RESIZEABLE

#define RTK_URI "http://gareus.org/oss/lv2/meters#"
#define RTK_GUI "bitmeterui"

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "src/uris.h"

#ifdef _WIN32
#define snprintf(s, l, ...) sprintf(s, __VA_ARGS__)
#endif

/*************************/
enum {
	FONT_M = 0,
	FONT_S,
	FONT_LAST,
};

typedef struct {
	LV2_Atom_Forge forge;

	LV2_URID_Map* map;
	EBULV2URIs   uris;

	LV2UI_Write_Function write;
	LV2UI_Controller     controller;

	RobWidget* vbox;
	RobWidget* box_btn;
	RobWidget* tbl_nfo;
	RobWidget* m0;

	RobTkCBtn* btn_freeze;
	RobTkCBtn* btn_avg;
	RobTkPBtn* btn_reset;

	RobTkLbl* lbl_desc[6];
	RobTkLbl* lbl_data[6];

	bool fontcache;
	PangoFontDescription *font[2];
	cairo_surface_t* m0_faceplate;

	bool disable_signals;

	/* current data */
	uint64_t integration_spl;
	int flt[BIM_LAST];
	int stats[3]; // nan, inf, den
	float sig[2]; // min, max

	int f_zero, f_pos;

	float rate;
	const char *nfo;
} BITui;


static void initialize_font_cache (BITui* ui) {
	ui->fontcache = true;
	ui->font[FONT_M] = pango_font_description_from_string ("Mono 9px");
	ui->font[FONT_S] = pango_font_description_from_string ("Sans 11px");
}

/******************************************************************************
 * Format Numerics
 */

static void format_num (char *buf, const int num) {
	if (num >= 1000000000) {
		sprintf (buf, "%.0fM", num / 1000000.f);
	} else if (num >= 100000000) {
		sprintf (buf, "%.1fM", num / 1000000.f);
	} else if (num >= 10000000) {
		sprintf (buf, "%.2fM", num / 1000000.f);
	} else if (num >= 100000) {
		sprintf (buf, "%.0fK", num / 1000.f);
	} else if (num >= 10000) {
		sprintf (buf, "%.1fK", num / 1000.f);
	} else {
		sprintf (buf, "%d", num);
	}
}

static void format_duration (char *buf, const float sec) {
	if (sec < 10) {
		sprintf (buf, "%.2f\"", sec);
	} else if (sec < 60) {
		sprintf (buf, "%.1f\"", sec);
	} else if (sec < 600) {
		int minutes = sec / 60;
		int seconds = ((int)floorf (sec)) % 60;
		int ds = 10 * (sec - seconds - 60 * minutes);
		sprintf (buf, "%d'%02d\"%d", minutes, seconds, ds);
	} else if (sec < 3600) {
		int minutes = sec / 60;
		int seconds = ((int)floorf (sec)) % 60;
		sprintf (buf, "%d'%02d\"", minutes, seconds);
	} else {
		int hours = sec / 3600;
		int minutes = ((int)floorf (sec / 60)) % 60;
		sprintf (buf, "%dh%02d'", hours, minutes);
	}
}

static void update_time (BITui* ui, uint64_t tme) {
	if (ui->integration_spl == tme) {
		return;
	}
	ui->integration_spl = tme;

	char buf[64];
	if (ui->integration_spl < .1 * ui->rate) {
		snprintf (buf, 64, "%u [spl]", (unsigned int) ui->integration_spl);
	} else {
		format_duration (buf, ui->integration_spl / ui->rate);
	}
	robtk_lbl_set_text (ui->lbl_data[3], buf);
}

static void update_oops (BITui* ui, int which, int32_t val) {
	assert (which >= 0 || which <= 3);
	if (ui->stats[which] == val) {
		return;
	}

	if (ui->stats[which] > 0 && val == 0) {
		float col[4];
		get_color_from_theme(0, col);
		robtk_lbl_set_color (ui->lbl_data[which], col[0], col[1], col[2], col[3]);
	} else if (ui->stats[which] == 0 && val > 0) {
		robtk_lbl_set_color (ui->lbl_data[which], 1.0, .2, .2, 1.0);
	}

	ui->stats[which] = val;
	char buf[32];
	format_num (buf, val);
	robtk_lbl_set_text (ui->lbl_data[which], buf);
}

static void update_minmax (BITui* ui, int which, float val) {
	assert (which == 0 || which == 1);
	if (ui->sig[which] == val) {
		return;
	}
	ui->sig[which] = val;
	char buf[32];
	if (val == INFINITY || val <= 0) {
		sprintf(buf, "N/A");
	} else {
		snprintf(buf, 32, "%.1f dBFS", 20.f * log10f (val));
	}
	robtk_lbl_set_text (ui->lbl_data[4 + which], buf);
}

/******************************************************************************
 * drawing function
 */

#define FONT(A) ui->font[(A)]

static void gen_faceplate (BITui* ui, const int ww, const int hh) {
	assert(!ui->m0_faceplate);
	ui->m0_faceplate = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ww, hh);
	cairo_t* cr = cairo_create (ui->m0_faceplate);

	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	const int spc = (int) floorf ((ww - 28) / 28.) & ~1;
	const int rad = ceilf (spc * .75);
	const int mid = rint (spc * .75 * .5);
	const int x0r = rint (ww * .5 + 12 * spc);
	const int xpr = rint (ww * .5 - 13 * spc);

	const int spc_s = (int) floorf (ww / 45.) & ~1;
	const int rad_s = ceilf (spc_s * .75);
	const int x0r_s  = ww * .5 + 20 * spc_s;

	const int y0   = hh - 60 - rad_s - spc;
	const int y0_s = hh - 20 - rad_s;

	const int y0_g = 10;
	const int y1_g = y0 - 4;
	const int yh_g = y1_g - y0_g;

	// grid & annotations -- TODO statically allocate surface
	const float x0_g = xpr - 2;
	const float x1_b = x0r + rad + 2;
	const float x1_g = x1_b + mid + 2;
	const float yc_g = rintf (y0_g + .5 * yh_g);
	const float y6_g = rintf (y0_g +  yh_g * 2. / 3.);
	const float y3_g = rintf (y0_g +  yh_g / 3.);

	cairo_rectangle (cr, x1_b, y0_g, mid, y3_g);
	cairo_set_source_rgba (cr, .8, .5, .1, 1.0);
	cairo_fill (cr);
	cairo_rectangle (cr, x1_b, y3_g, mid, y6_g - y3_g);
	cairo_set_source_rgba (cr, .1, .9, .1, 1.0);
	cairo_fill (cr);
	cairo_rectangle (cr, x1_b, y6_g, mid, y1_g - y6_g);
	cairo_set_source_rgba (cr, .1, .6, .9, 1.0);
	cairo_fill (cr);

	cairo_set_line_width (cr, 2);
	cairo_move_to (cr, x1_b, y0_g);
	cairo_line_to (cr, x1_b + mid, y0_g);
	cairo_set_source_rgba (cr, .9, .0, .0, 1.0);
	cairo_stroke (cr);
	cairo_move_to (cr, x1_b, y0_g + yh_g);
	cairo_line_to (cr, x1_b + mid, y0_g + yh_g);
	cairo_set_source_rgba (cr, .0, .0, .9, 1.0);
	cairo_stroke (cr);

	CairoSetSouerceRGBA(c_g80);
	cairo_set_line_width (cr, 1);

	cairo_save (cr);
	double dash = 1;
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
	cairo_set_dash (cr, &dash, 1, 0);

	cairo_move_to (cr, x0_g, y0_g - .5);
	cairo_line_to (cr, x1_b, y0_g - .5);
	cairo_stroke (cr);

	cairo_move_to (cr, x0_g, .5 + yc_g);
	cairo_line_to (cr, x0_g + spc + 4, .5 + yc_g);
	cairo_stroke (cr);

	cairo_move_to (cr, x0_g, .5 + y6_g);
	cairo_line_to (cr, x1_b, .5 + y6_g);
	cairo_stroke (cr);

	cairo_move_to (cr, x0_g, .5 + y3_g);
	cairo_line_to (cr, x1_b, .5 + y3_g);
	cairo_stroke (cr);

	cairo_move_to (cr, x0_g, y1_g + .5);
	cairo_line_to (cr, x1_b, y1_g + .5);
	cairo_stroke (cr);

	cairo_restore (cr);

	cairo_move_to (cr, 1.5 + rintf (x0r_s - 33 * spc_s), y0_s - 1.5);
	cairo_line_to (cr, 1.5 + rintf (x0r_s - 33 * spc_s), y0_s + rad_s + 3.5);
	cairo_line_to (cr, .5 + rintf (x0r_s - 35.5 * spc_s), y0_s + rad_s + 3.5);
	cairo_stroke (cr);


	write_text_full (cr, ">1.0",
				FONT(FONT_M), x0r_s - 33.0 * spc_s, hh - 2, 0, 4, c_wht);
	write_text_full (cr, "<markup>2<small><sup>-32</sup></small></markup>",
				FONT(FONT_M), x0r_s + 0.5 * spc_s, hh - 2, 0, 5, c_wht);
	write_text_full (cr, "<markup>2<small><sup>-24</sup></small></markup>",
				FONT(FONT_M), x0r_s - 8.0 * spc_s, hh - 2, 0, 5, c_wht);
	write_text_full (cr, "<markup>2<small><sup>-16</sup></small></markup>",
				FONT(FONT_M), x0r_s - 16.5 * spc_s, hh - 2, 0, 5, c_wht);
	write_text_full (cr, "<markup>2<small><sup>-8</sup></small></markup>",
				FONT(FONT_M), x0r_s - 25.0 * spc_s, hh - 2, 0, 5, c_wht);
	write_text_full (cr, "<markup>2<small><sup>7</sup></small></markup>",
				FONT(FONT_M), x0r_s - 40.5 * spc_s, hh - 2, 0, 5, c_wht);

	write_text_full (cr, "% time bit is set",
				FONT(FONT_M), x1_g, yc_g, -.5 * M_PI, 8, c_wht);

	write_text_full (cr, "100%", FONT(FONT_M), x0_g - 2, y0_g, 0, 1, c_wht);
	write_text_full (cr, "50%", FONT(FONT_M), x0_g - 2, yc_g, 0, 1, c_wht);
	write_text_full (cr, "0%", FONT(FONT_M), x0_g - 2, y1_g, 0, 1, c_wht);

	// sep
	int ysep = .5 * (y0 + rad + y0_s);
	CairoSetSouerceRGBA(c_g60);
	cairo_move_to (cr, 15, ysep + .5);
	cairo_line_to (cr, ww - 30 , ysep + .5);
	cairo_stroke (cr);
	write_text_full (cr, "Sign & Mantissa (23bit significand)", FONT(FONT_S), ww * .5, ysep - 2, 0, 5, c_wht);
	write_text_full (cr, "Full Scale", FONT(FONT_S), ww * .5, ysep + 3, 0, 8, c_wht);

	write_text_full (cr, ui->nfo, FONT(FONT_M), 2, hh -2, 1.5 * M_PI, 9, c_gry);

	cairo_destroy (cr);
}

static bool draw_bit_box (BITui* ui, cairo_t* cr, float x0, float y0, float rd, int hit, int set) {
	const int scnt = ui->integration_spl;
	if ( (hit < 0 && scnt == ui->f_zero) || (hit == 0)) {
		cairo_set_source_rgba (cr, .5, .5, .5, 1.0);
	} else if (set == 0) {
		cairo_set_source_rgba (cr, .0, .0, .9, 1.0);
	} else if (set == scnt) {
		cairo_set_source_rgba (cr, .9, .0, .0, 1.0);
	} else if (3.f * set / (float)scnt > 2.f) {
		cairo_set_source_rgba (cr, .8, .5, .1, 1.0);
	} else if (3.f * set / (float)scnt < 1.f) {
		cairo_set_source_rgba (cr, .1, .6, .9, 1.0);
	} else {
		cairo_set_source_rgba (cr, .1, .9, .1, 1.0);
	}

	cairo_rectangle (cr, x0 + .5, y0 + .5, rd, rd);
	cairo_fill_preserve (cr);

	CairoSetSouerceRGBA(c_blk);
	cairo_set_line_width (cr, 1);
	cairo_stroke (cr);

	return TRUE;
}

static void draw_bit_dist (cairo_t* cr, float x0, float y0, float w, float h, float val) {
	if (val < 0) {
		cairo_rectangle (cr, x0 + 2, y0, w - 3, h);
		CairoSetSouerceRGBA(c_gry);
		cairo_fill (cr);
	} else {
		cairo_rectangle (cr, x0 + 2, y0, w - 3, h * (1. - val));
		CairoSetSouerceRGBA(c_g20);
		cairo_fill (cr);
		cairo_rectangle (cr, x0 + 2, y0 + h, w - 3, -h * val);
		CairoSetSouerceRGBA(c_wht);
		cairo_fill (cr);
	}
}

static bool expose_event (RobWidget* rw, cairo_t* cr, cairo_rectangle_t *ev) {
	BITui* ui = (BITui*)GET_HANDLE(rw);
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	const int ww = rw->area.width;
	const int hh = rw->area.height;

	CairoSetSouerceRGBA(c_g30);
	cairo_rectangle (cr, 0, 0, ww, hh);
	cairo_fill (cr);

	if (!ui->m0_faceplate) {
		gen_faceplate (ui, ww, hh);
	}

	const int spc = (int) floorf ((ww - 28) / 28.) & ~1;
	const int rad = ceilf (spc * .75);
	const int x0r = rint (ww * .5 + 12 * spc);
	const int xpr = rint (ww * .5 - 13 * spc);

	const int spc_s = (int) floorf (ww / 45.) & ~1;
	const int rad_s = ceilf (spc_s * .75);
	const int x0r_s  = ww * .5 + 20 * spc_s;

	const int y0   = hh - 60 - rad_s - spc;
	const int y0_s = hh - 20 - rad_s;

	const int y0_g = 10;
	const int y1_g = y0 - 4;
	const int yh_g = y1_g - y0_g;

	// draw distribution
	if ((int)ui->integration_spl == ui->f_zero) { // all blank
		draw_bit_dist (cr, xpr, y0_g, rad, yh_g, -1);
		for (int k = 0; k < 23; ++k) {
			const float xp = x0r - rintf (spc * (.5 * (k / 8) + k));
			draw_bit_dist (cr, xp, y0_g, rad, yh_g, -1);
		}
	} else {
		const float scnt = ui->integration_spl;
		draw_bit_dist (cr, xpr, y0_g, rad, yh_g, ui->f_pos / scnt);
		for (int k = 0; k < 23; ++k) {
			const float xp = x0r - rintf (spc * (.5 * (k / 8) + k));
			const float v = ui->flt[k + BIM_DSET] / scnt;
			draw_bit_dist (cr, xp, y0_g, rad, yh_g, v);
		}
	}

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(cr, ui->m0_faceplate, 0, 0);
	cairo_paint (cr);

	/* quick glance boxes */
	// sign - bit
	draw_bit_box (ui, cr, xpr, y0, rad, -1, ui->f_pos);

	// "+" sign
	const float mid = .5 + rintf (spc * .75 * .5);
	const float span = ceilf (spc * .75 * .2);
	CairoSetSouerceRGBA(c_wht);
	cairo_set_line_width (cr, 1);
	cairo_move_to (cr, xpr + mid, y0 + mid - span);
	cairo_line_to (cr, xpr + mid, y0 + mid + span);
	cairo_stroke (cr);
	cairo_move_to (cr, xpr + mid - span, y0 + mid);
	cairo_line_to (cr, xpr + mid + span, y0 + mid);
	cairo_stroke (cr);

	// mantissa
	for (int k = 0; k < 23; ++k) {
		const float xp = x0r - rintf (spc * (.5 * (k / 8) + k));
		draw_bit_box (ui, cr, xp, y0, rad, -1, ui->flt[k + BIM_DSET]);
	}

	// magnitude
	for (int k = 0; k < 40; ++k) {
		const int o = k + 118;
		const float xp = .5 * (k / 8) + k;
		draw_bit_box (ui, cr, x0r_s - rintf (xp * spc_s), y0_s, rad_s, ui->flt[o + BIM_DHIT], ui->flt[o + BIM_DONE]);
	}

	if (ui->integration_spl == 0) {
		cairo_set_source_rgba (cr, 0, 0, 0, .6);
		cairo_rectangle (cr, 0, 0, ww, hh);
		cairo_fill (cr);
		write_text_full (cr, "<markup><b>No data available.</b></markup>",
				FONT(FONT_S), rintf(ww * .5f), rintf(hh * .5f), 0, 2, c_wht);
	}
	else if (ui->integration_spl >= 2147483647) {
		cairo_set_source_rgba (cr, .9, .9, .9, .5);
		cairo_rectangle (cr, 0, 0, ww, hh);
		cairo_fill (cr);
		write_text_full (cr, "<markup>Reached <b>2<sup><small>31</small></sup> sample limit.\nData acquisition suspended.</b></markup>",
				FONT(FONT_S), rintf(ww * .5f), rintf(hh * .5f), 0, 2, c_blk);
	} else if ((int)ui->integration_spl == ui->f_zero) { // all blank
		write_text_full (cr, "<markup><b>All samples are zero.</b></markup>",
				FONT(FONT_S), rintf(ww * .5f), rintf(y0_g + yh_g * .5f), 0, 2, c_wht);
	}
	return TRUE;
}

/******************************************************************************
 * LV2 UI -> plugin communication
 */

static void forge_message_kv (BITui* ui, LV2_URID uri, int key, float value) {
	uint8_t obj_buf[1024];
	if (ui->disable_signals) return;

	lv2_atom_forge_set_buffer (&ui->forge, obj_buf, 1024);
	LV2_Atom* msg = forge_kvcontrolmessage (&ui->forge, &ui->uris, uri, key, value);
	ui->write (ui->controller, 0, lv2_atom_total_size (msg), ui->uris.atom_eventTransfer, msg);
}

/******************************************************************************
 * UI callbacks
 */

static bool cb_btn_freeze (RobWidget *w, void* handle) {
	BITui* ui = (BITui*)handle;
	if (robtk_cbtn_get_active (ui->btn_freeze)) {
		forge_message_kv (ui, ui->uris.mtr_meters_cfg, CTL_PAUSE, 0);
	} else {
		forge_message_kv (ui, ui->uris.mtr_meters_cfg, CTL_START, 0);
	}
	return TRUE;
}

static bool cb_btn_avg (RobWidget *w, void* handle) {
	BITui* ui = (BITui*)handle;
	if (robtk_cbtn_get_active (ui->btn_avg)) {
		forge_message_kv (ui, ui->uris.mtr_meters_cfg, CTL_AVERAGE, 0);
	} else {
		forge_message_kv (ui, ui->uris.mtr_meters_cfg, CTL_WINDOWED, 0);
	}
	return TRUE;
}

static bool cb_btn_reset (RobWidget *w, void* handle) {
	BITui* ui = (BITui*)handle;
	forge_message_kv (ui, ui->uris.mtr_meters_cfg, CTL_RESET, 0);
	return TRUE;
}

static void btn_start_sens (BITui* ui) {
	if (ui->integration_spl >= 2147483647) {
		robtk_cbtn_set_sensitive (ui->btn_freeze, false);
	} else {
		robtk_cbtn_set_sensitive (ui->btn_freeze, true);
	}
}


/******************************************************************************
 * widget hackery
 */

static void
size_request (RobWidget* handle, int *w, int *h) {
	*w = 480;
	*h = 200;
}

static void
size_default(RobWidget* rw, int *w, int *h) {
	*w = 480;
	*h = 396;
}

static void
m0_size_allocate (RobWidget* rw, int w, int h) {
	BITui* ui = (BITui*)GET_HANDLE(rw);
	robwidget_set_size (rw, w, h);
	if (ui->m0_faceplate) {
		cairo_surface_destroy (ui->m0_faceplate);
		ui->m0_faceplate = NULL;
	}
	queue_draw (rw);
}


///////////////////////////////////////////////////////////////////////////////

static RobWidget * toplevel (BITui* ui, void * const top) {
	ui->vbox = rob_vbox_new (FALSE, 2);
	robwidget_make_toplevel (ui->vbox, top);
	ROBWIDGET_SETNAME(ui->vbox, "bitmeter");

	/* main drawing area */
	ui->m0 = robwidget_new (ui);
	ROBWIDGET_SETNAME (ui->m0, "sigco (m0)");
	robwidget_set_alignment (ui->m0, .5, .5);
	robwidget_set_expose_event (ui->m0, expose_event);
	robwidget_set_size_request (ui->m0, size_request);
	robwidget_set_size_allocate (ui->m0, m0_size_allocate);
	robwidget_set_size_default (ui->vbox, size_default);

	/* info - box */
	ui->tbl_nfo = rob_table_new (/*rows*/3, /*cols*/5, FALSE);
	ui->lbl_desc[0] = robtk_lbl_new ("Not a number:");
	ui->lbl_desc[1] = robtk_lbl_new ("Infinity:");
	ui->lbl_desc[2] = robtk_lbl_new ("Denormal:");
	ui->lbl_desc[3] = robtk_lbl_new ("Time window:");
	ui->lbl_desc[4] = robtk_lbl_new ("Smallest non zero sample:");
	ui->lbl_desc[5] = robtk_lbl_new ("Largest sample:");

	for (int i = 0; i < 6; ++i) {
		ui->lbl_data[i] = robtk_lbl_new ("XXXXXXXXXX");
		robtk_lbl_set_alignment(ui->lbl_desc[i], 1, .5);
		robtk_lbl_set_alignment(ui->lbl_data[i], 0, .5);
	}

#define GLB_W(PTR) robtk_lbl_widget(PTR)
	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_desc[3]), 0, 1, 0, 1);
	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_data[3]), 1, 2, 0, 1);
	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_desc[4]), 0, 1, 1, 2);
	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_data[4]), 1, 2, 1, 2);
	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_desc[5]), 0, 1, 2, 3);
	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_data[5]), 1, 2, 2, 3);

	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_desc[0]), 2, 3, 0, 1);
	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_data[0]), 3, 4, 0, 1);
	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_desc[1]), 2, 3, 1, 2);
	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_data[1]), 3, 4, 1, 2);
	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_desc[2]), 2, 3, 2, 3);
	rob_table_attach_defaults (ui->tbl_nfo, GLB_W(ui->lbl_data[2]), 3, 4, 2, 3);

	/* controls */
	ui->box_btn = rob_hbox_new (TRUE, 2);
	ui->btn_freeze = robtk_cbtn_new ("Freeze", GBT_LED_OFF, false);
	ui->btn_avg = robtk_cbtn_new ("Integrate", GBT_LED_OFF, false);
	ui->btn_reset = robtk_pbtn_new ("Reset");
	robtk_cbtn_set_color_checked (ui->btn_freeze, .8, .1, .1);

	robtk_pbtn_set_alignment (ui->btn_reset, 0.5, 0.5);
	robtk_cbtn_set_alignment (ui->btn_avg, 0.5, 0.5);
	robtk_cbtn_set_alignment (ui->btn_freeze, 0.5, 0.5);

	/* button packing */
	rob_hbox_child_pack (ui->box_btn, robtk_cbtn_widget (ui->btn_freeze), TRUE, TRUE);
	rob_hbox_child_pack (ui->box_btn, robtk_cbtn_widget (ui->btn_avg), TRUE, TRUE);
	rob_hbox_child_pack (ui->box_btn, robtk_pbtn_widget (ui->btn_reset), TRUE, TRUE);

	/* global packing */
	rob_vbox_child_pack (ui->vbox, ui->tbl_nfo, FALSE, TRUE);
	rob_vbox_child_pack (ui->vbox, ui->m0, TRUE, TRUE);
	rob_vbox_child_pack (ui->vbox, ui->box_btn, FALSE, TRUE);

	/* signals */
	robtk_cbtn_set_callback (ui->btn_freeze, cb_btn_freeze, ui);
	robtk_cbtn_set_callback (ui->btn_avg, cb_btn_avg, ui);
	robtk_pbtn_set_callback_up (ui->btn_reset, cb_btn_reset, ui);

	initialize_font_cache (ui);

	update_minmax (ui, 0, 0);
	update_minmax (ui, 1, 0);
	update_oops (ui, 0, 0);
	update_oops (ui, 1, 0);
	update_oops (ui, 2, 0);

	return ui->vbox;
}

static void gui_cleanup (BITui* ui) {
	if (ui->fontcache) {
		for (int i=0; i < FONT_LAST; ++i) {
			pango_font_description_free (ui->font[i]);
		}
	}

	if (ui->m0_faceplate) {
		cairo_surface_destroy (ui->m0_faceplate);
	}

	for (int i = 0; i < 6; ++i) {
		robtk_lbl_destroy (ui->lbl_desc[i]);
		robtk_lbl_destroy (ui->lbl_data[i]);
	}

	robtk_cbtn_destroy (ui->btn_freeze);
	robtk_cbtn_destroy (ui->btn_avg);
	robtk_pbtn_destroy (ui->btn_reset);

	robwidget_destroy (ui->m0);
	rob_table_destroy (ui->tbl_nfo);
	rob_box_destroy (ui->box_btn);
	rob_box_destroy (ui->vbox);

}

/******************************************************************************
 * RobTk + LV2
 */

static void ui_enable (LV2UI_Handle handle) {
	BITui* ui = (BITui*)handle;
	forge_message_kv (ui, ui->uris.mtr_meters_on, 0, 0); // may be too early
}

static void ui_disable (LV2UI_Handle handle) {
	BITui* ui = (BITui*)handle;
	forge_message_kv (ui, ui->uris.mtr_meters_off, 0, 0);
}

static LV2UI_Handle instantiate (
		void* const               ui_toplevel,
		const LV2UI_Descriptor*   descriptor,
		const char*               plugin_uri,
		const char*               bundle_path,
		LV2UI_Write_Function      write_function,
		LV2UI_Controller          controller,
		RobWidget**               widget,
		const LV2_Feature* const* features)
{
	BITui* ui = (BITui*)calloc (1,sizeof (BITui));
	if (!ui) { return NULL; }

	ui->write      = write_function;
	ui->controller = controller;

	for (int i = 0; features[i]; ++i) {
		if (!strcmp (features[i]->URI, LV2_URID_URI "#map")) {
			ui->map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!ui->map) {
		fprintf (stderr, "UI: Host does not support urid:map\n");
		free (ui);
		return NULL;
	}

	ui->nfo = robtk_info(ui_toplevel);
	ui->rate = 48000;
	ui->integration_spl = 0;
	ui->stats[0] = ui->stats[1] = ui->stats[2] = -1;
	ui->sig[0] = ui->sig[1] = -1;

	map_eburlv2_uris (ui->map, &ui->uris);
	lv2_atom_forge_init (&ui->forge, ui->map);

	*widget = toplevel (ui, ui_toplevel);

	ui_enable (ui);
	return ui;
}

static enum LVGLResize
plugin_scale_mode (LV2UI_Handle handle)
{
	return LVGL_LAYOUT_TO_FIT;
}

static void
cleanup (LV2UI_Handle handle)
{
	BITui* ui = (BITui*)handle;
	ui_disable (handle);
	gui_cleanup (ui);
	free (ui);
}

static const void*
extension_data (const char* uri)
{
	return NULL;
}

/******************************************************************************
 * handle data from backend
 */

#define PARSE_A_INT(var, dest) \
	if (var && var->type == uris->atom_Int) { \
		dest = ((LV2_Atom_Int*)var)->body; \
	}

#define CB_INT(var, FN, PM) \
	if (var && var->type == uris->atom_Int) { \
		FN(ui, PM, ((LV2_Atom_Int*)var)->body); \
	}

#define CB_DBL(var, FN, PM) \
	if (var && var->type == uris->atom_Double) { \
		FN(ui, PM, ((LV2_Atom_Double*)var)->body); \
	}

static void
port_event (LV2UI_Handle handle,
            uint32_t     port_index,
            uint32_t     buffer_size,
            uint32_t     format,
            const void*  buffer)
{
	BITui* ui = (BITui*)handle;
	const EBULV2URIs* uris = &ui->uris;

	if (format != uris->atom_eventTransfer) {
		return;
	}

	LV2_Atom* atom = (LV2_Atom*)buffer;
	if (atom->type != uris->atom_Blank && atom->type != uris->atom_Object) {
		fprintf (stderr, "UI: Unknown message type.\n");
		return;
	}

	LV2_Atom_Object* obj = (LV2_Atom_Object*)atom;

	if (obj->body.otype == uris->mtr_control) {
		int k; float v;
		get_cc_key_value (&ui->uris, obj, &k, &v);
		if (k == CTL_SAMPLERATE) {
			if (v > 0) {
				ui->rate = v;
			}
			queue_draw (ui->m0);
		}
	}

	else if (obj->body.otype == uris->bim_stats) {
		LV2_Atom *bcnt = NULL;
		LV2_Atom *bmin = NULL;
		LV2_Atom *bmax = NULL;
		LV2_Atom *bnan = NULL;
		LV2_Atom *binf = NULL;
		LV2_Atom *bden = NULL;
		LV2_Atom *bpos = NULL;
		LV2_Atom *bnul = NULL;
		LV2_Atom *bdat = NULL;

		if (9 == lv2_atom_object_get (obj,
					uris->ebu_integr_time, &bcnt,
					uris->bim_zero, &bnul,
					uris->bim_pos, &bpos,
					uris->bim_max, &bmax,
					uris->bim_min, &bmin,
					uris->bim_nan, &bnan,
					uris->bim_inf, &binf,
					uris->bim_den, &bden,
					uris->bim_data, &bdat,
					NULL)
				&& bcnt && bnul && bpos && bmin && bmax && bnan && binf && bden && bdat
				&& bcnt->type == uris->atom_Long
				&& bpos->type == uris->atom_Int
				&& bnul->type == uris->atom_Int
				&& bmin->type == uris->atom_Double
				&& bmax->type == uris->atom_Double
				&& bnan->type == uris->atom_Int
				&& binf->type == uris->atom_Int
				&& bden->type == uris->atom_Int
				&& bdat->type == uris->atom_Vector
				)
				{
					CB_INT(bnan, update_oops, 0);
					CB_INT(binf, update_oops, 1);
					CB_INT(bden, update_oops, 2);
					PARSE_A_INT(bpos, ui->f_pos);
					PARSE_A_INT(bnul, ui->f_zero);

					CB_DBL(bmin, update_minmax, 0);
					CB_DBL(bmax, update_minmax, 1);

					LV2_Atom_Vector* data = (LV2_Atom_Vector*)LV2_ATOM_BODY(bdat);
					if (data->atom.type == uris->atom_Int) {
						const size_t n_elem = (bdat->size - sizeof (LV2_Atom_Vector_Body)) / data->atom.size;
						assert (n_elem == BIM_LAST);
						const int32_t *d = (int32_t*) LV2_ATOM_BODY(&data->atom);
						memcpy (ui->flt, d, sizeof (int32_t) * n_elem);
					}

					update_time (ui, (uint64_t)(((LV2_Atom_Long*)bcnt)->body));
					btn_start_sens (ui); // maybe set 2^31 limit.
					queue_draw (ui->m0);
				}
	}

	else if (obj->body.otype == uris->bim_information) {

		LV2_Atom *ii = NULL;
		LV2_Atom *av = NULL;
		lv2_atom_object_get (obj,
				uris->ebu_integrating, &ii,
				uris->bim_averaging, &av,
				NULL);

		ui->disable_signals = true;
		if (ii && ii->type == uris->atom_Bool) {
			bool ix = ((LV2_Atom_Bool*)ii)->body;
			robtk_cbtn_set_active (ui->btn_freeze, !ix);
		}
		if (av && av->type == uris->atom_Bool) {
			bool ix = ((LV2_Atom_Bool*)av)->body;
			robtk_cbtn_set_active (ui->btn_avg, ix);
		}
		ui->disable_signals = false;
	}

	else  {
		fprintf (stderr, "UI: Unknown control message.\n");
	}
}
