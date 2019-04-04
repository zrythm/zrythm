/*
 * gui/widgets/inspector_track.c - A inspector_track widget
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

#include "gui/backend/events.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/inspector_track.h"
#include "project.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (InspectorTrackWidget,
               inspector_track_widget,
               GTK_TYPE_GRID)

static void
on_name_activate (
  GtkEntry * entry,
  InspectorTrackWidget * self)
{
  Track * track = TRACKLIST_SELECTIONS->tracks[0];
  track->name =
    g_strdup (gtk_entry_get_text (entry));

  EVENTS_PUSH (ET_TRACK_NAME_CHANGED,
               track);
}

static void
on_color_set (
  GtkColorButton *btn,
  InspectorTrackWidget * self)
{
  Track * track;
  for (int i = 0;
       i < TRACKLIST_SELECTIONS->num_tracks; i++)
    {
      track = TRACKLIST_SELECTIONS->tracks[i];
      gtk_color_chooser_get_rgba (
        GTK_COLOR_CHOOSER (btn),
        &track->color);

      EVENTS_PUSH (ET_TRACK_COLOR_CHANGED,
                   track);
    }
}

InspectorTrackWidget *
inspector_track_widget_new ()
{
  InspectorTrackWidget * self =
    g_object_new (INSPECTOR_TRACK_WIDGET_TYPE, NULL);
  gtk_widget_show_all (GTK_WIDGET (self));

  return self;
}

void
inspector_track_widget_show_tracks (
  InspectorTrackWidget * self,
  TracklistSelections *  tls)
{
  g_warn_if_fail (tls->num_tracks > 0);

  if (tls->num_tracks == 1)
    {
      gtk_label_set_text (self->header, _("Track"));
    }
  else
    {
      char * string =
        g_strdup_printf (_("Tracks (%d)"),
                         tls->num_tracks);
      gtk_label_set_text (self->header, string);
      g_free (string);
    }

  /* show info for first track */
  Track * track = tls->tracks[0];

  gtk_entry_set_text (self->name,
                      track->name);
  gtk_color_chooser_set_rgba (
    GTK_COLOR_CHOOSER (self->color),
    &track->color);
}

static void
inspector_track_widget_class_init (
  InspectorTrackWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "inspector_track.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    GTK_WIDGET_CLASS (klass), \
    InspectorTrackWidget, \
    child);

  BIND_CHILD (position_box);
  BIND_CHILD (name);
  BIND_CHILD (length_box);
  BIND_CHILD (color);
  BIND_CHILD (mute_toggle);
  BIND_CHILD (header);

#undef BIND_CHILD
}

static void
inspector_track_widget_init (
  InspectorTrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (
    G_OBJECT (self->color), "color-set",
    G_CALLBACK (on_color_set), self);
  g_signal_connect (
    G_OBJECT (self->name), "activate",
    G_CALLBACK (on_name_activate), self);
}
