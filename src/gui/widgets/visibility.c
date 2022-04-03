/*
 * Copyright (C) 2018-2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track_visibility_tree.h"
#include "gui/widgets/two_col_expander_box.h"
#include "gui/widgets/visibility.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  VisibilityWidget,
  visibility_widget,
  GTK_TYPE_BOX)

/**
 * Refreshes the visibility widget.
 */
void
visibility_widget_refresh (VisibilityWidget * self)
{
  track_visibility_tree_widget_refresh (
    self->track_visibility);
}

VisibilityWidget *
visibility_widget_new ()
{
  VisibilityWidget * self =
    g_object_new (VISIBILITY_WIDGET_TYPE, NULL);

  self->track_visibility =
    track_visibility_tree_widget_new ();
  gtk_box_append (
    GTK_BOX (self),
    GTK_WIDGET (self->track_visibility));
  gtk_widget_set_vexpand (
    GTK_WIDGET (self->track_visibility), true);

  return self;
}

static void
visibility_widget_class_init (
  VisibilityWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "visibility");
}

static void
visibility_widget_init (VisibilityWidget * self)
{
}
