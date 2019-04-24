struct MyGimpImage {
	unsigned int   width;
	unsigned int   height;
	unsigned int   bytes_per_pixel;
	unsigned char  pixel_data[];
};

/* load gimp-exported .c image into cairo surface */
static void img2surf (struct MyGimpImage const * img, cairo_surface_t **s, unsigned char **d) {
	unsigned int x,y;
	int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, img->width);

	(*d) = (unsigned char *) malloc (stride * img->height);
	(*s) = cairo_image_surface_create_for_data(*d,
			CAIRO_FORMAT_ARGB32, img->width, img->height, stride);

	cairo_surface_flush (*s);
	for (y = 0; y < img->height; ++y) {
		const int y0 = y * stride;
		const int ys = y * img->width * img->bytes_per_pixel;
		for (x = 0; x < img->width; ++x) {
			const int xs = x * img->bytes_per_pixel;
			const int xd = x * 4;

			if (img->bytes_per_pixel == 3) {
			(*d)[y0 + xd + 3] = 0xff;
			} else {
			(*d)[y0 + xd + 3] = img->pixel_data[ys + xs + 3]; // A
			}
			(*d)[y0 + xd + 2] = img->pixel_data[ys + xs];     // R
			(*d)[y0 + xd + 1] = img->pixel_data[ys + xs + 1]; // G
			(*d)[y0 + xd + 0] = img->pixel_data[ys + xs + 2]; // B
		}
	}
	cairo_surface_mark_dirty (*s);
}

#include "img/meter-dark.c"
#include "img/meter-bright.c"

static const float c_wsh[4] = {1.0, 1.0, 1.0, 0.5};

#define img_draw_needle(CR, VAL, R1, R2, COL, LW) \
	img_draw_needle_x(CR, VAL, _xc, _yc, R1, R2, COL, (LW) * _sc)

#define img_needle_label(CR, TXT, VAL, R1) \
	img_needle_label_col_x(CR, TXT, img_fontname, VAL, _xc, _yc, R1, c_wht)

#define img_needle_label_col(CR, TXT, VAL, R2, COL) \
	img_needle_label_col_x(CR, TXT, img_fontname, VAL, _xc, _yc, R2, COL)

#define img_nordic_triangle(CR, VAL, R2) \
	img_nordic_triangle_x(CR, VAL, _xc, _yc, R2, _sc)

#define MAKELOCALVARS(SCALE) \
	const float _sc = (float)(SCALE); \
	const float _rn = 150 * _sc; \
	const float _ri = 160 * _sc; \
	const float _rl = 180 * _sc; \
	const float _rs = 170 * _sc; \
	const float _xc = 149.5 * _sc; \
	const float _yc = 209.5 * _sc; \
	char img_fontname[48]; \
	char img_fonthuge[48]; \
	if (_sc <= /* 1.0 */ 0) { \
		sprintf(img_fontname, "Sans Bold 11px"); \
		sprintf(img_fonthuge, "Sans Bold 14px"); \
	} else { \
		sprintf(img_fontname, "Sans Bold %dpx", (int)rint(_rl/18.)); \
		sprintf(img_fonthuge, "Sans Bold %dpx", (int)rint(_rn/9.)); \
	}

static float img_radi (float v) {
	return (-M_PI/2 - M_PI/4) + v * M_PI/2;
}

static float img_deflect_nordic(float db) {
	return (1.0/48.0) * (db-18) + (54/48.0);
}

static float img_deflect_din(float db) {
	float v = pow(10, .05 *(db-6.0));
	v = sqrtf (sqrtf (2.002353f * v)) - 0.1885f;
	return (v < 0.0f) ? 0.0f : v;
}

static float img_deflect_iec2(float db) {
	float v = pow(10, .05 *db);
	v *= 3.17f;
	return 0.3f * logf (v) + 0.77633f;
}

static float img_deflect_vu(float db) {
	float v = pow(10, .05 *(db-18));
	return 5.6234149f * v;
}

static void img_draw_needle_x (
		cairo_t* cr, float val,
		const float _xc, const float _yc,
		const float _r1, const float _r2,
		const float * const col, const float lw)
{
	float  c, s;
	if (val < 0.00f) val = 0.00f;
	if (val > 1.05f) val = 1.05f;
	val = (val - 0.5f) * 1.5708f;
	c = cosf (val);
	s = sinf (val);
	cairo_new_path (cr);
	cairo_move_to (cr, _xc + s * _r1, _yc - c * _r1);
	cairo_line_to (cr, _xc + s * _r2, _yc - c * _r2);
	CairoSetSouerceRGBA(col);
	cairo_set_line_width (cr, lw);
	cairo_stroke (cr);

}

