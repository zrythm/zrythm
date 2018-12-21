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
#include "audio/bus_track.h"
#include "audio/track.h"
#include "audio/region.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/bus_track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (BusTrackWidget,
               bus_track_widget,
               GTK_TYPE_PANED)

#define GET_TRACK(self) Track * track = self->parent->track

static void
on_show_automation (GtkWidget * widget, void * data)
{
  BusTrackWidget * self =
    BUS_TRACK_WIDGET (data);

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
BusTrackWidget *
bus_track_widget_new (TrackWidget *     parent)
{
  BusTrackWidget * self = g_object_new (
                            BUS_TRACK_WIDGET_TYPE,
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

  gtk_container_add (
    GTK_CONTAINER (self->solo),
    gtk_image_new_from_resource (
     "/org/zrythm/solo.svg"));
  gtk_container_add (
    GTK_CONTAINER (self->mute),
    gtk_image_new_from_resource (
     "/org/zrythm/mute.svg"));
  gtk_container_add (
    GTK_CONTAINER (self->record),
    gtk_image_new_from_icon_name ("gtk-media-record",
                                  GTK_ICON_SIZE_BUTTON));
  gtk_widget_set_size_request (GTK_WIDGET (self->record),
                               16,
                               16);
  gtk_container_add (
    GTK_CONTAINER (self->show_automation),
    gtk_image_new_from_icon_name ("gtk-justify-fill",
                                  GTK_ICON_SIZE_BUTTON));

  gtk_image_set_from_resource (self->icon,
                               "/org/zrythm/bus.svg");

  g_signal_connect (self->show_automation, "clicked",
                    G_CALLBACK (on_show_automation), self);

  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);

  return self;
}

void
bus_track_widget_refresh (BusTrackWidget * self)
{
  BusTrack * it = (BusTrack *) self->parent->track;
  gtk_label_set_text (self->track_name,
                      ((BusTrack *)it)->channel->name);

  GET_TRACK (self);
  AutomationTracklist * automation_tracklist =
    track_get_automation_tracklist (track);
  automation_tracklist_widget_refresh (
    automation_tracklist->widget);
}

static void
bus_track_widget_init (BusTrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bus_track_widget_class_init (BusTrackWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
    GTK_WIDGET_CLASS (klass),
    "/org/zrythm/ui/bus_track.ui");

  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    BusTrackWidget,
    track_box);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    BusTrackWidget,
    track_grid);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    BusTrackWidget,
    track_name);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    BusTrackWidget,
    record);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    BusTrackWidget,
    solo);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    BusTrackWidget,
    mute);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    BusTrackWidget,
    show_automation);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    BusTrackWidget,
    icon);
}

