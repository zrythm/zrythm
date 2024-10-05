// SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou
// <alex@zrythm.org> SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/modulator.h"
#include "gui/cpp/gtk_widgets/modulator_inner.h"

#include <glib/gi18n.h>

#include "common/plugins/plugin.h"

G_DEFINE_TYPE (ModulatorWidget, modulator_widget, TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

void
modulator_widget_refresh (ModulatorWidget * self)
{
  modulator_inner_widget_refresh (self->inner);
}

ModulatorWidget *
modulator_widget_new (Plugin * modulator)
{
  z_return_val_if_fail (IS_PLUGIN (modulator), nullptr);

  ModulatorWidget * self = static_cast<ModulatorWidget *> (
    g_object_new (MODULATOR_WIDGET_TYPE, nullptr));

  self->modulator = modulator;

  self->inner = modulator_inner_widget_new (self);

  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self), modulator->get_name ().c_str ());
  expander_box_widget_set_icon_name (
    Z_EXPANDER_BOX_WIDGET (self), "gnome-icon-library-encoder-knob-symbolic");

  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (self->inner));
  two_col_expander_box_widget_set_scroll_policy (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_POLICY_NEVER, GTK_POLICY_NEVER);

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
  G_OBJECT_CLASS (modulator_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
modulator_widget_class_init (ModulatorWidgetClass * _klass)
{
  GObjectClass * klass = G_OBJECT_CLASS (_klass);

  klass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
modulator_widget_init (ModulatorWidget * self)
{
  expander_box_widget_set_orientation (
    Z_EXPANDER_BOX_WIDGET (self), GTK_ORIENTATION_HORIZONTAL);
}