static void img_write_text(cairo_t* cr,
		const char *txt, const char *font,
		float x, float y, float ang)
{
	int tw, th;
	cairo_save(cr);
	PangoLayout * pl = pango_cairo_create_layout(cr);
	PangoFontDescription *desc = pango_font_description_from_string(font);
	pango_layout_set_font_description(pl, desc);
	pango_layout_set_text(pl, txt, -1);
	pango_layout_get_pixel_size(pl, &tw, &th);
	cairo_translate (cr, x, y);
	cairo_rotate (cr, ang);
	cairo_translate (cr, -tw/2.0, -th/2.0);
	pango_cairo_show_layout(cr, pl);
	g_object_unref(pl);
	pango_font_description_free(desc);
	cairo_restore(cr);
	cairo_new_path (cr);
}

static void img_needle_label_col_x(cairo_t* cr,
		const char *txt, const char *font,
		float val,
		const float _xc, const float _yc,
		const float _r2, const float * const col)
{
	float  c, s;
	if (val < 0.00f) val = 0.00f;
	if (val > 1.05f) val = 1.05f;
	val = (val - 0.5f) * 1.5708f;
	c = cosf (val);
	s = sinf (val);

	CairoSetSouerceRGBA(col);
	img_write_text(cr, txt, font, _xc + s * _r2, _yc - c * _r2,  val);
}

static void img_nordic_triangle_x(cairo_t* cr,
		float val,
		const float _xc, const float _yc,
		const float _r2, const float scale) {
	float  c, s;
	if (val < 0.00f) val = 0.00f;
	if (val > 1.05f) val = 1.05f;
	val = (val - 0.5f) * 1.5708f;
	c = cosf (val);
	s = sinf (val);

	cairo_save(cr);
	cairo_translate (cr, _xc + s * _r2, _yc - c * _r2);
	cairo_rotate (cr, val);
	cairo_move_to (cr,  0.0, 10.0 * scale);
	cairo_line_to (cr,  6.9282 * scale,  -2.0 * scale);
	cairo_line_to (cr, -6.9282 * scale,  -2.0 * scale);
	cairo_close_path(cr);
	cairo_set_line_width (cr, 1.2 * scale);

	CairoSetSouerceRGBA(c_nrd);
	cairo_fill_preserve (cr);
	CairoSetSouerceRGBA(c_wht);
	cairo_stroke (cr);
	cairo_restore(cr);
}

static void img_nordic(cairo_t* cr, float scale) {
	MAKELOCALVARS(scale);
	const float _rx = 155 * _sc;
	const float	_yl = 95 * _sc;

	int db; char buf[48];
	for (db = -36; db <= 12 ; db+=6) {
		float v = img_deflect_nordic(db);
		if (db == 0) {
			img_nordic_triangle(cr, v, _rs);
			img_needle_label(cr, "TEST\n", v, _rl);
			continue;
		}
		img_draw_needle(cr, v, _ri, _rl, c_wht, 1.5);
		if (db == 12) {
			continue;
		}
		sprintf(buf, "%+d\n", db);
		img_needle_label(cr, buf, v, _rl);
	}
	for (db = -33; db <= 12 ; db+=6) {
		float v = img_deflect_nordic(db);
		img_draw_needle(cr, v, _ri, _rs, c_wht, 1.5);
		if (db == 9) {
			sprintf(buf, "%+d", db);
			img_needle_label(cr, buf, v, _rl);
		}
	}

	cairo_arc (cr, _xc, _yc, _rx,
			img_radi(img_deflect_nordic(6)),
			img_radi(img_deflect_nordic(12)));

	cairo_set_line_width (cr, 12.5 * _sc);
	CairoSetSouerceRGBA(c_wht);
	cairo_stroke_preserve(cr);
	CairoSetSouerceRGBA(c_nrd);
	cairo_set_line_width (cr, 10 * _sc);
	cairo_stroke(cr);

	img_draw_needle(cr, img_deflect_nordic(6),  _rx - 12.5/2.0 * _sc, _ri, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_nordic(12), _rx - 12.5/2.0 * _sc, _ri, c_wht, 1.5);

	CairoSetSouerceRGBA(c_wht);
	img_write_text(cr, "dB", img_fonthuge, _xc + .5, _yl, 0);
}

