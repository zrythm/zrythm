/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Ruler marker.
 *
 * For simplicity, each marker (loop start/end, song
 * start/end, etc.) should be one instance of this,
 * identifiable by the type member.
 */

#if 0

#  ifndef __GUI_WIDGETS_RULER_MARKER_H__
#    define __GUI_WIDGETS_RULER_MARKER_H__

#    include "utils/ui.h"

#    include <gtk/gtk.h>

#    define RULER_MARKER_WIDGET_TYPE \
      (ruler_marker_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RulerMarkerWidget,
  ruler_marker_widget,
  Z, RULER_MARKER_WIDGET,
  GtkDrawingArea)

typedef struct _RulerWidget RulerWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#    define TL_RULER_PLAYHEAD \
      (ruler_widget_get_private (Z_RULER_WIDGET (MW_RULER)) \
         ->playhead)
#    define EDITOR_RULER_PLAYHEAD \
      (ruler_widget_get_private ( \
         Z_RULER_WIDGET (EDITOR_RULER)) \
         ->playhead)
#    define AUDIO_RULER_PLAYHEAD \
      (ruler_widget_get_private (Z_RULER_WIDGET (AUDIO_RULER)) \
         ->playhead)

typedef enum RulerMarkerType
{
  RULER_MARKER_TYPE_LOOP_START,
  RULER_MARKER_TYPE_LOOP_END,
  RULER_MARKER_TYPE_CUE_POINT,
  RULER_MARKER_TYPE_CLIP_START,
  RULER_MARKER_TYPE_PLAYHEAD,
} RulerMarkerType;

typedef struct _RulerMarkerWidget
{
  GtkDrawingArea          parent_instance;
  UiCursorState           cursor_state;
  RulerMarkerType         type;
  RulerWidget *           ruler; ///< owner
  GtkWindow *            tooltip_win;
  GtkLabel *             tooltip_label;
} RulerMarkerWidget;

RulerMarkerWidget *
ruler_marker_widget_new (
  RulerWidget * ruler,
  RulerMarkerType type);

/**
 * Updates the tooltips.
 */
void
ruler_marker_widget_update_tooltip (
  RulerMarkerWidget * self,
  int              show);

/**
 * @}
 */

#  endif
#endif
