/* robtk fat1 gui
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"

#include "../src/fat1.h"

#define RTK_URI FAT1_URI
#define RTK_GUI "#ui"

#ifndef MAX
#define MAX(A,B) ((A) > (B)) ? (A) : (B)
#endif

#ifndef MIN
#define MIN(A,B) ((A) < (B)) ? (A) : (B)
#endif

struct CtrlRange {
	float min;
	float max;
	float dflt;
	float step;
	float mult;
	bool  log;
	const char* name;
};

struct PianoKey {
	int x, w, h;
	bool white;
};

typedef struct {
	LV2UI_Write_Function write;
	LV2UI_Controller controller;
	LV2UI_Touch* touch;

	PangoFontDescription* font[2];

	RobWidget* rw; // top-level container
	RobWidget* ctbl; // control element table

	/* keybooard drawing area */
	RobWidget* m0;
	int m0_width;
	int m0_height;

	RobTkDial* spn_ctrl[5];
	RobTkLbl*  lbl_ctrl[5];

	RobTkPBtn*   btn_panic;
	RobTkLbl*    lbl_mode;
	RobTkLbl*    lbl_mchn;
	RobTkSelect* sel_mode;
	RobTkSelect* sel_mchn;

	cairo_surface_t* m0_bg;
	cairo_surface_t* dial_bg[5];

	PianoKey pk[12];

	int hover;
	bool disable_signals;

	uint32_t notes; // selected notes on scale (manual)
	uint32_t mask;
	uint32_t set;
	float    err;

	int key_note;
	int key_mod;
	int key_majmin;

	int tt_id;
	int tt_timeout;
	cairo_rectangle_t* tt_pos;

	bool microtonal;
	const char* nfo;
} Fat1UI;

const struct CtrlRange ctrl_range[] = {
	{ 400, 480, 440, 0.20,  5, false, "Tuning"},
	{ 0.0, 1.0, 0.5, 0.01,  2, false, "Bias"},
	{ 0.5, .02, 0.1, 200.,  5, true,  "Filter"},
	{ 0.0, 1.0, 1.0, 0.01,  2, false, "Corr."},
	{ -2., 2.0, 0.0, 0.01,  5, false, "Offset"},
};

static const char* tooltips[] = {
	"<markup><b>Tuning.</b> This sets the frequency corresponding to 'A'\n"
		"(440 Hz in most cases). The exact value is displayed when\n"
		"this control is touched, and can be set in steps of 0.2 Hz.\n</markup>",

	"<markup><b>Bias.</b> Normally the pitch estimator will select the enabled\n"
		"note closest to the measured pitch. The Bias control adds\n"
		"some preference for the current note - this allows it to go\n"
		"off-tune more than would be the case otherwise.\n</markup>",

	"<markup><b>Filter.</b> This sets the amount of smoothing on the pitch\n"
		"correction while the current note does not change. If it\n"
		"does change the filter is bypassed and the correction\n"
		"jumps immediately to the new value.\n</markup>",

	"<markup><b>Correction.</b> Determines how much of the estimated\n"
		"pitch error gets corrected. Full correction may remove\n"
		"expression or vibrato.\n</markup>",

	"<markup><b>Offset.</b> Adds an offset in the range of +/- two\n"
		"semitones to the pitch correction. With the Correction\n"
		"control set to zero the result is a constant pitch change.\n</markup>"
};

float ctrl_to_gui (const uint32_t c, const float v) {
	if (!ctrl_range[c].log) { return v; }
	const float r = logf (ctrl_range[c].max / ctrl_range[c].min);
	return rintf (ctrl_range[c].step / r * (logf (v / ctrl_range[c].min)));
}

float gui_to_ctrl (const uint32_t c, const float v) {
	if (!ctrl_range[c].log) { return v; }
	const float r = log (ctrl_range[c].max / ctrl_range[c].min);
	return expf (logf (ctrl_range[c].min) + v * r / ctrl_range[c].step);
}

float k_min (const uint32_t c) {
	if (!ctrl_range[c].log) { return ctrl_range[c].min; }
	return 0;
}

float k_max (const uint32_t c) {
	if (!ctrl_range[c].log) { return ctrl_range[c].max; }
	return ctrl_range[c].step;
}

float k_step (const uint32_t c) {
	if (!ctrl_range[c].log) { return ctrl_range[c].step; }
	return 1;
}

///////////////////////////////////////////////////////////////////////////////

static const float c_dlf[4] = {0.8, 0.8, 0.8, 1.0}; // dial faceplate fg

///////////////////////////////////////////////////////////////////////////////

