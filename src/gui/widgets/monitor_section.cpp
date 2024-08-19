// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/tracklist_selections.h"
#include "dsp/control_port.h"
#include "dsp/control_room.h"
#include "dsp/engine.h"
#include "dsp/engine_jack.h"
#include "dsp/fader.h"
#include "dsp/tracklist.h"
#include "gui/widgets/active_hardware_mb.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/knob_with_name.h"
#include "gui/widgets/monitor_section.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (MonitorSectionWidget, monitor_section_widget, GTK_TYPE_BOX)

static const char *
monitor_out_name_getter (void * obj)
{
  return _ ("Monitor");
}

static const char *
mute_name_getter (void * obj)
{
  return _ ("Mute");
}

static const char *
listen_name_getter (void * obj)
{
  return _ ("Listen");
}

static const char *
dim_name_getter (void * obj)
{
  return _ ("Dim");
}

void
monitor_section_widget_refresh (MonitorSectionWidget * self)
{
  int num_muted = TRACKLIST->get_num_muted_tracks ();
  int num_soloed = TRACKLIST->get_num_soloed_tracks ();
  int num_listened = TRACKLIST->get_num_listened_tracks ();

  char str[200];
  snprintf (str, 200, _ ("<small>%d muted</small>"), num_muted);
  gtk_label_set_markup (self->muted_tracks_lbl, str);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->muted_tracks_lbl), _ ("Currently muted tracks"));

  snprintf (str, 200, _ ("<small>%d soloed</small>"), num_soloed);
  gtk_label_set_markup (self->soloed_tracks_lbl, str);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->soloed_tracks_lbl), _ ("Currently soloed tracks"));

  snprintf (str, 200, _ ("<small>%d listened</small>"), num_listened);
  gtk_label_set_markup (self->listened_tracks_lbl, str);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->listened_tracks_lbl), _ ("Currently listened tracks"));

  gtk_widget_set_sensitive (GTK_WIDGET (self->soloing_btn), num_soloed > 0);
  gtk_widget_set_sensitive (GTK_WIDGET (self->muting_btn), num_muted > 0);
  gtk_widget_set_sensitive (GTK_WIDGET (self->listening_btn), num_listened > 0);
}

static void
on_unsolo_all_clicked (GtkButton * btn, MonitorSectionWidget * self)
{
  /* remember selections */
  auto tracks_before = TRACKLIST_SELECTIONS->track_names_;

  /* unsolo all */
  TRACKLIST->select_all (true, false);
  try
    {
      UNDO_MANAGER->perform (std::make_unique<SoloTracksAction> (
        *TRACKLIST_SELECTIONS->gen_tracklist_selections (), false));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to unsolo all tracks"));
    }

  /* restore selections */
  for (auto &track_name : tracks_before)
    {
      auto tr = TRACKLIST->find_track_by_name (track_name);
      tr->select (true, track_name == tracks_before[0], false);
    }
}

static void
on_unmute_all_clicked (GtkButton * btn, MonitorSectionWidget * self)
{
  /* remember selections */
  auto tracks_before = TRACKLIST_SELECTIONS->track_names_;

  /* unmute all */
  TRACKLIST->select_all (true, false);
  try
    {
      UNDO_MANAGER->perform (std::make_unique<MuteTracksAction> (
        *TRACKLIST_SELECTIONS->gen_tracklist_selections (), false));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to unmute all tracks"));
    }

  /* restore selections */
  for (auto &track_name : tracks_before)
    {
      auto tr = TRACKLIST->find_track_by_name (track_name);
      tr->select (true, track_name == tracks_before[0], false);
    }
}

static void
on_unlisten_all_clicked (GtkButton * btn, MonitorSectionWidget * self)
{
  /* remember selections */
  auto tracks_before = TRACKLIST_SELECTIONS->track_names_;

  /* unlisten all */
  TRACKLIST->select_all (true, false);
  try
    {
      UNDO_MANAGER->perform (std::make_unique<ListenTracksAction> (
        *TRACKLIST_SELECTIONS->gen_tracklist_selections (), false));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to unlisten all tracks"));
    }

  /* restore selections */
  for (auto &track_name : tracks_before)
    {
      auto tr = TRACKLIST->find_track_by_name (track_name);
      tr->select (true, track_name == tracks_before[0], false);
    }
}

