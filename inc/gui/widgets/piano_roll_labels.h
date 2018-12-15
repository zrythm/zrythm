/*
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

/** \file */

#ifndef __GUI_WIDGETS_PIANO_ROLL_LABELS_H__
#define __GUI_WIDGETS_PIANO_ROLL_LABELS_H__

#include "gui/widgets/piano_roll.h"

#include <gtk/gtk.h>

#define PIANO_ROLL_LABELS_WIDGET_TYPE                  (piano_roll_labels_widget_get_type ())
G_DECLARE_FINAL_TYPE (PianoRollLabelsWidget,
                      piano_roll_labels_widget,
                      PIANO_ROLL_LABELS,
                      WIDGET,
                      GtkDrawingArea)

#define DEFAULT_PX_PER_NOTE 8
#define PIANO_ROLL_LABELS PIANO_ROLL->piano_roll_labels

typedef struct _PianoRollLabelsWidget
{
  GtkDrawingArea          parent_instance;
  int                     zoom_level;
  int                     px_per_note; ///< adjusted for zoom level
  int                     total_px;
  GtkGestureDrag          * drag;
  GtkGestureMultiPress    * multipress;
} PianoRollLabelsWidget;

#endif


