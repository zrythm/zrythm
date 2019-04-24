#ifdef DISPLAY_INTERFACE

#ifndef MIN
#define MIN(A,B) ((A) < (B)) ? (A) : (B)
#endif

#ifndef MAX
#define MAX(A,B) ((A) > (B)) ? (A) : (B)
#endif

#include "rtk/common.h"
//#include "rtk/style.h"

static inline float kmeter_deflect (float db, float krange) {
  db += krange;
  if (db < -40.0f) {
		return (db > -90.0f ? pow (10.0f, db * 0.05f) : 0.0f) * 500.0f / (krange + 45.0f);
  } else {
    const float rv = (db + 45.0f) / (krange + 45.0f);
		return MIN (rv, 1.0);
  }
}


#define UINT_TO_RGB(u,r,g,b) { (*(r)) = ((u)>>16)&0xff; (*(g)) = ((u)>>8)&0xff; (*(b)) = (u)&0xff; }
#define UINT_TO_RGBA(u,r,g,b,a) { UINT_TO_RGB(((u)>>8),r,g,b); (*(a)) = (u)&0xff; }

static void create_pattern (LV2meter* self, double w) {

	int clr[12];
	float stp[5];

	stp[4] = kmeter_deflect (  4 - self->kstandard, self->kstandard);
	stp[3] = kmeter_deflect (  3 - self->kstandard, self->kstandard);
	stp[2] = kmeter_deflect (  0 - self->kstandard, self->kstandard);
	stp[1] = kmeter_deflect (-20 - self->kstandard, self->kstandard);
	stp[0] = kmeter_deflect (-40 - self->kstandard, self->kstandard);

	clr[ 0]=0x003399ff; clr[ 1]=0x009933ff;
	clr[ 2]=0x00aa00ff; clr[ 3]=0x00bb00ff;
	clr[ 4]=0x00ff00ff; clr[ 5]=0x00ff00ff;
	clr[ 6]=0xfff000ff; clr[ 7]=0xfff000ff;
	clr[ 8]=0xff8000ff; clr[ 9]=0xff8000ff;
	clr[10]=0xff0000ff; clr[11]=0xff0000ff;

	guint8 r,g,b,a;
	const double onep  = 1.0 / w;

	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0, w, 0);

	cairo_pattern_add_color_stop_rgb (pat, 1.0, .0, .0, .0);

	// top/clip
	UINT_TO_RGBA (clr[11], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, 1.0 - onep, r/255.0, g/255.0, b/255.0);

	// -0dB
	UINT_TO_RGBA (clr[10], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, stp[4] + onep, r/255.0, g/255.0, b/255.0);
	UINT_TO_RGBA (clr[9], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, stp[4] - onep, r/255.0, g/255.0, b/255.0);

	// -3dB || -2dB
	UINT_TO_RGBA (clr[8], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, stp[3] + onep, r/255.0, g/255.0, b/255.0);
	UINT_TO_RGBA (clr[7], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, stp[3] - onep, r/255.0, g/255.0, b/255.0);

	// -9dB
	UINT_TO_RGBA (clr[6], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, stp[2] + onep, r/255.0, g/255.0, b/255.0);
	UINT_TO_RGBA (clr[5], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, stp[2] - onep, r/255.0, g/255.0, b/255.0);

	// -18dB
	UINT_TO_RGBA (clr[4], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, stp[1] + onep, r/255.0, g/255.0, b/255.0);
	UINT_TO_RGBA (clr[3], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, stp[1] - onep, r/255.0, g/255.0, b/255.0);

	// -40dB
	UINT_TO_RGBA (clr[2], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, stp[0] + onep, r/255.0, g/255.0, b/255.0);
	UINT_TO_RGBA (clr[1], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, stp[0] - onep, r/255.0, g/255.0, b/255.0);

	// -inf
	UINT_TO_RGBA (clr[0], &r, &g, &b, &a);
	cairo_pattern_add_color_stop_rgb (pat, onep, r/255.0, g/255.0, b/255.0);

	//Bottom
	cairo_pattern_add_color_stop_rgb (pat, 0.0, .0 , .0, .0);

	self->mpat= pat;
}

static LV2_Inline_Display_Image_Surface *
bargraph_render (LV2_Handle instance, uint32_t w, uint32_t max_h)
{
#ifdef WITH_SIGNATURE
	if (!is_licensed (instance)) { return NULL; }
#endif

	LV2meter* self = (LV2meter*)instance;

	uint32_t h = MIN (MAX (8, ceilf (w * self->chn / 16.f)), max_h);
	h &= ~1;

	if (!self->display || self->w != w || self->h != h) {
		if (self->display) cairo_surface_destroy(self->display);
		self->display = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
		self->w = w;
		self->h = h;
		if (self->mpat) {
			cairo_pattern_destroy (self->mpat);
			self->mpat = NULL;
		}
	}

	if (!self->mpat) {
		create_pattern (self, w);
	}

	cairo_t* cr = cairo_create (self->display);

	cairo_rectangle (cr, 0, 0, w, h);
	cairo_set_source_rgba (cr, .2, .2, .2, 1.0);
	cairo_fill (cr);

	int ypc = h / self->chn;

	for (uint32_t c = 0; c < self->chn; ++c) {
		float v = self->mval[c];
		v = v > .000031623f ? 20.0 * log10f(v) : -90.0;
		v = w * kmeter_deflect (v, self->kstandard);
		cairo_rectangle (cr, 1, c * ypc, v, ypc - 1);
		cairo_set_source(cr, self->mpat);
		cairo_fill (cr);
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
