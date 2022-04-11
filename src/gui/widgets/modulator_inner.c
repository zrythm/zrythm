/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/mixer_selections_action.h"
#include "actions/undo_manager.h"
#include "audio/control_port.h"
#include "audio/modulator_track.h"
#include "audio/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/dialogs/bind_cc_dialog.h"
#include "gui/widgets/dialogs/port_info.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/knob_with_name.h"
#include "gui/widgets/live_waveform.h"
#include "gui/widgets/modulator.h"
#include "gui/widgets/modulator_inner.h"
#include "gui/widgets/port_connections_popover.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ModulatorInnerWidget,
  modulator_inner_widget,
  GTK_TYPE_BOX)

static Plugin *
get_modulator (ModulatorInnerWidget * self)
{
  return self->parent->modulator;
}

static float
get_snapped_control_value (Port * port)
{
  float val = port_get_control_value (
    port, F_NOT_NORMALIZED);

  return val;
}

static void
on_show_hide_ui_toggled (
  GtkToggleButton *      btn,
  ModulatorInnerWidget * self)
{
  Plugin * modulator = get_modulator (self);

  modulator->visible = !modulator->visible;

  EVENTS_PUSH (
    ET_PLUGIN_VISIBILITY_CHANGED, modulator);
}

static void
on_delete_clicked (
  GtkButton *            btn,
  ModulatorInnerWidget * self)
{
  MixerSelections * sel = mixer_selections_new ();
  Plugin * modulator = get_modulator (self);
  mixer_selections_add_slot (
    sel, P_MODULATOR_TRACK, PLUGIN_SLOT_MODULATOR,
    modulator->id.slot, F_NO_CLONE);

  GError * err = NULL;
  bool ret = mixer_selections_action_perform_delete (
    sel, PORT_CONNECTIONS_MGR, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _ ("Failed to delete plugins"));
    }

  mixer_selections_free (sel);
}

static void
on_automate_clicked (
  GtkButton *            btn,
  ModulatorInnerWidget * self)
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
  g_return_if_fail (index >= 0);

  port_connections_popover_widget_refresh (
    self->connections_popover, self->ports[index]);
  gtk_popover_popup (
    GTK_POPOVER (self->connections_popover));

  /* TODO update label on closed */
#if 0
  g_signal_connect (
    G_OBJECT (popover), "closed",
    G_CALLBACK (on_popover_closed), self);
#endif
}

void
modulator_inner_widget_refresh (
  ModulatorInnerWidget * self)
{
  Plugin * modulator = get_modulator (self);
  g_signal_handlers_block_by_func (
    self->show_hide_ui_btn,
    on_show_hide_ui_toggled, self);
  gtk_toggle_button_set_active (
    self->show_hide_ui_btn, modulator->visible);
  g_signal_handlers_unblock_by_func (
    self->show_hide_ui_btn,
    on_show_hide_ui_toggled, self);
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
  menuitem = z_gtk_create_menu_item (
    _ ("Reset"), NULL, tmp);
  g_menu_append_item (menu, menuitem);

  sprintf (tmp, "app.bind-midi-cc::%p", port);
  menuitem = CREATE_MIDI_LEARN_MENU_ITEM (tmp);
  g_menu_append_item (menu, menuitem);

  sprintf (tmp, "app.port-view-info::%p", port);
  menuitem = z_gtk_create_menu_item (
    _ ("View info"), NULL, tmp);
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    NULL, x, y, menu);
}

/**
 * Creates a new widget.
 */
