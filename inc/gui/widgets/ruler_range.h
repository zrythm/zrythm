/*
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Ruler range box.
 */

#ifndef __GUI_WIDGETS_RULER_RANGE_H__
#define __GUI_WIDGETS_RULER_RANGE_H__

#include "utils/ui.h"

#include <gtk/gtk.h>

#define RULER_RANGE_WIDGET_TYPE \
  (ruler_range_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RulerRangeWidget,
  ruler_range_widget,
  Z,
  RULER_RANGE_WIDGET,
  GtkDrawingArea)
#define TIMELINE_RULER_RANGE \
  (ruler_widget_get_private ( \
     Z_RULER_WIDGET (MW_RULER)) \
     ->range)

typedef struct _RulerRangeWidget
{
  GtkDrawingArea parent_instance;
  UiCursorState  cursor_state;
} RulerRangeWidget;

RulerRangeWidget *
ruler_range_widget_new (void);

#endif
