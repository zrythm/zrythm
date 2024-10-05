// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/gtk_widgets/channel_slot.h"
#include "gui/cpp/gtk_widgets/channel_slot_activate_button.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include "common/utils/flags.h"

G_DEFINE_TYPE (
  ChannelSlotActivateButtonWidget,
  channel_slot_activate_button_widget,
  GTK_TYPE_TOGGLE_BUTTON)

static void
on_toggled (GtkToggleButton * btn, gpointer user_data)
{
  ChannelSlotActivateButtonWidget * self =
    Z_CHANNEL_SLOT_ACTIVATE_BUTTON_WIDGET (user_data);

  Plugin * pl = channel_slot_widget_get_plugin (self->owner);
  if (!pl)
    return;

  auto action = zrythm_app->lookup_action ("plugin-toggle-enabled");
  char tmp[500];
  sprintf (tmp, "%p", pl);
  auto var = Glib::Variant<Glib::ustring>::create (tmp);
  action->activate_variant (var);
}

/**
 * Creates a new ChannelSlotActivateButton widget.
 */
ChannelSlotActivateButtonWidget *
channel_slot_activate_button_widget_new (ChannelSlotWidget * owner)
{
  ChannelSlotActivateButtonWidget * self =
    static_cast<ChannelSlotActivateButtonWidget *> (
      g_object_new (CHANNEL_SLOT_ACTIVATE_BUTTON_WIDGET_TYPE, nullptr));

  self->owner = owner;

  self->toggled_id = g_signal_connect (
    G_OBJECT (self), "toggled", G_CALLBACK (on_toggled), self);

  return self;
}

static void
dispose (ChannelSlotActivateButtonWidget * self)
{
  G_OBJECT_CLASS (channel_slot_activate_button_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
channel_slot_activate_button_widget_init (ChannelSlotActivateButtonWidget * self)
{
  /*gtk_button_set_icon_name (GTK_BUTTON (self), "edit-tool");*/
  gtk_widget_add_css_class (GTK_WIDGET (self), "channel-slot-activate-btn");
  gtk_widget_set_halign (GTK_WIDGET (self), GTK_ALIGN_CENTER);
  gtk_widget_set_valign (GTK_WIDGET (self), GTK_ALIGN_CENTER);
}

static void
channel_slot_activate_button_widget_class_init (
  ChannelSlotActivateButtonWidgetClass * klass)
{
  /*GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);*/
  /*wklass->snapshot = channel_slot_activate_button_snapshot;*/
  /*gtk_widget_class_set_css_name (wklass, "channel-slot-btn");*/

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
