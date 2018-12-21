/*
 * gui/widgets/track.h - Track view
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

#ifndef __GUI_WIDGETS_MASTER_TRACK_H__
#define __GUI_WIDGETS_MASTER_TRACK_H__

#include "audio/channel.h"
#include "gui/widgets/track.h"

#include <gtk/gtk.h>

#define MASTER_TRACK_WIDGET_TYPE (master_track_widget_get_type ())
G_DECLARE_FINAL_TYPE (MasterTrackWidget,
                      master_track_widget,
                      MASTER_TRACK,
                      WIDGET,
                      GtkPaned)

typedef struct _AutomationTracklistWidget AutomationTracklistWidget;
typedef struct MasterTrack MasterTrack;

/**
 * Top is the track part and bot is the automation part
 */
typedef struct _MasterTrackWidget
{
  GtkPaned                      parent_instance;
  TrackWidget *                 parent;
  GtkBox *                      track_box;
  GtkGrid *                     track_grid;
  GtkLabel *                    track_name;
  GtkButton *                   record;
  GtkButton *                   solo;
  GtkButton *                   mute;
  GtkButton *                   show_automation;
} MasterTrackWidget;

/**
 * Creates a new track widget from the given track.
 */
MasterTrackWidget *
master_track_widget_new (TrackWidget *     parent);

/**
 * Updates changes in the backend to the ui
 */
void
master_track_widget_refresh (MasterTrackWidget * self);

#endif