/*** knob faceplates ***/
static void prepare_faceplates (Fat1UI* ui) {
	cairo_t* cr;
	float xlp, ylp;

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

#define RESPLABLEL(V) \
	{ \
	DIALDOTS(V, 4.5, 15.5) \
	xlp = rint (GED_CX + 4.5 + sinf (ang) * (GED_RADIUS + 9.5)); \
	ylp = rint (GED_CY + 15.5 - cosf (ang) * (GED_RADIUS + 9.5)); \
	}

	INIT_DIAL_SF(ui->dial_bg[0], GED_WIDTH + 8, GED_HEIGHT + 20);

	RESPLABLEL(0.00);
	write_text_full(cr, "400", ui->font[0], xlp+6, ylp, 0, 1, c_dlf);
	RESPLABLEL(.16);
	RESPLABLEL(.33);
	RESPLABLEL(0.5);
	write_text_full(cr, "440", ui->font[0], xlp,   ylp, 0, 2, c_dlf);
	RESPLABLEL(.66);
	RESPLABLEL(.83);
	RESPLABLEL(1.0);
	write_text_full(cr, "480", ui->font[0], xlp-6, ylp, 0, 3, c_dlf);
	cairo_destroy (cr);

	INIT_DIAL_SF(ui->dial_bg[1], GED_WIDTH + 8, GED_HEIGHT + 20);
	RESPLABLEL(0.00);
	write_text_full(cr,   "0", ui->font[0], xlp+2, ylp,  0, 1, c_dlf);
	RESPLABLEL(0.25);
	RESPLABLEL(0.5);
	write_text_full(cr, "0.5", ui->font[0], xlp,   ylp,  0, 2, c_dlf);
	RESPLABLEL(0.75);
	RESPLABLEL(1.0);
	write_text_full(cr,   "1", ui->font[0], xlp-2, ylp,  0, 3, c_dlf);
	cairo_destroy (cr);

	INIT_DIAL_SF(ui->dial_bg[2], GED_WIDTH + 8, GED_HEIGHT + 20);
	RESPLABLEL(0.00);
	write_text_full(cr, "Slow", ui->font[0], xlp+6, ylp,  0, 1, c_dlf);
	RESPLABLEL(0.25);
	RESPLABLEL(0.5);
	write_text_full(cr, "Med",  ui->font[0], xlp,   ylp,  0, 2, c_dlf);
	RESPLABLEL(0.75);
	RESPLABLEL(1.0);
	write_text_full(cr, "Fast", ui->font[0], xlp-6, ylp,  0, 3, c_dlf);
	cairo_destroy (cr);

	INIT_DIAL_SF(ui->dial_bg[3], GED_WIDTH + 8, GED_HEIGHT + 20);
	RESPLABLEL(0.00);
	write_text_full(cr,   "0", ui->font[0], xlp+2, ylp,  0, 1, c_dlf);
	RESPLABLEL(.16);
	RESPLABLEL(.33);
	RESPLABLEL(0.5);
	write_text_full(cr, "0.5", ui->font[0], xlp,   ylp,  0, 2, c_dlf);
	RESPLABLEL(.66);
	RESPLABLEL(.83);
	RESPLABLEL(1.0);
	write_text_full(cr,   "1", ui->font[0], xlp-2, ylp,  0, 3, c_dlf);
	cairo_destroy (cr);

	INIT_DIAL_SF(ui->dial_bg[4], GED_WIDTH + 8, GED_HEIGHT + 20);
	RESPLABLEL(0.00);
	write_text_full(cr, "-2", ui->font[0], xlp+2, ylp,  0, 1, c_dlf);
	RESPLABLEL(.16);
	RESPLABLEL(.33);
	RESPLABLEL(0.5);
	write_text_full(cr,  "0", ui->font[0], xlp,   ylp,  0, 2, c_dlf);
	RESPLABLEL(.66);
	RESPLABLEL(.83);
	RESPLABLEL(1.0);
	write_text_full(cr, "+2", ui->font[0], xlp-2, ylp,  0, 3, c_dlf);
	cairo_destroy (cr);

#undef DIALDOTS
#undef INIT_DIAL_SF
#undef RESPLABLEL
}

static void display_annotation (Fat1UI*ui, RobTkDial* d, cairo_t* cr, const char* txt) {
	int tw, th;
	cairo_save (cr);
	PangoLayout* pl = pango_cairo_create_layout (cr);
	pango_layout_set_font_description (pl, ui->font[0]);
	pango_layout_set_text (pl, txt, -1);
	pango_layout_get_pixel_size (pl, &tw, &th);
	cairo_translate (cr, d->w_width / 2, d->w_height - 2);
	cairo_translate (cr, -tw / 2.0, -th);
	cairo_set_source_rgba (cr, .0, .0, .0, .7);
	rounded_rectangle (cr, -1, -1, tw+3, th+1, 3);
	cairo_fill (cr);
	CairoSetSouerceRGBA (c_wht);
	pango_cairo_show_layout (cr, pl);
	g_object_unref (pl);
	cairo_restore (cr);
	cairo_new_path (cr);
}

static void dial_annotation_hz (RobTkDial* d, cairo_t* cr, void* data) {
	Fat1UI* ui = (Fat1UI*) (data);
	char txt[16];
	snprintf (txt, 16, "%5.1f Hz", d->cur);
	display_annotation (ui, d, cr, txt);
}

static void dial_annotation_val (RobTkDial* d, cairo_t* cr, void* data) {
	Fat1UI* ui = (Fat1UI*) (data);
	char txt[16];
	snprintf (txt, 16, "%+5.0f ct", d->cur * 100.f);
	display_annotation (ui, d, cr, txt);
}

///////////////////////////////////////////////////////////////////////////////

/*** global tooltip overlay ****/

static bool tooltip_overlay (RobWidget* rw, cairo_t* cr, cairo_rectangle_t* ev) {
	Fat1UI* ui = (Fat1UI*)rw->top;
	assert (ui->tt_id >= 0 && ui->tt_id < 5);

	cairo_save(cr);
	rw->resized = TRUE;
	rcontainer_expose_event (rw, cr, ev);
	cairo_restore(cr);

	cairo_rectangle (cr, 0, 0, rw->area.width, ui->tt_pos->y + 1);
  cairo_set_source_rgba (cr, 0, 0, 0, .7);
	cairo_fill (cr);

	rounded_rectangle (cr, ui->tt_pos->x + 1, ui->tt_pos->y + 1,
			ui->tt_pos->width + 3, ui->tt_pos->height + 1, 3);
  cairo_set_source_rgba (cr, 1, 1, 1, .5);
	cairo_fill (cr);

	const float* color = c_wht;
	PangoFontDescription *font;
	font = pango_font_description_from_string("Sans 11px");

	const float xp = rw->area.width * .5;
	const float yp = rw->area.height * .5;

	cairo_save (cr);
	cairo_scale (cr, rw->widget_scale, rw->widget_scale);
	write_text_full (cr, tooltips[ui->tt_id], font,
			xp / rw->widget_scale, yp / rw->widget_scale,
			0,  2, color);
	cairo_restore (cr);

	pango_font_description_free (font);
	return TRUE;
}

