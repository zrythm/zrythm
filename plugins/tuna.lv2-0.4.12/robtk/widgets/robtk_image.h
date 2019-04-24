/* Image widget
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

#ifndef _ROB_TK_IMAGE_H_
#define _ROB_TK_IMAGE_H_

typedef struct {
	RobWidget *rw;
	float w_width, w_height;
	uint8_t *img_data;
	cairo_surface_t *img_surface;
} RobTkImg;

static bool robtk_img_expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	RobTkImg* d = (RobTkImg *)GET_HANDLE(handle);
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	cairo_set_source_surface(cr, d->img_surface, 0, 0);
	cairo_paint(cr);

	return TRUE;
}

/******************************************************************************
 * RobWidget stuff
 */

static void
priv_img_size_request(RobWidget* handle, int *w, int *h) {
	RobTkImg* d = (RobTkImg*)GET_HANDLE(handle);
	*w = d->w_width;
	*h = d->w_height;
}


/******************************************************************************
 * public functions
 */

static RobTkImg * robtk_img_new(const unsigned int w, const unsigned int h, const unsigned bpp, const uint8_t * const img) {
	RobTkImg *d = (RobTkImg *) malloc(sizeof(RobTkImg));
	assert(bpp == 3 || bpp == 4);
	d->w_width = w;
	d->w_height = h;

	unsigned int x,y;
	int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, w);
	d->img_data = (unsigned char *) malloc (stride * h);

	d->img_surface = cairo_image_surface_create_for_data(d->img_data,
			CAIRO_FORMAT_ARGB32, w, h, stride);

	cairo_surface_flush (d->img_surface);
	for (y = 0; y < h; ++y) {
		const int y0 = y * stride;
		const int ys = y * w * bpp;
		for (x = 0; x < w; ++x) {
			const int xs = x * bpp;
			const int xd = x * 4;

			if (bpp == 3) {
				d->img_data[y0 + xd + 3] = 0xff;
			} else {
				d->img_data[y0 + xd + 3] = img[ys + xs + 3]; // A
			}
			d->img_data[y0 + xd + 2] = img[ys + xs];     // R
			d->img_data[y0 + xd + 1] = img[ys + xs + 1]; // G
			d->img_data[y0 + xd + 0] = img[ys + xs + 2]; // B
		}
	}
	cairo_surface_mark_dirty (d->img_surface);


	d->rw = robwidget_new(d);
	ROBWIDGET_SETNAME(d->rw, "Image");
	robwidget_set_expose_event(d->rw, robtk_img_expose_event);
	robwidget_set_size_request(d->rw, priv_img_size_request);

	return d;
}

static void robtk_img_destroy(RobTkImg *d) {
	robwidget_destroy(d->rw);
	cairo_surface_destroy(d->img_surface);
	free(d->img_data);
	free(d);
}

static void robtk_img_set_alignment(RobTkImg *d, float x, float y) {
	robwidget_set_alignment(d->rw, x, y);
}

static RobWidget * robtk_img_widget(RobTkImg *d) {
	return d->rw;
}
#endif
