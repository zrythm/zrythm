/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/backend/events.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/instrument_track_info_expander.h"
#include "gui/widgets/ports_expander.h"
#include "project.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (InspectorTrackWidget,
               inspector_track_widget,
               GTK_TYPE_BOX)

void
inspector_track_widget_show_tracks (
  InspectorTrackWidget * self,
  TracklistSelections *  tls)
{
  /* show info for first track */
  Track * track;
  if (tls->num_tracks > 0)
    {
      track = tls->tracks[0];

      instrument_track_info_expander_widget_setup (
        self->instrument_track_info,
        track);

      gtk_widget_set_visible (
        GTK_WIDGET (self->sends), 0);
      gtk_widget_set_visible (
        GTK_WIDGET (self->stereo_in), 0);
      gtk_widget_set_visible (
        GTK_WIDGET (self->midi_in), 0);
      gtk_widget_set_visible (
        GTK_WIDGET (self->midi_out), 0);

      if (track->channel)
        {
          gtk_widget_set_visible (
            GTK_WIDGET (self->sends), 1);
          /*gtk_widget_set_visible (*/
            /*GTK_WIDGET (self->stereo_in), 1);*/
          /*gtk_widget_set_visible (*/
            /*GTK_WIDGET (self->midi_in), 1);*/
          /*gtk_widget_set_visible (*/
            /*GTK_WIDGET (self->midi_out), 1);*/

          ports_expander_widget_setup_track (
            self->sends,
            track, PE_TRACK_PORT_TYPE_SENDS);
          /*ports_expander_widget_setup_track (*/
            /*self->stereo_in,*/
            /*track, PE_TRACK_PORT_TYPE_STEREO_IN);*/
          /*ports_expander_widget_setup_track (*/
            /*self->midi_in,*/
            /*track, PE_TRACK_PORT_TYPE_MIDI_IN);*/
          /*ports_expander_widget_setup_track (*/
            /*self->midi_out,*/
            /*track, PE_TRACK_PORT_TYPE_MIDI_OUT);*/
        }
      else
        {
          /*gtk_widget_set_visible (*/
            /*GTK_WIDGET (self->prefader_sends), 0);*/
          /*gtk_widget_set_visible (*/
            /*GTK_WIDGET (self->postfader_sends), 0);*/
          /*gtk_widget_set_visible (*/
            /*GTK_WIDGET (self->stereo_in), 0);*/
          /*gtk_widget_set_visible (*/
            /*GTK_WIDGET (self->midi_in), 0);*/
          /*gtk_widget_set_visible (*/
            /*GTK_WIDGET (self->midi_out), 0);*/
        }
    }
}

static void
inspector_track_widget_class_init (
  InspectorTrackWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "inspector_track.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    GTK_WIDGET_CLASS (klass), \
    InspectorTrackWidget, \
    child);

  BIND_CHILD (instrument_track_info);
  BIND_CHILD (sends);
  BIND_CHILD (stereo_in);
  BIND_CHILD (midi_in);
  BIND_CHILD (midi_out);

#undef BIND_CHILD
}

static void
inspector_track_widget_init (
  InspectorTrackWidget * self)
{
  g_type_ensure (
    INSTRUMENT_TRACK_INFO_EXPANDER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));
}