static bool
tooltip_cnt (RobWidget* rw, cairo_t* cr, cairo_rectangle_t* ev)
{
	Fat1UI* ui = (Fat1UI*)rw->top;
	if (++ui->tt_timeout < 8) {
		rcontainer_expose_event (rw, cr, ev);
		queue_draw (rw);
	} else {
		rw->expose_event = tooltip_overlay;
		rw->resized      = TRUE;
		tooltip_overlay (rw, cr, ev);
	}
	return TRUE;
}


static void ttip_handler (RobTkLbl* d, bool on, void *handle) {
	Fat1UI* ui = (Fat1UI*)handle;
	ui->tt_id = -1;
	ui->tt_timeout = 0;
	for (int i = 0; i < 5; ++i) {
		if (d == ui->lbl_ctrl[i]) { ui->tt_id = i; break;}
	}
	if (on && ui->tt_id >= 0) {
		ui->tt_pos = &d->rw->area;
		ui->ctbl->expose_event = tooltip_cnt;
		ui->ctbl->resized = TRUE;
		queue_draw (ui->ctbl);
	} else {
		ui->ctbl->expose_event = rcontainer_expose_event;
		ui->ctbl->parent->resized = TRUE; //full re-expose
		queue_draw (ui->rw);
	}
}

///////////////////////////////////////////////////////////////////////////////

/*** knob & button callbacks ****/

static bool cb_spn_ctrl (RobWidget* w, void* handle) {
	Fat1UI* ui = (Fat1UI*)handle;
	if (ui->disable_signals) return TRUE;

	for (uint32_t i = 0; i < 5; ++i) {
		if (w != ui->spn_ctrl[i]->rw) {
			continue;
		}
		const float val = gui_to_ctrl (i, robtk_dial_get_value (ui->spn_ctrl[i]));
		ui->write (ui->controller, FAT_TUNE + i, sizeof (float), 0, (const void*) &val);
		break;
	}
	return TRUE;
}

