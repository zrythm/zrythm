// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/velocity_settings.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "utils/gtk.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (VelocitySettingsWidget, velocity_settings_widget, GTK_TYPE_WIDGET)

static gboolean
get_mapping (GValue * value, GVariant * variant, VelocitySettingsWidget * self)
{
  const char * str = g_variant_get_string (variant, nullptr);

  guint val = Velocity::setting_str_to_enum (str);
  g_value_set_uint (value, val);

  return true;
}

static GVariant *
set_mapping (
  const GValue *           value,
  const GVariantType *     expected_type,
  VelocitySettingsWidget * self)
{
  guint val = g_value_get_uint (value);

  const char * str = Velocity::setting_enum_to_str (val);

  return g_variant_new_string (str);
}

static void
on_dispose (GObject * object)
{
  z_gtk_widget_remove_all_children (GTK_WIDGET (object));

  G_OBJECT_CLASS (velocity_settings_widget_parent_class)->dispose (object);
}

static void
velocity_settings_widget_class_init (VelocitySettingsWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "velocity-settings");

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BOX_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = on_dispose;
}

static void
velocity_settings_widget_init (VelocitySettingsWidget * self)
{
  gtk_widget_add_css_class (GTK_WIDGET (self), "toolbar-child-box");

  GtkWidget * lbl = gtk_label_new (_ ("Velocity"));
  gtk_widget_set_parent (lbl, GTK_WIDGET (self));

  const char * list[] = { _ ("Last Note"), "40", "90", "120", NULL };
  self->default_velocity_dropdown =
    GTK_DROP_DOWN (gtk_drop_down_new_from_strings (list));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->default_velocity_dropdown),
    _ ("Velocity used on newly created notes"));
  gtk_widget_set_parent (
    GTK_WIDGET (self->default_velocity_dropdown), GTK_WIDGET (self));

#if 0
  g_signal_connect (
    G_OBJECT (self->default_velocity_dropdown),
    "activate",
    G_CALLBACK (on_default_velocity_activate), self);
#endif

  g_settings_bind_with_mapping (
    S_UI, "piano-roll-default-velocity",
    G_OBJECT (self->default_velocity_dropdown), "selected",
    G_SETTINGS_BIND_DEFAULT, (GSettingsBindGetMapping) get_mapping,
    (GSettingsBindSetMapping) set_mapping, self, nullptr);
}
