// clang-format off
// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "gui/widgets/project_toolbar.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (ProjectToolbarWidget, project_toolbar_widget, GTK_TYPE_BOX)

static void
project_toolbar_widget_init (ProjectToolbarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable (GTK_ACTIONABLE (self->x), tooltip)
  SET_TOOLTIP (new, _ ("New Project"));
  SET_TOOLTIP (save_btn, _ ("Save"));
  SET_TOOLTIP (save_as_btn, _ ("Save As"));
  SET_TOOLTIP (open, _ ("Open Project"));
  SET_TOOLTIP (export_as, _ ("Export As"));
  SET_TOOLTIP (export_graph, _ ("Export Graph"));
#undef SET_TOOLTIP
}

static void
project_toolbar_widget_class_init (ProjectToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "project_toolbar.ui");

  gtk_widget_class_set_css_name (klass, "project-toolbar");
  gtk_widget_class_set_accessible_role (klass, GTK_ACCESSIBLE_ROLE_TOOLBAR);

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, ProjectToolbarWidget, x)

  BIND_CHILD (new);
  BIND_CHILD (save_btn);
  BIND_CHILD (save_as_btn);
  BIND_CHILD (open);
  BIND_CHILD (export_as);
  BIND_CHILD (export_graph);

#undef BIND_CHILD
}
