/*
 * gui/widgets/track.c - Track
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

/** \file
 */

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/bus_track.h"
#include "audio/master_track.h"
#include "audio/track.h"
#include "audio/region.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/master_track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MasterTrackWidget,
               master_track_widget,
               GTK_TYPE_PANED)

#define GET_TRACK(self) Track * track = self->parent->track

static void
on_show_automation (GtkWidget * widget, void * data)
{
  MasterTrackWidget * self =
    MASTER_TRACK_WIDGET (data);

  GET_TRACK (self);

  /* toggle visibility flag */
  track->bot_paned_visible =
    track->bot_paned_visible ? 0 : 1;

  /* FIXME rename to refresh */
  tracklist_widget_show (MW_TRACKLIST);
}

/**
 * Creates a new track widget using the given track.
 *
 * 1 track has 1 track widget.
 * The track widget must always have at least 1 automation track in the automation
 * paned.
 */
MasterTrackWidget *
master_track_widget_new (TrackWidget *     parent)
{
  MasterTrackWidget * self = g_object_new (
                            MASTER_TRACK_WIDGET_TYPE,
                            NULL);
  self->parent = parent;

  GET_TRACK (self);
  AutomationTracklist * automation_tracklist =
    track_get_automation_tracklist (track);
  automation_tracklist->widget =
    automation_tracklist_widget_new (automation_tracklist);
  gtk_paned_pack2 (GTK_PANED (self),
                   GTK_WIDGET (automation_tracklist->widget),
                   Z_GTK_RESIZE,
                   Z_GTK_NO_SHRINK);

  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);

  return self;
}

void
master_track_widget_refresh (MasterTrackWidget * self)
{
  GET_TRACK (self);
  Channel * channel = track_get_channel (track);
  gtk_label_set_text (self->track_name,
                      channel->name);

  AutomationTracklist * automation_tracklist =
    track_get_automation_tracklist (track);
  automation_tracklist_widget_refresh (
    automation_tracklist->widget);
}

static void
master_track_widget_init (MasterTrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
master_track_widget_class_init (MasterTrackWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "master_track.ui");

  gtk_widget_class_bind_template_child (
    klass,
    MasterTrackWidget,
    track_box);
  gtk_widget_class_bind_template_child (
    klass,
    MasterTrackWidget,
    track_grid);
  gtk_widget_class_bind_template_child (
    klass,
    MasterTrackWidget,
    track_name);
  gtk_widget_class_bind_template_child (
    klass,
    MasterTrackWidget,
    record);
  gtk_widget_class_bind_template_child (
    klass,
    MasterTrackWidget,
    solo);
  gtk_widget_class_bind_template_child (
    klass,
    MasterTrackWidget,
    mute);
  gtk_widget_class_bind_template_child (
    klass,
    MasterTrackWidget,
    show_automation);
  gtk_widget_class_bind_template_callback (
    klass,
    on_show_automation);
}