ModulatorInnerWidget *
modulator_inner_widget_new (
  ModulatorWidget * parent)
{
  ModulatorInnerWidget * self = g_object_new (
    MODULATOR_INNER_WIDGET_TYPE, NULL);

  self->parent = parent;

  Plugin * modulator = get_modulator (self);
  for (int i = 0; i < modulator->num_in_ports; i++)
    {
      Port * port = modulator->in_ports[i];

      if (
        port->id.type != TYPE_CONTROL
        || port->id.flow != FLOW_INPUT
        || port->id.flags & PORT_FLAG_NOT_ON_GUI)
        continue;

      KnobWidget * knob = knob_widget_new_simple (
        control_port_get_val,
        control_port_get_default_val,
        control_port_set_real_val, port,
        port->minf, port->maxf, 24, port->zerof);
      knob->snapped_getter = (GenericFloatGetter)
        get_snapped_control_value;
      KnobWithNameWidget * knob_with_name =
        knob_with_name_widget_new (
          &port->id,
          (GenericStringGetter)
            port_identifier_get_label,
          NULL, knob, GTK_ORIENTATION_HORIZONTAL,
          false, 3);

      array_double_size_if_full (
        self->knobs, self->num_knobs,
        self->knobs_size, KnobWithNameWidget *);
      array_append (
        self->knobs, self->num_knobs,
        knob_with_name);

      gtk_box_append (
        GTK_BOX (self->controls_box),
        GTK_WIDGET (knob_with_name));

      /* add context menu */
      GtkGestureClick * mp = GTK_GESTURE_CLICK (
        gtk_gesture_click_new ());
      gtk_gesture_single_set_button (
        GTK_GESTURE_SINGLE (mp),
        GDK_BUTTON_SECONDARY);
      g_signal_connect (
        G_OBJECT (mp), "pressed",
        G_CALLBACK (on_knob_right_click), port);
      gtk_widget_add_controller (
        GTK_WIDGET (knob_with_name),
        GTK_EVENT_CONTROLLER (mp));
    }

  for (int i = 0; i < modulator->num_out_ports; i++)
    {
      Port * port = modulator->out_ports[i];
      if (port->id.type != TYPE_CV)
        continue;

      int index = self->num_waveforms++;

      self->ports[index] = port;

      /* create waveform */
      self->waveforms[index] =
        live_waveform_widget_new_port (port);
      gtk_widget_set_size_request (
        GTK_WIDGET (self->waveforms[index]), 48,
        48);
      gtk_widget_set_visible (
        GTK_WIDGET (self->waveforms[index]), true);

      /* create waveform overlay */
      self->waveform_overlays[index] =
        GTK_OVERLAY (gtk_overlay_new ());
      gtk_widget_set_visible (
        GTK_WIDGET (self->waveform_overlays[index]),
        true);
      gtk_overlay_set_child (
        GTK_OVERLAY (self->waveform_overlays[index]),
        GTK_WIDGET (self->waveforms[index]));

      /* add button for selecting automatable */
      self->waveform_automate_buttons[index] =
        GTK_BUTTON (gtk_button_new_from_icon_name (
          "automate"));
      gtk_widget_set_visible (
        GTK_WIDGET (
          self->waveform_automate_buttons[index]),
        true);
      gtk_overlay_add_overlay (
        self->waveform_overlays[index],
        GTK_WIDGET (
          self->waveform_automate_buttons[index]));
      gtk_widget_set_halign (
        GTK_WIDGET (
          self->waveform_automate_buttons[index]),
        GTK_ALIGN_END);
      gtk_widget_set_valign (
        GTK_WIDGET (
          self->waveform_automate_buttons[index]),
        GTK_ALIGN_START);

      g_signal_connect (
        G_OBJECT (
          self->waveform_automate_buttons[index]),
        "clicked",
        G_CALLBACK (on_automate_clicked), self);

      gtk_box_append (
        GTK_BOX (self->waveforms_box),
        GTK_WIDGET (self->waveform_overlays[index]));

      if (self->num_waveforms == 16)
        break;
    }

  EVENTS_PUSH (
    ET_PLUGIN_VISIBILITY_CHANGED, modulator);
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

  G_OBJECT_CLASS (
    modulator_inner_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
dispose (ModulatorInnerWidget * self)
{
  gtk_widget_unparent (
    GTK_WIDGET (self->connections_popover));

  G_OBJECT_CLASS (
    modulator_inner_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
modulator_inner_widget_init (
  ModulatorInnerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->knobs =
    calloc (1, sizeof (KnobWithNameWidget *));
  self->knobs_size = 1;

  self->connections_popover =
    port_connections_popover_widget_new (
      GTK_WIDGET (self));
  gtk_widget_set_parent (
    GTK_WIDGET (self->connections_popover),
    GTK_WIDGET (self));
}

static void
modulator_inner_widget_class_init (
  ModulatorInnerWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "modulator_inner.ui");
  gtk_widget_class_set_css_name (
    klass, "modulator_inner");

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
  oklass->dispose = (GObjectFinalizeFunc) dispose;

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ModulatorInnerWidget, x)

  BIND_CHILD (controls_box);
  BIND_CHILD (toolbar);
  BIND_CHILD (show_hide_ui_btn);
  BIND_CHILD (waveforms_box);
  gtk_widget_class_bind_template_callback (
    klass, on_delete_clicked);
  gtk_widget_class_bind_template_callback (
    klass, on_show_hide_ui_toggled);

#undef BIND_CHILD
}
