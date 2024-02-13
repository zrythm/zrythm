// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/control_port.h"
#include "dsp/control_room.h"
#include "dsp/engine.h"
#include "dsp/engine_jack.h"
#include "dsp/fader.h"
#include "gui/widgets/active_hardware_mb.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/knob_with_name.h"
#include "gui/widgets/monitor_section.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
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
  int num_muted = tracklist_get_num_muted_tracks (TRACKLIST);
  int num_soloed = tracklist_get_num_soloed_tracks (TRACKLIST);
  int num_listened = tracklist_get_num_listened_tracks (TRACKLIST);

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
  int num_tracks_before = TRACKLIST_SELECTIONS->num_tracks;
  int tracks_before[num_tracks_before];
  for (int i = 0; i < num_tracks_before; i++)
    {
      tracks_before[i] = TRACKLIST_SELECTIONS->tracks[i]->pos;
    }

  /* unsolo all */
  tracklist_select_all (TRACKLIST, F_SELECT, F_NO_PUBLISH_EVENTS);
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_solo (
    TRACKLIST_SELECTIONS, F_NO_SOLO, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to unsolo all tracks"));
    }

  /* restore selections */
  for (int i = 0; i < num_tracks_before; i++)
    {
      track_select (
        TRACKLIST->tracks[tracks_before[i]], F_SELECT, i == 0,
        F_NO_PUBLISH_EVENTS);
    }
}

static void
on_unmute_all_clicked (GtkButton * btn, MonitorSectionWidget * self)
{
  /* remember selections */
  int num_tracks_before = TRACKLIST_SELECTIONS->num_tracks;
  int tracks_before[num_tracks_before];
  for (int i = 0; i < num_tracks_before; i++)
    {
      tracks_before[i] = TRACKLIST_SELECTIONS->tracks[i]->pos;
    }

  /* unmute all */
  tracklist_select_all (TRACKLIST, F_SELECT, F_NO_PUBLISH_EVENTS);
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_mute (
    TRACKLIST_SELECTIONS, F_NO_MUTE, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to unmute all tracks"));
    }

  /* restore selections */
  for (int i = 0; i < num_tracks_before; i++)
    {
      track_select (
        TRACKLIST->tracks[tracks_before[i]], F_SELECT, i == 0,
        F_NO_PUBLISH_EVENTS);
    }
}

static void
on_unlisten_all_clicked (GtkButton * btn, MonitorSectionWidget * self)
{
  /* remember selections */
  int num_tracks_before = TRACKLIST_SELECTIONS->num_tracks;
  int tracks_before[num_tracks_before];
  for (int i = 0; i < num_tracks_before; i++)
    {
      tracks_before[i] = TRACKLIST_SELECTIONS->tracks[i]->pos;
    }

  /* unlisten all */
  tracklist_select_all (TRACKLIST, F_SELECT, F_NO_PUBLISH_EVENTS);
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_listen (
    TRACKLIST_SELECTIONS, F_NO_LISTEN, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to unlisten all tracks"));
    }

  /* restore selections */
  for (int i = 0; i < num_tracks_before; i++)
    {
      track_select (
        TRACKLIST->tracks[tracks_before[i]], F_SELECT, i == 0,
        F_NO_PUBLISH_EVENTS);
    }
}

static void
on_mono_toggled (GtkToggleButton * btn, MonitorSectionWidget * self)
{
  bool active = gtk_toggle_button_get_active (btn);
  fader_set_mono_compat_enabled (MONITOR_FADER, active, F_NO_PUBLISH_EVENTS);
  g_settings_set_boolean (S_MONITOR, "mono", active);
}

static void
on_dim_toggled (GtkToggleButton * btn, MonitorSectionWidget * self)
{
  bool active = gtk_toggle_button_get_active (btn);
  CONTROL_ROOM->dim_output = active;
  g_settings_set_boolean (S_MONITOR, "dim-output", active);
}

static void
on_mute_toggled (GtkToggleButton * btn, MonitorSectionWidget * self)
{
  bool active = gtk_toggle_button_get_active (btn);
  MONITOR_FADER->mute->control = active ? 1.f : 0.f;
  g_settings_set_boolean (S_MONITOR, "mute", active);
}

static void
on_devices_updated (MonitorSectionWidget * self)
{
#ifdef HAVE_JACK
  /* reconnect to devices */
  GError * err = NULL;
  bool     ret = engine_jack_reconnect_monitor (AUDIO_ENGINE, true, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _ ("Failed to connect to left monitor output port"));
      return;
    }
  ret = engine_jack_reconnect_monitor (AUDIO_ENGINE, false, &err);
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
    fader_get_fader_val, fader_get_default_fader_val, fader_set_fader_val,
    MONITOR_FADER, 0.f, 1.f, 78, 0.f);
  knob->hover_str_getter = fader_db_string_getter;
  self->monitor_level = knob_with_name_widget_new (
    NULL, monitor_out_name_getter, NULL, knob, GTK_ORIENTATION_VERTICAL, false,
    2);
  gtk_box_append (
    GTK_BOX (self->monitor_level_box), GTK_WIDGET (self->monitor_level));

  /* mute */
  knob = knob_widget_new_simple (
    fader_get_fader_val, fader_get_default_fader_val, fader_set_fader_val,
    CONTROL_ROOM->mute_fader, 0.f, 1.f, 52, 0.f);
  knob->hover_str_getter = fader_db_string_getter;
  self->mute_level = knob_with_name_widget_new (
    NULL, mute_name_getter, NULL, knob, GTK_ORIENTATION_VERTICAL, false, 2);
  gtk_box_append (GTK_BOX (self->mute_level_box), GTK_WIDGET (self->mute_level));

  /* listen */
  knob = knob_widget_new_simple (
    fader_get_fader_val, fader_get_default_fader_val, fader_set_fader_val,
    CONTROL_ROOM->listen_fader, 0.f, 1.f, 52, 0.f);
  knob->hover_str_getter = fader_db_string_getter;
  self->listen_level = knob_with_name_widget_new (
    NULL, listen_name_getter, NULL, knob, GTK_ORIENTATION_VERTICAL, false, 2);
  gtk_box_append (
    GTK_BOX (self->listen_level_box), GTK_WIDGET (self->listen_level));

  /* dim */
  knob = knob_widget_new_simple (
    fader_get_fader_val, fader_get_default_fader_val, fader_set_fader_val,
    CONTROL_ROOM->dim_fader, 0.f, 1.f, 52, 0.f);
  knob->hover_str_getter = fader_db_string_getter;
  self->dim_level = knob_with_name_widget_new (
    NULL, dim_name_getter, NULL, knob, GTK_ORIENTATION_VERTICAL, false, 2);
  gtk_box_append (GTK_BOX (self->dim_level_box), GTK_WIDGET (self->dim_level));

  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (self->mono_toggle), "codicons-merge", _ ("Mono"), true,
    GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_toggle_button_set_active (
    self->mono_toggle, fader_get_mono_compat_enabled (MONITOR_FADER));

  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (self->dim_toggle), "dim", _ ("Dim"), true,
    GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_toggle_button_set_active (self->dim_toggle, CONTROL_ROOM->dim_output);

  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (self->mute_toggle), "mute", _ ("Mute"), true,
    GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_toggle_button_set_active (
    self->mute_toggle, control_port_is_toggled (MONITOR_FADER->mute));

  /* left/right outputs */
  if (AUDIO_ENGINE->audio_backend == AUDIO_BACKEND_JACK)
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
  MonitorSectionWidget * self = g_object_new (MONITOR_SECTION_WIDGET_TYPE, NULL);

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
