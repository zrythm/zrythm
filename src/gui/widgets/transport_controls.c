/*
 * gui/widgets/transport_controls.c - transport controls (play/pause/stop...)
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

#include "project.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

static void
play_clicked_cb (GtkButton *button,
               gpointer       user_data)
{
  if (TRANSPORT->play_state == PLAYSTATE_ROLLING)
    {
      position_set_to_pos (&TRANSPORT->playhead_pos,
                           &TRANSPORT->cue_pos);
    }
  else
    {
      transport_request_roll ();
    }
}

static void
stop_clicked_cb (GtkButton *button,
               gpointer       user_data)
{
  if (TRANSPORT->play_state == PLAYSTATE_PAUSED)
    {
      position_set_to_pos (&TRANSPORT->playhead_pos,
                           &TRANSPORT->cue_pos);
    }
  else
    transport_request_pause ();
}

static void
record_toggled_cb (GtkToggleButton * tg,
                   gpointer        user_data)
{
  /* TODO */
  if (gtk_toggle_button_get_active (tg))
    {
      GtkWidget * image = gtk_image_new_from_resource (
              "/online/alextee/zrythm/record-on.svg");
      gtk_button_set_image (GTK_BUTTON (MAIN_WINDOW->trans_record),
                            image);
    }
  else
    {
      GtkWidget * image = gtk_image_new_from_resource (
              "/online/alextee/zrythm/record.svg");
      gtk_button_set_image (GTK_BUTTON (MAIN_WINDOW->trans_record),
                            image);

    }
}

static void
loop_toggled_cb (GtkToggleButton * loop,
                 gpointer        user_data)
{
  if (gtk_toggle_button_get_active (loop))
    {
      TRANSPORT->loop = 1;
    }
  else
    {
      TRANSPORT->loop = 0;
    }
}

/**
 * Initializes the transport controls in the main window.
 */
void
transport_controls_init (MainWindowWidget * mw)
{
  /* add icons */
  GtkWidget * image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/play.svg");
  gtk_button_set_image (mw->play, image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/stop.svg");
  gtk_button_set_image (mw->stop, image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/record.svg");
  gtk_button_set_image (GTK_BUTTON (mw->trans_record), image);

  g_signal_connect (GTK_WIDGET (mw->play),
                      "clicked",
                      G_CALLBACK (play_clicked_cb),
                      NULL);
  g_signal_connect (GTK_WIDGET (mw->stop),
                      "clicked",
                      G_CALLBACK (stop_clicked_cb),
                      NULL);
  g_signal_connect (GTK_WIDGET (mw->trans_record),
                      "toggled",
                      G_CALLBACK (record_toggled_cb),
                      NULL);
  GTK_WIDGET (mw->loop);
  g_signal_connect (GTK_WIDGET (mw->loop),
                      "toggled",
                      G_CALLBACK (loop_toggled_cb),
                      NULL);
}