static bool cb_mchn (RobWidget* w, void* handle) {
	Fat1UI* ui = (Fat1UI*)handle;
	if (ui->disable_signals) return TRUE;
	const float val = robtk_select_get_value (ui->sel_mchn);
	ui->write (ui->controller, FAT_MCHN, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

static bool cb_mode (RobWidget* w, void* handle) {
	Fat1UI* ui = (Fat1UI*)handle;
	const float val = robtk_select_get_value (ui->sel_mode);
	robtk_select_set_sensitive (ui->sel_mchn, val != 2);
	robtk_pbtn_set_sensitive (ui->btn_panic, val != 2);
	if (ui->disable_signals) return TRUE;
	ui->write (ui->controller, FAT_MODE, sizeof (float), 0, (const void*) &val);
	queue_draw (ui->m0);
	return TRUE;
}

static bool cb_btn_panic (RobWidget *w, void* handle) {
	Fat1UI* ui = (Fat1UI*)handle;
	float val = robtk_pbtn_get_pushed (ui->btn_panic) ? 1.f : 0.f;
	ui->write (ui->controller, FAT_PANIC, sizeof (float), 0, (const void*) &val);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

/*** scale overlay display ****/
static void keysel_apply (Fat1UI* ui)
{
	if (ui->touch) {
		for (uint32_t n = 0; n < 12; ++n) {
			ui->touch->touch (ui->touch->handle, FAT_NOTE + n, true);
		}
	}

	int k = 0;
	switch (ui->key_note) {
		case 0: k = 12; break;
		case 1: k = 10; break;
		case 2: k = 8; break;
		case 3: k = 7; break;
		case 4: k = 5; break;
		case 5: k = 3; break;
		case 6: k = 1; break;
		case 7:
			for (uint32_t n = 0; n < 12; ++n) {
				float val = 1.0;
				ui->write (ui->controller, FAT_NOTE + n, sizeof (float), 0, (const void*) &val);
			}
			if (ui->touch) {
				for (uint32_t n = 0; n < 12; ++n) {
					ui->touch->touch (ui->touch->handle, FAT_NOTE + n, false);
				}
			}
			return;
		default:
			return;
	}

	if (ui->key_mod == 1) { k += 11; }
	if (ui->key_mod == 2) { ++k; }
	if (ui->key_majmin == 1) { k += 9; }

	static const float western[12] = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 };

	for (uint32_t n = 0; n < 12; ++n) {
		float val = western [ (n + k) % 12 ];
		ui->write (ui->controller, FAT_NOTE + n, sizeof (float), 0, (const void*) &val);
	}
	if (ui->touch) {
		for (uint32_t n = 0; n < 12; ++n) {
			ui->touch->touch (ui->touch->handle, FAT_NOTE + n, false);
		}
	}
}

static bool keysel_overlay (RobWidget* rw, cairo_t* cr, cairo_rectangle_t* ev) {
	Fat1UI* ui = (Fat1UI*)rw->top;
	cairo_save(cr);
	rw->resized = TRUE;
	rcontainer_expose_event (rw, cr, ev);
	cairo_restore(cr);

	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
  cairo_set_source_rgba (cr, 0, 0, 0, .7);
	cairo_fill (cr);

	const int nbtn_col = 8;
	const int nbtn_row = 2;
	float bt_w = rw->area.width / (float)(nbtn_col * 1.5 + .5);
	float bt_h = rw->area.height / (float)(nbtn_row * 2 + 1);

	PangoFontDescription *font;
	static const char scale[16][8] = {
		"C", "D", "E", "F", "G", "A", "B", "All",
		"" , "\u266F", "\u266D",  "", "maj", "min", "", "X"
	};

	font = pango_font_description_from_string("Sans 12px");
	for (int y = 0; y < nbtn_row; ++y) {
		for (int x = 0; x < nbtn_col; ++x) {
			int pos = x + y * nbtn_col;

			if (strlen (scale[pos]) == 0) {
				continue;
			}

			float x0 = floor ((.5 + 1.5 * x) * bt_w);
			float y0 = floor ((1 + 2 * y) * bt_h);

			rounded_rectangle (cr, x0, y0, floor (bt_w), floor (bt_h), 8);
			CairoSetSouerceRGBA(c_wht);
			cairo_set_line_width(cr, 1.5);
			cairo_stroke_preserve (cr);

			const float* color = c_wht;

			if (pos < 8 && pos == ui->key_note) {
				cairo_set_source_rgba (cr, .0, .8, .0, 1.0);
			}
			else if ((pos == 9 && ui->key_mod & 1) || (pos == 10 && ui->key_mod & 2)) {
				cairo_set_source_rgba (cr, .8, .6, .0, 1.0);
			}
			else if ((pos == 12 && ui->key_majmin == 0) || (pos == 13 && ui->key_majmin == 1)) {
				cairo_set_source_rgba (cr, .1, .1, .8, 1.0);
			}
			else if (pos == 7)  {
				cairo_set_source_rgba (cr, .3, .0, .3, 1.0);
			}
			else if (pos == 15)  {
				cairo_set_source_rgba (cr, .9, .9, .9, 1.0);
				color = c_blk;
			}
			else {
				cairo_set_source_rgba (cr, .2, .2, .2, 1.0);
			}

			cairo_fill (cr);
			cairo_save (cr);

			cairo_scale (cr, rw->widget_scale, rw->widget_scale);
			write_text_full (cr, scale[pos], font,
					floor(x0 + bt_w * .5) / rw->widget_scale,
					floor(y0 + bt_h * .5) / rw->widget_scale,
					0, 2, color);
			cairo_restore (cr);
		}
	}
	pango_font_description_free (font);

	return TRUE;
}

static int keysel_click (RobWidget* rw, RobTkBtnEvent *ev) {
	Fat1UI* ui = (Fat1UI*)rw->top;
	const int nbtn_col = 8;
	const int nbtn_row = 2;
	float bt_w = rw->area.width / (float)(nbtn_col * 3 + 1);
	float bt_h = rw->area.height / (float)(nbtn_row * 2 + 1);

	int xp = floor (ev->x / bt_w);
	int yp = floor (ev->y / bt_h);
	if ((xp % 3) == 0 || (yp & 1) == 0) {
		return 0;
	}
	const int pos = (xp - 1) / 3 + nbtn_col * (yp - 1) / 2;
	if (pos < 0 || pos >= nbtn_col * nbtn_row) {
		return 0;
	}

	if (pos >= 0 && pos < 8) {
		ui->key_note = pos;
		return 1;
	}
	else if (pos == 9) {
		ui->key_mod ^= 1;
		ui->key_mod &= 1;
		return 1;
	}
	else if (pos == 10) {
		ui->key_mod ^= 2;
		ui->key_mod &= 2;
		return 1;
	}
	else if (pos == 12) {
		ui->key_majmin = 0;
		return 1;
	}
	else if (pos == 13) {
		ui->key_majmin = 1;
		return 1;
	}
	else if (pos == 15) {
		return -1;
	}
	return 0;
}

static void keysel_toggle (Fat1UI* ui) {
	if (ui->ctbl->block_events) {
		ui->ctbl->block_events = FALSE;
		ui->ctbl->expose_event = rcontainer_expose_event;
		ui->ctbl->parent->resized = TRUE; //full re-expose
		queue_draw (ui->rw);
	} else {
		ui->ctbl->expose_event = keysel_overlay;
		ui->ctbl->block_events = TRUE;
		ui->ctbl->resized = TRUE;
		ui->key_note = -1;
		ui->key_mod = 0;
		ui->key_majmin = 0;
		queue_draw (ui->ctbl);
	}
}

static RobWidget* keysel_mousedown (RobWidget* rw, RobTkBtnEvent *ev) {
	if (rw->block_events) {
		Fat1UI* ui = (Fat1UI*)rw->top;
		const int rv = (ev->button == 1) ? keysel_click (rw, ev) : 0;

		if (rv == 1) {
			keysel_apply (ui);
			queue_draw (ui->ctbl);
		}
		else if (rv) {
			keysel_toggle (ui);
		}
	}
	return rcontainer_mousedown (rw, ev);
}

///////////////////////////////////////////////////////////////////////////////

/*** keyboard display ****/

static void
calc_keys (Fat1UI* ui)
{
	const int max_w0 = floor (.75 * (ui->m0_height - 10) / 4);
	const int max_w1 = floor ((ui->m0_width - 8) / 7);

	const int key_width = MIN (max_w0, max_w1);
	const int black_key_width = rint (.8 * key_width);

	const int height = 4 * key_width;
	const int piano_width = 7 * key_width;
	const int margin = (ui->m0_width - piano_width) / 2;

	int white_key = 0;
	for (uint32_t n = 0; n < 12; ++n) {
		if (n == 1 || n == 3 || n == 6 || n == 8 || n == 10) {
			ui->pk[n].x = margin + white_key * key_width - black_key_width / 2;
			ui->pk[n].w = black_key_width;
			ui->pk[n].h = height / 1.7;
			ui->pk[n].white = 0;
			continue;
		}

		ui->pk[n].x = margin + white_key * key_width;
		ui->pk[n].w = key_width;
		ui->pk[n].h = height;
		ui->pk[n].white = 1;

		++white_key;
	}
}

static void
draw_key (Fat1UI* ui, cairo_t* cr, int n)
{
	const int white = ui->pk[n].white;
	bool masked = ui->mask & (1 << n);

	if (masked) {
		if (white) {
			cairo_set_source_rgb (cr, 1.0f, 1.0f, 1.0f);
		} else {
			cairo_set_source_rgb (cr, 0.0f, 0.0f, 0.0f);
		}
	} else {
		if (white) {
			cairo_set_source_rgb (cr, 0.40f, 0.40f, 0.40f);
		} else {
			cairo_set_source_rgb (cr, 0.35f, 0.35f, 0.35f);
		}
	}

	cairo_set_line_width (cr, 1.0);
	cairo_rectangle (cr, ui->pk[n].x, 5, ui->pk[n].w, ui->pk[n].h);
	cairo_fill_preserve (cr);

	if (n == ui->hover && robtk_select_get_value (ui->sel_mode) != 1) {
		if (white && masked) {
			cairo_set_source_rgba (cr, .5, .5, .5, .3);
		} else {
			cairo_set_source_rgba (cr, 1, 1, 1, .3);
		}
		cairo_fill_preserve (cr);
	}
#if 0 // highlight key
	if (ui->set & (1 << n)) {
		cairo_set_source_rgba (cr, .7, .7, .0, .8);
		cairo_fill_preserve (cr);
	}
	cairo_set_source_rgb(cr, 0.0f, 0.0f, 0.0f);
	cairo_stroke (cr);
#else // draw dot
	cairo_set_source_rgb(cr, 0.0f, 0.0f, 0.0f);
	cairo_stroke (cr);
	if (ui->set & (1 << n)) {
		const float rad = ui->pk[1].w * .44;
		cairo_arc (cr, ui->pk[n].x + .5 * ui->pk[n].w, ui->pk[n].h * .95 - rad, rad, 0, 2 * M_PI);
		cairo_set_source_rgba (cr, .5, .5, .5, .5);
		cairo_stroke_preserve (cr);
		cairo_set_source_rgba (cr, .2, .8, .2, .95);
		cairo_fill (cr);
	}
#endif
}


static void
m0_size_request (RobWidget* handle, int* w, int* h) {
	*w = 120;
	*h = 100;
}

static void
m0_size_allocate (RobWidget* handle, int w, int h) {
	Fat1UI* ui = (Fat1UI*)GET_HANDLE (handle);
	ui->m0_width = w;
	ui->m0_height = h;
	robwidget_set_size (ui->m0, w, h);
	calc_keys (ui);
	queue_draw (ui->m0);
}

static RobWidget* m0_mouse_up (RobWidget* handle, RobTkBtnEvent* ev) {
	Fat1UI* ui = (Fat1UI*)GET_HANDLE (handle);
	if (ev->button != 1) {
		return NULL;
	}
	if (ui->disable_signals) {
		return NULL;
	}
	if (robtk_select_get_value (ui->sel_mode) == 1) {
		return NULL;
	}
	const int n = ui->hover;
	if (n < 0 || n >= 12) {
		return NULL;
	}

	float val;
	if (ui->notes & (1 << n)) {
		val = 0.f;
		ui->notes &= ~(1 << n);
	} else {
		val = 1.f;
		ui->notes |= (1 << n);
	}
	ui->write (ui->controller, FAT_NOTE + n, sizeof (float), 0, (const void*) &val);

	if (ui->touch) {
		ui->touch->touch (ui->touch->handle, FAT_NOTE + n, false);
	}

	queue_draw (ui->m0);
	if (ui->ctbl->block_events) {
		ui->key_note = -1;
		queue_draw (ui->ctbl);
	}
	return NULL;
}

static void m0_leave (RobWidget* handle) {
	Fat1UI* ui = (Fat1UI*)GET_HANDLE (handle);
	if (ui->hover == -1) {
		return;
	}
	ui->hover = -1;
	queue_draw (ui->m0);
}

static RobWidget* m0_mouse_down (RobWidget* handle, RobTkBtnEvent* ev) {
	Fat1UI* ui = (Fat1UI*)GET_HANDLE (handle);
	if (ev->button == 1) {
		const int n = ui->hover;
		if (n >= 0 && n < 12 && ui->touch) {
			ui->touch->touch (ui->touch->handle, FAT_NOTE + n, true);
		}
		return handle;
	}
	if (ev->button == 3 && robtk_select_get_value (ui->sel_mode) != 1) {
		keysel_toggle (ui);
	}
	return NULL;
}

static RobWidget* m0_mouse_move (RobWidget* handle, RobTkBtnEvent* ev) {
	Fat1UI* ui = (Fat1UI*)GET_HANDLE (handle);

	int hover = -1;
	for (uint32_t n = 0; n < 12; ++n) {
		if (ui->pk[n].x <= ev->x && ui->pk[n].x + ui->pk[n].w > ev->x &&
				5 <= ev->y && 5 + ui->pk[n].h > ev->y) {
			hover = n;
			break;
		}
	}
	if (ui->hover != hover) {
		ui->hover = hover;
		queue_draw (ui->m0);
	}

	return handle;
}

/*** main keyboard drawing function ***/
static bool m0_expose_event (RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	Fat1UI* ui = (Fat1UI*)GET_HANDLE (handle);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip_preserve (cr);

  float c[4];
  get_color_from_theme(1, c);
  cairo_set_source_rgb (cr, c[0], c[1], c[2]);
	cairo_fill (cr);

	/* white keys below */
	for (uint32_t n = 0; n < 12; ++n) {
		if (ui->pk[n].white) {
			draw_key (ui, cr, n);
		}
	}
	/* black keys on top */
	for (uint32_t n = 0; n < 12; ++n) {
		if (!ui->pk[n].white) {
			draw_key (ui, cr, n);
		}
	}

	float by0 = rint (ui->m0_height * .85);
	float bh  = rint (ui->m0_height * .09);
	int bw = rint (bh / 3.6);
	bw |= 1 ; // odd number

	// TODO cache grid & labels onto an image surface
	rounded_rectangle (cr, 8, by0, ui->m0_width - 16, bh, 3);
	CairoSetSouerceRGBA (c_g20);
	cairo_fill (cr);

#define XPOS(V) rintf (12. + (ui->m0_width - 24.) * ((V) + 1.) * .5)

	cairo_save (cr);
	rounded_rectangle (cr, 8, by0, ui->m0_width - 16, bh, 3);
	cairo_clip (cr);

	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0, ui->m0_width, 0.0);
	cairo_rectangle (cr, 0, by0, ui->m0_width, bh);
	cairo_pattern_add_color_stop_rgba (pat, 0.00,  1.0, 0.0, 0.0, 0.2);
	cairo_pattern_add_color_stop_rgba (pat, 0.40,  0.7, 0.6, 0.1, 0.2);
	cairo_pattern_add_color_stop_rgba (pat, 0.45,  0.0, 1.0, 0.0, 0.2);
	cairo_pattern_add_color_stop_rgba (pat, 0.55,  0.0, 1.0, 0.0, 0.2);
	cairo_pattern_add_color_stop_rgba (pat, 0.60,  0.7, 0.6, 0.1, 0.2);
	cairo_pattern_add_color_stop_rgba (pat, 1.00,  1.0, 0.0, 0.0, 0.2);
	cairo_set_source (cr, pat);
	cairo_fill (cr);
	cairo_pattern_destroy (pat);


	float ex = XPOS(ui->err) - (bw / 2) - 1;
	cairo_rectangle (cr, ex, by0, bw, bh);

	if (fabsf (ui->err) < .15) {
		cairo_set_source_rgba (cr, .1, 1., .1, 1.);
	} else if (fabsf (ui->err) < .5) {
		cairo_set_source_rgba (cr, .9, .9, .1, 1.);
	} else {
		cairo_set_source_rgba (cr, 1., .6, .2, 1.);
	}
	cairo_fill (cr);
	cairo_restore (cr);

	cairo_set_line_width(cr, 1.0);
	CairoSetSouerceRGBA (c_wht);

#define LINE(V) \
	ex = XPOS(V) - .5; \
	cairo_move_to (cr, ex, by0); \
	cairo_line_to (cr, ex, by0 + bh); \
	cairo_stroke (cr); \

	LINE(-1);
	write_text_full(cr, "-1", ui->font[0], ex-.5, by0, 0, 5, c_dlf);
	LINE(-.5);
	LINE(0);
	write_text_full(cr,  "0", ui->font[0], ex-.5, by0, 0, 5, c_dlf);
	LINE(.5);
	LINE(1);
	write_text_full(cr, "+1", ui->font[0], ex-.5, by0, 0, 5, c_dlf);

#undef LINE
#undef XPOS

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

static RobWidget* toplevel (Fat1UI* ui, void* const top) {
	/* main widget: layout */
	ui->rw = rob_hbox_new (FALSE, 2);
	robwidget_make_toplevel (ui->rw, top);
	robwidget_toplevel_enable_scaling (ui->rw);

	ui->font[0] = pango_font_description_from_string ("Mono 9px");
	ui->font[1] = pango_font_description_from_string ("Mono 10px");

	prepare_faceplates (ui);

	/* graph display */
	ui->m0 = robwidget_new (ui);
	robwidget_set_alignment (ui->m0, .5, .5);
	robwidget_set_expose_event (ui->m0, m0_expose_event);
	robwidget_set_size_request (ui->m0, m0_size_request);
	robwidget_set_size_allocate (ui->m0, m0_size_allocate);
	robwidget_set_mousemove (ui->m0, m0_mouse_move);
	robwidget_set_mouseup (ui->m0, m0_mouse_up);
	robwidget_set_mousedown (ui->m0, m0_mouse_down);
	robwidget_set_leave_notify(ui->m0, m0_leave);

	ui->ctbl = rob_table_new (/*rows*/3, /*cols*/ 7, FALSE);
	robwidget_set_mousedown (ui->ctbl, keysel_mousedown);
	ui->ctbl->top = (void*)ui;

#define GSP_W(PTR) robtk_dial_widget (PTR)
#define GLB_W(PTR) robtk_lbl_widget (PTR)
#define GSL_W(PTR) robtk_select_widget (PTR)

	for (uint32_t i = 0; i < 5; ++i) {
		ui->lbl_ctrl[i] = robtk_lbl_new (ctrl_range[i].name);
		ui->spn_ctrl[i] = robtk_dial_new_with_size (
				k_min (i), k_max (i), k_step(i),
				GED_WIDTH + 8, GED_HEIGHT + 20, GED_CX + 4, GED_CY + 15, GED_RADIUS);
		ui->spn_ctrl[i]->with_scroll_accel = false;

		robtk_dial_set_callback (ui->spn_ctrl[i], cb_spn_ctrl, ui);
		robtk_dial_set_value (ui->spn_ctrl[i], ctrl_to_gui (i, ctrl_range[i].dflt));
		robtk_dial_set_default (ui->spn_ctrl[i], ctrl_to_gui (i, ctrl_range[i].dflt));
		robtk_dial_set_detent_default (ui->spn_ctrl[i], true);
		robtk_dial_set_scroll_mult (ui->spn_ctrl[i], ctrl_range[i].mult);

		if (ui->touch) {
			robtk_dial_set_touch (ui->spn_ctrl[i], ui->touch->touch, ui->touch->handle, FAT_TUNE + i);
		}

		robtk_dial_set_scaled_surface_scale (ui->spn_ctrl[i], ui->dial_bg[i], 2.0);

		robtk_lbl_annotation_callback(ui->lbl_ctrl[i], ttip_handler, ui);
	
		rob_table_attach (ui->ctbl, GSP_W (ui->spn_ctrl[i]), i + 1, i + 2, 0, 4, 0, 0, RTK_EXANDF, RTK_SHRINK);
		rob_table_attach (ui->ctbl, GLB_W (ui->lbl_ctrl[i]), i + 1, i + 2, 4, 5, 0, 0, RTK_EXANDF, RTK_SHRINK);
	}

	robtk_dial_set_detent_default (ui->spn_ctrl[3], false);
	ui->spn_ctrl[1]->displaymode = 3; // use dots
	ui->spn_ctrl[2]->displaymode = 3;
	ui->spn_ctrl[3]->displaymode = 3;

	/* these numerics are meaningful */
	robtk_dial_annotation_callback (ui->spn_ctrl[0], dial_annotation_hz, ui);
	robtk_dial_annotation_callback (ui->spn_ctrl[4], dial_annotation_val, ui);

	/* some custom colors */
	{
		const float c_bg[4] = {.2, .2, .3, 1.0};
		create_dial_pattern (ui->spn_ctrl[0], c_bg);
	}
	{
		const float c_bg[4] = {.4, .4, .45, 1.0};
		create_dial_pattern (ui->spn_ctrl[1], c_bg);
		create_dial_pattern (ui->spn_ctrl[2], c_bg);
		// black dot
		ui->spn_ctrl[1]->dcol[0][0] =
			ui->spn_ctrl[1]->dcol[0][1] =
			ui->spn_ctrl[1]->dcol[0][2] =
			ui->spn_ctrl[2]->dcol[0][0] =
			ui->spn_ctrl[2]->dcol[0][1] =
			ui->spn_ctrl[2]->dcol[0][2] = .1;
	}
	{
		const float c_bg[4] = {.2, .7, .2, 1.0};
		create_dial_pattern (ui->spn_ctrl[3], c_bg);
		ui->spn_ctrl[3]->dcol[0][0] =
			ui->spn_ctrl[3]->dcol[0][1] =
			ui->spn_ctrl[3]->dcol[0][2] = .05;
	}
	{
		const float c_bg[4] = {.7, .7, .0, 1.0};
		create_dial_pattern (ui->spn_ctrl[4], c_bg);
		ui->spn_ctrl[4]->dcol[0][0] =
			ui->spn_ctrl[4]->dcol[0][1] =
			ui->spn_ctrl[4]->dcol[0][2] = .05;
	}

	/* mode + midi channel */
	ui->sel_mchn = robtk_select_new ();
	robtk_select_add_item (ui->sel_mchn, 0, "Omni");
	for (int mc = 1; mc <= 16; ++mc) {
		char buf[8];
		snprintf (buf, 8, "%d", mc);
		robtk_select_add_item (ui->sel_mchn, mc, buf);
	}
	robtk_select_set_default_item (ui->sel_mchn, 0);
	robtk_select_set_value (ui->sel_mchn, 0);
	robtk_select_set_callback (ui->sel_mchn, cb_mchn, ui);

	ui->sel_mode = robtk_select_new ();
	robtk_select_add_item (ui->sel_mode, 0, "Auto");
	robtk_select_add_item (ui->sel_mode, 1, "MIDI");
	robtk_select_add_item (ui->sel_mode, 2, "Manual");
	robtk_select_set_default_item (ui->sel_mode, 0);
	robtk_select_set_value (ui->sel_mode, 0);
	robtk_select_set_callback (ui->sel_mode, cb_mode, ui);

	if (ui->touch) {
		robtk_select_set_touch (ui->sel_mchn, ui->touch->touch, ui->touch->handle, FAT_MCHN);
		robtk_select_set_touch (ui->sel_mode, ui->touch->touch, ui->touch->handle, FAT_MODE);
	}

	ui->lbl_mode = robtk_lbl_new ("Mode");
	ui->lbl_mchn = robtk_lbl_new ("MIDI Chn.");
	ui->btn_panic = robtk_pbtn_new ("MIDI Panic");
	robtk_pbtn_set_callback (ui->btn_panic, cb_btn_panic, ui);

	rob_table_attach (ui->ctbl, GLB_W (ui->lbl_mode), 0, 1, 0, 1, 2, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, GSL_W (ui->sel_mode), 0, 1, 1, 2, 2, 0, RTK_EXANDF, RTK_SHRINK);

	rob_table_attach (ui->ctbl, GLB_W (ui->lbl_mchn), 0, 1, 2, 3, 2, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, GSL_W (ui->sel_mchn), 0, 1, 3, 4, 2, 0, RTK_EXANDF, RTK_SHRINK);
	rob_table_attach (ui->ctbl, robtk_pbtn_widget (ui->btn_panic), 0, 1, 4, 5, 2, 0, RTK_EXANDF, RTK_SHRINK);

	/* top-level packing */
	rob_hbox_child_pack (ui->rw, ui->m0, TRUE, TRUE);
	rob_hbox_child_pack (ui->rw, ui->ctbl, FALSE, TRUE);
	return ui->rw;
}

static void gui_cleanup (Fat1UI* ui) {
	for (int i = 0; i < 5; ++i) {
		robtk_dial_destroy (ui->spn_ctrl[i]);
		robtk_lbl_destroy (ui->lbl_ctrl[i]);
		cairo_surface_destroy (ui->dial_bg[i]);
	}

	robtk_lbl_destroy (ui->lbl_mode);
	robtk_lbl_destroy (ui->lbl_mchn);
	robtk_pbtn_destroy (ui->btn_panic);
	robtk_select_destroy (ui->sel_mode);
	robtk_select_destroy (ui->sel_mchn);

	pango_font_description_free (ui->font[0]);
	pango_font_description_free (ui->font[1]);

	if (ui->m0_bg) {
		cairo_surface_destroy (ui->m0_bg);
	}

	robwidget_destroy (ui->m0);
	rob_table_destroy (ui->ctbl);
	rob_box_destroy (ui->rw);
}

/******************************************************************************
 * RobTk + LV2
 */

#define LVGL_RESIZEABLE

static void ui_enable (LV2UI_Handle handle) { }
static void ui_disable (LV2UI_Handle handle) { }

static enum LVGLResize
plugin_scale_mode (LV2UI_Handle handle)
{
	return LVGL_LAYOUT_TO_FIT;
}

static LV2UI_Handle
instantiate (
		void* const               ui_toplevel,
		const LV2UI_Descriptor*   descriptor,
		const char*               plugin_uri,
		const char*               bundle_path,
		LV2UI_Write_Function      write_function,
		LV2UI_Controller          controller,
		RobWidget**               widget,
		const LV2_Feature* const* features)
{
	Fat1UI* ui = (Fat1UI*) calloc (1, sizeof (Fat1UI));
	if (!ui) {
		return NULL;
	}

	if (0 == strcmp(plugin_uri, FAT1_URI)) {
		ui->microtonal = false;
	} else if (0 == strcmp(plugin_uri, FAT1_URI "#microtonal")) {
		ui->microtonal = true;
	} else {
		free (ui);
		return 0;
	}

	const LV2_Options_Option* options = NULL;
	const LV2_URID_Map*       map     = NULL;

	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_UI__touch)) {
			ui->touch = (LV2UI_Touch*)features[i]->data;
		} else if (!strcmp (features[i]->URI, LV2_URID__map)) {
			map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_OPTIONS__options)) {
			options = (LV2_Options_Option*)features[i]->data;
		}
	}

	ui->nfo        = robtk_info (ui_toplevel);
	ui->write      = write_function;
	ui->controller = controller;
	ui->hover      = -1;
	ui->disable_signals = true;
	*widget = toplevel (ui, ui_toplevel);
	ui->disable_signals = false;

	if (options && map) {
		LV2_URID atom_Float = map->map (map->handle, LV2_ATOM__Float);
		LV2_URID ui_scale   = map->map (map->handle, "http://lv2plug.in/ns/extensions/ui#scaleFactor");
		for (const LV2_Options_Option* o = options; o->key; ++o) {
			if (o->context == LV2_OPTIONS_INSTANCE && o->key == ui_scale && o->type == atom_Float) {
				float ui_scale = *(const float*)o->value;
				if (ui_scale < 1.0) { ui_scale = 1.0; }
				if (ui_scale > 2.0) { ui_scale = 2.0; }
				robtk_queue_scale_change (ui->rw, ui_scale);
			}
		}
	}

	return ui;
}

