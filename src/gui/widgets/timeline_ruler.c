/*
 * gui/widgets/timeline_ruler.c - Ruler
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_ruler.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineRulerWidget,
               timeline_ruler_widget,
               RULER_WIDGET_TYPE)

static void
timeline_ruler_widget_class_init (
  TimelineRulerWidgetClass * klass)
{
}

static void
timeline_ruler_widget_init (
  TimelineRulerWidget * self)
{
  g_message ("initing timeline ruler...");
}
