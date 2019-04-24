/* meters.lv2
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2008-2012 Fons Adriaensen <fons@linuxaudio.org>
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
#include <string.h>
#include <math.h>

#include "img/screw.c"

#define RTK_URI "http://gareus.org/oss/lv2/meters#"
#define RTK_GUI "needle"
#define MTR_URI RTK_URI
#define LVGL_RESIZEABLE

/* meter types */
enum MtrType {
	MT_BBC = 1,
	MT_BM6,
	MT_EBU,
	MT_DIN,
	MT_NOR,
	MT_VU,
	MT_COR
};

typedef struct {
	RobWidget *rw;

	LV2UI_Write_Function write;
	LV2UI_Controller     controller;

	cairo_surface_t * bg;
	cairo_surface_t * adj;
	cairo_surface_t * sf_nfo;
	unsigned char   * img0;
	unsigned char   * img1;
	float col[3];

	bool naned[2];
	float lvl[2];
	float cal;
	float cal_rad;
	int chn;
	enum MtrType type;

	float drag_x, drag_y, drag_cal;
	int width, height;

	PangoFontDescription *font;

	/*** pixel design ***/
	float scale;
	float s_scale;
	/* screw area design */
	float s_xc;
	float s_yc;
	float s_w2;
	float s_h2;
	cairo_rectangle_t screwrect;
	cairo_rectangle_t textrect;

	/* meter size */
	float m_width;
	float m_height;
	float n_height;

	/* needle */
	float n_xc;
	float n_yc;
	float m_r0;
	float m_r1;

	const char *nfo;

} MetersLV2UI;

#include "gui/meterimage.c"

static void setup_images (MetersLV2UI* ui) {
	ui->bg = render_front_face(ui->type, ui->m_width, ui->m_height);
	img2surf((struct MyGimpImage const *)&img_screw, &ui->adj, &ui->img1);
}

static int width_scale(MetersLV2UI* ui) {
	switch (ui->type) {
		case MT_BBC:
		case MT_BM6:
			return 1;
			break;
		default:
			return ui->chn;
	}
}

static void set_needle_sizes(MetersLV2UI* ui) {
	const float scale = ui->scale;
	ui->s_scale = scale;
	if (ui->s_scale > 2.0) ui->s_scale = 2.0;

	/* screw area design */
	ui->s_xc = 150.0 * scale; // was (300.0 * ui->chn)/2.0;
	ui->s_yc = 153.0 * scale;
	ui->s_w2 = ui->s_h2 =  12.5 * ui->s_scale;
	ui->screwrect.x = (ui->s_xc - ui->s_w2) - 2;
	ui->screwrect.y = (ui->s_yc - ui->s_w2) - 2;
	ui->screwrect.width = ui->screwrect.height = 4 + 2 * ui->s_w2;

	ui->textrect.x = (150 + ui->s_w2) * scale;
	ui->textrect.y = (153 -     15  ) * scale;
	ui->textrect.width = 150;
	ui->textrect.height = 30;

	/* meter size */
	ui->m_width  = rint(300.0 * scale);
	ui->m_height = rint(170.0 * scale);
	ui->n_height = rint(135.0 * scale); // bottom separator

	/* needle */
	ui->n_xc =  149.5 * scale;
	ui->n_yc =  209.5 * scale;
	ui->m_r0 =  180.0 * scale;
	ui->m_r1 =   72.0 * scale;

	ui->width = ui->m_width * width_scale(ui);
	ui->height = ui->m_height;

	if (ui->bg) cairo_surface_destroy(ui->bg);
	if (ui->font) pango_font_description_free(ui->font);

	ui->bg = render_front_face(ui->type, ui->m_width, ui->m_height);

	char fontname[32];
	sprintf(fontname, "Sans %dpx", (int)rint(10.0 * ui->scale));
	ui->font = pango_font_description_from_string(fontname);

	if (ui->sf_nfo) {
		cairo_surface_destroy(ui->sf_nfo);
		ui->sf_nfo = NULL;
	}
	if (ui->nfo) {
		PangoFontDescription *fd = pango_font_description_from_string("Sans 10px");
		create_text_surface2(&ui->sf_nfo,
				ui->width, 12,
				ui->width - 2, 0,
				ui->nfo, fd, 0, 7, c_g30);
		pango_font_description_free(fd);
	}
}

