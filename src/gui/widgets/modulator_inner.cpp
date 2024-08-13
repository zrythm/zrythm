// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/mixer_selections_action.h"
#include "actions/undo_manager.h"
#include "dsp/control_port.h"
#include "dsp/modulator_track.h"
#include "dsp/port_identifier.h"
#include "dsp/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/dialogs/bind_cc_dialog.h"
#include "gui/widgets/dialogs/port_info.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/knob_with_name.h"
#include "gui/widgets/live_waveform.h"
#include "gui/widgets/modulator.h"
#include "gui/widgets/modulator_inner.h"
#include "gui/widgets/popovers/port_connections_popover.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (ModulatorInnerWidget, modulator_inner_widget, GTK_TYPE_BOX)

static Plugin *
get_modulator (ModulatorInnerWidget * self)
{
  return self->parent->modulator;
}

static float
get_snapped_control_value (void * data)
{
  auto  port = static_cast<ControlPort *> (data);
  float val = port->get_control_value (false);
  return val;
}

static void
on_show_hide_ui_toggled (GtkToggleButton * btn, ModulatorInnerWidget * self)
{
  Plugin * modulator = get_modulator (self);

  // FIXME use a method on the modulator to set the visibility
  modulator->visible_ = !modulator->visible_;

  EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, modulator);
}

static void
on_delete_clicked (GtkButton * btn, ModulatorInnerWidget * self)
{
  auto              sel = std::make_unique<FullMixerSelections> ();
  Plugin *          modulator = get_modulator (self);
  sel->add_slot (
    *P_MODULATOR_TRACK, PluginSlotType::Modulator, modulator->id_.slot_,
    F_PUBLISH_EVENTS);

  try
    {
      UNDO_MANAGER->perform (std::make_unique<MixerSelectionsDeleteAction> (
        *sel, *PORT_CONNECTIONS_MGR));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to delete plugins"));
    }
}

static void
on_automate_clicked (GtkButton * btn, ModulatorInnerWidget * self)
{
  int index = -1;
  for (int i = 0; i < 16; i++)
    {
      if (self->waveform_automate_buttons[i] == btn)
        {
          index = i;
          break;
        }
    }
  z_return_if_fail (index >= 0);

  port_connections_popover_widget_refresh (
    self->connections_popover, self->ports[index]);
  gtk_popover_popup (GTK_POPOVER (self->connections_popover));

  /* TODO update label on closed */
#if 0
  g_signal_connect (
    G_OBJECT (popover), "closed",
    G_CALLBACK (on_popover_closed), self);
#endif
}

void
modulator_inner_widget_refresh (ModulatorInnerWidget * self)
{
  Plugin * modulator = get_modulator (self);
  g_signal_handlers_block_by_func (
    self->show_hide_ui_btn, (gpointer) on_show_hide_ui_toggled, self);
  gtk_toggle_button_set_active (self->show_hide_ui_btn, modulator->visible_);
  g_signal_handlers_unblock_by_func (
    self->show_hide_ui_btn, (gpointer) on_show_hide_ui_toggled, self);
}

static void
on_knob_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  Port *            port)
{
  if (n_press != 1)
    return;

  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  char tmp[600];
  sprintf (tmp, "app.reset-control::%p", port);
  menuitem = z_gtk_create_menu_item (_ ("Reset"), nullptr, tmp);
  g_menu_append_item (menu, menuitem);

  sprintf (tmp, "app.bind-midi-cc::%p", port);
  menuitem = CREATE_MIDI_LEARN_MENU_ITEM (tmp);
  g_menu_append_item (menu, menuitem);

  sprintf (tmp, "app.port-view-info::%p", port);
  menuitem = z_gtk_create_menu_item (_ ("View info"), nullptr, tmp);
  g_menu_append_item (menu, menuitem);

  KnobWithNameWidget * kwn = Z_KNOB_WITH_NAME_WIDGET (
    gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture)));
  z_gtk_show_context_menu_from_g_menu (kwn->popover_menu, x, y, menu);
}

/**
 * Creates a new widget.
 */
