/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/engine.h"
#include "audio/exporter.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/dialogs/bounce_dialog.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  BounceDialogWidget,
  bounce_dialog_widget,
  GTK_TYPE_DIALOG)

static void
on_cancel_clicked (GtkButton * btn,
                   BounceDialogWidget * self)
{
  gtk_window_close (GTK_WINDOW (self));
  /*gtk_widget_destroy (GTK_WIDGET (self));*/
}

static void
on_bounce_clicked (
  GtkButton * btn,
  BounceDialogWidget * self)
{
  ExportSettings settings;
  switch (self->type)
    {
    case BOUNCE_DIALOG_REGIONS:
      timeline_selections_mark_for_bounce (
        TL_SELECTIONS);
      settings.mode = EXPORT_MODE_REGIONS;
      break;
    case BOUNCE_DIALOG_TRACKS:
      tracklist_selections_mark_for_bounce (
        TRACKLIST_SELECTIONS);
      settings.mode = EXPORT_MODE_TRACKS;
      break;
    }

  if (self->bounce_to_file)
    {
      /* TODO */
    }
  else
    {
      export_settings_set_bounce_defaults (
        &settings, NULL, self->bounce_name);
    }

  /* start exporting in a new thread */
  GThread * thread =
    g_thread_new (
      "bounce_thread",
      (GThreadFunc) exporter_generic_export_thread,
      &settings);

  /* create a progress dialog and block */
  ExportProgressDialogWidget * progress_dialog =
    export_progress_dialog_widget_new (
      &settings, 1, 0);
  gtk_window_set_transient_for (
    GTK_WINDOW (progress_dialog),
    GTK_WINDOW (self));
  gtk_dialog_run (GTK_DIALOG (progress_dialog));
  gtk_widget_destroy (GTK_WIDGET (progress_dialog));

  g_thread_join (thread);

  if (!self->bounce_to_file)
    {
      /* create audio track with bounced material */
      exporter_create_audio_track_after_bounce (
        &settings);
    }

  export_settings_free_members (&settings);
}

static void
on_tail_value_changed (
  GtkSpinButton *      spin,
  BounceDialogWidget * self)
{
  g_settings_set_int (
    S_UI, "bounce-tail",
    gtk_spin_button_get_value_as_int (spin));
}

/**
 * Creates a bounce dialog.
 */
BounceDialogWidget *
bounce_dialog_widget_new (
  BounceDialogWidgetType type,
  const char *           bounce_name)
{
  BounceDialogWidget * self =
    g_object_new (BOUNCE_DIALOG_WIDGET_TYPE, NULL);

  self->type = type;
  /* TODO free when destroying */
  self->bounce_name = g_strdup (bounce_name);

  g_signal_connect (
    G_OBJECT (self->tail_spin), "value-changed",
    G_CALLBACK (on_tail_value_changed), self);

  return self;
}

static void
bounce_dialog_widget_class_init (
  BounceDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "bounce_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, BounceDialogWidget, x)

  BIND_CHILD (cancel_btn);
  BIND_CHILD (bounce_btn);
  BIND_CHILD (with_effects_check);
  BIND_CHILD (tail_spin);

  gtk_widget_class_bind_template_callback (
    klass, on_cancel_clicked);
  gtk_widget_class_bind_template_callback (
    klass, on_bounce_clicked);
}

static void
bounce_dialog_widget_init (BounceDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON (self->with_effects_check),
    g_settings_get_boolean (
      S_UI, "bounce-with-effects"));
  gtk_spin_button_set_value (
    self->tail_spin,
    (double)
    g_settings_get_int (
      S_UI, "bounce-tail"));
}

