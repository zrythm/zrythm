#ifdef DISPLAY_INTERFACE

#ifndef MIN
#define MIN(A,B) ((A) < (B)) ? (A) : (B)
#endif

#ifndef MAX
#define MAX(A,B) ((A) > (B)) ? (A) : (B)
#endif

static inline void calc_needle_pos(float val,
		const float x0, const float y0, const float rad,
		const float xoff,
		float * const x, float * const y) {

	const float xc = x0 + xoff;

	if (val < 0.00f) val = 0.00f;
	if (val > 1.05f) val = 1.05f;
	val = (val - 0.5f) * 1.5708f;

	*x = xc + sinf (val) * rad;
	*y = y0 - cosf (val) * rad;
}

static float meter_deflect (int type, float v) {
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

#include "rtk/common.h"
#include "rtk/style.h"
#include "gui/meterimage.c"

static LV2_Inline_Display_Image_Surface *
needle_render (LV2_Handle instance, uint32_t w, uint32_t max_h)
{
#ifdef WITH_SIGNATURE
	if (!is_licensed (instance)) { return NULL; }
#endif
	uint32_t h = MIN (ceilf (w * 17.f/30.f), max_h);

	LV2meter* self = (LV2meter*)instance;
	if (!self->display || self->w != w || self->h != h) {
		if (self->display) cairo_surface_destroy(self->display);
		self->display = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
		self->w = w;
		self->h = h;
		if (self->face) {
			cairo_surface_destroy(self->face);
			self->face = NULL;
		}
	}

	if (!self->face) {
		self->face = render_front_face_sf (self->type, NULL, w, h);
	}


	int n_chn = self->chn;
	const float *mcol[2];
	switch (self->type) {
		case MT_COR:
			mcol[0] = c_wht;
			mcol[1] = c_wht;
			n_chn = 1;
			break;
		case MT_VU:
			if (self->chn == 2) {
				mcol[0] = c_red;
				mcol[1] = c_grn;
			} else {
				mcol[1] = c_blk;
				mcol[0] = c_blk;
			}
			break;
		case MT_BM6:
				mcol[0] = c_wht;
				mcol[1] = c_nyl;
			break;
		default:
			if (self->chn == 2) {
				mcol[0] = c_red;
				mcol[1] = c_grn;
			} else {
				mcol[0] = c_wht;
				mcol[1] = c_wht;
			}
			break;
	}

	cairo_t* cr = cairo_create (self->display);

	cairo_rectangle (cr, 0, 0, w, h);
	cairo_set_source_rgba (cr, .2, .2, .2, 1.0);
	cairo_fill (cr);

	float x0 = floorf (w * .5f) + .5;
	float rad = MIN (w * .6f, h * 1.4f);
	float y0 = MAX (h, rad * 1.17f);

	cairo_save(cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface (cr, self->face, 0, 0);
	cairo_paint (cr);
	cairo_restore(cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	// needle area
	cairo_rectangle (cr, 0, 0, w, h - 2); // XXX
	cairo_clip (cr);

	for (int c = n_chn; c > 0; --c) {
		const float val = meter_deflect(self->type, self->mval[c - 1]);
		float px, py;
		calc_needle_pos(val, x0, y0, rad, 0, &px, &py);

		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		CairoSetSouerceRGBA(mcol[c-1]);
		cairo_set_line_width (cr, 1.5);

		cairo_move_to (cr, x0, y0);
		cairo_line_to (cr, px, py);
		cairo_stroke (cr);
	}

	cairo_destroy (cr);

	cairo_surface_flush (self->display);
	self->surf.width = cairo_image_surface_get_width (self->display);
	self->surf.height = cairo_image_surface_get_height (self->display);
	self->surf.stride = cairo_image_surface_get_stride (self->display);
	self->surf.data = cairo_image_surface_get_data  (self->display);

	return &self->surf;
}
#endif