static void
on_mono_toggled (GtkToggleButton * btn, MonitorSectionWidget * self)
{
  bool active = gtk_toggle_button_get_active (btn);
  MONITOR_FADER->set_mono_compat_enabled (active, false);
  g_settings_set_boolean (S_MONITOR, "mono", active);
}

static void
on_dim_toggled (GtkToggleButton * btn, MonitorSectionWidget * self)
{
  bool active = gtk_toggle_button_get_active (btn);
  CONTROL_ROOM->dim_output_ = active;
  g_settings_set_boolean (S_MONITOR, "dim-output", active);
}

static void
on_mute_toggled (GtkToggleButton * btn, MonitorSectionWidget * self)
{
  bool active = gtk_toggle_button_get_active (btn);
  MONITOR_FADER->mute_->control_ = active ? 1.f : 0.f;
  g_settings_set_boolean (S_MONITOR, "mute", active);
}

static void
on_devices_updated (MonitorSectionWidget * self)
{
#ifdef HAVE_JACK
  /* reconnect to devices */
  GError * err = NULL;
  bool ret = engine_jack_reconnect_monitor (AUDIO_ENGINE.get (), true, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _ ("Failed to connect to left monitor output port"));
      return;
    }
  ret = engine_jack_reconnect_monitor (AUDIO_ENGINE.get (), false, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _ ("Failed to connect to right monitor output port"));
      return;
    }
#endif
}

void
monitor_section_widget_setup (
  MonitorSectionWidget * self,
  ControlRoom *          control_room)
{
  self->control_room = control_room;

  KnobWidget * knob = knob_widget_new_simple (
    Fader::fader_val_getter, Fader::default_fader_val_getter,
    Fader::fader_val_setter, MONITOR_FADER.get (), 0.f, 1.f, 78, 0.f);
  knob->hover_str_getter = Fader::db_string_getter_static;
  self->monitor_level = knob_with_name_widget_new (
    nullptr, monitor_out_name_getter, nullptr, knob, GTK_ORIENTATION_VERTICAL,
    false, 2);
  gtk_box_append (
    GTK_BOX (self->monitor_level_box), GTK_WIDGET (self->monitor_level));

  /* mute */
  knob = knob_widget_new_simple (
    Fader::fader_val_getter, Fader::default_fader_val_getter,
    Fader::fader_val_setter, CONTROL_ROOM->mute_fader_.get (), 0.f, 1.f, 52,
    0.f);
  knob->hover_str_getter = Fader::db_string_getter_static;
  self->mute_level = knob_with_name_widget_new (
    nullptr, mute_name_getter, nullptr, knob, GTK_ORIENTATION_VERTICAL, false,
    2);
  gtk_box_append (GTK_BOX (self->mute_level_box), GTK_WIDGET (self->mute_level));

  /* listen */
  knob = knob_widget_new_simple (
    Fader::fader_val_getter, Fader::default_fader_val_getter,
    Fader::fader_val_setter, CONTROL_ROOM->listen_fader_.get (), 0.f, 1.f, 52,
    0.f);
  knob->hover_str_getter = Fader::db_string_getter_static;
  self->listen_level = knob_with_name_widget_new (
    nullptr, listen_name_getter, nullptr, knob, GTK_ORIENTATION_VERTICAL, false,
    2);
  gtk_box_append (
    GTK_BOX (self->listen_level_box), GTK_WIDGET (self->listen_level));

  /* dim */
  knob = knob_widget_new_simple (
    Fader::fader_val_getter, Fader::default_fader_val_getter,
    Fader::fader_val_setter, CONTROL_ROOM->dim_fader_.get (), 0.f, 1.f, 52, 0.f);
  knob->hover_str_getter = Fader::db_string_getter_static;
  self->dim_level = knob_with_name_widget_new (
    nullptr, dim_name_getter, nullptr, knob, GTK_ORIENTATION_VERTICAL, false, 2);
  gtk_box_append (GTK_BOX (self->dim_level_box), GTK_WIDGET (self->dim_level));

  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (self->mono_toggle), "codicons-merge", _ ("Mono"), true,
    GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_toggle_button_set_active (
    self->mono_toggle, MONITOR_FADER->get_mono_compat_enabled ());

  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (self->dim_toggle), "dim", _ ("Dim"), true,
    GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_toggle_button_set_active (self->dim_toggle, CONTROL_ROOM->dim_output_);

  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (self->mute_toggle), "mute", _ ("Mute"), true,
    GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_toggle_button_set_active (
    self->mute_toggle, MONITOR_FADER->mute_->is_toggled ());

  /* left/right outputs */
  if (AUDIO_ENGINE->audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
    {
      self->left_outputs = active_hardware_mb_widget_new ();
      active_hardware_mb_widget_setup (
        self->left_outputs, F_NOT_INPUT, F_NOT_MIDI, S_MONITOR, "l-devices");
      self->left_outputs->callback = (GenericCallback) on_devices_updated;
      self->left_outputs->object = self;
      gtk_box_append (
        GTK_BOX (self->left_output_box), GTK_WIDGET (self->left_outputs));

      self->right_outputs = active_hardware_mb_widget_new ();
      active_hardware_mb_widget_setup (
        self->right_outputs, F_NOT_INPUT, F_NOT_MIDI, S_MONITOR, "r-devices");
      self->right_outputs->callback = (GenericCallback) on_devices_updated;
      self->right_outputs->object = self;
      gtk_box_append (
        GTK_BOX (self->right_output_box), GTK_WIDGET (self->right_outputs));
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->l_label), false);
      gtk_widget_set_visible (GTK_WIDGET (self->r_label), false);
    }

  /* set tooltips */
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->soloing_btn), _ ("Unsolo all tracks"));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->muting_btn), _ ("Unmute all tracks"));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->listening_btn), _ ("Unlisten all tracks"));

  /* set signals */
  g_signal_connect (
    G_OBJECT (self->mono_toggle), "toggled", G_CALLBACK (on_mono_toggled), self);
  g_signal_connect (
    G_OBJECT (self->dim_toggle), "toggled", G_CALLBACK (on_dim_toggled), self);
  g_signal_connect (
    G_OBJECT (self->mute_toggle), "toggled", G_CALLBACK (on_mute_toggled), self);

  g_signal_connect (
    G_OBJECT (self->soloing_btn), "clicked", G_CALLBACK (on_unsolo_all_clicked),
    self);
  g_signal_connect (
    G_OBJECT (self->muting_btn), "clicked", G_CALLBACK (on_unmute_all_clicked),
    self);
  g_signal_connect (
    G_OBJECT (self->listening_btn), "clicked",
    G_CALLBACK (on_unlisten_all_clicked), self);

  monitor_section_widget_refresh (self);
}

