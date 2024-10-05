// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/project.h"
#include "gui/cpp/gtk_widgets/balance_control.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/fader.h"
#include "gui/cpp/gtk_widgets/fader_buttons.h"
#include "gui/cpp/gtk_widgets/fader_controls_expander.h"
#include "gui/cpp/gtk_widgets/inspector_track.h"
#include "gui/cpp/gtk_widgets/left_dock_edge.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/meter.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include "common/dsp/track.h"
#include "common/utils/flags.h"
#include "common/utils/gtk.h"
#include "common/utils/math.h"
#include "common/utils/resources.h"
#include "common/utils/ui.h"

G_DEFINE_TYPE (FaderButtonsWidget, fader_buttons_widget, GTK_TYPE_BOX)

static void
on_record_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  auto * track = self->track;
  if (track)
    {
      if (!track->is_selected ())
        {
          track->select (true, true, true);
        }
      auto recordable_track = dynamic_cast<RecordableTrack *> (track);
      if (recordable_track)
        {
          recordable_track->set_recording (
            gtk_toggle_button_get_active (btn), true);
        }
    }
}

static void
on_solo_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  auto track = self->track;
  if (track)
    {
      track->set_soloed (gtk_toggle_button_get_active (btn), true, true, true);
    }
}

static void
on_mute_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  auto track = self->track;
  if (track)
    {
      track->set_muted (gtk_toggle_button_get_active (btn), true, true, true);
    }
}

static void
on_listen_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  auto track = self->track;
  if (track)
    {
      track->set_listened (gtk_toggle_button_get_active (btn), true, true, true);
    }
}

static void
on_mono_compat_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  auto track = self->track;
  if (track)
    {
      if (!track->is_selected ())
        {
          track->select (true, true, true);
        }
      track->channel_->set_mono_compat_enabled (
        gtk_toggle_button_get_active (btn), true);
    }
}

static void
on_swap_phase_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  auto track = self->track;
  if (track)
    {
      if (!track->is_selected ())
        {
          track->select (true, true, true);
        }
      track->channel_->set_swap_phase (gtk_toggle_button_get_active (btn), true);
    }
}

void
fader_buttons_widget_block_signal_handlers (FaderButtonsWidget * self)
{
#if 0
  z_debug (
    "blocking signal handlers for %s...",
    self->track->name);
#endif

  g_signal_handler_block (
    self->mono_compat, self->mono_compat_toggled_handler_id);
  g_signal_handler_block (self->swap_phase, self->swap_phase_toggled_handler_id);
  g_signal_handler_block (self->solo, self->solo_toggled_handler_id);
  g_signal_handler_block (self->mute, self->mute_toggled_handler_id);
  g_signal_handler_block (self->listen, self->listen_toggled_handler_id);
  g_signal_handler_block (self->record, self->record_toggled_handler_id);
}

void
fader_buttons_widget_unblock_signal_handlers (FaderButtonsWidget * self)
{
#if 0
  z_debug (
    "unblocking signal handlers for %s...",
    self->track->name);
#endif

  g_signal_handler_unblock (
    self->mono_compat, self->mono_compat_toggled_handler_id);
  g_signal_handler_unblock (
    self->swap_phase, self->swap_phase_toggled_handler_id);
  g_signal_handler_unblock (self->solo, self->solo_toggled_handler_id);
  g_signal_handler_unblock (self->mute, self->mute_toggled_handler_id);
  g_signal_handler_unblock (self->listen, self->listen_toggled_handler_id);
  g_signal_handler_unblock (self->record, self->record_toggled_handler_id);
}

void
fader_buttons_widget_refresh (FaderButtonsWidget * self, ChannelTrack * track)
{
  self->track = track;

  if (track)
    {
      fader_buttons_widget_block_signal_handlers (self);
      if (Track::type_has_mono_compat_switch (track->type_))
        {
          gtk_toggle_button_set_active (
            self->mono_compat, track->channel_->get_mono_compat_enabled ());
          gtk_widget_set_visible (GTK_WIDGET (self->mono_compat), true);
        }
      else
        {
          gtk_widget_set_visible (GTK_WIDGET (self->mono_compat), false);
        }
      if (track->out_signal_type_ == PortType::Audio)
        {
          gtk_toggle_button_set_active (
            self->swap_phase, track->channel_->get_swap_phase ());
          gtk_widget_set_visible (GTK_WIDGET (self->swap_phase), true);
        }
      else
        {
          gtk_widget_set_visible (GTK_WIDGET (self->swap_phase), false);
        }
      gtk_toggle_button_set_active (self->mute, track->get_muted ());
      if (track->can_record ())
        {
          gtk_widget_set_visible (GTK_WIDGET (self->record), true);
          auto recordable_track = dynamic_cast<RecordableTrack *> (track);
          gtk_toggle_button_set_active (
            self->record, recordable_track->get_recording ());
        }
      else
        {
          gtk_widget_set_visible (GTK_WIDGET (self->record), false);
        }
      gtk_toggle_button_set_active (self->solo, track->get_soloed ());
      gtk_toggle_button_set_active (self->listen, track->get_listened ());
      fader_buttons_widget_unblock_signal_handlers (self);
    }
}

