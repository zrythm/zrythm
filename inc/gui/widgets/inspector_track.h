/*
 * gui/widgets/inspector_track.h - A inspector_track widget
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

#ifndef __GUI_WIDGETS_INSPECTOR_TRACK_H__
#define __GUI_WIDGETS_INSPECTOR_TRACK_H__

#include <gtk/gtk.h>

#define INSPECTOR_TRACK_WIDGET_TYPE                  (inspector_track_widget_get_type ())
G_DECLARE_FINAL_TYPE (InspectorTrackWidget,
                      inspector_track_widget,
                      INSPECTOR_TRACK,
                      WIDGET,
                      GtkGrid)

typedef struct Track Track;

typedef struct _InspectorTrackWidget
{
  GtkGrid             parent_instance;
  GtkLabel *          header;
  GtkBox *            position_box;
  GtkBox *            length_box;
  GtkColorButton *    color;
  GtkToggleButton *   mute_toggle;
} InspectorTrackWidget;

/**
 * Creates the inspector_track widget.
 *
 * Only once per project.
 */
InspectorTrackWidget *
inspector_track_widget_new ();

void
inspector_track_widget_show_tracks (InspectorTrackWidget * self,
                                      Track **               tracks,
                                      int                     num_tracks);

#endif



