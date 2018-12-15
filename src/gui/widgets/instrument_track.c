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
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "audio/region.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (InstrumentTrackWidget,
               instrument_track_widget,
               GTK_TYPE_PANED)

static void
on_show_automation (GtkWidget * widget, void * data)
{
  TrackWidget * self = TRACK_WIDGET (data);

  /* toggle visibility flag */
  InstrumentTrack * ins_track =
    (InstrumentTrack *) self->track;
  ((Track *)ins_track)->bot_paned_visible =
    ((Track *)ins_track)->bot_paned_visible ? 0 : 1;

  tracklist_widget_show (MW_TRACKLIST);
}

/**
 * Creates a new track widget using the given track.
 *
 * 1 track has 1 track widget.
 * The track widget must always have at least 1 automation track in the automation
 * paned.
 */
InstrumentTrackWidget *
instrument_track_widget_new (InstrumentTrack * ins_track,
                             TrackWidget *     parent)
{
  InstrumentTrackWidget * self = g_object_new (
                            INSTRUMENT_TRACK_WIDGET_TYPE,
                            NULL);
  self->parent = parent;

  self->automation_tracklist_widget = automation_tracklist_widget_new (self->parent);

  instrument_track_widget_update_all (self);

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

  switch (ins_track->channel->type)
    {
    case CT_MIDI:
      gtk_image_set_from_resource (self->icon,
                                   "/org/zrythm/instrument.svg");
      break;
    case CT_AUDIO:
    case CT_MASTER:
      gtk_image_set_from_resource (self->icon,
                                   "/org/zrythm/audio.svg");
      break;
    case CT_BUS:
      gtk_image_set_from_resource (self->icon,
                                   "/org/zrythm/bus.svg");
      break;
    }

  g_signal_connect (self->show_automation, "clicked",
                    G_CALLBACK (on_show_automation), self);

  return self;
}

static void
instrument_track_widget_init (InstrumentTrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
instrument_track_widget_class_init (InstrumentTrackWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
    GTK_WIDGET_CLASS (klass),
    "/org/zrythm/ui/instrument_track.ui");

  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InstrumentTrackWidget,
    track_box);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InstrumentTrackWidget,
    track_grid);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InstrumentTrackWidget,
    track_name);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InstrumentTrackWidget,
    record);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InstrumentTrackWidget,
    solo);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InstrumentTrackWidget,
    mute);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InstrumentTrackWidget,
    show_automation);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InstrumentTrackWidget,
    icon);
}

void
instrument_track_widget_update_all (InstrumentTrackWidget * self)
{
  InstrumentTrack * it = (InstrumentTrack *) self->parent->track;
  gtk_label_set_text (self->track_name,
                      it->channel->name);

  for (int i = 0; i < it->num_automation_tracks; i++)
    {
      AutomationTrack * at = it->automation_tracks[i];
      if (at->widget)
        {
          automation_track_widget_update (at->widget);
        }
    }
}

/**
 * Makes sure the track widget and its elements have the visibility they should.
 */
void
instrument_track_widget_show (InstrumentTrackWidget * self)
{
  g_message ("showing track widget for %s",
             ((InstrumentTrack *) self->parent->track)->channel->name);
  gtk_widget_show (GTK_WIDGET (self));
  gtk_widget_show_all (GTK_WIDGET (self->track_box));
  automation_tracklist_widget_show (self->automation_tracklist_widget);
}