static void img_phase(cairo_t* cr, float scale) {
	MAKELOCALVARS(scale);
	const float _rx = 155 * _sc;
	const float	_yl = 95 * _sc;

	CairoSetSouerceRGBA(c_nrd);
	cairo_arc (cr, _xc, _yc, _ri+5*_sc, -M_PI/2 - M_PI/4, -M_PI/2 );
	cairo_set_line_width (cr, 10*_sc);
	cairo_stroke (cr);

	CairoSetSouerceRGBA(c_ngr);
	cairo_arc (cr, _xc, _yc, _ri+5*_sc, -M_PI/2, -M_PI/2 + M_PI/4 );
	cairo_set_line_width (cr, 10*_sc);
	cairo_stroke (cr);

	cairo_save(cr);
	cairo_arc (cr, _xc, _yc, _ri+10*_sc, -M_PI/2 - M_PI/4, -M_PI/2 + M_PI/4 );
	cairo_arc_negative (cr, _xc, _yc, _ri, -M_PI/2 + M_PI/4, -M_PI/2 - M_PI/4);
	cairo_close_path(cr);
	cairo_clip(cr);

#if 1 // S*fam
	cairo_arc (cr, _xc, _yc, _rs, -M_PI/2 - M_PI/8, -M_PI/2 + M_PI/8 );
	cairo_arc_negative (cr, 200*_sc, 129.5*_sc, 80*_sc, -M_PI/2 + M_PI/36 , -M_PI/2 - M_PI/8);
	cairo_arc_negative(cr, _xc, _yc, _ri, -M_PI/2,  -M_PI/2 - M_PI/36);
	cairo_arc_negative(cr, 99*_sc, 129.5*_sc, 80.5*_sc, -M_PI/2 + M_PI/8  , -M_PI/2 - M_PI/36);
	CairoSetSouerceRGBA(c_nyl);
	cairo_fill(cr);
#endif
	cairo_restore(cr);

	CairoSetSouerceRGBA(c_wht);
	cairo_arc (cr, _xc, _yc, _ri+10*_sc, -M_PI/2 - M_PI/8, -M_PI/2 + M_PI/8 );
	cairo_set_line_width (cr, .5 * _sc);
	cairo_stroke (cr);

	cairo_arc (cr, _xc, _yc, _ri, -M_PI/2 - M_PI/4, -M_PI/2 + M_PI/4 );
	cairo_set_line_width (cr, 1 * _sc);
	cairo_stroke (cr);

	int v;
	for (v = 0; v <= 20 ; v++) {
		if (v == 0 || v == 5 || v == 10 || v == 15 || v == 20) {
			continue;
		} else {
			img_draw_needle(cr, v/20.0, _ri, _rs, c_wsh, 1.0);
		}
	}

	img_draw_needle(cr, 0.0,  _rx, _rl-5, c_wht, 1.5);
	img_draw_needle(cr, 0.25, _rx, _rl-5, c_wht, 1.5);
	img_draw_needle(cr, 0.5,  _rx, _rl-5, c_wht, 1.5);
	img_draw_needle(cr, 0.75, _rx, _rl-5, c_wht, 1.5);
	img_draw_needle(cr, 1.0,  _rx, _rl-5, c_wht, 1.5);

	sprintf(img_fontname, "Sans Bold %dpx", (int)rint(_rl/20));

	img_needle_label(cr, "-1",     0, _rs-24*_sc);
	img_needle_label(cr, "-0.5", .25, _rs-24*_sc);
	img_needle_label(cr,  "0",   0.5, _rs-24*_sc);
	img_needle_label(cr, "+0.5", .75, _rs-24*_sc);
	img_needle_label(cr, "+1",   1.0, _rs-24*_sc);

	img_needle_label(cr,"   180\u00B0\n",   0, _rl);
	img_needle_label(cr,  " 135\u00B0\n", .25, _rl);
	img_needle_label(cr,   " 90\u00B0\n", 0.5, _rl);
	img_needle_label(cr,   " 45\u00B0\n", .75, _rl);
	img_needle_label(cr,    " 0\u00B0\n", 1.0, _rl);

	CairoSetSouerceRGBA(c_wht);
	img_write_text(cr, "Corr", img_fonthuge, _xc + .5, _yl, 0);
}