static void
cleanup (LV2UI_Handle handle)
{
	Fat1UI* ui = (Fat1UI*)handle;
	gui_cleanup (ui);
	free (ui);
}

/* receive information from DSP */
static void
port_event (LV2UI_Handle handle,
		uint32_t     port_index,
		uint32_t     buffer_size,
		uint32_t     format,
		const void*  buffer)
{
	Fat1UI* ui = (Fat1UI*)handle;
	if (format != 0 || port_index <= FAT_OUTPUT) return;

	if (port_index == FAT_PANIC) {
		return;
	}

	const float v = *(float*)buffer;
	ui->disable_signals = true;

	if (port_index >= FAT_TUNE && port_index <= FAT_OFFS) {
		uint32_t ctrl = port_index - FAT_TUNE;
		robtk_dial_set_value (ui->spn_ctrl[ctrl], ctrl_to_gui(ctrl, v));
	}
	else if (port_index == FAT_MODE) {
		robtk_select_set_value (ui->sel_mode, v);
	}
	else if (port_index == FAT_MCHN) {
		robtk_select_set_value (ui->sel_mchn, v);
	}
	else if (port_index >= FAT_NOTE && port_index < FAT_NOTE + 12) {
		uint32_t k = port_index - FAT_NOTE;
		if (v > 0) {
			ui->notes |= 1 << k;
		} else {
			ui->notes &= ~(1 << k);
		}
		queue_draw (ui->m0);
	}
	else if (port_index == FAT_MASK) {
		uint32_t m = v;
		if (ui->mask != m) {
			ui->mask = m;
			queue_draw (ui->m0);
		}
	}
	else if (port_index == FAT_NSET) {
		uint32_t m = v;
		if (ui->set != m) {
			ui->set = m;
			queue_draw (ui->m0);
		}
	}
	else if (port_index == FAT_ERRR) {
		if (ui->err != v) {
			ui->err = v;
			queue_draw (ui->m0);
		}
	}
	else if (port_index >= FAT_SCALE && port_index < FAT_LAST && ui->microtonal) {
		//uint32_t k = port_index - FAT_SCALE;
		// TODO microtonal scale
	}

	ui->disable_signals = false;
}

static const void*
extension_data (const char* uri)
{
	return NULL;
}