static void draw_background (MetersLV2UI* ui, cairo_t* cr, float xoff, float yoff) {
	float w =  cairo_image_surface_get_width (ui->bg);
	float h =  cairo_image_surface_get_height (ui->bg);

	cairo_save(cr);
	cairo_scale(cr, ui->m_width / w, ui->m_height / h);
	cairo_set_source_surface(cr, ui->bg, xoff * w / ui->m_width, yoff);
	cairo_rectangle (cr, xoff * w / ui->m_width, 0, w, h);
	cairo_fill(cr);
	cairo_restore(cr);

	if (ui->sf_nfo) {
		cairo_set_source_surface(cr, ui->sf_nfo, 0, ui->m_height - 12);
		cairo_paint (cr);
	}
}


/******************************************************************************
 * some simple maths helpers
 */

static float cal2rad(enum MtrType t, float v) {
	/* rotate screw  [-30..0]  -> [-M_PI/4 .. M_PI/4] */
	return .0837758 * (v + (t == MT_DIN ? 15.0 : 18.0));
}

static inline void calc_needle_pos(MetersLV2UI* ui, float val, const float xoff, float * const x, float * const y) {
	const float _xc = ui->n_xc + xoff;

	if (val < 0.00f) val = 0.00f;
	if (val > 1.05f) val = 1.05f;
	val = (val - 0.5f) * 1.5708f;

	*x = _xc + sinf (val) * ui->m_r0;
	*y = ui->n_yc - cosf (val) * ui->m_r0;
}

static inline void calc_needle_area(MetersLV2UI* ui, float val, const float xoff, cairo_rectangle_t *r) {
	const float _xc = ui->n_xc + xoff;

	if (val < 0.00f) val = 0.00f;
	if (val > 1.05f) val = 1.05f;
	val = (val - 0.5f) * 1.5708f;

	const float _sf = sinf (val) ;
	const float _cf = cosf (val) ;

	const float _x0 = _xc + _sf * ui->m_r0;
	const float _y0 = ui->n_yc - _cf * ui->m_r0;
	const float _x1 = _xc + _sf * ui->m_r1;
	const float _y1 = ui->n_yc - _cf * ui->m_r1;

	r->x      = MIN(_x0, _x1) - 3.0 * ui->scale;
	r->y      = MIN(_y0, _y1) - 3.0 * ui->scale;
	r->width  = MAX(_x0 - _x1, _x1 - _x0) + 6.0 * ui->scale;
	r->height = MAX(0, (ui->n_height - r->y)) + 6.0 * ui->scale;
}

static float meter_deflect(int type, float v) {
	switch(type) {
		case MT_VU:
			return 5.6234149f * v;
		case MT_BBC:
		case MT_BM6:
		case MT_EBU:
			v *= 3.17f;
			if (v < 0.1f) return v * 0.855f;
			else return 0.3f * logf (v) + 0.77633f;
		case MT_DIN:
			v = sqrtf (sqrtf (2.002353f * v)) - 0.1885f;
			return (v < 0.0f) ? 0.0f : v;
		case MT_NOR:
			if (v < 1e-5) return 0;
			return .4166666f * log10(v) + 1.125f; // (20.0/48.0) *log(v) + (54/48.0)  -> -54dBFS ^= 0, -12dB ^= 1.0
		case MT_COR:
			return 0.5f * (1.0f + v);
		default:
			return 0;
	}
}

/******************************************************************************
 * Drawing helpers
 */

static void draw_needle (MetersLV2UI* ui, cairo_t* cr, float val,
		const float xoff, const float * const col, const float lw) {
	cairo_save(cr);

	/* needle area */
	cairo_rectangle (cr, xoff, 0, ui->m_width, ui->n_height);
	cairo_clip (cr);

	/* draw needle */
	const float _xc = ui->n_xc + xoff;
	float px, py;

	calc_needle_pos(ui, val, xoff, &px, &py);

	cairo_new_path (cr);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to (cr, _xc, ui->n_yc);
	cairo_line_to (cr, px, py);
	CairoSetSouerceRGBA(col);
	cairo_set_line_width (cr, lw * ui->scale);
	cairo_stroke (cr);

	cairo_restore(cr);
}

