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

#ifndef __GUI_WIDGETS_TIMELINE_RULER_H__
#define __GUI_WIDGETS_TIMELINE_RULER_H__

#include "gui/widgets/ruler.h"

#include <gtk/gtk.h>

#define TIMELINE_RULER_WIDGET_TYPE                  (timeline_ruler_widget_get_type ())
G_DECLARE_FINAL_TYPE (TimelineRulerWidget,
                      timeline_ruler_widget,
                      TIMELINE_RULER,
                      WIDGET,
                      RulerWidget)

#define MW_RULER MW_CENTER_DOCK->ruler

typedef struct _TimelineRulerWidget
{
  RulerWidget           parent_instance;
} TimelineRulerWidget;

#endif
