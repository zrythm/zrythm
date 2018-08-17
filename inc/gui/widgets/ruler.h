/*
 * inc/gui/widgets/ruler.h - Ruler
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_RULER_H__
#define __GUI_WIDGETS_RULER_H__

typedef int gboolean;
typedef void* gpointer;
struct GtkWidget;
struct cairo_t;

/**
 * Sets the ruler on the given blank drawing area
 */
void
set_ruler (GtkWidget * scrolled_window);

gboolean
draw_callback (GtkWidget *, cairo_t *, gpointer data);

#endif