#define NANED(X,Y, COL) \
	cairo_save(cr); \
	rounded_rectangle (cr, (X) - 30 * ui->scale, (Y) - 5 * ui->scale, 60 * ui->scale,  20 * ui->scale, 4*ui->scale); \
	CairoSetSouerceRGBA(COL); \
	cairo_fill_preserve(cr); \
	cairo_set_line_width (cr, .75 * ui->scale); \
	CairoSetSouerceRGBA(c_gry); \
	cairo_stroke(cr); \
	write_text_full(cr, "NaN", ui->font, (X), (Y) + 5 * ui->scale, 0, 2, c_wht); \
	cairo_restore(cr);


/******************************************************************************
 * main drawing function
 */
static bool expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	MetersLV2UI* ui = (MetersLV2UI*)GET_HANDLE(handle);
	float const * col;
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	switch(ui->type) {
		case MT_VU:
			col = c_blk;
			break;
		default:
			col = c_wht;
			break;
	}

	if (ui->type == MT_COR) {
		draw_background (ui, cr, 0, 0);
		draw_needle (ui, cr, ui->lvl[0], 0, col, 2.0);
		return TRUE;
	}
	else if (ui->type == MT_BBC && ui->chn == 2) {
		draw_background (ui, cr, 0, 0);
		if (ui->naned[0]) { NANED(ui->m_width/2, ui->height*2/3 - 20 * ui->scale, c_red); }
		if (ui->naned[1]) { NANED(ui->m_width/2, ui->height*2/3 +  2 * ui->scale, c_grn); }
		draw_needle (ui, cr, ui->lvl[1], 0, c_grn, 2.0);
		draw_needle (ui, cr, ui->lvl[0], 0, c_red, 2.0);
	}
	else if (ui->type == MT_BM6 && ui->chn == 2) {
		draw_background (ui, cr, 0, 0);
		if (ui->naned[0]) { NANED(ui->m_width/2, ui->height*2/3 - 20 * ui->scale, c_red); }
		if (ui->naned[1]) { NANED(ui->m_width/2, ui->height*2/3 +  2 * ui->scale, c_grn); }
		draw_needle (ui, cr, ui->lvl[1], 0, c_nyl, 2.0); // XXX
		draw_needle (ui, cr, ui->lvl[0], 0, c_wht, 2.0);
	} else {
		int c;
		for (c=0; c < ui->chn; ++c) {
			draw_background (ui, cr, ui->m_width * c, 0);
			if (ui->naned[c]) { NANED(ui->m_width * c + ui->m_width/2, ui->height*2/3, c_red); }
			draw_needle (ui, cr, ui->lvl[c], ui->m_width * c, col, 1.4);
		}
	}

	/* draw callibration text */
	if (rect_intersect(ev , &ui->textrect) && (ui->drag_x >= 0 || ui->drag_y >=0)) {
		char buf[48];
		/* default gain -18.0dB in meters.cc, except DIN: -15dB (deflection) */
		switch (ui->type) {
			case MT_VU:
				sprintf(buf, "0 VU = %.1f dBFS", -36 - ui->cal);
				break;
			case MT_BBC:
			case MT_BM6:
				sprintf(buf, " '4' = %.1f dBFS", -36 - ui->cal);
				break;
			case MT_DIN:
				/* specs: -3dBu = '-9' ^= -18 dbFS - so these are eqivalent: */
				//sprintf(buf, " '-6' = %.1f dBFS",  -30 - ui->cal); // no '-6' label
				//sprintf(buf, "'50%%' = %.1f dBFS", -30 - ui->cal); // mmh
				//sprintf(buf, " 0dBu = %.1f dBFS",  -30 - ui->cal);
				sprintf(buf, " '-9' = %.1f dBFS",  -33 - ui->cal);
				break;
			case MT_EBU:
			case MT_NOR:
				sprintf(buf, " 'TEST' = %.1f dBFS", -36 - ui->cal);
				break;
			default:
				/* not reached */
				break;
		}

		write_text_full(cr, buf, ui->font, ui->s_xc + ui->s_w2 + 8, ui->s_yc, 0, 3, c_wht);
	}

	/* draw callibration screw */
	if (rect_intersect(ev , &ui->screwrect)) {
		cairo_save(cr);
		cairo_translate (cr, ui->s_xc, ui->s_yc);
		cairo_rotate (cr, ui->cal_rad);
		cairo_translate (cr, -ui->s_w2, -ui->s_h2);
		cairo_scale(cr, ui->s_scale, ui->s_scale);
		cairo_set_source_surface(cr, ui->adj, 0, 0);
		cairo_rectangle (cr, 0, 0, 25.0, 25.0 /* 2.0 * ui->s_w2 / ui->scale, 2.0 * ui->s_h2 / ui->scale */);
		cairo_fill(cr);
		cairo_restore(cr);

		cairo_save(cr);
		cairo_translate (cr, ui->s_xc, ui->s_yc);
		CairoSetSouerceRGBA(c_scr);
		cairo_arc(cr, 0, 0, ui->s_w2, 0, 2 * M_PI);
		cairo_set_line_width (cr, 1.0);
		cairo_stroke(cr);
		cairo_restore(cr);
	}

	return TRUE;
}

