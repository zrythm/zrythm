// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/track.h"
#include "gui/widgets/balance_control.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/fader_buttons.h"
#include "gui/widgets/fader_controls_expander.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/meter.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (FaderButtonsWidget, fader_buttons_widget, GTK_TYPE_BOX)

static void
on_record_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  Track * track = self->track;
  if (track)
    {
      if (!track_is_selected (track))
        {
          track_select (track, F_SELECT, F_EXCLUSIVE, F_PUBLISH_EVENTS);
        }
      track_set_recording (track, gtk_toggle_button_get_active (btn), 1);
    }
}

static void
on_solo_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  Track * track = self->track;
  if (track)
    {
      track_set_soloed (
        self->track, gtk_toggle_button_get_active (btn), F_TRIGGER_UNDO,
        F_AUTO_SELECT, F_PUBLISH_EVENTS);
    }
}

static void
on_mute_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  Track * track = self->track;
  if (track)
    {
      track_set_muted (
        self->track, gtk_toggle_button_get_active (btn), F_TRIGGER_UNDO,
        F_AUTO_SELECT, F_PUBLISH_EVENTS);
    }
}

static void
on_listen_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  Track * track = self->track;
  if (track)
    {
      track_set_listened (
        self->track, gtk_toggle_button_get_active (btn), F_TRIGGER_UNDO,
        F_AUTO_SELECT, F_PUBLISH_EVENTS);
    }
}

static void
on_mono_compat_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  Track * track = self->track;
  if (track)
    {
      if (!track_is_selected (track))
        {
          track_select (track, F_SELECT, F_EXCLUSIVE, F_PUBLISH_EVENTS);
        }
      channel_set_mono_compat_enabled (
        track->channel, gtk_toggle_button_get_active (btn), F_PUBLISH_EVENTS);
    }
}

static void
on_swap_phase_toggled (GtkToggleButton * btn, FaderButtonsWidget * self)
{
  Track * track = self->track;
  if (track)
    {
      if (!track_is_selected (track))
        {
          track_select (track, F_SELECT, F_EXCLUSIVE, F_PUBLISH_EVENTS);
        }
      channel_set_swap_phase (
        track->channel, gtk_toggle_button_get_active (btn), F_PUBLISH_EVENTS);
    }
}

void
fader_buttons_widget_block_signal_handlers (FaderButtonsWidget * self)
{
#if 0
  g_debug (
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
  g_debug (
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
fader_buttons_widget_refresh (FaderButtonsWidget * self, Track * track)
{
  self->track = track;

  if (track)
    {
      fader_buttons_widget_block_signal_handlers (self);
      if (track_type_has_mono_compat_switch (track->type))
        {
          gtk_toggle_button_set_active (
            self->mono_compat, channel_get_mono_compat_enabled (track->channel));
          gtk_widget_set_visible (GTK_WIDGET (self->mono_compat), true);
        }
      else
        {
          gtk_widget_set_visible (GTK_WIDGET (self->mono_compat), false);
        }
      if (track->out_signal_type == ZPortType::Z_PORT_TYPE_AUDIO)
        {
          gtk_toggle_button_set_active (
            self->swap_phase, channel_get_swap_phase (track->channel));
          gtk_widget_set_visible (GTK_WIDGET (self->swap_phase), true);
        }
      else
        {
          gtk_widget_set_visible (GTK_WIDGET (self->swap_phase), false);
        }
      gtk_toggle_button_set_active (self->mute, track_get_muted (track));
      if (track_type_can_record (track->type))
        {
          gtk_widget_set_visible (GTK_WIDGET (self->record), true);
          gtk_toggle_button_set_active (
            self->record, track_get_recording (track));
        }
      else
        {
          gtk_widget_set_visible (GTK_WIDGET (self->record), false);
        }
      gtk_toggle_button_set_active (self->solo, track_get_soloed (track));
      gtk_toggle_button_set_active (self->listen, track_get_listened (track));
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

  Fader * fader = track_get_fader (self->track, true);

  Port * port = NULL;
  if (widget == GTK_WIDGET (self->mute))
    {
      port = fader->mute;
    }
  else if (widget == GTK_WIDGET (self->solo))
    {
      port = fader->solo;
    }
  else if (widget == GTK_WIDGET (self->listen))
    {
      port = fader->listen;
    }
  else if (widget == GTK_WIDGET (self->mono_compat))
    {
      port = fader->mono_compat_enabled;
    }
  else if (widget == GTK_WIDGET (self->swap_phase))
    {
      port = fader->swap_phase;
    }
  else if (widget == GTK_WIDGET (self->record))
    {
      port = self->track->recording;
    }

  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  char tmp[600];
  sprintf (tmp, "app.bind-midi-cc::%p", port);
  menuitem = CREATE_MIDI_LEARN_MENU_ITEM (tmp);
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
fader_buttons_widget_new (Track * track)
{
  FaderButtonsWidget * self = static_cast<FaderButtonsWidget *> (
    g_object_new (FADER_BUTTONS_WIDGET_TYPE, NULL));

  fader_buttons_widget_refresh (self, track);
  return self;
}

static void
fader_buttons_finalize (FaderButtonsWidget * self)
{
  g_debug ("finalizing...");

  G_OBJECT_CLASS (fader_buttons_widget_parent_class)->finalize (G_OBJECT (self));

  g_debug ("done");
}

static void
fader_buttons_widget_init (FaderButtonsWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->popover_menu = GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
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
