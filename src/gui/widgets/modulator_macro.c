/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/control_port.h"
#include "audio/modulator_track.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/knob_with_name.h"
#include "gui/widgets/live_waveform.h"
#include "gui/widgets/modulator_macro.h"
#include "gui/widgets/port_connections_popover.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/string.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ModulatorMacroWidget, modulator_macro_widget,
  GTK_TYPE_GRID)

void
modulator_macro_widget_refresh (
  ModulatorMacroWidget * self)
{
}

static void
set_val (
  void * _port,
  float  val)
{
  Port * port = (Port *) _port;
  g_return_if_fail (IS_PORT (port));
  port_set_control_value (
    port, val, F_NOT_NORMALIZED, F_PUBLISH_EVENTS);
}

ModulatorMacroWidget *
modulator_macro_widget_new (
  int modulator_macro_idx)
{
  ModulatorMacroWidget * self =
    g_object_new (MODULATOR_MACRO_WIDGET_TYPE, NULL);

  self->modulator_macro_idx = modulator_macro_idx;

  Port * port =
    P_MODULATOR_TRACK->modulator_macros[
      modulator_macro_idx];

  KnobWidget * knob =
    knob_widget_new_simple (
      control_port_get_val, set_val,
      port, port->minf, port->maxf, 48, port->zerof);
  self->knob_with_name =
    knob_with_name_widget_new (
      port->id.label, knob,
      GTK_ORIENTATION_VERTICAL, true, 2);
  gtk_grid_attach (
    GTK_GRID (self),
    GTK_WIDGET (self->knob_with_name), 1, 0, 1, 2);

  return self;
}

static void
finalize (
  ModulatorMacroWidget * self)
{
  G_OBJECT_CLASS (
    modulator_macro_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
modulator_macro_widget_class_init (
  ModulatorMacroWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);

  resources_set_class_template (
    klass, "modulator_macro.ui");

  gtk_widget_class_set_css_name (
    klass, "modulator-macro");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ModulatorMacroWidget, x)

  BIND_CHILD (inputs);
  BIND_CHILD (output);
  BIND_CHILD (add_input);

  GObjectClass * goklass = G_OBJECT_CLASS (_klass);
  goklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
modulator_macro_widget_init (
  ModulatorMacroWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