ModulatorInnerWidget *
modulator_inner_widget_new (ModulatorWidget * parent)
{
  ModulatorInnerWidget * self = static_cast<ModulatorInnerWidget *> (
    g_object_new (MODULATOR_INNER_WIDGET_TYPE, nullptr));

  self->parent = parent;

  Plugin * modulator = get_modulator (self);
  for (auto &port : modulator->in_ports_)
    {
      if (
        port->id_.type_ != PortType::Control || port->id_.flow_ != PortFlow::Input
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id_.flags_,
          PortIdentifier::Flags::NotOnGui))
        continue;

      KnobWidget * knob = knob_widget_new_simple (
        ControlPort::val_getter, ControlPort::default_val_getter,
        ControlPort::real_val_setter, port.get (), port->minf_, port->maxf_, 24,
        port->zerof_);
      knob->snapped_getter = get_snapped_control_value;
      KnobWithNameWidget * knob_with_name = knob_with_name_widget_new (
        &port->id_, PortIdentifier::label_getter, nullptr, knob,
        GTK_ORIENTATION_HORIZONTAL, false, 3);

      array_double_size_if_full (
        self->knobs, self->num_knobs, self->knobs_size, KnobWithNameWidget *);
      array_append (self->knobs, self->num_knobs, knob_with_name);

      gtk_box_append (GTK_BOX (self->controls_box), GTK_WIDGET (knob_with_name));

      /* add context menu */
      GtkGestureClick * mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
      gtk_gesture_single_set_button (
        GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
      g_signal_connect (
        G_OBJECT (mp), "pressed", G_CALLBACK (on_knob_right_click), port.get ());
      gtk_widget_add_controller (
        GTK_WIDGET (knob_with_name), GTK_EVENT_CONTROLLER (mp));
    }

  for (auto &port : modulator->out_ports_)
    {
      if (port->id_.type_ != PortType::CV)
        continue;

      int index = self->num_waveforms++;

      self->ports[index] = port.get ();

      /* create waveform */
      self->waveforms[index] =
        live_waveform_widget_new_port (dynamic_cast<AudioPort *> (port.get ()));
      gtk_widget_set_size_request (GTK_WIDGET (self->waveforms[index]), 48, 48);
      gtk_widget_set_visible (GTK_WIDGET (self->waveforms[index]), true);

      /* create waveform overlay */
      self->waveform_overlays[index] = GTK_OVERLAY (gtk_overlay_new ());
      gtk_widget_set_visible (GTK_WIDGET (self->waveform_overlays[index]), true);
      gtk_overlay_set_child (
        GTK_OVERLAY (self->waveform_overlays[index]),
        GTK_WIDGET (self->waveforms[index]));

      /* add button for selecting automatable */
      self->waveform_automate_buttons[index] =
        GTK_BUTTON (gtk_button_new_from_icon_name ("automate"));
      gtk_widget_set_visible (
        GTK_WIDGET (self->waveform_automate_buttons[index]), true);
      gtk_overlay_add_overlay (
        self->waveform_overlays[index],
        GTK_WIDGET (self->waveform_automate_buttons[index]));
      gtk_widget_set_halign (
        GTK_WIDGET (self->waveform_automate_buttons[index]), GTK_ALIGN_END);
      gtk_widget_set_valign (
        GTK_WIDGET (self->waveform_automate_buttons[index]), GTK_ALIGN_START);

      g_signal_connect (
        G_OBJECT (self->waveform_automate_buttons[index]), "clicked",
        G_CALLBACK (on_automate_clicked), self);

      gtk_box_append (
        GTK_BOX (self->waveforms_box),
        GTK_WIDGET (self->waveform_overlays[index]));

      if (self->num_waveforms == 16)
        break;
    }

  EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, modulator);
  /*modulator_inner_widget_refresh (self);*/

  return self;
}

static void
finalize (ModulatorInnerWidget * self)
{
  if (self->knobs)
    {
      free (self->knobs);
    }

  G_OBJECT_CLASS (modulator_inner_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
dispose (ModulatorInnerWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->connections_popover));

  G_OBJECT_CLASS (modulator_inner_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
modulator_inner_widget_init (ModulatorInnerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->knobs = static_cast<KnobWithNameWidget **> (
    calloc (1, sizeof (KnobWithNameWidget *)));
  self->knobs_size = 1;

  self->connections_popover =
    port_connections_popover_widget_new (GTK_WIDGET (self));
  gtk_widget_set_parent (
    GTK_WIDGET (self->connections_popover), GTK_WIDGET (self));
}

static void
modulator_inner_widget_class_init (ModulatorInnerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "modulator_inner.ui");
  gtk_widget_class_set_css_name (klass, "modulator_inner");

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
  oklass->dispose = (GObjectFinalizeFunc) dispose;

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, ModulatorInnerWidget, x)

  BIND_CHILD (controls_box);
  BIND_CHILD (toolbar);
  BIND_CHILD (show_hide_ui_btn);
  BIND_CHILD (waveforms_box);
  gtk_widget_class_bind_template_callback (klass, on_delete_clicked);
  gtk_widget_class_bind_template_callback (klass, on_show_hide_ui_toggled);

#undef BIND_CHILD
}
