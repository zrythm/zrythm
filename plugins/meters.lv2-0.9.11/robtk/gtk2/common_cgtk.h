/* robwidget - gtk2 & GL wrapper
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef COMMON_CAIRO_H
#define COMMON_CAIRO_H

static PangoFontDescription * get_font_from_gtk () {
  PangoFontDescription * rv;
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget *foobar = gtk_label_new("Foobar");
	gtk_container_add(GTK_CONTAINER(window), foobar);
	gtk_widget_ensure_style(foobar);

	PangoContext* pc = gtk_widget_get_pango_context(foobar);
	PangoFontDescription const * pfd = pango_context_get_font_description (pc);
	rv = pango_font_description_copy (pfd);
	gtk_widget_destroy(foobar);
	gtk_widget_destroy(window);
	assert(rv);
	return rv;
}

static void get_color_from_gtk (GdkColor *c, int which) {
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget *foobar = gtk_label_new("Foobar");
	gtk_container_add(GTK_CONTAINER(window), foobar);
	gtk_widget_ensure_style(foobar);

	GtkStyle *style = gtk_widget_get_style(foobar);
	switch (which) {
		default:
			memcpy((void*) c, (void*) &style->fg[GTK_STATE_NORMAL], sizeof(GdkColor));
			break;
		case 1:
			memcpy((void*) c, (void*) &style->bg[GTK_STATE_NORMAL], sizeof(GdkColor));
			break;
		case 2:
			memcpy((void*) c, (void*) &style->fg[GTK_STATE_ACTIVE], sizeof(GdkColor));
			break;
	}
	gtk_widget_destroy(foobar);
	gtk_widget_destroy(window);
}

static float   robtk_colorcache[3][4];
static bool    robtk_colorcached[3] = { false, false, false };

void get_color_from_theme (int which, float *col) {
	GdkColor color;
	assert(which >= 0 && which <= 2);

	if (robtk_colorcached[which]) {
		memcpy(col, robtk_colorcache[which], 4 * sizeof(float));
	} else {
		get_color_from_gtk(&color, which);
		col[0] = color.red/65536.0;
		col[1] = color.green/65536.0;
		col[2] = color.blue/65536.0;
		col[3] = 1.0;
		memcpy(robtk_colorcache[which], col, 4 * sizeof(float));
		robtk_colorcached[which] = true;
	}
}

static PangoFontDescription * get_font_from_theme () {
	return get_font_from_gtk(); // TODO cache this -- but how to free on exit?
}

#endif
