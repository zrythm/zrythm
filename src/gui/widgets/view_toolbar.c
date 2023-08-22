// SPDX-FileCopyrightText: © 2019, 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/view_toolbar.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ViewToolbarWidget,
  view_toolbar_widget,
  GTK_TYPE_BOX)

static void
view_toolbar_widget_init (ViewToolbarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), tooltip)
  SET_TOOLTIP (status_bar, _ ("Toggle Status Bar"));
  SET_TOOLTIP (fullscreen, _ ("Fullscreen"));
#undef SET_TOOLTIP
}

static void
view_toolbar_widget_class_init (
  ViewToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "view_toolbar.ui");

  gtk_widget_class_set_css_name (klass, "view-toolbar");
  gtk_widget_class_set_accessible_role (
    klass, GTK_ACCESSIBLE_ROLE_TOOLBAR);

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ViewToolbarWidget, x)

  BIND_CHILD (status_bar);
  BIND_CHILD (fullscreen);
  BIND_CHILD (left_panel);
  BIND_CHILD (bot_panel);
  BIND_CHILD (top_panel);
  BIND_CHILD (right_panel);

#undef BIND_CHILD
}
