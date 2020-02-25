/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/accel.h"
#include "gui/widgets/header_notebook.h"
#include "gui/widgets/help_toolbar.h"
#include "gui/widgets/home_toolbar.h"
#include "gui/widgets/project_toolbar.h"
#include "gui/widgets/snap_box.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/toolbox.h"
#include "gui/widgets/view_toolbar.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (HeaderNotebookWidget,
               header_notebook_widget,
               GTK_TYPE_NOTEBOOK)

void
header_notebook_widget_setup (
  HeaderNotebookWidget * self,
  const char * title)
{
  header_notebook_widget_set_subtitle (
    self, title);
}

void
header_notebook_widget_set_subtitle (
  HeaderNotebookWidget * self,
  const char * title)
{
  /* hack to center the title */
  char title_w_spaces[600];
  strcpy (title_w_spaces, title);
  strcat (
    title_w_spaces,
    "                                            "
    "                                            "
    "    ");
  gtk_label_set_text (
    self->prj_name_label, title_w_spaces);
}

static void
header_notebook_widget_init (
  HeaderNotebookWidget * self)
{
  g_type_ensure (HOME_TOOLBAR_WIDGET_TYPE);
  g_type_ensure (TOOLBOX_WIDGET_TYPE);
  g_type_ensure (SNAP_BOX_WIDGET_TYPE);
  g_type_ensure (SNAP_GRID_WIDGET_TYPE);
  g_type_ensure (HELP_TOOLBAR_WIDGET_TYPE);
  g_type_ensure (VIEW_TOOLBAR_WIDGET_TYPE);
  g_type_ensure (PROJECT_TOOLBAR_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  GtkStyleContext *context;
  context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self));
  gtk_style_context_add_class (
    context, "header_notebook");

  /* set tooltips */
#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    tooltip)
  SET_TOOLTIP (z_icon, _("About Zrythm"));
  SET_TOOLTIP (preferences, _("Preferences"));
  SET_TOOLTIP (log_viewer, _("Log viewer"));
  SET_TOOLTIP (
    scripting_interface, _("Scripting interface"));
#undef SET_TOOLTIP
}

static void
header_notebook_widget_class_init (
  HeaderNotebookWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);

  resources_set_class_template (
    klass, "header_notebook.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    HeaderNotebookWidget, \
    x)

  BIND_CHILD (home_toolbar);
  BIND_CHILD (project_toolbar);
  BIND_CHILD (view_toolbar);
  BIND_CHILD (help_toolbar);
  BIND_CHILD (preferences);
  BIND_CHILD (prj_name_label);
  BIND_CHILD (z_icon);
  BIND_CHILD (preferences);
  BIND_CHILD (log_viewer);
  BIND_CHILD (scripting_interface);

#undef BIND_CHILD
}

