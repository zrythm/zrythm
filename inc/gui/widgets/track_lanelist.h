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
 * A multi-paned holding multiple
 * TrackLaneWidgets.
 */

#ifndef __GUI_WIDGETS_TRACK_LANELIST_H__
#define __GUI_WIDGETS_TRACK_LANELIST_H__

#include "gui/widgets/dzl/dzl-multi-paned.h"

#include <gtk/gtk.h>

#define TRACK_LANELIST_WIDGET_TYPE \
  (track_lanelist_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TrackLanelistWidget,
  track_lanelist_widget,
  Z, TRACK_LANELIST_WIDGET,
  DzlMultiPaned)

/**
 * A DzlMultiPaned holding multiple TrackLaneWidgets.
 */
typedef struct _TrackLanelistWidget
{
  DzlMultiPaned   parent_instance;

  /** The Track this is for. */
  Track *         track;
} TrackLanelistWidget;

/**
 * Creates a new TrackLanelistWidget.
 */
TrackLanelistWidget *
track_lanelist_widget_new (
  Track * track);

/**
 * Show or hide all automation track widgets.
 */
void
track_lanelist_widget_refresh (
  TrackLanelistWidget * self);

#endif