/******************************************************************************
 * UI event handling
 */

/* calibration screw drag/drop handling */
static RobWidget* mousedown(RobWidget* handle, RobTkBtnEvent *event) {
	MetersLV2UI* ui = (MetersLV2UI*)GET_HANDLE(handle);

	if (event->state & ROBTK_MOD_CTRL) {
		robwidget_resize_toplevel(ui->rw, 300 * width_scale(ui), 170);
		return NULL;
	}

	if (ui->naned[0]) { ui->naned[0] = FALSE; queue_draw(ui->rw); }
	if (ui->naned[1]) { ui->naned[1] = FALSE; queue_draw(ui->rw); }
	if (   event->x < ui->s_xc - ui->s_w2
			|| event->x > ui->s_xc + ui->s_w2
			|| event->y < ui->s_yc - ui->s_h2
			|| event->y > ui->s_yc + ui->s_h2
			) {
		/* outside of adj-screw area */
		return NULL;
	}

	if (event->state & ROBTK_MOD_SHIFT) {
		/* shift-click -> reset to default */
		switch(ui->type) {
			case MT_VU: ui->cal = -22; break;
			case MT_DIN: ui->cal = -15; break;
			default: ui->cal = -18; break;
		}
		ui->write(ui->controller, 0, sizeof(float), 0, (const void*) &ui->cal);
		ui->cal_rad = cal2rad(ui->type, ui->cal);
		queue_draw(ui->rw);
		return NULL;
	}

	ui->drag_x = event->x;
	ui->drag_y = event->y;
	ui->drag_cal = ui->cal;
	queue_draw(ui->rw);
	return handle;
}

/* stereo-phase correlation - resize to 100% only */
static RobWidget* mousedown_cor(RobWidget* handle, RobTkBtnEvent *event) {
	MetersLV2UI* ui = (MetersLV2UI*)GET_HANDLE(handle);
	if (event->state & ROBTK_MOD_CTRL) {
		robwidget_resize_toplevel(ui->rw, 300 * width_scale(ui), 170);
	}
	return NULL;
}


static RobWidget* mouseup(RobWidget* handle, RobTkBtnEvent *event) {
	MetersLV2UI* ui = (MetersLV2UI*)GET_HANDLE(handle);
	ui->drag_x = ui->drag_y = -1;
	queue_draw(ui->rw);
	return NULL;
}

