// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/control_port.h"
#include "common/dsp/modulator_track.h"
#include "common/dsp/port_identifier.h"
#include "common/dsp/tracklist.h"
#include "common/utils/gtk.h"
#include "common/utils/rt_thread_id.h"
#include "gui/cpp/backend/actions/mixer_selections_action.h"
#include "gui/cpp/backend/actions/undo_manager.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/dialogs/bind_cc_dialog.h"
#include "gui/cpp/gtk_widgets/dialogs/port_info.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/knob.h"
#include "gui/cpp/gtk_widgets/knob_with_name.h"
#include "gui/cpp/gtk_widgets/live_waveform.h"
#include "gui/cpp/gtk_widgets/modulator.h"
#include "gui/cpp/gtk_widgets/modulator_inner.h"
#include "gui/cpp/gtk_widgets/popovers/port_connections_popover.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (ModulatorInnerWidget, modulator_inner_widget, GTK_TYPE_BOX)

static Plugin *
get_modulator (ModulatorInnerWidget * self)
{
  return self->parent->modulator;
}

static float
get_snapped_control_value (ControlPort * port)
{
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
  auto     sel = std::make_unique<FullMixerSelections> ();
  Plugin * modulator = get_modulator (self);
  sel->add_slot (
    *P_MODULATOR_TRACK, PluginSlotType::Modulator, modulator->id_.slot_, true);

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
  for (auto port : modulator->in_ports_ | type_is<ControlPort> ())
    {
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id_.flags_,
          PortIdentifier::Flags::NotOnGui))
        continue;

      KnobWidget * knob = knob_widget_new_simple (
        bind_member_function (*port, &ControlPort::get_val),
        bind_member_function (*port, &ControlPort::get_default_val),
        bind_member_function (*port, &ControlPort::set_real_val), port,
        port->minf_, port->maxf_, 24, port->zerof_);
      knob->snapped_getter = [port] () {
        return get_snapped_control_value (port);
      };
      KnobWithNameWidget * knob_with_name = knob_with_name_widget_new (
        &port->id_, bind_member_function (port->id_, &PortIdentifier::get_label),
        nullptr, knob, GTK_ORIENTATION_HORIZONTAL, false, 3);

      self->knobs.push_back (knob_with_name);

      gtk_box_append (GTK_BOX (self->controls_box), GTK_WIDGET (knob_with_name));

      /* add context menu */
      GtkGestureClick * mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
      gtk_gesture_single_set_button (
        GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
      g_signal_connect (
        G_OBJECT (mp), "pressed", G_CALLBACK (on_knob_right_click), port);
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
finalize (GObject * gobj)
{
  ModulatorInnerWidget * self = Z_MODULATOR_INNER_WIDGET (gobj);

  std::destroy_at (&self->knobs);

  G_OBJECT_CLASS (modulator_inner_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
dispose (GObject * gobj)
{
  ModulatorInnerWidget * self = Z_MODULATOR_INNER_WIDGET (gobj);

  gtk_widget_unparent (GTK_WIDGET (self->connections_popover));

  G_OBJECT_CLASS (modulator_inner_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
modulator_inner_widget_init (ModulatorInnerWidget * self)
{
  std::construct_at (&self->knobs);

  gtk_widget_init_template (GTK_WIDGET (self));

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
  oklass->finalize = finalize;
  oklass->dispose = dispose;

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
