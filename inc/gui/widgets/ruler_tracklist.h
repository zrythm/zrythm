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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * The ruler tracklist contains special tracks that
 * are shown above the normal tracklist (Chord
 * tracks, Marker tracks, etc.).
 */

#ifndef __GUI_WIDGETS_RULER_TRACKLIST_H__
#define __GUI_WIDGETS_RULER_TRACKLIST_H__

#include "audio/region.h"
#include "gui/widgets/region.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#define RULER_TRACKLIST_WIDGET_TYPE \
  (ruler_tracklist_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RulerTracklistWidget,
  ruler_tracklist_widget,
  Z, RULER_TRACKLIST_WIDGET,
  GtkBox);

#define MW_RULER_TRACKLIST \
  MW_CENTER_DOCK->ruler_tracklist

/**
 * The RulerTracklistWidget contains special tracks
 * (chord, marker, etc.) as thin boxes above the
 * normal tracklist.
 *
 * The contents of each track will be shown in the
 * RulerTracklistArrangerWidget.
 */
typedef struct _RulerTracklistWidget
{
  GtkBox     parent_instance;
} RulerTracklistWidget;

#endif
