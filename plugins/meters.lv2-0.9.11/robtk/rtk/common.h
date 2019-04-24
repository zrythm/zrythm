/* robTK - gtk2 & GL cairo-wrapper
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef RTK_CAIRO_H
#define RTK_CAIRO_H

#include <string.h>
#include <math.h>

static void rounded_rectangle (cairo_t* cr, double x, double y, double w, double h, double r)
{
  double degrees = M_PI / 180.0;

  cairo_new_sub_path (cr);
  cairo_arc (cr, x + w - r, y + r, r, -90 * degrees, 0 * degrees);
  cairo_arc (cr, x + w - r, y + h - r, r, 0 * degrees, 90 * degrees);
  cairo_arc (cr, x + r, y + h - r, r, 90 * degrees, 180 * degrees);
  cairo_arc (cr, x + r, y + r, r, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);
}

static void get_text_geometry( const char *txt, PangoFontDescription *font, int *tw, int *th) {
	cairo_surface_t* tmp = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 8, 8);
	cairo_t *cr = cairo_create (tmp);
	PangoLayout * pl = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(pl, font);
	if (strncmp(txt, "<markup>", 8)) {
		pango_layout_set_text(pl, txt, -1);
	} else {
		pango_layout_set_markup(pl, txt, -1);
	}
	pango_layout_get_pixel_size(pl, tw, th);
	g_object_unref(pl);
	cairo_destroy (cr);
	cairo_surface_destroy(tmp);
}

static void write_text_full(
		cairo_t* cr,
		const char *txt,
		PangoFontDescription *font,
		const float x, const float y,
		const float ang, const int align,
		const float * const col) {
	int tw, th;
	cairo_save(cr);

	PangoLayout * pl = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(pl, font);
	if (strncmp(txt, "<markup>", 8)) {
		pango_layout_set_text(pl, txt, -1);
	} else {
		pango_layout_set_markup(pl, txt, -1);
	}
	pango_layout_get_pixel_size(pl, &tw, &th);
	cairo_translate (cr, rintf(x), rintf(y));
	if (ang != 0) { cairo_rotate (cr, ang); }
	switch(abs(align)) {
		case 1:
			cairo_translate (cr, -tw, ceil(th/-2.0));
			pango_layout_set_alignment (pl, PANGO_ALIGN_RIGHT);
			break;
		case 2:
			cairo_translate (cr, ceil(tw/-2.0), ceil(th/-2.0));
			pango_layout_set_alignment (pl, PANGO_ALIGN_CENTER);
			break;
		case 3:
			cairo_translate (cr, 0, ceil(th/-2.0));
			pango_layout_set_alignment (pl, PANGO_ALIGN_LEFT);
			break;
		case 4:
			cairo_translate (cr, -tw, -th);
			pango_layout_set_alignment (pl, PANGO_ALIGN_RIGHT);
			break;
		case 5:
			cairo_translate (cr, ceil(tw/-2.0), -th);
			pango_layout_set_alignment (pl, PANGO_ALIGN_CENTER);
			break;
		case 6:
			cairo_translate (cr, 0, -th);
			pango_layout_set_alignment (pl, PANGO_ALIGN_LEFT);
			break;
		case 7:
			cairo_translate (cr, -tw, 0);
			pango_layout_set_alignment (pl, PANGO_ALIGN_RIGHT);
			break;
		case 8:
			cairo_translate (cr, ceil(tw/-2.0), 0);
			pango_layout_set_alignment (pl, PANGO_ALIGN_CENTER);
			break;
		case 9:
			cairo_translate (cr, 0, 0);
			pango_layout_set_alignment (pl, PANGO_ALIGN_LEFT);
			break;
		default:
			break;
	}
	if (align < 0) {
		cairo_set_source_rgba (cr, .0, .0, .0, .5);
		cairo_rectangle (cr, 0, 0, tw, th);
		cairo_fill (cr);
	}
#if 1
	cairo_set_source_rgba (cr, col[0], col[1], col[2], col[3]);
	pango_cairo_show_layout(cr, pl);
#else
	cairo_set_source_rgba (cr, col[0], col[1], col[2], col[3]);
	pango_cairo_layout_path(cr, pl);
	cairo_fill(cr);
#endif
	g_object_unref(pl);
	cairo_restore(cr);
	cairo_new_path (cr);
}

static void create_text_surface3(cairo_surface_t ** sf,
		const float w, const float h,
		const float x, const float y,
		const char * txt, PangoFontDescription *font,
		const float * const c_col, const float scale) {
	assert(sf);

	if (*sf) {
		cairo_surface_destroy(*sf);
	}
	*sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceilf(w), ceilf(h));
	cairo_t *cr = cairo_create (*sf);
	cairo_set_source_rgba (cr, .0, .0, .0, 0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr, 0, 0, ceil(w), ceil(h));
	cairo_fill (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	cairo_scale (cr, scale, scale);
	write_text_full(cr, txt, font, ceil(x / scale), ceil(y / scale), 0, 2, c_col);
	cairo_surface_flush(*sf);
	cairo_destroy (cr);
}

static void create_text_surface(cairo_surface_t ** sf,
		const float w, const float h,
		const float x, const float y,
		const char * txt, PangoFontDescription *font,
		const float * const c_col) {
	return create_text_surface3 (sf, w, h, x, y, txt, font, c_col, 1.0);
}


static void create_text_surface2(cairo_surface_t ** sf,
		const float w, const float h,
		const float x, const float y,
		const char * txt, PangoFontDescription *font,
		float ang, int align,
		const float * const c_col) {
	assert(sf);

	if (*sf) {
		cairo_surface_destroy(*sf);
	}
	*sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceilf(w), ceilf(h));
	cairo_t *cr = cairo_create (*sf);
	cairo_set_source_rgba (cr, .0, .0, .0, 0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr, 0, 0, ceil(w), ceil(h));
	cairo_fill (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	write_text_full(cr, txt, font, ceil(x), ceil(y), ang, align, c_col);
	cairo_surface_flush(*sf);
	cairo_destroy (cr);
}


#ifdef _WIN32
#  include <windows.h>
#  include <shellapi.h>
#elif defined __APPLE__
#ifdef __cplusplus
extern "C" {
#endif
	// defined in pugl/pugl_osx.m
	extern bool rtk_osx_open_url (const char* url);
#ifdef __cplusplus
}
#endif
#else
#  include <stdlib.h>
#endif

static void rtk_open_url (const char *url) {
	// assume URL is escaped and shorter than 1024 chars;
#ifdef _WIN32
	ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#elif defined __APPLE__
	rtk_osx_open_url (url);
#else
	char tmp[1024];
	sprintf(tmp, "xdg-open %s &", url);
	(void) system (tmp);
#endif
}


#endif