static RobWidget* mousemove(RobWidget* handle, RobTkBtnEvent *event) {
	MetersLV2UI* ui = (MetersLV2UI*)GET_HANDLE(handle);
	if (ui->drag_x < 0 || ui->drag_y < 0) return NULL;

	const float diff = rint(((event->x - ui->drag_x) - (event->y - ui->drag_y)) / 5.0 ) * .5;
	float cal = ui->drag_cal + diff;
	if (cal < -30.0) cal = -30.0;
	if (cal > 0.0) cal = 0.0;

	//printf("Mouse move.. %f %f -> %f   (%f -> %f)\n", event->x, event->y, diff, ui->drag_cal, cal);
	ui->write(ui->controller, 0, sizeof(float), 0, (const void*) &cal);
	ui->cal = cal;
	ui->cal_rad = cal2rad(ui->type, ui->cal);
	queue_draw(ui->rw);

	return handle;
}


/******************************************************************************
 * widget hackery
 */

static void
size_request(RobWidget* rw, int *w, int *h) {
	MetersLV2UI* ui = (MetersLV2UI*)GET_HANDLE(rw);
	*w = 300 * .75 * width_scale(ui);
	*h = 170 * .75;
}

static void
size_default(RobWidget* rw, int *w, int *h) {
	MetersLV2UI* ui = (MetersLV2UI*)GET_HANDLE(rw);
	*w = 300 * width_scale(ui);
	*h = 170;
}

static void
size_limit(RobWidget* rw, int *w, int *h) {
	MetersLV2UI* ui = (MetersLV2UI*)GET_HANDLE(rw);
	int dflw, dflh;
	size_default(rw, &dflw, &dflh);
	float scale = MIN(*w/(float)dflw, *h/(float)dflh);
	if (scale < .5 ) scale = .5;
	if (scale > 3.5 ) scale = 3.5;
	ui->scale  = scale;
	set_needle_sizes(ui); // sets ui->width, ui->height
	robwidget_set_size(rw, ui->width, ui->height); // single top-level
	*w = ui->width;
	*h = ui->height;
	queue_draw(rw);
}

