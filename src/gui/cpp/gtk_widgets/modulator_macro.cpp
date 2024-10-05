// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/control_port.h"
#include "common/dsp/modulator_track.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/plugins/plugin.h"
#include "common/utils/cairo.h"
#include "common/utils/gtk.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/dialogs/bind_cc_dialog.h"
#include "gui/cpp/gtk_widgets/dialogs/port_info.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/knob.h"
#include "gui/cpp/gtk_widgets/knob_with_name.h"
#include "gui/cpp/gtk_widgets/modulator_macro.h"
#include "gui/cpp/gtk_widgets/popovers/port_connections_popover.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (ModulatorMacroWidget, modulator_macro_widget, GTK_TYPE_WIDGET)

static void
on_inputs_draw (
  GtkDrawingArea * drawing_area,
  cairo_t *        cr,
  int              width,
  int              height,
  gpointer         user_data)
{
  ModulatorMacroWidget * self = Z_MODULATOR_MACRO_WIDGET (user_data);
  GtkWidget *            widget = GTK_WIDGET (drawing_area);

  auto &port =
    P_MODULATOR_TRACK->modulator_macro_processors_[self->modulator_macro_idx]
      ->cv_in_;

  if (port->srcs_.size () == 0)
    {
      const char * str = _ ("No inputs");
      int          w, h;
      z_cairo_get_text_extents_for_widget (widget, self->layout, str, &w, &h);
      cairo_set_source_rgba (cr, 1, 1, 1, 1);

#if 0
      cairo_save (cr);
      cairo_translate (cr, width / 2, height / 2);
      cairo_rotate (cr, - 1.570796);
      cairo_move_to (
        cr, - w / 2.0, - h / 2.0);
#endif
      cairo_move_to (
        cr, (double) width / 2.0 - (double) w / 2.0,
        (double) height / 2.0 - (double) h / 2.0);
      z_cairo_draw_text (cr, widget, self->layout, str);

#if 0
      cairo_restore (cr);
#endif
    }
  else
    {
      double val_w = ((double) width / (double) port->srcs_.size ());
      for (size_t i = 0; i < port->srcs_.size (); i++)
        {
          double val_h =
            (double) ((port->srcs_[i]->buf_[0] - port->minf_)
                      / (port->maxf_ - port->minf_))
            * (double) height;
          cairo_set_source_rgba (cr, 1, 1, 0, 1);
          cairo_rectangle (
            cr, val_w * (double) i, (double) height - val_h, val_w, 1);
          cairo_fill (cr);

          if (i != 0)
            {
              cairo_set_source_rgba (cr, 0.4, 0.4, 0.4, 1);
              z_cairo_draw_vertical_line (cr, val_w * i, 0, height, 1);
            }
        }
    }
}

static void
on_output_draw (
  GtkDrawingArea * drawing_area,
  cairo_t *        cr,
  int              width,
  int              height,
  gpointer         user_data)
{
  ModulatorMacroWidget * self = Z_MODULATOR_MACRO_WIDGET (user_data);

  auto &port =
    P_MODULATOR_TRACK->modulator_macro_processors_[self->modulator_macro_idx]
      ->cv_out_;

  cairo_set_source_rgba (cr, 1, 1, 0, 1);
  double val_h =
    (double) ((port->buf_[0] - port->minf_) / (port->maxf_ - port->minf_))
    * (double) height;
  cairo_rectangle (cr, 0, (double) height - val_h, width, 1);
  cairo_fill (cr);
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

  KnobWidget * knob = Z_KNOB_WIDGET (
    gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture)));

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

  z_gtk_show_context_menu_from_g_menu (knob->popover_menu, x, y, menu);
}

void
modulator_macro_widget_refresh (ModulatorMacroWidget * self)
{
}

static void
on_automate_clicked (GtkButton * btn, Port * port)
{
  z_return_if_fail (port);

  ModulatorMacroWidget * self =
    Z_MODULATOR_MACRO_WIDGET (g_object_get_data (G_OBJECT (btn), "owner"));
  port_connections_popover_widget_refresh (self->connections_popover, port);
  gtk_popover_popup (GTK_POPOVER (self->connections_popover));

#if 0
  g_signal_connect (
    G_OBJECT (popover), "closed",
    G_CALLBACK (on_popover_closed), self);
#endif
}