/**
 * Creates a MonitorSectionWidget.
 */
MonitorSectionWidget *
monitor_section_widget_new (void)
{
  MonitorSectionWidget * self = static_cast<MonitorSectionWidget *> (
    g_object_new (MONITOR_SECTION_WIDGET_TYPE, nullptr));

  return self;
}

static void
monitor_section_widget_init (MonitorSectionWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#if 0
  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (self->soloing_toggle), "solo",
    _("Soloing"), true,
    GTK_ORIENTATION_HORIZONTAL, 1);
  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (self->muting_toggle), "mute",
    _("Muting"), true,
    GTK_ORIENTATION_HORIZONTAL, 1);
  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (self->listening_toggle), "listen",
    _("Listening"), true,
    GTK_ORIENTATION_HORIZONTAL, 1);
#endif
}

static void
monitor_section_widget_class_init (MonitorSectionWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "monitor_section.ui");

  gtk_widget_class_set_css_name (klass, "control-room");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, MonitorSectionWidget, x)

  BIND_CHILD (soloing_btn);
  BIND_CHILD (soloed_tracks_lbl);
  BIND_CHILD (muting_btn);
  BIND_CHILD (muted_tracks_lbl);
  BIND_CHILD (listening_btn);
  BIND_CHILD (listened_tracks_lbl);
  BIND_CHILD (mute_level_box);
  BIND_CHILD (listen_level_box);
  BIND_CHILD (dim_level_box);
  BIND_CHILD (mono_toggle);
  BIND_CHILD (dim_toggle);
  BIND_CHILD (mute_toggle);
  BIND_CHILD (monitor_level_box);
  BIND_CHILD (left_output_box);
  BIND_CHILD (l_label);
  BIND_CHILD (right_output_box);
  BIND_CHILD (r_label);

#undef BIND_CHILD
}
