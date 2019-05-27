/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * The background (main overlay child) of the
 * RulerTracklistArrangerWidget.
 */

#ifndef __GUI_WIDGETS_RULER_TRACKLIST_ARRANGER_BG_H__
#define __GUI_WIDGETS_RULER_TRACKLIST_ARRANGER_BG_H__

#include "gui/widgets/arranger_bg.h"

#include <gtk/gtk.h>

#define RULER_TRACKLIST_ARRANGER_BG_WIDGET_TYPE \
  (ruler_tracklist_arranger_bg_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RulerTracklistArrangerBgWidget,
  ruler_tracklist_arranger_bg_widget,
  Z, RULER_TRACKLIST_ARRANGER_BG_WIDGET,
  ArrangerBgWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define RULER_TRACKLIST_ARRANGER_BG \
  Z_RULER_TRACKLIST_ARRANGER_BG_WIDGET ( \
    arranger_widget_get_private ( \
      Z_ARRANGER_WIDGET ( \
        MW_RULER_TRACKLIST_ARRANGER))->bg)

typedef struct _RulerTracklistArrangerBgWidget
{
  ArrangerBgWidget     parent_instance;
} RulerTracklistArrangerBgWidget;

/**
 * Creates a ruler_tracklist_arranger widget using the given ruler_tracklist_arranger data.
 */
RulerTracklistArrangerBgWidget *
ruler_tracklist_arranger_bg_widget_new (
  RulerWidget *    ruler,
  ArrangerWidget * arranger);

/**
 * @}
 */

#endif