static void img_draw_bbc(cairo_t* cr, float scale) {
	MAKELOCALVARS(scale);
	(void) /* unused variable */ _ri;
	int db; char buf[48]; int i = 1;
	for (db = -30; db <= -6 ; db+=4, ++i) {
		float v = img_deflect_iec2(db);
		img_draw_needle(cr, v, _rs, _rl, c_wht, 1.5);
		sprintf(buf, "%d\n", i);
		img_needle_label(cr, buf, v, _rl + 2 * _sc);
	}
	img_draw_needle(cr, img_deflect_iec2(-33), _rs, _rl, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_iec2(-3),  _rs, _rl, c_wht, 1.5);
}

static void img_draw_ebu(cairo_t* cr, float scale) {
	MAKELOCALVARS(scale);
	int db; char buf[48]; int i = 1;
	for (db = -30; db <= -6 ; db+=2, ++i) {
		float v = img_deflect_iec2(db);
		if (i%2) {
			img_draw_needle(cr, v, _ri, _rl, c_wht, 1.5);
			if (db == -18)
				sprintf(buf, "TEST\n");
			else
				sprintf(buf, "%+d\n", db+18);
			img_needle_label(cr, buf, v, _rl+2*_sc);
		} else {
			img_draw_needle(cr, v, _ri, _rs, c_wht, 1.5);
		}
	}
	img_draw_needle(cr, img_deflect_iec2(-33), _ri, _rs, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_iec2(-9), _ri, _rs+5*_sc, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_iec2(-3), _ri, _rs, c_wht, 1.5);
}

static void img_draw_din(cairo_t* cr, float scale) {
	MAKELOCALVARS(scale);
	const float _rf = 164 * _sc;
	const float _rg = 176 * _sc;
	const float _rd = 190 * _sc;
	const float	_yl = 95 * _sc;

	CairoSetSouerceRGBA(c_wht);
	cairo_arc (cr, _xc, _yc, _rs,
			img_radi(img_deflect_din(-60)),
			img_radi(img_deflect_din(6)));
	cairo_set_line_width (cr, 1.5 * _sc);
	cairo_stroke(cr);

	cairo_arc (cr, _xc, _yc, _rs+3.5*_sc,
			img_radi(img_deflect_din(0)),
			img_radi(img_deflect_din(6)));
	CairoSetSouerceRGBA(c_nvu);
	cairo_set_line_width (cr, 5.5 * _sc);
	cairo_stroke(cr);

	img_draw_needle(cr, img_deflect_din(-60), _ri, _rg, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(-50), _rs, _rd, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(-45), _rs, _rg, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(-40), _rs, _rd, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(-35), _rs, _rg, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(-40), _rs, _rd, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(-35), _rs, _rg, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(-30), _rs, _rd, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(-25), _rs, _rg, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(-20), _rs, _rd, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(-15), _rs, _rg, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(-10), _rs, _rd, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din( -9), _rs-.75*_sc, _rl, c_red, 1.5);
	img_draw_needle(cr, img_deflect_din( -5), _rs, _rd, c_wht, 1.5);

	for (int d = -4; d<0; d++) {
		img_draw_needle(cr, img_deflect_din(d), _rs, _rg, c_wht, 1.5);
	}
	for (int d = 1; d<=5; d++) {
		img_draw_needle(cr, img_deflect_din(d), _rs, _rg, c_wsh, 1.5);
	}

	img_draw_needle(cr, img_deflect_din(  0), _rf, _rd, c_red, 1.5);
	img_draw_needle(cr, img_deflect_din(  5), _rs, _rd, c_wht, 1.5);
	img_draw_needle(cr, img_deflect_din(  6), _ri, _rg, c_wht, 1.5);

	img_draw_needle(cr, img_deflect_din(    -40), _rf, _rs, c_wht, 1.5); //  1 %
	img_draw_needle(cr, img_deflect_din(-33.979), _rf, _rs, c_wht, 1.5); //  2 %
	img_draw_needle(cr, img_deflect_din(-30.457), _rf, _rs, c_wht, 1.5); //  3 %
	img_draw_needle(cr, img_deflect_din(-26.021), _rf, _rs, c_wht, 1.5); //  5 %
	img_draw_needle(cr, img_deflect_din(    -20), _rf, _rs, c_wht, 1.5); // 10 %
	img_draw_needle(cr, img_deflect_din(-16.478), _rf, _rs, c_wht, 1.5); // 15 %
	img_draw_needle(cr, img_deflect_din(-13.979), _rf, _rs, c_wht, 1.5); // 20 %
	img_draw_needle(cr, img_deflect_din(-10.458), _rf, _rs, c_wht, 1.5); // 30 %
	img_draw_needle(cr, img_deflect_din(-6.0205), _rf, _rl, c_red, 1.5); // 50 %

	sprintf(img_fontname, "Sans %dpx", (int)rint(_rn / 19.));

	img_needle_label_col(cr,  "\n200", img_deflect_din(6.0205), _ri, c_wht);
	img_needle_label_col(cr,  "\n100", img_deflect_din(0     ), _ri, c_red);
	img_needle_label_col(cr,  "\n50", img_deflect_din(-6.0205), _ri, c_red);
	img_needle_label(cr,      "\n30", img_deflect_din(-10.458), _ri);
	img_needle_label(cr,      "\n10", img_deflect_din(    -20), _ri);
	img_needle_label(cr,       "\n5", img_deflect_din(-26.021), _ri);
	img_needle_label(cr,       "\n3", img_deflect_din(-30.457), _ri);
	img_needle_label(cr,       "\n1", img_deflect_din(    -40), _ri);
	img_needle_label(cr,       "\n0", img_deflect_din(    -60), _ri);

	sprintf(img_fontname, "Sans Bold %dpx", (int)rint(_rn / 19.)); // XXX

	img_needle_label(cr, "-50\n", img_deflect_din(-50), _rd);
	img_needle_label(cr, "-30\n", img_deflect_din(-30), _rd);
	img_needle_label(cr, "-20\n", img_deflect_din(-20), _rd);
	img_needle_label(cr, "-10\n", img_deflect_din(-10), _rd);
	img_needle_label(cr,  "-9\n", img_deflect_din( -9), _rl);
	img_needle_label(cr,  "-5\n", img_deflect_din( -5), _rd);
	img_needle_label(cr,   "0\n", img_deflect_din(  0), _rd);
	img_needle_label(cr,  "+5\n", img_deflect_din(  5), _rd);

	CairoSetSouerceRGBA(c_wht);
	img_write_text(cr, "dB", img_fonthuge, _xc + .5, _yl, 0);
	img_write_text(cr, "\n\n%", img_fonthuge, _xc + .5, _yl, 0);
}

