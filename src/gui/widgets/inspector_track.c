/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/center_dock.h"
#include "gui/widgets/fader_controls_expander.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/plugin_strip_expander.h"
#include "gui/widgets/ports_expander.h"
#include "gui/widgets/track_input_expander.h"
#include "gui/widgets/track_properties_expander.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  InspectorTrackWidget,
  inspector_track_widget,
  GTK_TYPE_BOX)

void
inspector_track_widget_show_tracks (
  InspectorTrackWidget * self,
  TracklistSelections *  tls)
{
  if (gtk_notebook_get_current_page (
        GTK_NOTEBOOK (
          MW_LEFT_DOCK_EDGE->inspector_notebook)) !=
      0)
    {
      gtk_notebook_set_current_page (
        GTK_NOTEBOOK (
          MW_LEFT_DOCK_EDGE->inspector_notebook), 0);
    }

  /* show info for first track */
  Track * track = NULL;
  if (tls->num_tracks > 0)
    {
      track = tls->tracks[0];

      track_properties_expander_widget_refresh (
        self->instrument_track_info,
        track);

      gtk_widget_set_visible (
        GTK_WIDGET (self->sends), false);
      gtk_widget_set_visible (
        GTK_WIDGET (self->controls), false);
      gtk_widget_set_visible (
        GTK_WIDGET (self->inputs), false);
      gtk_widget_set_visible (
        GTK_WIDGET (self->inserts), false);
      gtk_widget_set_visible (
        GTK_WIDGET (self->fader), false);

      if (track_type_has_channel (track->type))
        {
          gtk_widget_set_visible (
            GTK_WIDGET (self->sends), true);
          gtk_widget_set_visible (
            GTK_WIDGET (self->controls), true);
          gtk_widget_set_visible (
            GTK_WIDGET (self->fader), true);

          if (track_has_inputs (track))
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self->inputs), true);
              track_input_expander_widget_refresh (
                self->inputs, track);
            }
          ports_expander_widget_setup_track (
            self->sends,
            track, PE_TRACK_PORT_TYPE_SENDS);
          ports_expander_widget_setup_track (
            self->controls,
            track, PE_TRACK_PORT_TYPE_CONTROLS);

          plugin_strip_expander_widget_setup (
            self->inserts, PSE_TYPE_INSERTS,
            PSE_POSITION_INSPECTOR, track);

          fader_controls_expander_widget_setup (
            self->fader, track);
        }
    }
  else /* no tracks selected */
    {
      track_properties_expander_widget_refresh (
        self->instrument_track_info, NULL);
      ports_expander_widget_setup_track (
        self->sends,
        track, PE_TRACK_PORT_TYPE_SENDS);
      ports_expander_widget_setup_track (
        self->sends,
        track, PE_TRACK_PORT_TYPE_CONTROLS);
      /*gtk_widget_set_visible (*/
        /*GTK_WIDGET (self->sends), 0);*/
      /*gtk_widget_set_visible (*/
        /*GTK_WIDGET (self->inputs), 0);*/
      /*gtk_widget_set_visible (*/
        /*GTK_WIDGET (self->controls), 0);*/
    }
}

/**
 * Sets up the inspector track widget for the first
 * time.
 */
void
inspector_track_widget_setup (
  InspectorTrackWidget * self,
  TracklistSelections *  tls)
{
  g_return_if_fail (tls);
  Track * track = tls->tracks[0];
  g_return_if_fail (track);

  track_properties_expander_widget_setup (
    self->instrument_track_info,
    track);
}

InspectorTrackWidget *
inspector_track_widget_new (void)
{
  InspectorTrackWidget * self =
    g_object_new (
      INSPECTOR_TRACK_WIDGET_TYPE, NULL);

  return self;
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
  BIND_CHILD (controls);
  BIND_CHILD (inputs);
  BIND_CHILD (inserts);
  BIND_CHILD (fader);

#undef BIND_CHILD
}

static void
inspector_track_widget_init (
  InspectorTrackWidget * self)
{
  g_type_ensure (
    TRACK_PROPERTIES_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    TRACK_INPUT_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    PORTS_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    PLUGIN_STRIP_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    FADER_CONTROLS_EXPANDER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  z_gtk_widget_add_style_class (
    GTK_WIDGET (self), "inspector");

  expander_box_widget_set_vexpand (
    Z_EXPANDER_BOX_WIDGET (self->inserts), false);
}