static void
on_btn_right_click (
  GtkGestureClick *    gesture,
  gint                 n_press,
  gdouble              x_dbl,
  gdouble              y_dbl,
  FaderButtonsWidget * self)
{
  GtkWidget * widget =
    gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));

  auto fader = self->track->get_fader (true);

  ControlPort * port = nullptr;
  if (widget == GTK_WIDGET (self->mute))
    {
      port = fader->mute_.get ();
    }
  else if (widget == GTK_WIDGET (self->solo))
    {
      port = fader->solo_.get ();
    }
  else if (widget == GTK_WIDGET (self->listen))
    {
      port = fader->listen_.get ();
    }
  else if (widget == GTK_WIDGET (self->mono_compat))
    {
      port = fader->mono_compat_enabled_.get ();
    }
  else if (widget == GTK_WIDGET (self->swap_phase))
    {
      port = fader->swap_phase_.get ();
    }
  else if (widget == GTK_WIDGET (self->record))
    {
      auto recordable_track = dynamic_cast<RecordableTrack *> (self->track);
      port = recordable_track->recording_.get ();
    }

  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem = CREATE_MIDI_LEARN_MENU_ITEM (
    fmt::format ("app.bind-midi-cc::{}", fmt::ptr (port)).c_str ());
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (self->popover_menu, x_dbl, y_dbl, menu);
}

static void
on_e_clicked (GtkButton * self, gpointer user_data)
{
  ui_show_error_message (
    _ ("Unimplemented"), _ ("This feature is not implemented yet"));
}

FaderButtonsWidget *
fader_buttons_widget_new (ChannelTrack * track)
{
  FaderButtonsWidget * self = static_cast<FaderButtonsWidget *> (
    g_object_new (FADER_BUTTONS_WIDGET_TYPE, nullptr));

  fader_buttons_widget_refresh (self, track);
  return self;
}

static void
fader_buttons_finalize (FaderButtonsWidget * self)
{
  z_debug ("finalizing...");

  G_OBJECT_CLASS (fader_buttons_widget_parent_class)->finalize (G_OBJECT (self));

  z_debug ("done");
}

static void
fader_buttons_widget_init (FaderButtonsWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->popover_menu));

  /* add css classes */
  gtk_widget_add_css_class (GTK_WIDGET (self->record), "record-button");
  gtk_widget_add_css_class (GTK_WIDGET (self->solo), "solo-button");

  self->mono_compat_toggled_handler_id = g_signal_connect (
    G_OBJECT (self->mono_compat), "toggled",
    G_CALLBACK (on_mono_compat_toggled), self);
  self->swap_phase_toggled_handler_id = g_signal_connect (
    G_OBJECT (self->swap_phase), "toggled", G_CALLBACK (on_swap_phase_toggled),
    self);
  self->solo_toggled_handler_id = g_signal_connect (
    G_OBJECT (self->solo), "toggled", G_CALLBACK (on_solo_toggled), self);
  self->mute_toggled_handler_id = g_signal_connect (
    G_OBJECT (self->mute), "toggled", G_CALLBACK (on_mute_toggled), self);
  self->listen_toggled_handler_id = g_signal_connect (
    G_OBJECT (self->listen), "toggled", G_CALLBACK (on_listen_toggled), self);
  self->record_toggled_handler_id = g_signal_connect (
    G_OBJECT (self->record), "toggled", G_CALLBACK (on_record_toggled), self);

  g_signal_connect (
    G_OBJECT (self->e), "clicked", G_CALLBACK (on_e_clicked), self);
  gtk_widget_set_visible (GTK_WIDGET (self->e), false);

  /* add right click menus */
  GtkGestureClick * mp;

#define ADD_RIGHT_CLICK_CB(widget) \
  mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ()); \
  gtk_gesture_single_set_button ( \
    GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY); \
  g_signal_connect ( \
    G_OBJECT (mp), "pressed", G_CALLBACK (on_btn_right_click), self); \
  gtk_widget_add_controller (GTK_WIDGET (widget), GTK_EVENT_CONTROLLER (mp))

  ADD_RIGHT_CLICK_CB (self->mute);
  ADD_RIGHT_CLICK_CB (self->solo);
  ADD_RIGHT_CLICK_CB (self->listen);
  ADD_RIGHT_CLICK_CB (self->mono_compat);
  ADD_RIGHT_CLICK_CB (self->swap_phase);
  ADD_RIGHT_CLICK_CB (self->record);

#undef ADD_RIGHT_CLICK_CB
}

static void
fader_buttons_widget_class_init (FaderButtonsWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "fader_buttons.ui");

  gtk_widget_class_set_css_name (klass, "fader-buttons");
  gtk_widget_class_set_accessible_role (klass, GTK_ACCESSIBLE_ROLE_GROUP);

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, FaderButtonsWidget, x)

  BIND_CHILD (mono_compat);
  BIND_CHILD (swap_phase);
  BIND_CHILD (solo);
  BIND_CHILD (mute);
  BIND_CHILD (listen);
  BIND_CHILD (record);
  BIND_CHILD (e);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) fader_buttons_finalize;
}
