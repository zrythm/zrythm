/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/marker_track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/bounce_step_selector.h"
#include "gui/widgets/dialogs/bounce_dialog.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/gtk.h"
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
      settings.mode = EXPORT_MODE_REGIONS;
      break;
    case BOUNCE_DIALOG_TRACKS:
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

  Position start_pos;
  position_init (&start_pos);
  switch (self->type)
    {
    case BOUNCE_DIALOG_REGIONS:
      timeline_selections_mark_for_bounce (
        TL_SELECTIONS,
        settings.bounce_with_parents);
      settings.mode = EXPORT_MODE_REGIONS;
      arranger_selections_get_start_pos (
        (ArrangerSelections *) TL_SELECTIONS,
        &start_pos, F_GLOBAL);
      break;
    case BOUNCE_DIALOG_TRACKS:
      {
        tracklist_selections_mark_for_bounce (
          TRACKLIST_SELECTIONS,
          settings.bounce_with_parents,
          F_NO_MARK_MASTER);
        settings.mode = EXPORT_MODE_TRACKS;

        /* start at start marker */
        Marker * m =
          marker_track_get_start_marker (
            P_MARKER_TRACK);
        ArrangerObject * m_obj =
          (ArrangerObject *) m;
        position_set_to_pos (
          &start_pos, &m_obj->pos);
      }
      break;
    }

  GPtrArray * conns =
    exporter_prepare_tracks_for_export (&settings);

  /* start exporting in a new thread */
  GThread * thread =
    g_thread_new (
      "bounce_thread",
      (GThreadFunc) exporter_generic_export_thread,
      &settings);

  /* create a progress dialog and block */
  ExportProgressDialogWidget * progress_dialog =
    export_progress_dialog_widget_new (
      &settings, true, false, F_CANCELABLE);
  gtk_window_set_transient_for (
    GTK_WINDOW (progress_dialog),
    GTK_WINDOW (self));
  z_gtk_dialog_run (
    GTK_DIALOG (progress_dialog), true);

  g_thread_join (thread);

  exporter_return_connections_post_export (
    &settings, conns);

  if (!self->bounce_to_file &&
      !settings.progress_info.has_error &&
      !settings.progress_info.cancelled)
    {
      /* create audio track with bounced material */
      exporter_create_audio_track_after_bounce (
        &settings, &start_pos);
    }

  export_settings_free_members (&settings);

  gtk_window_close (GTK_WINDOW (self));
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

static void
on_bounce_with_parents_toggled (
  GtkCheckButton *     btn,
  BounceDialogWidget * self)
{
  bool toggled = gtk_check_button_get_active (btn);
  gtk_widget_set_visible (
    GTK_WIDGET (self->bounce_step_box), !toggled);
  g_settings_set_boolean (
    S_UI, "bounce-with-parents", toggled);
}

static void
on_disable_after_bounce_toggled (
  GtkCheckButton *     btn,
  BounceDialogWidget * self)
{
  bool toggled = gtk_check_button_get_active (btn);
  g_settings_set_boolean (
    S_UI, "disable-after-bounce", toggled);
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

  self->bounce_step_selector =
    bounce_step_selector_widget_new ();
  gtk_box_append (
    GTK_BOX (self->bounce_step_box),
    GTK_WIDGET (self->bounce_step_selector));

  g_signal_connect (
    G_OBJECT (self->tail_spin), "value-changed",
    G_CALLBACK (on_tail_value_changed), self);
  g_signal_connect (
    G_OBJECT (self->bounce_with_parents), "toggled",
    G_CALLBACK (on_bounce_with_parents_toggled),
    self);

  if (type == BOUNCE_DIALOG_TRACKS)
    {
      g_signal_connect (
        G_OBJECT (self->disable_after_bounce),
        "toggled",
        G_CALLBACK (on_disable_after_bounce_toggled),
        self);
    }
  else
    {
      /* hide 'disable after bounce' if not bouncing
       * tracks */
      gtk_widget_set_visible (
        GTK_WIDGET (self->disable_after_bounce),
        false);
    }

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
  BIND_CHILD (bounce_with_parents);
  BIND_CHILD (bounce_step_box);
  BIND_CHILD (tail_spin);
  BIND_CHILD (disable_after_bounce);

  gtk_widget_class_bind_template_callback (
    klass, on_cancel_clicked);
  gtk_widget_class_bind_template_callback (
    klass, on_bounce_clicked);
}

static void
bounce_dialog_widget_init (
  BounceDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_spin_button_set_value (
    self->tail_spin,
    (double)
    g_settings_get_int (S_UI, "bounce-tail"));

  bool bounce_with_parents =
    g_settings_get_boolean (
      S_UI, "bounce-with-parents");
  gtk_check_button_set_active (
    GTK_CHECK_BUTTON (self->bounce_with_parents),
    bounce_with_parents);
  gtk_widget_set_visible (
    GTK_WIDGET (self->bounce_step_box),
    !bounce_with_parents);

  bool disable_after_bounce =
    g_settings_get_boolean (
      S_UI, "disable-after-bounce");
  gtk_check_button_set_active (
    GTK_CHECK_BUTTON (self->disable_after_bounce),
    disable_after_bounce);
}