static bool
redraw_cb (
  GtkWidget *            widget,
  GdkFrameClock *        frame_clock,
  ModulatorMacroWidget * self)
{
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

ModulatorMacroWidget *
modulator_macro_widget_new (int modulator_macro_idx)
{
  ModulatorMacroWidget * self = static_cast<ModulatorMacroWidget *> (
    g_object_new (MODULATOR_MACRO_WIDGET_TYPE, nullptr));

  self->modulator_macro_idx = modulator_macro_idx;

  auto &macro =
    P_MODULATOR_TRACK->modulator_macro_processors_[modulator_macro_idx];
  auto &port = macro->macro_;

  KnobWidget * knob = knob_widget_new_simple (
    bind_member_function (*port, &ControlPort::get_val),
    bind_member_function (*port, &ControlPort::get_default_val),
    bind_member_function (*port, &ControlPort::set_real_val), port.get (),
    port->minf_, port->maxf_, 48, port->zerof_);
  self->knob_with_name = knob_with_name_widget_new (
    macro.get (),
    bind_member_function (*macro, &ModulatorMacroProcessor::get_name),
    bind_member_function (*macro, &ModulatorMacroProcessor::set_name), knob,
    GTK_ORIENTATION_VERTICAL, true, 2);
  gtk_grid_attach (
    GTK_GRID (self->grid), GTK_WIDGET (self->knob_with_name), 1, 0, 1, 2);

  /* add context menu */
  GtkGestureClick * mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed", G_CALLBACK (on_knob_right_click), port.get ());
  gtk_widget_add_controller (GTK_WIDGET (knob), GTK_EVENT_CONTROLLER (mp));

  g_object_set_data (G_OBJECT (self->outputs), "owner", self);
  g_object_set_data (G_OBJECT (self->add_input), "owner", self);

  g_signal_connect (
    G_OBJECT (self->outputs), "clicked", G_CALLBACK (on_automate_clicked),
    P_MODULATOR_TRACK->modulator_macro_processors_[modulator_macro_idx]
      ->cv_out_.get ());
  g_signal_connect (
    G_OBJECT (self->add_input), "clicked", G_CALLBACK (on_automate_clicked),
    P_MODULATOR_TRACK->modulator_macro_processors_[modulator_macro_idx]
      ->cv_in_.get ());

  gtk_drawing_area_set_draw_func (
    GTK_DRAWING_AREA (self->inputs), on_inputs_draw, self, nullptr);
  gtk_drawing_area_set_draw_func (
    GTK_DRAWING_AREA (self->output), on_output_draw, self, nullptr);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self->inputs), (GtkTickCallback) redraw_cb, self, nullptr);
  gtk_widget_add_tick_callback (
    GTK_WIDGET (self->output), (GtkTickCallback) redraw_cb, self, nullptr);

  return self;
}

static void
finalize (ModulatorMacroWidget * self)
{
  G_OBJECT_CLASS (modulator_macro_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
dispose (ModulatorMacroWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->connections_popover));
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (modulator_macro_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
modulator_macro_widget_class_init (ModulatorMacroWidgetClass * _klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);

  resources_set_class_template (wklass, "modulator_macro.ui");

  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (wklass, "modulator-macro");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (wklass, ModulatorMacroWidget, x)

  BIND_CHILD (grid);
  BIND_CHILD (inputs);
  BIND_CHILD (output);
  BIND_CHILD (add_input);
  BIND_CHILD (outputs);

#undef BIND_CHILD

  GObjectClass * goklass = G_OBJECT_CLASS (_klass);
  goklass->dispose = (GObjectFinalizeFunc) dispose;
  goklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
modulator_macro_widget_init (ModulatorMacroWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_set_size_request (GTK_WIDGET (self->inputs), -1, 12);
  /*gtk_widget_set_size_request (*/
  /*GTK_WIDGET (self->inputs), 24, -1);*/

  self->layout =
    z_cairo_create_pango_layout_from_string (
      GTK_WIDGET (self->inputs), "7", PANGO_ELLIPSIZE_NONE, -1)
      .release ();

  self->connections_popover =
    port_connections_popover_widget_new (GTK_WIDGET (self));
  gtk_widget_set_parent (
    GTK_WIDGET (self->connections_popover), GTK_WIDGET (self));

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));
}
