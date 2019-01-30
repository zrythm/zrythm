/*
 * gui/widgets/ruler_ruler.h - Playhead widget for
 *   rulers
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_RULER_PLAYHEAD_H__
#define __GUI_WIDGETS_RULER_PLAYHEAD_H__

#include <gtk/gtk.h>

#define RULER_PLAYHEAD_WIDGET_TYPE \
  (ruler_playhead_widget_get_type ())
G_DECLARE_FINAL_TYPE (RulerPlayheadWidget,
                      ruler_playhead_widget,
                      Z,
                      RULER_PLAYHEAD_WIDGET,
                      GtkDrawingArea)
#define TIMELINE_RULER_PLAYHEAD \
  (ruler_widget_get_private ( \
    Z_RULER_WIDGET (MW_RULER))->playhead)
#define MIDI_RULER_PLAYHEAD \
  (ruler_widget_get_private ( \
    Z_RULER_WIDGET (MIDI_RULER))->playhead)

#define PLAYHEAD_TRIANGLE_WIDTH 12
#define PLAYHEAD_TRIANGLE_HEIGHT 8

typedef struct _RulerPlayheadWidget
{
  GtkDrawingArea          parent_instance;
} RulerPlayheadWidget;

RulerPlayheadWidget *
ruler_playhead_widget_new ();

#endif
