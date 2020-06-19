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

#include "project.h"
#include "audio/transport.h"
#include "audio/midi.h"
#include "gui/widgets/transport_controls.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TransportControlsWidget,
               transport_controls_widget,
               GTK_TYPE_BOX)

static void
play_clicked_cb (
  GtkButton * button,
  gpointer    user_data)
{
  if (TRANSPORT->play_state == PLAYSTATE_ROLLING)
    {
      position_set_to_pos (
        &TRANSPORT->playhead_pos,
        &TRANSPORT->cue_pos);
    }
  else
    {
      transport_request_roll (TRANSPORT);
    }
}

static void
stop_clicked_cb (GtkButton *button,
               gpointer       user_data)
{
  /*g_message ("playstate %d", TRANSPORT->play_state);*/
  if (TRANSPORT->play_state == PLAYSTATE_PAUSED)
    {
      transport_set_playhead_pos (
        TRANSPORT, &TRANSPORT->cue_pos);
    }
  else
    transport_request_pause (TRANSPORT);

  midi_panic_all (1);
}

static void
record_toggled_cb (GtkToggleButton * tg,
                   gpointer        user_data)
{
  if (gtk_toggle_button_get_active (tg))
    {
      TRANSPORT->recording = 1;
    }
  else
    {
      TRANSPORT->recording = 0;
    }
}

static void
forward_clicked_cb (GtkButton * forward,
                    gpointer          user_data)
{
  Position *pos =
    snap_grid_get_nearby_snap_point (
      &PROJECT->snap_grid_timeline,
      &TRANSPORT->playhead_pos, 0);
  transport_move_playhead (
    TRANSPORT, pos, F_PANIC, F_SET_CUE_POINT);
}

static void
backward_clicked_cb (GtkButton * backward,
                     gpointer    user_data)
{
  Position * pos =
    snap_grid_get_nearby_snap_point (
      &PROJECT->snap_grid_timeline,
      &TRANSPORT->playhead_pos, 1);
  transport_move_playhead (
    TRANSPORT, pos, F_PANIC, F_SET_CUE_POINT);
}

static void
transport_controls_widget_class_init (
  TransportControlsWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "transport_controls.ui");

  gtk_widget_class_set_css_name (
    klass, "transport-controls");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, TransportControlsWidget, x)

  BIND_CHILD (play);
  BIND_CHILD (stop);
  BIND_CHILD (backward);
  BIND_CHILD (forward);
  BIND_CHILD (trans_record);
  BIND_CHILD (loop);
  BIND_CHILD (return_to_cue_toggle);

#undef BIND_CHILD
}

static void
transport_controls_widget_init (
  TransportControlsWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_toggle_button_set_active (
    self->loop,
    g_settings_get_boolean (S_TRANSPORT, "loop"));

  g_settings_bind (
    S_TRANSPORT, "return-to-cue",
    self->return_to_cue_toggle, "active",
    G_SETTINGS_BIND_DEFAULT);

  g_signal_connect (
    GTK_WIDGET (self->play), "clicked",
    G_CALLBACK (play_clicked_cb), NULL);
  g_signal_connect (
    GTK_WIDGET (self->stop), "clicked",
    G_CALLBACK (stop_clicked_cb), NULL);
  g_signal_connect (
    GTK_WIDGET (self->trans_record), "toggled",
    G_CALLBACK (record_toggled_cb), NULL);
  g_signal_connect (
    GTK_WIDGET(self->forward), "clicked",
    G_CALLBACK(forward_clicked_cb), NULL);
  g_signal_connect (
    GTK_WIDGET(self->backward), "clicked",
    G_CALLBACK(backward_clicked_cb), NULL);
}
