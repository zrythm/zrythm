/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/project_toolbar.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ProjectToolbarWidget,
  project_toolbar_widget,
  GTK_TYPE_BOX)

static void
project_toolbar_widget_init (
  ProjectToolbarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), tooltip)
  SET_TOOLTIP (new, _ ("New Project"));
  SET_TOOLTIP (save_btn, _ ("Save"));
  SET_TOOLTIP (save_as_btn, _ ("Save As"));
  SET_TOOLTIP (open, _ ("Open Project"));
  SET_TOOLTIP (export_as, _ ("Export As"));
  SET_TOOLTIP (export_graph, _ ("Export Graph"));
#undef SET_TOOLTIP
}

static void
project_toolbar_widget_class_init (
  ProjectToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "project_toolbar.ui");

  gtk_widget_class_set_css_name (
    klass, "project-toolbar");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ProjectToolbarWidget, x)

  BIND_CHILD (new);
  BIND_CHILD (save_btn);
  BIND_CHILD (save_as_btn);
  BIND_CHILD (open);
  BIND_CHILD (export_as);
  BIND_CHILD (export_graph);

#undef BIND_CHILD
}