static void img_draw_vu(cairo_t* cr, float scale) {
	MAKELOCALVARS(scale);
	const float _rx = 155 * _sc;
	const float	_yl = 95 * _sc;

	sprintf(img_fontname, "Sans %dpx", (int)rint(_rn / 21)); // XXX

	const float percZ = img_deflect_vu(-60);
	const float percS = (img_deflect_vu(0) - percZ)/10.0;
	for (int pc = 0; pc <= 10; ++pc) {
		img_draw_needle(cr, percZ + pc * percS,  _ri, _rs, c_blk, 1.5);
		if ((pc%2) == 0) {
			char buf[8];
			if (pc==10)
				sprintf(buf, " 100%%");
			else
				sprintf(buf, "%d", pc*10);
			img_needle_label_col(cr, buf, percZ + pc * percS, _rx, c_blk);
		}
	}

	sprintf(img_fontname, "Sans Bold %dpx", (int)rint(_rl/20));

	CairoSetSouerceRGBA(c_blk);

	cairo_arc (cr, _xc, _yc, _rs,
			img_radi(img_deflect_vu(-60)),
			img_radi(img_deflect_vu(0)));
	cairo_set_line_width (cr, 1.5 * _sc);
	cairo_stroke(cr);

	cairo_arc (cr, _xc, _yc, _rs + .75 * _sc,
			img_radi(img_deflect_vu(0)),
			img_radi(img_deflect_vu(3)));
	cairo_set_line_width (cr, 3.0 * _sc);
	CairoSetSouerceRGBA(c_nvu);
	cairo_stroke(cr);

	img_draw_needle(cr, img_deflect_vu(-60), _rs, _rl, c_blk, 1.5);
	img_draw_needle(cr, img_deflect_vu(-20), _rs, _rl, c_blk, 1.5);
	img_draw_needle(cr, img_deflect_vu(-10), _rs, _rl, c_blk, 1.5);
	img_draw_needle(cr, img_deflect_vu(-7),  _rs, _rl, c_blk, 1.5);
	img_draw_needle(cr, img_deflect_vu(-6),  _rs, _rl, c_blk, 1.5);
	img_draw_needle(cr, img_deflect_vu(-5),  _rs, _rl, c_blk, 1.5);
	img_draw_needle(cr, img_deflect_vu(-4),  _rs, _rl, c_blk, 1.5);
	img_draw_needle(cr, img_deflect_vu(-3),  _rs, _rl, c_blk, 1.5);
	img_draw_needle(cr, img_deflect_vu(-2),  _rs, _rl, c_blk, 1.5);
	img_draw_needle(cr, img_deflect_vu(-1),  _rs, _rl, c_blk, 1.5);

	img_draw_needle(cr, img_deflect_vu(0),  _rs-.75*_sc, _rl, c_nrd, 1.5);
	img_draw_needle(cr, img_deflect_vu(1),  _rs, _rl, c_nrd, 1.5);
	img_draw_needle(cr, img_deflect_vu(2),  _rs, _rl, c_nrd, 1.5);
	img_draw_needle(cr, img_deflect_vu(3),  _rs-.75*_sc, _rl, c_nrd, 1.5);

	img_needle_label_col(cr, "-20\n", img_deflect_vu(-20), _rl, c_blk);
	img_needle_label_col(cr,  "10\n", img_deflect_vu(-10), _rl, c_blk);
	img_needle_label_col(cr,   "7\n", img_deflect_vu(-7), _rl, c_blk);
	img_needle_label_col(cr,   "5\n", img_deflect_vu(-5), _rl, c_blk);
	img_needle_label_col(cr,   "3\n", img_deflect_vu(-3), _rl, c_blk);
	img_needle_label_col(cr,   "2\n", img_deflect_vu(-2), _rl, c_blk);
	img_needle_label_col(cr,   "1\n", img_deflect_vu(-1), _rl, c_blk);
	img_needle_label_col(cr,   "0\n", img_deflect_vu(0), _rl, c_red);
	img_needle_label_col(cr,   "1\n", img_deflect_vu(1), _rl, c_red);
	img_needle_label_col(cr,   "2\n", img_deflect_vu(2), _rl, c_red);
	img_needle_label_col(cr,  "+3\n", img_deflect_vu(3), _rl, c_red);

	CairoSetSouerceRGBA(c_blk);
	img_write_text(cr, "VU", img_fonthuge, _xc + .5, _yl, 0);
}


