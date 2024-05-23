// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/exporter.h"
#include "dsp/marker_track.h"
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
#include "utils/objects.h"
#include "utils/progress_info.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (BounceDialogWidget, bounce_dialog_widget, GTK_TYPE_DIALOG)

static void
on_cancel_clicked (GtkButton * btn, BounceDialogWidget * self)
{
  gtk_window_close (GTK_WINDOW (self));
  /*gtk_widget_destroy (GTK_WIDGET (self));*/
}

static void
progress_close_cb (ExportData * data)
{
  g_thread_join (data->thread);

  exporter_post_export (data->info, data->conns, data->state);

  BounceDialogWidget * self = Z_BOUNCE_DIALOG_WIDGET (data->parent_owner);

  ProgressInfo * pinfo = data->info->progress_info;
  if (
    !self->bounce_to_file
    && progress_info_get_completion_type (pinfo) == PROGRESS_COMPLETED_SUCCESS)
    {
      /* create audio track with bounced material */
      exporter_create_audio_track_after_bounce (data->info, &self->start_pos);
    }

  gtk_window_close (GTK_WINDOW (self));
}

static void
on_bounce_clicked (GtkButton * btn, BounceDialogWidget * self)
{
  ExportSettings * info = export_settings_new ();
  ExportData *     data = export_data_new (GTK_WIDGET (self), info);

  switch (self->type)
    {
    case BounceDialogWidgetType::BOUNCE_DIALOG_REGIONS:
      data->info->mode = ExportMode::EXPORT_MODE_REGIONS;
      break;
    case BounceDialogWidgetType::BOUNCE_DIALOG_TRACKS:
      data->info->mode = ExportMode::EXPORT_MODE_TRACKS;
      break;
    }

  if (self->bounce_to_file)
    {
      /* TODO */
      g_return_if_reached ();
    }
  else
    {
      export_settings_set_bounce_defaults (
        data->info, ExportFormat::EXPORT_FORMAT_WAV, NULL, self->bounce_name);
    }

  position_init (&self->start_pos);
  switch (self->type)
    {
    case BounceDialogWidgetType::BOUNCE_DIALOG_REGIONS:
      timeline_selections_mark_for_bounce (
        TL_SELECTIONS, data->info->bounce_with_parents);
      data->info->mode = ExportMode::EXPORT_MODE_REGIONS;
      arranger_selections_get_start_pos (
        (ArrangerSelections *) TL_SELECTIONS, &self->start_pos, F_GLOBAL);
      break;
    case BounceDialogWidgetType::BOUNCE_DIALOG_TRACKS:
      {
        tracklist_selections_mark_for_bounce (
          TRACKLIST_SELECTIONS, data->info->bounce_with_parents,
          F_NO_MARK_MASTER);
        data->info->mode = ExportMode::EXPORT_MODE_TRACKS;

        /* start at start marker */
        Marker *         m = marker_track_get_start_marker (P_MARKER_TRACK);
        ArrangerObject * m_obj = (ArrangerObject *) m;
        position_set_to_pos (&self->start_pos, &m_obj->pos);
      }
      break;
    }

  data->conns = exporter_prepare_tracks_for_export (data->info, data->state);

  /* start exporting in a new thread */
  data->thread = g_thread_new (
    "bounce_thread", (GThreadFunc) exporter_generic_export_thread, data->info);

  /* create a progress dialog and block */
  ExportProgressDialogWidget * progress_dialog =
    export_progress_dialog_widget_new (
      data, true, progress_close_cb, false, F_CANCELABLE);
  adw_dialog_present (ADW_DIALOG (progress_dialog), GTK_WIDGET (self));
}

static void
on_tail_value_changed (GtkSpinButton * spin, BounceDialogWidget * self)
{
  g_settings_set_int (
    S_UI, "bounce-tail", gtk_spin_button_get_value_as_int (spin));
}

static void
on_bounce_with_parents_toggled (GtkCheckButton * btn, BounceDialogWidget * self)
{
  bool toggled = gtk_check_button_get_active (btn);
  gtk_widget_set_visible (GTK_WIDGET (self->bounce_step_box), !toggled);
  g_settings_set_boolean (S_UI, "bounce-with-parents", toggled);
}

static void
on_disable_after_bounce_toggled (GtkCheckButton * btn, BounceDialogWidget * self)
{
  bool toggled = gtk_check_button_get_active (btn);
  g_settings_set_boolean (S_UI, "disable-after-bounce", toggled);
}

/**
 * Creates a bounce dialog.
 */
BounceDialogWidget *
bounce_dialog_widget_new (BounceDialogWidgetType type, const char * bounce_name)
{
  BounceDialogWidget * self = static_cast<BounceDialogWidget *> (
    g_object_new (BOUNCE_DIALOG_WIDGET_TYPE, NULL));

  self->type = type;
  /* TODO free when destroying */
  self->bounce_name = g_strdup (bounce_name);

  self->bounce_step_selector = bounce_step_selector_widget_new ();
  gtk_box_append (
    GTK_BOX (self->bounce_step_box), GTK_WIDGET (self->bounce_step_selector));

  g_signal_connect (
    G_OBJECT (self->tail_spin), "value-changed",
    G_CALLBACK (on_tail_value_changed), self);
  g_signal_connect (
    G_OBJECT (self->bounce_with_parents), "toggled",
    G_CALLBACK (on_bounce_with_parents_toggled), self);

  if (type == BounceDialogWidgetType::BOUNCE_DIALOG_TRACKS)
    {
      g_signal_connect (
        G_OBJECT (self->disable_after_bounce), "toggled",
        G_CALLBACK (on_disable_after_bounce_toggled), self);
    }
  else
    {
      /* hide 'disable after bounce' if not bouncing
       * tracks */
      gtk_widget_set_visible (GTK_WIDGET (self->disable_after_bounce), false);
    }

  return self;
}

static void
bounce_dialog_widget_class_init (BounceDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "bounce_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, BounceDialogWidget, x)

  BIND_CHILD (cancel_btn);
  BIND_CHILD (bounce_btn);
  BIND_CHILD (bounce_with_parents);
  BIND_CHILD (bounce_step_box);
  BIND_CHILD (tail_spin);
  BIND_CHILD (disable_after_bounce);

  gtk_widget_class_bind_template_callback (klass, on_cancel_clicked);
  gtk_widget_class_bind_template_callback (klass, on_bounce_clicked);
}

static void
bounce_dialog_widget_init (BounceDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_window_set_focus (GTK_WINDOW (self), GTK_WIDGET (self->bounce_btn));

  gtk_spin_button_set_value (
    self->tail_spin, (double) g_settings_get_int (S_UI, "bounce-tail"));

  bool bounce_with_parents =
    g_settings_get_boolean (S_UI, "bounce-with-parents");
  gtk_check_button_set_active (
    GTK_CHECK_BUTTON (self->bounce_with_parents), bounce_with_parents);
  gtk_widget_set_visible (
    GTK_WIDGET (self->bounce_step_box), !bounce_with_parents);

  bool disable_after_bounce =
    g_settings_get_boolean (S_UI, "disable-after-bounce");
  gtk_check_button_set_active (
    GTK_CHECK_BUTTON (self->disable_after_bounce), disable_after_bounce);
}
