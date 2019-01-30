/*
 * gui/widgets/playhead_arranger.h - Playhead widget for
 *   arrangers
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

#ifndef __GUI_WIDGETS_ARRANGER_PLAYHEAD_H__
#define __GUI_WIDGETS_ARRANGER_PLAYHEAD_H__

#include <gtk/gtk.h>

#define ARRANGER_PLAYHEAD_WIDGET_TYPE \
  (arranger_playhead_widget_get_type ())
G_DECLARE_FINAL_TYPE (ArrangerPlayheadWidget,
                      arranger_playhead_widget,
                      Z,
                      ARRANGER_PLAYHEAD_WIDGET,
                      GtkDrawingArea)
#define TIMELINE_ARRANGER_PLAYHEAD \
  (arranger_widget_get_private ( \
    Z_ARRANGER_WIDGET (MW_TIMELINE))->playhead)
#define MIDI_ARRANGER_PLAYHEAD \
  (arranger_widget_get_private ( \
    Z_ARRANGER_WIDGET (MIDI_ARRANGER))->playhead)
#define MIDI_MODIFIER_ARRANGER_PLAYHEAD \
  (arranger_widget_get_private ( \
    Z_ARRANGER_WIDGET (MIDI_MODIFIER_ARRANGER))->playhead)

typedef struct _ArrangerPlayheadWidget
{
  GtkDrawingArea          parent_instance;
} ArrangerPlayheadWidget;

ArrangerPlayheadWidget *
arranger_playhead_widget_new ();

#endif
