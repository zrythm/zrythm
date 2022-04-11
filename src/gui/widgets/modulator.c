/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/track.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/knob_with_name.h"
#include "gui/widgets/live_waveform.h"
#include "gui/widgets/modulator.h"
#include "gui/widgets/modulator_inner.h"
#include "gui/widgets/port_connections_popover.h"
#include "plugins/plugin.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/string.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ModulatorWidget,
  modulator_widget,
  TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

void
modulator_widget_refresh (ModulatorWidget * self)
{
  modulator_inner_widget_refresh (self->inner);
}

ModulatorWidget *
modulator_widget_new (Plugin * modulator)
{
  g_return_val_if_fail (
    IS_PLUGIN (modulator), NULL);

  ModulatorWidget * self =
    g_object_new (MODULATOR_WIDGET_TYPE, NULL);

  self->modulator = modulator;

  self->inner = modulator_inner_widget_new (self);

  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self),
    modulator->setting->descr->name);
  expander_box_widget_set_icon_name (
    Z_EXPANDER_BOX_WIDGET (self), "modulator");

  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->inner));
  two_col_expander_box_widget_set_scroll_policy (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_POLICY_NEVER, GTK_POLICY_NEVER);

  /* TODO */
#if 0
  GtkBox * box =
    two_col_expander_box_widget_get_content_box (
      Z_TWO_COL_EXPANDER_BOX_WIDGET (self));
  gtk_box_set_child_packing (
    box, GTK_WIDGET (self->inner),
    F_NO_EXPAND, F_FILL, 0, GTK_PACK_START);
#endif

  g_object_ref (self);

  return self;
}

static void
finalize (ModulatorWidget * self)
{
  G_OBJECT_CLASS (modulator_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
modulator_widget_class_init (
  ModulatorWidgetClass * _klass)
{
  GObjectClass * klass = G_OBJECT_CLASS (_klass);

  klass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
modulator_widget_init (ModulatorWidget * self)
{
  expander_box_widget_set_orientation (
    Z_EXPANDER_BOX_WIDGET (self),
    GTK_ORIENTATION_HORIZONTAL);
}