static void
size_allocate(RobWidget* rw, int w, int h) {
	int ww = w;
	int wh = h;
	size_limit(rw, &ww, &wh);
	robwidget_set_alignment(rw, .5, .5);
	rw->area.x = rint((w - rw->area.width) * rw->xalign);
	rw->area.y = rint((h - rw->area.height) * rw->yalign);
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
	MetersLV2UI* ui = (MetersLV2UI*)calloc(1, sizeof(MetersLV2UI));
	*widget = NULL;

	if (!ui) {
		fprintf (stderr, "meters.lv2: out of memory.\n");
		return NULL;
	}

	if      (!strcmp(plugin_uri, MTR_URI "VUmono"))    { ui->chn = 1; ui->type = MT_VU; }
	else if (!strcmp(plugin_uri, MTR_URI "VUstereo"))  { ui->chn = 2; ui->type = MT_VU; }
	else if (!strcmp(plugin_uri, MTR_URI "BBCmono"))   { ui->chn = 1; ui->type = MT_BBC; }
	else if (!strcmp(plugin_uri, MTR_URI "BBCstereo")) { ui->chn = 2; ui->type = MT_BBC; }
	else if (!strcmp(plugin_uri, MTR_URI "BBCM6"))     { ui->chn = 2; ui->type = MT_BM6; }
	else if (!strcmp(plugin_uri, MTR_URI "EBUmono"))   { ui->chn = 1; ui->type = MT_EBU; }
	else if (!strcmp(plugin_uri, MTR_URI "EBUstereo")) { ui->chn = 2; ui->type = MT_EBU; }
	else if (!strcmp(plugin_uri, MTR_URI "DINmono"))   { ui->chn = 1; ui->type = MT_DIN; }
	else if (!strcmp(plugin_uri, MTR_URI "DINstereo")) { ui->chn = 2; ui->type = MT_DIN; }
	else if (!strcmp(plugin_uri, MTR_URI "NORmono"))   { ui->chn = 1; ui->type = MT_NOR; }
	else if (!strcmp(plugin_uri, MTR_URI "NORstereo")) { ui->chn = 2; ui->type = MT_NOR; }
	else if (!strcmp(plugin_uri, MTR_URI "COR"))       { ui->chn = 1; ui->type = MT_COR; }

	if (ui->type == 0) {
		free(ui);
		return NULL;
	}

	ui->write      = write_function;
	ui->controller = controller;
	ui->lvl[0]     = ui->lvl[1] = 0;
	ui->naned[0]   = ui->naned[1] = FALSE;
	ui->cal        = -18.0;
	ui->cal_rad    = cal2rad(ui->type, ui->cal);
	ui->bg         = NULL;
	ui->adj        = NULL;
	ui->sf_nfo     = NULL;
	ui->img0       = NULL;
	ui->drag_x     = ui->drag_y = -1;
	ui->scale      = 1.0;
	ui->font       = NULL;

	ui->nfo = robtk_info(ui_toplevel);
	set_needle_sizes(ui);

	setup_images(ui);

	ui->rw = robwidget_new(ui);
	robwidget_make_toplevel(ui->rw, ui_toplevel);
	ROBWIDGET_SETNAME(ui->rw, "needle");

	robwidget_set_expose_event(ui->rw, expose_event);
	robwidget_set_size_request(ui->rw, size_request);
	robwidget_set_size_allocate(ui->rw, size_allocate);
	robwidget_set_size_limit(ui->rw, size_limit);
	robwidget_set_size_default(ui->rw, size_default);

	if (ui->type != MT_COR) {
		robwidget_set_mousedown(ui->rw, mousedown);
		robwidget_set_mouseup(ui->rw, mouseup);
		robwidget_set_mousemove(ui->rw, mousemove)
	} else {
		robwidget_set_mousedown(ui->rw, mousedown_cor);
	}

	*widget = ui->rw;

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
	MetersLV2UI* ui = (MetersLV2UI*)handle;
	cairo_surface_destroy(ui->sf_nfo);
	cairo_surface_destroy(ui->bg);
	cairo_surface_destroy(ui->adj);
	pango_font_description_free(ui->font);
	robwidget_destroy(ui->rw);
	free(ui->img0);
	free(ui->img1);
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

#define MIN2(A,B) ( (A) < (B) ? (A) : (B) )
#define MAX2(A,B) ( (A) > (B) ? (A) : (B) )
#define MIN3(A,B,C) (  (A) < (B)  ? MIN2 (A,C) : MIN2 (B,C) )
#define MAX3(A,B,C) (  (A) > (B)  ? MAX2 (A,C) : MAX2 (B,C) )

static void invalidate_area(MetersLV2UI* ui, int c, float oldval, float newval) {
	if (!ui->naned[c] && (isnan(newval) || isinf(newval)))  {
		ui->naned[c] = TRUE;
		queue_draw(ui->rw);
	}
	if (oldval < 0.00f) oldval = 0.00f;
	if (oldval > 1.05f) oldval = 1.05f;
	if (newval < 0.00f) newval = 0.00f;
	if (newval > 1.05f) newval = 1.05f;

	if (rint(newval * 540) == rint(oldval * 540)) {
		return;
	}

	float xoff = ui->m_width * c;
	if (c == 1 && (ui->type == MT_BBC || ui->type == MT_BM6)) {
		xoff = 0;
	}
	cairo_rectangle_t r1, r2;
	calc_needle_area(ui, oldval, xoff, &r1);
	calc_needle_area(ui, newval, xoff, &r2);
	rect_combine(&r1, &r2, &r1);
	queue_tiny_area(ui->rw, r1.x, r1.y, r1.width, r1.height);
}

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
	MetersLV2UI* ui = (MetersLV2UI*)handle;

	if ( format != 0 ) { return; }

	if (port_index == 3) {
		float nl = meter_deflect(ui->type, *(float *)buffer);
		invalidate_area(ui, 0, ui->lvl[0], nl);
		ui->lvl[0] = nl;
	} else
	if (port_index == 6) {
		float nl = meter_deflect(ui->type, *(float *)buffer);
		invalidate_area(ui, 1, ui->lvl[1], nl);
		ui->lvl[1] = nl;
	} else
	if (port_index == 0) {
		ui->cal = *(float *)buffer;
		ui->cal_rad = cal2rad(ui->type, ui->cal);
		queue_draw(ui->rw);
	}
}
