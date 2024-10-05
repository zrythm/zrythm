// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/engine.h"
#include "common/dsp/exporter.h"
#include "common/dsp/marker_track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/progress_info.h"
#include "common/utils/resources.h"
#include "common/utils/ui.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/backend/timeline_selections.h"
#include "gui/cpp/backend/tracklist_selections.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/bounce_step_selector.h"
#include "gui/cpp/gtk_widgets/dialogs/bounce_dialog.h"
#include "gui/cpp/gtk_widgets/dialogs/export_progress_dialog.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

G_DEFINE_TYPE (BounceDialogWidget, bounce_dialog_widget, GTK_TYPE_DIALOG)

static void
on_cancel_clicked (GtkButton * btn, BounceDialogWidget * self)
{
  gtk_window_close (GTK_WINDOW (self));
  /*gtk_widget_destroy (GTK_WIDGET (self));*/
}

static void
progress_close_cb (std::shared_ptr<Exporter> exporter)
{
  exporter->join_generic_thread ();
  exporter->post_export ();

  BounceDialogWidget * self = Z_BOUNCE_DIALOG_WIDGET (exporter->parent_owner_);

  auto &pinfo = exporter->progress_info_;
  if (
    !self->bounce_to_file
    && pinfo->get_completion_type () == ProgressInfo::CompletionType::SUCCESS)
    {
      /* create audio track with bounced material */
      exporter->create_audio_track_after_bounce (self->start_pos);
    }

  gtk_window_close (GTK_WINDOW (self));
}

static void
on_bounce_clicked (GtkButton * btn, BounceDialogWidget * self)
{
  auto settings = Exporter::Settings ();

  switch (self->type)
    {
    case BounceDialogWidgetType::BOUNCE_DIALOG_REGIONS:
      settings.mode_ = Exporter::Mode::Regions;
      break;
    case BounceDialogWidgetType::BOUNCE_DIALOG_TRACKS:
      settings.mode_ = Exporter::Mode::Tracks;
      break;
    }

  if (self->bounce_to_file)
    {
      /* TODO */
      z_return_if_reached ();
    }
  else
    {
      settings.set_bounce_defaults (
        Exporter::Format::WAV, "", self->bounce_name);
    }

  self->start_pos.zero ();
  switch (self->type)
    {
    case BounceDialogWidgetType::BOUNCE_DIALOG_REGIONS:
      {
        TL_SELECTIONS->mark_for_bounce (settings.bounce_with_parents_);
        settings.mode_ = Exporter::Mode::Regions;
        auto [obj, pos] = TL_SELECTIONS->get_first_object_and_pos (true);
        self->start_pos = pos;
      }
      break;
    case BounceDialogWidgetType::BOUNCE_DIALOG_TRACKS:
      {
        TRACKLIST_SELECTIONS->mark_for_bounce (
          settings.bounce_with_parents_, false);
        settings.mode_ = Exporter::Mode::Tracks;

        /* start at start marker */
        auto m = P_MARKER_TRACK->get_start_marker ();
        self->start_pos = m->pos_;
      }
      break;
    }

  self->exporter =
    std::make_shared<Exporter> (settings, GTK_WIDGET (self), nullptr);

  self->exporter->prepare_tracks_for_export (*AUDIO_ENGINE, *TRANSPORT);

  /* start exporting in a new thread */
  self->exporter->begin_generic_thread ();

  /* create a progress dialog and block */
  ExportProgressDialogWidget * progress_dialog =
    export_progress_dialog_widget_new (
      self->exporter, true, progress_close_cb, false, true);
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
bounce_dialog_widget_new (
  BounceDialogWidgetType type,
  const std::string     &bounce_name)
{
  auto * self = static_cast<BounceDialogWidget *> (
    g_object_new (BOUNCE_DIALOG_WIDGET_TYPE, nullptr));

  self->type = type;
  self->bounce_name = bounce_name;

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
bounce_dialog_widget_finalize (GObject * object)
{
  BounceDialogWidget * self = Z_BOUNCE_DIALOG_WIDGET (object);

  std::destroy_at (&self->exporter);
  std::destroy_at (&self->bounce_name);

  G_OBJECT_CLASS (bounce_dialog_widget_parent_class)->finalize (object);
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

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = bounce_dialog_widget_finalize;
}

static void
bounce_dialog_widget_init (BounceDialogWidget * self)
{
  std::construct_at (&self->exporter);
  std::construct_at (&self->bounce_name);

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
