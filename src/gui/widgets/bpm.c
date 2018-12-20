/*
 * gui/widgets/bpm.c - BPM widget
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

#include "audio/transport.h"
#include "gui/widgets/bpm.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (BpmWidget, bpm_widget, GTK_TYPE_SPIN_BUTTON)

static void
value_changed_cb (GtkSpinButton *spin_button,
               gpointer       user_data)
{
  transport_set_bpm (TRANSPORT,
                     AUDIO_ENGINE,
                     gtk_spin_button_get_value (spin_button));
}

static void
bpm_widget_class_init (BpmWidgetClass * klass)
{
}

static void
bpm_widget_init (BpmWidget * self)
{
}

BpmWidget *
bpm_widget_new ()
{
  BpmWidget * self = g_object_new (BPM_WIDGET_TYPE, NULL);
  GtkAdjustment * adj =
    gtk_adjustment_new (TRANSPORT->bpm,
                        40.f,
                        400.f,
                        1.f,
                        1.f,
                        0.f);
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (self),
                                  adj);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (self),
                              2);
  gtk_entry_set_input_purpose (GTK_ENTRY (self),
                               GTK_INPUT_PURPOSE_NUMBER);

  g_signal_connect (GTK_WIDGET (self),
                    "value-changed",
                    G_CALLBACK (value_changed_cb),
                    NULL);
  gtk_widget_show (GTK_WIDGET (self));
  return self;
}