static cairo_surface_t* render_front_face_sf (enum MtrType t, cairo_surface_t *bg, int w, int h) {
	cairo_surface_t *sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
	cairo_t* cr = cairo_create(sf);

	if (bg) {
		cairo_save(cr);
		cairo_scale(cr, (float)w/300.0, (float)h/170.0);
		cairo_set_source_surface(cr, bg, 0, 0);
		cairo_rectangle (cr, 0, 0, 300, 170);
		cairo_fill(cr);
		cairo_restore(cr);
	} else if (t==MT_VU) {
		cairo_rectangle (cr, 0, 0, w, h);
		cairo_set_source_rgba (cr, .94, .94, .78, 1.0);
		cairo_fill(cr);
	} else {
		cairo_rectangle (cr, 0, 0, w, h);
		cairo_set_source_rgba (cr, .15, .15, .15, 1.0);
		cairo_fill(cr);
	}

	const float _sc = (float)w/300.0;

	cairo_rectangle (cr, 7 * _sc, 7 * _sc, 284 * _sc, 128 * _sc);
	cairo_clip (cr);

	switch (t) {
		case MT_EBU:
			img_draw_ebu(cr, _sc);
			break;
		case MT_BBC:
		case MT_BM6:
			img_draw_bbc(cr, _sc);
			break;
		case MT_NOR:
			img_nordic(cr, _sc);
			break;
		case MT_COR:
			img_phase(cr, _sc);
			break;
		case MT_VU:
			img_draw_vu(cr, _sc);
			break;
		case MT_DIN:
			img_draw_din(cr, _sc);
			break;
		default:
			break;
	}
	cairo_destroy(cr);
	return sf;
}

static cairo_surface_t* render_front_face(enum MtrType t, int w, int h) {
	cairo_surface_t *bg = NULL;
	unsigned char * img_tmp;
	if (t==MT_VU) {
		img2surf((struct MyGimpImage const *) &img_meter_bright, &bg, &img_tmp);
	} else {
		img2surf((struct MyGimpImage const *) &img_meter_dark, &bg, &img_tmp);
	}
	cairo_surface_t* sf = render_front_face_sf (t, bg, w, h);
	free(img_tmp);
	return sf;
}
